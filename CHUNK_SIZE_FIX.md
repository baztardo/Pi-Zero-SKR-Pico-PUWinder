# Chunk Size Fix - Breaking Large Moves Into Smaller Chunks

## Problem Summary

After fixing the first-sync bug, a new issue emerged:
- The "move to start" command generated **122,700 steps in ONE chunk**
- At 333µs per step, this chunk takes **40.8 seconds** to execute
- Meanwhile, `sync_traverse_to_spindle()` generates new chunks every 50ms
- Result: Queue fills up faster than the ISR can process the giant first chunk

## Evidence from Logs

```
Diagnostic Output (after 10 seconds of winding):
  - ISR calls:           1,914,742  ✅ ISR running at 20kHz
  - Chunks loaded:       2          ❌ Only 2 chunks loaded!
  - Steps executed:      12,661     ❌ Only 12K of 122K steps done
  - Active Chunk:
    - Steps remaining:   120,035    ❌ Still executing first chunk!
  - Queue depth:         30/128     ⚠️  Filling up
```

After 10 seconds:
- Only 2 chunks have been loaded (the first 122K chunk + 1 winding chunk)
- Only 12,661 steps executed (10% of the first chunk)
- The queue is filling with winding chunks that can't be processed yet
- System enters "skipping chunks" mode because queue depth hits 100

## Root Cause

In `stepcompress.cpp`, line 103:

```cpp
std::vector<StepChunk> StepCompressor::compress_constant_velocity(
    uint32_t total_steps,
    double velocity,
    double max_err_us)
{
    // ...
    StepChunk c;
    c.interval_us = interval_us;
    c.add_us = 0;
    c.count = total_steps;  // ❌ ALL steps in ONE chunk!
    chunks.push_back(c);
    
    return chunks;
}
```

**For the "move to start" command:**
- `total_steps = 122,700` (20mm × 6,135 steps/mm)
- `interval_us = 333` (3,000 steps/sec = TRAVERSE_RAPID_SPEED)
- **Result**: One chunk with 122,700 steps

**Execution time:**
```
122,700 steps × 333µs/step = 40,839,000µs = 40.8 seconds
```

## The Fix

Modified `compress_constant_velocity()` to break large moves into smaller chunks:

```cpp
std::vector<StepChunk> StepCompressor::compress_constant_velocity(
    uint32_t total_steps,
    double velocity,
    double max_err_us)
{
    std::vector<StepChunk> chunks;
    
    if (total_steps == 0 || velocity <= 0) return chunks;
    
    uint32_t interval_us = (uint32_t)(1e6 / velocity);
    
    // ⭐ CRITICAL FIX: Break large moves into chunks of max 5000 steps
    // This prevents a single massive chunk from blocking the queue
    // At 3000 steps/sec, 5000 steps = 1.67 seconds per chunk (manageable)
    const uint32_t MAX_STEPS_PER_CHUNK = 5000;
    
    uint32_t remaining_steps = total_steps;
    while (remaining_steps > 0) {
        uint32_t chunk_steps = (remaining_steps > MAX_STEPS_PER_CHUNK) 
                              ? MAX_STEPS_PER_CHUNK 
                              : remaining_steps;
        
        StepChunk c;
        c.interval_us = interval_us;
        c.add_us = 0;  // No acceleration
        c.count = chunk_steps;
        chunks.push_back(c);
        
        remaining_steps -= chunk_steps;
    }
    
    return chunks;
}
```

## Expected Behavior After Fix

**For "move to start" (122,700 steps):**
- Breaks into: 122,700 ÷ 5,000 = **25 chunks**
  - 24 chunks of 5,000 steps each
  - 1 chunk of 2,700 steps
- Each chunk executes in: 5,000 × 333µs = **1.67 seconds**
- Total move time: Still ~40 seconds, but **non-blocking**

**Benefits:**
1. **Faster queue turnover**: ISR loads new chunks every 1.67 seconds instead of 40 seconds
2. **Better interleaving**: Winding chunks can be processed between "move to start" chunks
3. **Lower queue depth**: Queue doesn't fill up because chunks are consumed regularly
4. **Smoother operation**: System can adjust to changing conditions more quickly

## Why 5000 Steps Per Chunk?

**Trade-offs:**
- **Too small** (e.g., 100 steps): Too many chunks, overhead increases
- **Too large** (e.g., 100K steps): Blocks queue, prevents interleaving
- **5000 steps**: Sweet spot
  - At 3000 steps/sec: 1.67 seconds per chunk
  - At 6000 steps/sec: 0.83 seconds per chunk
  - Allows ~30 winding chunks (293 steps each) to execute between "move to start" chunks

## Expected Log Output After Fix

```
[WindingController] Generating 122700 steps (20.00 mm)
[WindingController] Generated 25 chunks  ✅ Not 1 chunk!
[WindingController] Pushed 25/25 chunks successfully
[WindingController] Queue depth: 25

[Monitor] Depth: 25/128 Active:Y  ✅ Under limit
...
[SYNC] Pushed 1/1 chunks, Queue depth: 20, Active: YES  ✅ Stable
[SYNC] Pushed 1/1 chunks, Queue depth: 18, Active: YES  ✅ Decreasing
[SYNC] Pushed 1/1 chunks, Queue depth: 16, Active: YES  ✅ Good!
```

## Why This Happens vs Klipper

**Klipper Architecture:**
- Pre-computes entire tool path on host (Pi)
- Breaks into optimal chunk sizes BEFORE sending to MCU
- MCU receives balanced, manageable chunks
- MCU never has to deal with massive single-chunk moves

**Our Current Architecture:**
- Pico generates chunks on-the-fly
- `StepCompressor` didn't have chunk size limits
- Large moves created large chunks
- No lookahead or balancing

**Long-term solution**: Move motion planning to Pi Zero (as discussed in `KLIPPER_STYLE_ARCHITECTURE.md`)

## Related Fixes

1. **First Sync Bug** (`FIRST_SYNC_BUG_FIX.md`): Prevented 779K step generation on first call
2. **Chunk Size Fix** (this document): Prevents single massive chunks from blocking queue
3. **Next**: If still failing, implement Pi Zero motion planning

## Files Modified

- `/pico_firmware/src/stepcompress.cpp` - Added MAX_STEPS_PER_CHUNK limit

## Upload Instructions

The compiled firmware is at:
```
/Users/ssnow/Documents/GitHub/Pi-Zero-SKR-Pico-PUWinder/pico_firmware/build/pico_spindle_controller.uf2
```

1. Hold BOOTSEL button on Pico while connecting USB
2. Copy `.uf2` file to `RPI-RP2` drive
3. Pico auto-reboots with fix
4. Test with `WIND T500 S100 W0.056 B12 O22`
5. Verify "Generated 25 chunks" in log (not "Generated 1 chunks")

---

**Date**: October 29, 2025  
**Impact**: Critical - Prevents queue overflow during winding  
**Status**: Fixed and compiled

