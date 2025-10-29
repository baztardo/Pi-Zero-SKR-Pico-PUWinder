# Motion System Architecture - Complete Explanation

## Your Questions Answered

### Q1: Are we using Absolute Position?
**YES** - Everything is measured from home position (0mm)

### Q2: Incremental or Absolute?
**INCREMENTAL packets, ABSOLUTE tracking**
- Each packet to MoveQueue = "move X steps at Y speed"
- Position tracking = absolute from home (0mm)

### Q3: Who's Tracking Everything?
**The Pico tracks everything in real-time. Pi Zero is just the UI/interface.**

---

## Complete Motion Flow (With Your Example)

### Physical Setup
```
Home Switch (0mm) ---- Air Gap ---- Start (22mm) ========== End (34mm) ---- Limit (120mm)
                                         |<-- 12mm bobbin -->|
```

### Step-by-Step Motion Flow

#### 1️⃣ **HOMING** (Once at startup)
```
TraverseController moves LEFT until home switch triggers
Position set to: current_traverse_position_mm = 0.0mm
State: HOMING → MOVING_TO_START
```

#### 2️⃣ **MOVE TO START** (Move from 0mm → 22mm)
```
WindingController calculates:
- Current position: 0.0mm
- Target position: 22.0mm (params.start_position_mm)
- Distance: 22.0mm
- Steps needed: 22.0mm × 6135 steps/mm = 134,970 steps

Action:
1. StepCompressor breaks this into chunks (typically 1-3 chunks)
2. Each chunk = {steps: 50000, interval: 333µs, accel: 0}
3. Push chunks to MoveQueue
4. Update position: current_traverse_position_mm = 22.0mm
5. State: MOVING_TO_START → RAMPING_UP
```

#### 3️⃣ **WINDING STARTS** (Continuous motion 22mm ↔ 34mm)

**Every 50ms, `sync_traverse_to_spindle()` is called:**

**Example at RPM=900, Wire=0.056mm:**

```
Calculation:
- RPM: 900
- Traverse speed needed: 900 RPM × 0.056mm = 50.4 mm/min = 0.84 mm/s
- Steps per second: 0.84 mm/s × 6135 steps/mm = 5,153 steps/s
- Time slice: 50ms = 0.05s
- Steps this cycle: 5,153 × 0.05 = 258 steps

Action (every 50ms):
1. Check edge detection:
   - Current position: 22.5mm
   - Right limit: 34mm - 0.5mm = 33.5mm
   - Left limit: 22mm + 0.5mm = 22.5mm
   - Status: OK, continue moving RIGHT

2. Check queue depth:
   - Queue depth: 45/128
   - Status: OK (< 100)

3. Generate step packet:
   - Steps: 258 steps
   - Direction: RIGHT (traverse_direction = true)
   - Speed: 5,153 steps/s → interval = 194µs between steps

4. Compress into chunk:
   - StepCompressor creates: {steps: 258, interval: 194, accel: 0}

5. Push to MoveQueue:
   - move_queue->push_chunk(chunk)
   - Queue now has 46 chunks

6. Update position (predictive):
   - Distance: 258 steps / 6135 steps/mm = 0.042mm
   - New position: 22.5mm + 0.042mm = 22.542mm

Result: 
- current_traverse_position_mm = 22.542mm
- Packet queued for execution
```

**Next cycle (50ms later):**
```
Position: 22.584mm (moved another 0.042mm)
Still moving RIGHT...
```

**After ~280 cycles (~14 seconds at 900 RPM):**
```
Edge Detection Triggers:
- Current position: 33.6mm
- Right limit: 33.5mm
- EDGE DETECTED!

Action:
1. Set traverse_direction = false (LEFT)
2. move_queue->set_direction(false)
3. Increment current_layer++
4. Reset turns_this_layer = 0

Next packet will move LEFT:
- Steps: 258 steps
- Direction: LEFT
- New position: 33.6mm - 0.042mm = 33.558mm
```

---

## The Packet System Explained

### What is a "Chunk"?
```cpp
struct MoveChunk {
    uint32_t count;       // Number of steps (e.g., 258)
    uint32_t interval_us; // Time between steps (e.g., 194µs)
    int32_t add_us;       // Acceleration (usually 0 for constant velocity)
};
```

### MoveQueue (The FIFO Buffer)
```
Capacity: 128 chunks
Current implementation: Keeps ~40-60 chunks queued

[Tail] → Chunk 1 (258 steps @ 194µs) → Executing NOW (ISR)
         Chunk 2 (260 steps @ 193µs) → Next
         Chunk 3 (255 steps @ 196µs) → Queued
         ...
         Chunk 45 (258 steps @ 194µs) → Queued
[Head] ← New chunks pushed here (every 50ms)
```

### Scheduler (The Heartbeat)
```
Hardware Timer Alarm 0
Frequency: 20kHz (50µs period)
Calls: move_queue->traverse_isr_handler()

Every 50µs:
1. Check if time to step (based on chunk interval)
2. If yes:
   - Toggle STEP pin
   - Decrement chunk.count
   - If chunk.count == 0, load next chunk from queue
3. Return (must be FAST - no printf!)
```

---

## Position Tracking: Who Knows What?

### Pico (Real-Time Controller)
```cpp
// Absolute position from home
float current_traverse_position_mm;  // e.g., 28.3mm

// Direction of travel
bool traverse_direction;  // true=RIGHT, false=LEFT

// Layer tracking
uint32_t current_layer;      // e.g., 2
uint32_t turns_this_layer;   // e.g., 45
uint32_t turns_completed;    // e.g., 145
```

**The Pico knows:**
- ✅ Exact absolute position (22-34mm range)
- ✅ Current direction (LEFT/RIGHT)
- ✅ Current layer number
- ✅ Turns completed
- ✅ Current RPM
- ✅ Queue depth

### Pi Zero (UI/Command Interface)
```python
# Sends commands:
controller.send_command("WIND T500 S100 W0.056 B12 O22")
controller.send_command("M5")  # Stop

# Receives status:
"Status: Layer 2/1, Turns 145/500, RPM 896.3"
```

**The Pi Zero knows:**
- ✅ What commands were sent
- ✅ Status reports from Pico
- ❌ Real-time position (not needed)
- ❌ Queue state (not needed)

**The Pi Zero does NOT control motion** - it's just a fancy serial terminal!

---

## Watchdog / Tracking Concerns

### Current Safety Mechanisms

#### 1. **Queue Depth Monitoring**
```cpp
if (queue_depth >= MAX_QUEUE_DEPTH) {
    // Skip this sync cycle - let ISR catch up
    return;
}
```
**Purpose:** Prevents queue overflow
**Status:** ✅ Implemented

#### 2. **Edge Detection**
```cpp
if (current_traverse_position_mm >= right_limit) {
    traverse_direction = false;  // Reverse
    move_queue->set_direction(false);
}
```
**Purpose:** Prevents hitting physical limits
**Status:** ✅ Just implemented

#### 3. **Emergency Stop (M5)**
```cpp
winding_controller->stop();
move_queue->clear_queue();
move_queue->set_enable(false);
spindle->set_brake(true);
```
**Purpose:** Stop all motion immediately
**Status:** ✅ Just implemented

### Missing Safety Features (Future Improvements)

#### 4. **Position Verification Watchdog** ⚠️ NOT IMPLEMENTED
```cpp
// Compare predicted vs actual position periodically
if (abs(predicted_position - actual_position) > 2.0mm) {
    // Position drift detected - trigger alarm
    emergency_stop();
}
```
**Why needed:** If steps are lost (motor stall, skipped steps), position tracking drifts

#### 5. **Motion Timeout Watchdog** ⚠️ NOT IMPLEMENTED
```cpp
// If no new chunks received for >500ms during winding
if (winding_active && (time - last_chunk_time) > 500000) {
    // Communication lost - stop motion
    emergency_stop();
}
```
**Why needed:** If Pico crashes or communication fails mid-winding

#### 6. **Hard Limit Switches** ⚠️ NOT IMPLEMENTED
```
Physical switches at:
- 0mm (home switch) ✅ EXISTS
- 120mm (right limit) ❌ NEEDED
```
**Why needed:** Backup if position tracking fails completely

---

## Do You Need These Extra Watchdogs?

### For Initial Testing: **NO**
Your current implementation has:
- ✅ Edge detection (software limits)
- ✅ Queue overflow prevention
- ✅ Emergency stop (M5)
- ✅ Home switch (prevents going too far left)

### For Production: **YES, Eventually**
Add these in priority order:

1. **Physical right limit switch** (hardware safety)
   - Triggers emergency stop if hit
   - Last line of defense

2. **Position verification** (periodic sanity check)
   - Every 100 turns, verify position makes sense
   - Flag if position > 120mm or < 0mm

3. **Communication watchdog** (detect Pico hang)
   - Pi Zero pings Pico every 1 second
   - If no response for 3 seconds → assume crash

---

## Summary: Who Does What

| Task | Component | Method |
|------|-----------|--------|
| **Position Tracking** | Pico | Absolute (from 0mm) |
| **Motion Commands** | Pico | Incremental (step packets) |
| **Direction Control** | Pico | Absolute (LEFT/RIGHT) |
| **Edge Detection** | Pico | Real-time (every 50ms) |
| **Step Execution** | Pico ISR | Hardware timer (20kHz) |
| **Safety Monitoring** | Pico | Queue depth, edges, M5 |
| **User Interface** | Pi Zero | Send commands, display status |
| **Ultimate Safety** | Home Switch | Hardware interrupt |

## Your Winding Example (Complete Trace)

```
T=0.0s: Home → 0mm
T=2.0s: Move to start → 22mm
T=3.0s: Spindle ramps to 900 RPM
T=3.5s: Start winding
        Position: 22.0mm, Direction: RIGHT
        
T=4.0s: Position: 22.5mm, Direction: RIGHT
T=5.0s: Position: 24.2mm, Direction: RIGHT
T=10.0s: Position: 30.8mm, Direction: RIGHT
T=14.0s: Position: 33.6mm, Direction: RIGHT
        ⚠️  EDGE DETECTED - REVERSE TO LEFT
        
T=14.5s: Position: 33.2mm, Direction: LEFT
T=18.0s: Position: 29.7mm, Direction: LEFT
T=24.0s: Position: 22.4mm, Direction: LEFT
        ⚠️  EDGE DETECTED - REVERSE TO RIGHT
        Layer++ (now Layer 2)
        
T=24.5s: Position: 22.8mm, Direction: RIGHT
... cycle repeats ...
```

Each 50ms sync generates ~258 steps (~0.042mm movement)
Each layer (22mm → 34mm → 22mm) = 24mm total = ~571 sync cycles = ~28 seconds @ 900 RPM

---

## Next Steps

1. **Test current implementation** - verify edge detection works
2. **Monitor position tracking** - watch for drift over many layers
3. **Add right limit switch** - physical safety (optional for now)
4. **Fine-tune edge margins** - adjust 0.5mm if needed

Your system is now complete for basic winding! 🎉

