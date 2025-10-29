# Klipper-Style Architecture for PUWinder

## Why We're Struggling vs Klipper/FluidNC

### Current Problem
We're doing **real-time motion control** on the Pico, which means:
- Calculating traverse velocity every 50ms based on current RPM
- Generating step chunks every 50ms
- Compressing steps every 50ms
- Managing queue depth every 50ms
- All while running 20kHz ISR for step execution

**Result**: Pico is overwhelmed, queue fills up, motion control fails.

### How Klipper Does It

```
┌─────────────────────────────────────────────────────────────┐
│                    RASPBERRY PI (Host)                       │
├─────────────────────────────────────────────────────────────┤
│  1. G-code Parser                                            │
│  2. Motion Planner (trapezoid velocity profiles)             │
│  3. Lookahead Buffer (acceleration/deceleration)             │
│  4. Step Compressor                                          │
│  5. Command Queue                                            │
│     ↓                                                         │
│     Sends pre-computed "move commands" via UART              │
└─────────────────────────────────────────────────────────────┘
                            ↓ UART
┌─────────────────────────────────────────────────────────────┐
│                  PICO (Microcontroller)                      │
├─────────────────────────────────────────────────────────────┤
│  1. Receive move commands into buffer                        │
│  2. ISR executes steps from buffer (20kHz)                   │
│  3. Send status updates back to host                         │
│                                                               │
│  NO MATH - JUST EXECUTION!                                   │
└─────────────────────────────────────────────────────────────┘
```

### Key Klipper Concepts

#### 1. **Move Commands** (Not Steps!)
Klipper doesn't send individual step commands. It sends **compressed move commands**:

```c
struct move_command {
    uint32_t interval;     // Step interval (µs)
    uint32_t count;        // Number of steps
    int32_t add;           // Acceleration adjustment per step
};
```

Example: "Execute 10,000 steps at 50µs intervals with +2µs acceleration per step"

#### 2. **MCU Clock Synchronization**
- Pi and Pico clocks are synchronized
- All timing is in "MCU clock ticks"
- Move commands scheduled in advance

#### 3. **Lookahead Buffer**
- Host plans 100-200ms ahead
- MCU always has work to do
- Smooth velocity transitions

## Proposed Architecture for PUWinder

### Phase 1: Offload Math to Pi Zero

```
┌─────────────────────────────────────────────────────────────┐
│              PI ZERO (simple_control.py)                     │
├─────────────────────────────────────────────────────────────┤
│  1. Monitor spindle RPM from Pico                            │
│  2. Calculate traverse velocity (RPM × wire_diameter)        │
│  3. Generate step chunks for next 100ms                      │
│  4. Send chunks to Pico via UART                             │
│  5. Monitor queue depth, adjust generation rate              │
│     ↓                                                         │
│     UART: "PUSH_CHUNK <steps> <interval> <accel>"            │
└─────────────────────────────────────────────────────────────┘
                            ↓ UART
┌─────────────────────────────────────────────────────────────┐
│                         PICO                                 │
├─────────────────────────────────────────────────────────────┤
│  1. Read spindle pulses, calculate RPM                       │
│  2. Send RPM updates to Pi Zero (every 100ms)                │
│  3. Receive PUSH_CHUNK commands from Pi Zero                 │
│  4. Push chunks to MoveQueue                                 │
│  5. ISR executes steps (20kHz)                               │
│                                                               │
│  NO REAL-TIME MATH!                                          │
└─────────────────────────────────────────────────────────────┘
```

### Phase 2: Implement Lookahead (Like Klipper)

```python
# On Pi Zero
class WindingPlanner:
    def __init__(self):
        self.lookahead_buffer = []  # 200ms worth of moves
        self.current_rpm = 0
        self.target_position = 0
        
    def update(self):
        # 1. Get current RPM from Pico
        self.current_rpm = self.read_rpm_from_pico()
        
        # 2. Calculate required traverse velocity
        velocity_mm_per_sec = (self.current_rpm * wire_diameter) / 60.0
        
        # 3. Generate moves for next 200ms
        moves = self.generate_lookahead_moves(velocity_mm_per_sec, 0.2)  # 200ms
        
        # 4. Send to Pico
        for move in moves:
            self.send_move_to_pico(move)
            
    def generate_lookahead_moves(self, velocity, duration):
        """Generate smooth trapezoid velocity profile"""
        steps_per_sec = velocity * steps_per_mm
        total_steps = int(steps_per_sec * duration)
        
        # Break into 50ms chunks
        chunks = []
        for i in range(4):  # 4 × 50ms = 200ms
            chunk_steps = total_steps // 4
            chunk = {
                'steps': chunk_steps,
                'interval_us': int(1000000 / steps_per_sec),
                'accel': 0  # Can add acceleration here
            }
            chunks.append(chunk)
        return chunks
```

## Immediate Test Fix

Let's test if offloading helps **right now** by moving the sync logic to Pi Zero:

### Modified Pi Zero Script (simple_control.py)

```python
import serial
import time
import threading

class PiZeroWindingController:
    def __init__(self, port="/dev/tty.usbmodem314101"):
        self.ser = serial.Serial(port, 115200, timeout=0.1)
        self.current_rpm = 0.0
        self.running = False
        self.wire_diameter = 0.056
        self.steps_per_mm = 6135.0
        
    def monitor_rpm(self):
        """Background thread to monitor RPM from Pico"""
        while self.running:
            line = self.ser.readline().decode('utf-8', errors='ignore').strip()
            if "[RPM]" in line:
                try:
                    # Parse RPM from Pico status
                    rpm = float(line.split("RPM:")[1].split()[0])
                    self.current_rpm = rpm
                except:
                    pass
                    
    def generate_traverse_chunks(self):
        """Generate traverse chunks on Pi Zero (not Pico!)"""
        while self.running:
            if self.current_rpm > 50:  # Only if spindle is spinning
                # Calculate traverse velocity
                velocity_mm_per_sec = (self.current_rpm * self.wire_diameter) / 60.0
                steps_per_sec = velocity_mm_per_sec * self.steps_per_mm
                
                # Generate 50ms worth of steps
                steps_for_50ms = int(steps_per_sec * 0.05)
                
                if steps_for_50ms > 0:
                    # Send to Pico
                    interval_us = int(1000000 / steps_per_sec)
                    cmd = f"PUSH_CHUNK {steps_for_50ms} {interval_us}\n"
                    self.ser.write(cmd.encode())
                    
            time.sleep(0.05)  # 50ms sync rate
            
    def start_winding(self, turns, rpm, wire_dia, width, offset):
        """Start winding with Pi Zero handling motion planning"""
        self.wire_diameter = wire_dia
        self.running = True
        
        # Start RPM monitor thread
        rpm_thread = threading.Thread(target=self.monitor_rpm)
        rpm_thread.daemon = True
        rpm_thread.start()
        
        # Start chunk generation thread
        chunk_thread = threading.Thread(target=self.generate_traverse_chunks)
        chunk_thread.daemon = True
        chunk_thread.start()
        
        # Send WIND command to Pico (Pico still controls spindle)
        cmd = f"WIND T{turns} S{rpm} W{wire_dia} B{width} O{offset}\n"
        self.ser.write(cmd.encode())
```

## Benefits of Pi Zero Handling Motion

1. **More CPU Power**: Pi Zero has ~1GHz CPU vs Pico's 133MHz
2. **Python Flexibility**: Easier to implement complex motion planning
3. **No Real-time Constraints**: Can take 5-10ms for calculations
4. **Better Debugging**: Can log/visualize motion profiles
5. **Scalability**: Can easily handle multiple steppers

## What Stays on Pico

1. **Spindle Control**: PWM, RPM measurement, Hall sensor reading
2. **Step Execution**: 20kHz ISR for precise timing
3. **Emergency Stop**: Real-time safety responses
4. **Homing**: Low-level limit switch handling
5. **Queue Management**: MoveQueue execution only

## Implementation Priority

### Quick Test (This Upload)
- Fix the first-sync bug (prevents 779K step generation)
- Test if basic motion works

### Next Step (If Still Failing)
- Move `sync_traverse_to_spindle()` logic to Pi Zero
- Pico becomes "dumb step executor"
- Pi Zero sends pre-computed chunks via UART

### Long Term (Klipper-Style)
- Implement lookahead buffer on Pi Zero
- Clock synchronization between Pi and Pico
- Trapezoid velocity profiles
- Multi-stepper coordination

## Why This Will Work

**Klipper Example**: A Raspberry Pi Zero can easily control:
- 4 stepper motors
- 2 extruders
- Heated bed
- Multiple temperature sensors
- All at 100+ mm/s speeds

**Why?** Because the heavy math is on the Pi, and the MCU just executes pre-computed commands.

**Our Case**: We only have:
- 1 stepper (traverse)
- 1 BLDC (spindle)
- 1 sensor (Hall effect)

This should be **trivial** for a Klipper-style architecture!

---

**Conclusion**: First test the bug fix. If it still fails, we move motion planning to Pi Zero where it belongs. The Pico should be a "dumb" step executor, not trying to do real-time motion control calculations.

