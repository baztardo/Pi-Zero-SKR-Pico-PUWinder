# First Sync Bug Fix - Massive Step Generation

## Problem Summary

The system was generating **779,164 steps** on the very first call to `sync_traverse_to_spindle()`, causing:
1. The traverse to move FAR beyond its intended range (from 20mm to 91mm, when it should only go from 22mm to 34mm)
2. The queue to fill up immediately and stay full
3. The traverse to keep moving past layer edges without reversing

## Root Cause

In `winding_controller.cpp`, line 469-471:

```cpp
static uint32_t last_sync_time = 0;
uint32_t current_time = time_us_32();
uint32_t delta_time_us = current_time - last_sync_time;
```

### The Bug

On the **very first call** to `sync_traverse_to_spindle()`:
- `last_sync_time` is initialized to `0`
- `current_time` might be something like `31,195,000` µs (31 seconds since Pico boot)
- `delta_time_us = 31,195,000 - 0 = 31,195,000` µs (**31 seconds!**)

### The Calculation

```cpp
float delta_time_sec = delta_time_us / 1000000.0f;  // = 31.195 seconds
int32_t steps_to_generate = (int32_t)(required_steps_per_sec * delta_time_sec);
```

With `required_steps_per_sec ≈ 5,870` steps/sec (at 895 RPM):
```
steps_to_generate = 5,870 × 31.195 = 183,134 steps
```

This explains why we saw **779,164 steps** in the log - the Pico had been running for about 132 seconds before the WIND command was issued.

### Why This Broke Everything

1. **Position Tracking**: The first sync moved `current_traverse_position_mm` by **127mm** (779,164 steps ÷ 6,135 steps/mm), jumping from 20mm to 147mm
2. **Queue Overflow**: 779,164 steps compressed into chunks filled the entire 128-slot queue immediately
3. **No Edge Detection**: Since the position jumped so far, the edge detection logic couldn't catch up
4. **Queue Stuck**: The queue stayed at depth 100 (the MAX_QUEUE_DEPTH threshold), preventing new chunks from being pushed

## The Fix

Added initialization check on first call:

```cpp
// Calculate how many steps to generate this update cycle
static uint32_t last_sync_time = 0;
uint32_t current_time = time_us_32();

// ⭐ CRITICAL: Initialize last_sync_time on first call
if (last_sync_time == 0) {
    last_sync_time = current_time;
    return;  // Skip first call to establish baseline
}

uint32_t delta_time_us = current_time - last_sync_time;
```

Now:
1. First call: Sets `last_sync_time = current_time` and returns without generating steps
2. Second call onwards: Calculates proper `delta_time_us` (should be ~50ms or 50,000 µs)
3. Normal operation: Generates ~293 steps per sync (5,870 steps/sec × 0.05 sec)

## Expected Behavior After Fix

- **First sync call**: Initializes timer, generates 0 steps
- **Subsequent calls**: Generate ~293 steps every 50ms
- **Position tracking**: Updates by ~0.048mm per sync (293 ÷ 6,135)
- **Edge detection**: Will trigger correctly when reaching layer limits
- **Queue depth**: Should stay below 100, allowing continuous operation

## Verification

After uploading the fixed firmware:
1. The first `[SYNC]` message should show a small step count (< 500)
2. `current_traverse_position_mm` should increment smoothly from 20mm to 34mm
3. Edge detection should trigger at ~33.5mm (right limit) and reverse direction
4. Queue depth should fluctuate normally instead of staying pinned at 100

## Related Files

- **Fixed**: `/pico_firmware/src/winding_controller.cpp` - Line 472-476
- **Compiled**: `/pico_firmware/build/pico_spindle_controller.uf2`

## Upload Instructions

1. Hold BOOTSEL button on Pico while connecting USB
2. Pico mounts as `RPI-RP2` drive
3. Copy `pico_spindle_controller.uf2` to the drive
4. Pico auto-reboots with fixed firmware
5. Connect via serial and send `WIND T500 S100 W0.056 B12 O22` command
6. Verify the first `[SYNC]` log shows reasonable step count

---

**Date**: October 29, 2025  
**Impact**: Critical - Prevents winding operation  
**Status**: Fixed and compiled

