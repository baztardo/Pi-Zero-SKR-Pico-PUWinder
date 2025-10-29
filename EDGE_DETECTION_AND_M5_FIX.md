# Edge Detection and M5 Stop Fix

## Date: October 29, 2024

## Problem Summary

The winding system had two critical issues:

### 1. No Edge Detection (Traverse Never Reverses)
- The traverse would move in one direction during winding and never reverse
- Eventually it would hit the home switch and crash
- Root cause: `current_traverse_position_mm` was set once at startup but never updated during winding
- Without position tracking, edge detection couldn't work

### 2. M5 Command Doesn't Stop Winding
- The M5 (spindle stop) command only stopped the spindle motor
- The traverse motor would keep moving even after M5
- The winding controller state was not reset

## Solutions Implemented

### Fix 1: Edge Detection with Position Tracking

#### Added to `sync_traverse_to_spindle()` (winding_controller.cpp):

**A. Edge Detection Logic (at start of function):**
```cpp
// Check for layer edge and reverse direction if needed
float edge_margin = 0.5f;  // 0.5mm safety margin from edges
float left_limit = params.start_position_mm + edge_margin;
float right_limit = params.start_position_mm + params.layer_width_mm - edge_margin;

if (traverse_direction && current_traverse_position_mm >= right_limit) {
    // Hit right edge - reverse to LEFT
    printf("[SYNC] ⚠️  RIGHT EDGE DETECTED at %.3f mm - REVERSING to LEFT\n", 
           current_traverse_position_mm);
    traverse_direction = false;
    move_queue->set_direction(traverse_direction);
    current_layer++;
    turns_this_layer = 0;
} else if (!traverse_direction && current_traverse_position_mm <= left_limit) {
    // Hit left edge - reverse to RIGHT
    printf("[SYNC] ⚠️  LEFT EDGE DETECTED at %.3f mm - REVERSING to RIGHT\n", 
           current_traverse_position_mm);
    traverse_direction = true;
    move_queue->set_direction(traverse_direction);
    current_layer++;
    turns_this_layer = 0;
}
```

**Key Points:**
- Edge detection happens **first** in every sync cycle
- Runs even if queue is full (so direction reversal is never blocked)
- Uses 0.5mm safety margin to prevent hitting physical limits
- Updates layer count and resets turn counter on reversal

**B. Position Tracking Update (after pushing chunks):**
```cpp
// Update position based on steps we QUEUED
float distance_moved_mm = (float)abs(steps_to_generate) / steps_per_mm;
if (traverse_direction) {
    current_traverse_position_mm += distance_moved_mm;  // Moving RIGHT
} else {
    current_traverse_position_mm -= distance_moved_mm;  // Moving LEFT
}

if (should_print) {
    printf("[SYNC] Position updated: %.3f mm (moved %.3f mm %s)\n",
           current_traverse_position_mm, distance_moved_mm,
           traverse_direction ? "RIGHT" : "LEFT");
}
```

**Key Points:**
- Position is updated based on **queued steps** (not executed steps)
- This provides predictive edge detection
- Direction is properly accounted for (+/- movement)
- Debug prints show position updates (every 20th cycle)

### Fix 2: M5 Command Now Stops Everything

#### Updated `execute_m5()` (gcode_interface.cpp):

```cpp
bool GCodeInterface::execute_m5() {
    if (!spindle_controller) {
        set_error("ERROR_SPINDLE_NOT_INIT");
        return false;
    }
    
    // ⭐ CRITICAL: Stop the winding controller first (if running)
    if (winding_controller) {
        WindingState current_state = winding_controller->get_state();
        if (current_state != WindingState::IDLE && current_state != WindingState::COMPLETE) {
            printf("[M5] Stopping winding controller (state=%d)...\n", (int)current_state);
            winding_controller->stop();
        }
    }
    
    // ⭐ CRITICAL: Stop the MoveQueue (traverse motor)
    if (move_queue && move_queue->is_active()) {
        printf("[M5] Stopping traverse motor (MoveQueue)...\n");
        move_queue->clear_queue();
        move_queue->set_enable(false);  // Disable motor
    }
    
    // Stop the spindle motor
    spindle_controller->set_pwm_duty(0.0f);  // Stop PWM
    spindle_controller->set_brake(true);     // Engage brake
    printf("✓ M5: All motion stopped (Spindle=OFF, Traverse=OFF, Winding=OFF)\n");
    send_response("OK");
    return true;
}
```

**Key Points:**
1. **Winding Controller**: Stops winding state machine if not IDLE/COMPLETE
2. **MoveQueue**: Clears all queued chunks and disables traverse motor
3. **Spindle Motor**: Stops PWM and engages brake
4. Comprehensive status message confirms all systems stopped

## How It Works Together

### During Normal Winding:

1. **Every 50ms** (`sync_traverse_to_spindle()` is called):
   - Check current position vs layer boundaries
   - If at edge, reverse direction and update `MoveQueue` direction
   - Check queue depth (skip if >100 to prevent overflow)
   - Calculate steps needed based on current RPM
   - Push step chunks to queue
   - Update `current_traverse_position_mm` based on queued steps

2. **ISR (20kHz)** executes queued steps:
   - Steps are executed from queue
   - Position calculated in sync function is predictive (tracks where traverse will be)

3. **Edge detection is predictive**:
   - We calculate where the traverse will be after current chunks execute
   - If that position exceeds boundaries, we reverse direction NOW
   - This prevents overshoot while accounting for queue lag

### When M5 is Called:

1. Winding controller transitions to IDLE/COMPLETE state
2. All queued traverse chunks are cleared
3. Traverse motor is disabled
4. Spindle stops and brake engages
5. System is ready for next command

## Expected Behavior

### Successful Winding:
```
[SYNC] Position updated: 29.500 mm (moved 0.048 mm RIGHT)
[SYNC] Position updated: 30.123 mm (moved 0.047 mm RIGHT)
...
[SYNC] Position updated: 40.456 mm (moved 0.048 mm RIGHT)
[SYNC] ⚠️  RIGHT EDGE DETECTED at 40.502 mm - REVERSING to LEFT
[SYNC] Position updated: 40.454 mm (moved 0.048 mm LEFT)
[SYNC] Position updated: 40.406 mm (moved 0.048 mm LEFT)
...
[SYNC] Position updated: 29.548 mm (moved 0.047 mm LEFT)
[SYNC] ⚠️  LEFT EDGE DETECTED at 29.500 mm - REVERSING to RIGHT
```

### M5 Emergency Stop:
```
[M5] Stopping winding controller (state=3)...
[M5] Stopping traverse motor (MoveQueue)...
✓ M5: All motion stopped (Spindle=OFF, Traverse=OFF, Winding=OFF)
RESPONSE: OK
```

## Files Modified

1. **pico_firmware/src/winding_controller.cpp**
   - Added edge detection logic at start of `sync_traverse_to_spindle()`
   - Added position tracking update after pushing chunks

2. **pico_firmware/src/gcode_interface.cpp**
   - Updated `execute_m5()` to stop winding controller and move queue

## Firmware Build

Location: `/Users/ssnow/Documents/GitHub/Pi-Zero-SKR-Pico-PUWinder/pico_firmware/build/pico_spindle_controller.uf2`

Size: 269K

Compiled: October 29, 2024

## Testing Instructions

1. **Upload Firmware:**
   - Hold BOOTSEL button on Pico
   - Connect USB
   - Copy `.uf2` file to RPI-RP2 drive
   - Pico will reboot automatically

2. **Test Edge Detection:**
   - Send: `WIND T500 S100 W0.056 B12 O29`
   - Observe serial output for edge detection messages
   - Verify traverse reverses at ~29.5mm and ~41mm positions
   - Monitor layer count increments

3. **Test M5 Stop:**
   - During winding, send: `M5`
   - Verify all motion stops immediately
   - Check serial output confirms all systems stopped

## Known Limitations

1. **Position tracking is predictive**: Tracks where traverse will be after queue executes, not where it currently is
2. **Edge margin hardcoded**: 0.5mm safety margin from edges (could be made configurable)
3. **No position recovery**: If position tracking drifts, there's no homing correction during winding

## Next Steps (Optional Improvements)

1. Add position feedback from executed steps (via `move_queue->get_step_count()`)
2. Make edge margin configurable via WIND command
3. Add mid-winding position correction/verification
4. Implement soft limits to prevent hard crashes
5. Add position display in status updates

