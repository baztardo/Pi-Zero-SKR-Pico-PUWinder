# Workload Distribution: Pico vs Pi Zero

## Current Architecture Analysis

### What the Pico Does NOW (Every Cycle)

#### Real-Time Critical (MUST stay on Pico):
1. **ISR @ 20kHz** - Step pulse generation
   - Time: ~5µs per call
   - Load: 10% CPU (5µs × 20,000 = 100ms per second)
   - **CANNOT MOVE** - Hardware timing critical

2. **Edge Detection** @ 20Hz (every 50ms)
   - Compare position to limits
   - Reverse direction if needed
   - Time: ~2µs
   - **SHOULD STAY** - Needs instant response

3. **Queue Management** @ 20Hz
   - Check queue depth
   - Push new chunks
   - Time: ~10µs
   - **SHOULD STAY** - Tightly coupled to ISR

#### Complex Math (CAN move to Pi Zero):
4. **Velocity Calculation** @ 20Hz (every 50ms)
   ```cpp
   float rpm = spindle_motor->get_rpm();
   float traverse_velocity = rpm * wire_diameter / 60.0f;
   float steps_per_sec = traverse_velocity * steps_per_mm;
   int32_t steps_to_generate = steps_per_sec * delta_time_sec;
   ```
   - Time: ~50µs (floating point is SLOW on RP2040)
   - **CAN MOVE** - Not time-critical

5. **StepCompressor** @ 20Hz
   ```cpp
   auto chunks = StepCompressor::compress_constant_velocity(steps, speed);
   ```
   - Time: ~100µs (memory allocation, loop)
   - **CAN MOVE** - Heavy computation

6. **Status Formatting** @ 10Hz
   ```cpp
   snprintf(buffer, "Status: Layer %u/%u, Turns %u/%u...");
   ```
   - Time: ~200µs (string formatting is EXPENSIVE)
   - **CAN MOVE** - Not time-critical

7. **RPM Calculation** @ 100Hz
   ```cpp
   float rpm = (60.0 × 1000000.0) / (pulse_interval × pulses_per_rev);
   ```
   - Time: ~30µs
   - **CAN MOVE** - Just needs pulse timestamps

---

## Proposed New Architecture

### Option A: Command-Based Offloading (Simple)

**Pi Zero becomes the "Motion Planner"**
**Pico becomes the "Motion Executor"**

#### What Pi Zero Does:
```python
class MotionPlanner:
    def __init__(self):
        self.position_mm = 0.0
        self.direction = True  # RIGHT
        self.layer_width = 12.0
        self.start_pos = 22.0
        
    def calculate_next_move(self, current_rpm, wire_diameter, time_slice_ms):
        """Calculate how many steps to move in this time slice"""
        
        # Check edge detection
        if self.direction and self.position_mm >= (self.start_pos + self.layer_width - 0.5):
            self.direction = False  # Reverse to LEFT
            print(f"Edge detected at {self.position_mm}mm - reversing LEFT")
            
        elif not self.direction and self.position_mm <= (self.start_pos + 0.5):
            self.direction = True  # Reverse to RIGHT
            print(f"Edge detected at {self.position_mm}mm - reversing RIGHT")
        
        # Calculate velocity
        traverse_velocity_mm_per_min = current_rpm * wire_diameter
        traverse_velocity_mm_per_sec = traverse_velocity_mm_per_min / 60.0
        
        # Calculate steps for this time slice
        steps_per_mm = 6135.0
        distance_mm = traverse_velocity_mm_per_sec * (time_slice_ms / 1000.0)
        steps = int(distance_mm * steps_per_mm)
        
        # Update position (predictive)
        if self.direction:
            self.position_mm += distance_mm
        else:
            self.position_mm -= distance_mm
            steps = -steps  # Negative for LEFT
        
        # Calculate step interval
        steps_per_sec = traverse_velocity_mm_per_sec * steps_per_mm
        interval_us = int(1000000.0 / steps_per_sec) if steps_per_sec > 0 else 1000
        
        return {
            'steps': abs(steps),
            'direction': self.direction,
            'interval_us': interval_us
        }

# Usage:
planner = MotionPlanner()
while winding:
    # Get current RPM from Pico
    rpm = pico.get_rpm()
    
    # Calculate next move (heavy math on Pi Zero)
    move = planner.calculate_next_move(rpm, wire_diameter=0.056, time_slice_ms=50)
    
    # Send simple command to Pico
    pico.send_command(f"MOVE_STEPS S{move['steps']} D{move['direction']} I{move['interval_us']}")
    
    time.sleep(0.05)  # 50ms update rate
```

#### What Pico Does:
```cpp
// NEW: Simple move command handler
bool GCodeInterface::execute_move_steps() {
    // Parse: "MOVE_STEPS S258 D1 I194"
    int32_t steps = get_param('S');      // 258 steps
    bool direction = get_param('D');     // 1 = RIGHT
    uint32_t interval = get_param('I');  // 194µs between steps
    
    // Validate
    if (steps <= 0 || steps > 10000) {
        set_error("INVALID_STEPS");
        return false;
    }
    
    // Create chunk (NO COMPRESSION - Pi did the math!)
    MoveChunk chunk = {
        .count = (uint32_t)steps,
        .interval_us = interval,
        .add_us = 0
    };
    
    // Set direction and push
    move_queue->set_direction(direction);
    bool success = move_queue->push_chunk(chunk);
    
    send_response(success ? "OK" : "QUEUE_FULL");
    return success;
}
```

**Benefits:**
- ✅ Pico CPU load drops ~40% (no float math, no compression)
- ✅ Pi Zero has full visibility of position/state
- ✅ Easier to tune/debug (Python vs C++)
- ✅ Can add complex features (acceleration profiles, look-ahead planning)

**Drawbacks:**
- ⚠️ Latency: 50ms command loop (vs 50ms currently, so same)
- ⚠️ Serial bandwidth: ~20 commands/sec (easily handled by 115200 baud)

---

### Option B: Smart Streaming (Advanced)

**Pi Zero pre-calculates entire layer as a "G-code" file**

```python
class LayerPlanner:
    def plan_layer(self, start_mm, width_mm, wire_diameter, rpm):
        """Pre-calculate all moves for one complete layer pass"""
        moves = []
        position = start_mm
        direction = True  # RIGHT
        
        # Calculate constant velocity for this layer
        velocity = (rpm * wire_diameter) / 60.0  # mm/s
        steps_per_sec = velocity * 6135.0
        interval_us = int(1000000.0 / steps_per_sec)
        
        # Plan moves in 1mm increments (or smaller)
        step_size_mm = 1.0
        steps_per_move = int(step_size_mm * 6135)
        
        while True:
            # Moving RIGHT
            while position < (start_mm + width_mm - 0.5):
                moves.append({
                    'steps': steps_per_move,
                    'direction': True,
                    'interval': interval_us
                })
                position += step_size_mm
            
            # Reverse at right edge
            moves.append({'reverse': True})
            direction = False
            
            # Moving LEFT
            while position > (start_mm + 0.5):
                moves.append({
                    'steps': steps_per_move,
                    'direction': False,
                    'interval': interval_us
                })
                position -= step_size_mm
            
            # Reverse at left edge
            moves.append({'reverse': True})
            direction = True
            
            # Complete one layer
            break
        
        return moves

# Usage:
planner = LayerPlanner()
moves = planner.plan_layer(start_mm=22, width_mm=12, wire_diameter=0.056, rpm=900)

# Stream to Pico (fills queue ahead of time)
for move in moves:
    if 'reverse' in move:
        pico.send_command("REVERSE")
    else:
        pico.send_command(f"MOVE_STEPS S{move['steps']} D{move['direction']} I{move['interval']}")
```

**Benefits:**
- ✅ Pico becomes ultra-simple (just executes commands)
- ✅ Can implement complex features (acceleration, jerk control, look-ahead)
- ✅ Perfect for repeatable patterns

**Drawbacks:**
- ⚠️ Less adaptive to RPM changes mid-winding
- ⚠️ More complex Pi Zero code

---

### Option C: Hybrid (Recommended)

**Pi Zero: High-level planning**
**Pico: Real-time adjustments**

```python
# Pi Zero: Calculate target velocity every 100ms
class HybridPlanner:
    def update_target_velocity(self, rpm, wire_diameter):
        """Send target velocity to Pico, let it handle micro-adjustments"""
        velocity_mm_per_sec = (rpm * wire_diameter) / 60.0
        steps_per_sec = int(velocity_mm_per_sec * 6135.0)
        
        # Send target to Pico
        pico.send_command(f"SET_VELOCITY V{steps_per_sec}")
        
    def check_and_reverse(self, position_mm):
        """Monitor position and command reversals"""
        if position_mm >= 33.5:  # Right edge
            pico.send_command("REVERSE LEFT")
        elif position_mm <= 22.5:  # Left edge
            pico.send_command("REVERSE RIGHT")
```

```cpp
// Pico: Simple velocity follower
class VelocityFollower {
    uint32_t target_steps_per_sec = 0;
    
    void set_target_velocity(uint32_t steps_per_sec) {
        target_steps_per_sec = steps_per_sec;
    }
    
    void update() {
        // Generate chunks at target velocity (simple, fast)
        if (target_steps_per_sec > 0) {
            uint32_t interval_us = 1000000 / target_steps_per_sec;
            
            MoveChunk chunk = {
                .count = 500,  // Fixed chunk size
                .interval_us = interval_us,
                .add_us = 0
            };
            
            move_queue->push_chunk(chunk);
        }
    }
};
```

**Benefits:**
- ✅ Best of both worlds
- ✅ Pico stays responsive (handles micro-timing)
- ✅ Pi Zero handles heavy lifting (velocity calc, edge logic)
- ✅ Adaptive to RPM changes (Pico can interpolate)

---

## Current CPU Load Analysis

### Pico RP2040 @ 125MHz (estimated):

| Task | Frequency | Time per call | CPU % |
|------|-----------|---------------|-------|
| ISR (stepping) | 20kHz | 5µs | 10% |
| sync_traverse | 20Hz | 300µs | 0.6% |
| - Float math | 20Hz | 50µs | 0.1% |
| - StepCompressor | 20Hz | 100µs | 0.2% |
| - Queue ops | 20Hz | 150µs | 0.3% |
| RPM calculation | 100Hz | 30µs | 0.3% |
| Status updates | 10Hz | 200µs | 0.2% |
| Main loop overhead | 1kHz | 50µs | 5% |
| **TOTAL** | | | **~16%** |

**Current load: 16% CPU (PLENTY of headroom!)**

### If we offload to Pi Zero:

| Task | Frequency | Time per call | CPU % |
|------|-----------|---------------|-------|
| ISR (stepping) | 20kHz | 5µs | 10% |
| Command parsing | 20Hz | 50µs | 0.1% |
| Queue push | 20Hz | 20µs | 0.04% |
| Main loop overhead | 1kHz | 50µs | 5% |
| **TOTAL** | | | **~15%** |

**Offloaded load: 15% CPU (only 1% savings)**

---

## Recommendation

### For Current System: **DON'T OFFLOAD YET**

**Why:**
1. **Pico has plenty of headroom** (16% CPU usage)
2. **Offloading adds complexity** (communication protocol, sync issues)
3. **Current architecture works** (as designed)

### When to Offload:

#### Scenario 1: Adding Advanced Features
If you want to add:
- **Acceleration/deceleration profiles** (complex math)
- **Multi-layer patterns** (zigzag, cross-wind)
- **Tension control** (PID loops with sensor fusion)
- **Recipe management** (load/save winding patterns)

→ Use **Option A** (Command-Based) or **Option C** (Hybrid)

#### Scenario 2: UI/Monitoring Overhead
If you add:
- **Live graphing** (position vs time plots)
- **Web interface** (WiFi status page)
- **Data logging** (CSV files)
- **Camera integration** (computer vision alignment)

→ Keep motion on Pico, run UI on Pi Zero (already the case!)

#### Scenario 3: You Hit CPU Limits
If you see:
- ISR taking too long (>10µs)
- Missed steps
- Queue overflows

→ Profile and optimize Pico code FIRST, offload SECOND

---

## What You CAN Offload Right Now (Easy Wins)

### 1. Status Formatting → Pi Zero
```cpp
// OLD (Pico):
snprintf(buffer, "Status: Layer %u/%u, Turns %u/%u, RPM %.1f", ...);
uart_puts(buffer);

// NEW (Pico):
uart_printf("STATUS %u %u %u %u %d\n", layer, max_layers, turns, max_turns, (int)rpm);

// NEW (Pi Zero):
def parse_status(data):
    parts = data.split()
    status = {
        'layer': int(parts[1]),
        'turns': int(parts[3]),
        'rpm': int(parts[5])
    }
    print(f"Status: Layer {status['layer']}/{parts[2]}, Turns {status['turns']}/{parts[4]}, RPM {status['rpm']}")
```
**Savings: ~180µs per status update (0.18% CPU)**

### 2. Error Messages → Pi Zero
```cpp
// OLD (Pico):
printf("❌ ERROR: Queue depth %u - skipping chunk generation\n", depth);

// NEW (Pico):
uart_puts("ERR:QUEUE_FULL\n");

// NEW (Pi Zero):
ERROR_MESSAGES = {
    'QUEUE_FULL': '❌ ERROR: Queue is full - skipping chunk generation',
    'EDGE_LEFT': '⚠️  Left edge detected - reversing to RIGHT',
    # ... more friendly messages
}
```
**Savings: ~150µs per error (varies)**

### 3. Configuration → Pi Zero
```python
# Pi Zero: Load winding recipe from file
recipe = {
    'wire_diameter': 0.056,
    'layer_width': 12.0,
    'start_offset': 22.0,
    'turns': 500,
    'rpm': 100,
    'edge_margin': 0.5,
    'ramp_time': 2.0
}

# Send to Pico as compact command
pico.send_command(f"WIND T{recipe['turns']} S{recipe['rpm']} W{recipe['wire_diameter']} ...")
```
**Benefit: User can edit recipes without recompiling firmware**

---

## Summary

### Current Status: ✅ PICO IS NOT BOGGED DOWN
- CPU load: ~16%
- Plenty of headroom for current tasks

### When to Offload:
1. ❌ **NOT NOW** - System is efficient
2. ✅ **LATER** - When adding advanced features (acceleration, patterns, etc.)
3. ✅ **ALWAYS** - Keep UI/logging/web on Pi Zero (already doing this)

### Easy Wins (Low effort, small gains):
1. Move status string formatting to Pi Zero (~0.2% CPU savings)
2. Move error messages to Pi Zero (cleaner code)
3. Move recipe management to Pi Zero (better UX)

### Big Wins (More effort, when needed):
1. Implement **Option A** (Command-Based) when adding acceleration
2. Implement **Option C** (Hybrid) for advanced multi-layer patterns
3. Keep motion control on Pico (it's designed for this!)

**Bottom line: Your current architecture is good. Don't fix what ain't broke!** 🎯

Focus on testing the edge detection first, THEN optimize if you hit limits.

