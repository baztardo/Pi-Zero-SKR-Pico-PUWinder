# COMPLETE INTEGRATION GUIDE - WORKING SCHEDULER SYSTEM

## What This Fix Does

1. **Fixes the architectural conflict** between TraverseController and MoveQueue
2. **Enables proper handoff** - TraverseController for homing, MoveQueue for winding
3. **Adds comprehensive debugging** to see exactly what's happening
4. **Ensures scheduler ISR works correctly** with full visibility

## Files Provided

1. **main_complete.cpp** - Complete working main.cpp
2. **move_queue_complete.cpp** - Fixed move_queue.cpp with debug
3. **winding_controller_complete.cpp** - Fixed winding_controller.cpp with handoff
4. **traverse_controller_additions.md** - New method to add to TraverseController

## Installation Steps

### Step 1: Backup Current Code

```bash
cd pico_firmware
cp main.cpp main.cpp.backup
cp src/move_queue.cpp src/move_queue.cpp.backup
cp src/winding_controller.cpp src/winding_controller.cpp.backup
cp src/traverse_controller.h src/traverse_controller.h.backup
cp src/traverse_controller.cpp src/traverse_controller.cpp.backup
```

### Step 2: Replace Main Files

```bash
# Copy the complete working versions
cp /path/to/main_complete.cpp main.cpp
cp /path/to/move_queue_complete.cpp src/move_queue.cpp
cp /path/to/winding_controller_complete.cpp src/winding_controller.cpp
```

### Step 3: Add stop_steps() to TraverseController

**Edit src/traverse_controller.h:**

Find the public section and add:
```cpp
public:
    // ... existing methods ...
    void stop_steps();  // NEW - for MoveQueue handoff
```

**Edit src/traverse_controller.cpp:**

Add at the end of the file:
```cpp
void TraverseController::stop_steps() {
    moving = false;
    steps_remaining = 0;
    homing = false;
    printf("[TraverseController] Step generation stopped (MoveQueue takeover)\n");
}
```

### Step 4: Clean Build

```bash
cd pico_firmware
rm -rf build
mkdir build
cd build
cmake ..
make -j4
```

### Step 5: Upload Firmware

```bash
# Enter BOOTSEL mode:
# - Hold BOOTSEL button
# - Plug USB (or press RESET while holding BOOTSEL)
# - Release BOOTSEL

# Copy firmware
cp pico_uart_test.uf2 /Volumes/RPI-RP2/

# Or on Linux:
cp pico_uart_test.uf2 /media/$(whoami)/RPI-RP2/
```

## Testing Process

### Test 1: Verify Initialization

Connect to serial:
```bash
screen /dev/tty.usbmodem* 115200
```

You should see:
```
========================================
Pi Zero SKR Pico PUWinder - FIXED
========================================

Initializing spindle controller...
✓ Spindle controller initialized
Initializing traverse controller...
✓ Traverse controller initialized
  (Used for homing and manual moves only)
Initializing move queue (ISR-driven)...
✓ Move queue initialized
  (Handles stepping during winding)
Initializing scheduler...
✓ Scheduler started at 20kHz

========================================
SYSTEM READY
========================================
```

✅ **PASS** if you see all ✓ marks

### Test 2: Verify Scheduler ISR Running

After initialization, you should see periodic messages:
```
[MoveQueue-ISR] #2000: depth=0, active=NO, h=0, t=0
[MoveQueue-ISR] #4000: depth=0, active=NO, h=0, t=0
```

✅ **PASS** if you see these every ~0.1 seconds
❌ **FAIL** if no ISR messages → Scheduler not running

### Test 3: Test Homing

From Pi Zero, send homing command:
```bash
echo "G28" > /dev/serial0
```

You should see:
```
[TraverseController] Starting homing sequence...
[TraverseController] Phase 1: Moving towards home switch...
```

Motor should move towards home switch.

✅ **PASS** if motor homes normally
❌ **FAIL** if motor doesn't move → Check TraverseController::is_moving()

### Test 4: Test Winding

From Pi Zero, send WIND command:
```bash
echo "WIND 100 1000 0.1 10 10" > /dev/serial0
```

You should see extensive debug output:
```
[WindingController] ========================================
[WindingController] MOVING TO START POSITION
[WindingController] ========================================
[WindingController] Target: 10.00 mm
[WindingController] Stopping TraverseController step generation
[WindingController] MoveQueue taking control of traverse motor
[WindingController] Motor ENABLED via MoveQueue
[WindingController] Generated 1 chunks
[WindingController] Pushed 1/1 chunks successfully
[WindingController] Queue depth: 1

[MoveQueue-ISR] #6000: depth=1, active=NO, h=1, t=0
[MoveQueue-ISR] *** LOADING CHUNK ***: 61350 steps @ 163 us/step
[MoveQueue-ISR] Queue depth now: 0, Active: YES
[MoveQueue] ✓ Step #100 executed
[MoveQueue] ✓ Step #200 executed
```

✅ **PASS** if you see:
- Chunks being generated
- Chunks being pushed
- MoveQueue-ISR loading chunks
- Steps being executed
- Motor physically moving

❌ **FAIL** scenarios and diagnosis:

## Troubleshooting Guide

### Problem: No ISR messages at all

**Diagnosis:**
```
[MoveQueue-ISR] #2000: depth=0, active=NO, h=0, t=0
```
Does NOT appear

**Cause:** Scheduler ISR not running

**Fix:**
1. Check scheduler->start() returned true
2. Check PICO_SDK_PATH is set correctly
3. Verify hardware timer initialized
4. Check for compile errors in scheduler.cpp

---

### Problem: Chunks not being generated

**Diagnosis:**
```
[SYNC] Generated 0 chunks
```
OR no [SYNC] messages at all

**Cause:** sync_traverse_to_spindle() not being called or RPM too low

**Fix:**
1. Check spindle is spinning (RPM > 10)
2. Check winding_controller->update() is being called
3. Check state machine is in WINDING state

---

### Problem: Chunks generated but not pushed

**Diagnosis:**
```
[SYNC] Generated 1 chunks
[SYNC] Pushed 0/1 chunks
```

**Cause:** Queue full or push_chunk() failing

**Fix:**
1. Check queue isn't full (capacity=128)
2. Check for error message: "Queue FULL"
3. Clear queue if stuck: move_queue->clear_queue()

---

### Problem: Chunks pushed but queue depth stays 0

**Diagnosis:**
```
[SYNC] Pushed 1/1 chunks
[SYNC] Queue depth: 0
```

**Cause:** head/tail indices corrupted or queue being cleared

**Fix:**
1. Check no other code is calling clear_queue()
2. Check head/tail values in ISR debug
3. Add breakpoint in push_chunk() to verify head increments

---

### Problem: Queue has chunks but ISR not loading

**Diagnosis:**
```
[MoveQueue-ISR] #6000: depth=1, active=NO, h=1, t=0
```
But no "LOADING CHUNK" message

**Cause:** active_running stuck true, or ISR not entering load code

**Fix:**
1. Check if active_running is true when it shouldn't be
2. Add printf at start of traverse_isr_handler() to verify it's called
3. Check if feeding_paused is true (safety check)

---

### Problem: Chunks loaded but not executing steps

**Diagnosis:**
```
[MoveQueue-ISR] *** LOADING CHUNK ***: 100 steps @ 1000 us/step
```
But no "Step executed" messages

**Cause:** Timing check failing or execute_step_pulse() not called

**Fix:**
1. Check interval_us is reasonable (>50us typically)
2. Add printf in execute_step_pulse() to verify it's called
3. Check time_diff calculation isn't overflowing

---

### Problem: Steps executing but motor not moving

**Diagnosis:**
```
[MoveQueue] ✓ Step #100 executed
```
But no physical movement

**Cause:** Hardware issue

**Fix:**
1. Check ENA pin: Should be 0 (enabled)
   ```cpp
   printf("ENA pin state: %d\n", gpio_get(TRAVERSE_ENA_PIN));
   ```
   Should print 0, not 1

2. Check STEP pin pulsing with multimeter/scope
   - Should see 3.3V pulses on GPIO 6
   - 5us pulse width

3. Check DIR pin state
   ```cpp
   printf("DIR pin state: %d\n", gpio_get(TRAVERSE_DIR_PIN));
   ```

4. Check TMC2209 driver:
   - Power LED on
   - VREF set correctly
   - Motor wires connected

5. Test with different motor to rule out bad motor

## Expected Normal Output

When everything works, you should see:

```
[WindingController] MOVING TO START POSITION
[WindingController] Pushed 1/1 chunks successfully
[WindingController] Queue depth: 1

[MoveQueue-ISR] #2000: depth=1, active=NO, h=1, t=0
[MoveQueue-ISR] *** LOADING CHUNK ***: 61350 steps @ 163 us/step
[MoveQueue-ISR] #4000: depth=0, active=YES, h=1, t=1

[MoveQueue] ✓ Step #100 executed
[MoveQueue] ✓ Step #200 executed
[MoveQueue] ✓ Step #300 executed

[SYNC] ========================================
[SYNC] Sync #10
[SYNC] Spindle RPM: 901.4
[SYNC] Traverse velocity: 0.961 mm/s
[SYNC] Steps to generate: 59
[SYNC] Generated 1 chunks
[SYNC] Pushed 1/1 chunks
[SYNC] Queue depth: 1
[SYNC] ========================================

[MoveQueue-ISR] *** LOADING CHUNK ***: 59 steps @ 16393 us/step
[MoveQueue] ✓ Step #400 executed
```

AND:
- FAN1 LED blinking (scheduler ISR running)
- FAN2 LED flashing rapidly (steps executing)
- Motor physically moving smoothly

## Success Criteria Checklist

- [ ] Initialization completes with all ✓ marks
- [ ] ISR heartbeat messages appear every ~0.1 sec
- [ ] Homing works normally
- [ ] Winding generates chunks
- [ ] Chunks successfully pushed to queue
- [ ] Queue depth increases when chunks pushed
- [ ] ISR loads chunks (see "LOADING CHUNK" message)
- [ ] ISR executes steps (see "Step executed" message)
- [ ] FAN1 LED blinks (ISR alive)
- [ ] FAN2 LED flashes (steps executing)
- [ ] Motor physically moves
- [ ] Movement synchronized to spindle RPM

If ALL boxes checked: ✅ **SYSTEM WORKING PERFECTLY**

If ANY box unchecked: Use troubleshooting guide above to diagnose.

## Quick Reference - Debug Levels

The code has different debug output frequencies:

- **Every ISR tick** (20kHz): None (too fast)
- **Every 10 steps**: LED toggle
- **Every 50 steps**: Step execution message
- **Every 100 steps**: Step count message
- **Every 1000 ISR ticks** (0.05sec): None
- **Every 2000 ISR ticks** (0.1sec): ISR heartbeat
- **Every 10 syncs** (~1 sec): Full sync debug

This prevents flooding serial output while still giving visibility.

## If Still Not Working

Send me the serial output from startup through a WIND command attempt. The debug messages will show exactly where it's failing.
