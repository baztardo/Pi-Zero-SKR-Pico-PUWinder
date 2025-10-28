# 🔧 QUEUE OVERFLOW FIX GUIDE

## 📋 Problem Summary

Your coil winder is experiencing a **queue overflow** where the MoveQueue fills to 127/127 capacity but shows **"Active: NO"**, meaning chunks aren't being processed. This causes the winding process to stall.

## 🔍 Root Causes Identified

### 1. **ISR Printf Overload** (PRIMARY CAUSE)
- Your `traverse_isr_handler()` contains multiple `printf()` statements
- Printf can take **200-500µs** to execute
- At 20kHz ISR rate (50µs period), even occasional printf causes massive delays
- This prevents the ISR from keeping up with chunk processing

### 2. **Excessive Sync Frequency**
- `sync_traverse_to_spindle()` called every ~10ms (100Hz)
- Generates 1+ chunks per call
- Queue fills faster than ISR can drain it
- No backpressure mechanism to slow down when queue is full

### 3. **No Queue Depth Monitoring**
- No visibility into queue state from main loop
- Can't detect when queue is approaching full
- No early warning system

## ✅ Complete Fix Strategy

### Fix 1: Remove ALL Printf from ISR (CRITICAL)

**Problem:** Printf in ISR blocks processing

**Solution:** Replace with LED indicators and diagnostic counters

**Files to Replace:**
- `pico_firmware/src/move_queue.cpp` → Use `move_queue_FIXED.cpp`
- `pico_firmware/src/move_queue.h` → Use `move_queue_FIXED.h`

**Key Changes:**
```cpp
// ❌ OLD (in ISR):
printf("[MoveQueue-ISR] Loading chunk...\n");

// ✅ NEW (in ISR):
g_chunks_loaded++;  // Just increment counter
gpio_put(18, 1);    // LED indicator

// NEW: Print diagnostics from main loop instead
move_queue->print_diagnostics();  // Call from main loop
```

### Fix 2: Add Queue Backpressure

**Problem:** Sync generates chunks too fast

**Solution:** Check queue depth before generating chunks

**Key Changes in sync_traverse_to_spindle():**
```cpp
// ✅ NEW: Check queue depth first
uint32_t queue_depth = move_queue->get_queue_depth();
const uint32_t MAX_QUEUE_DEPTH = 100;  // Leave headroom

if (queue_depth >= MAX_QUEUE_DEPTH) {
    return;  // Skip sync - queue too full
}

// ✅ NEW: Reduce sync frequency
const uint32_t MIN_SYNC_INTERVAL_US = 50000;  // 50ms instead of 10ms
```

### Fix 3: Add Diagnostic Monitoring

**Problem:** No visibility into queue health

**Solution:** Real-time monitoring from main loop

**New File:** `diagnostic_monitor.h`

**Usage:**
```cpp
// In main.cpp
DiagnosticMonitor* monitor = new DiagnosticMonitor(move_queue);

// In main loop
monitor->update(1000);  // Print status every 1 second
```

## 🚀 Implementation Steps

### Step 1: Backup Current Code
```bash
cd ~/pico_firmware/src
cp move_queue.cpp move_queue.cpp.backup
cp move_queue.h move_queue.h.backup
cp winding_controller.cpp winding_controller.cpp.backup
```

### Step 2: Replace MoveQueue Files

1. **Replace move_queue.cpp:**
   - Copy contents of `move_queue_FIXED.cpp`
   - Key changes:
     - Removed all `printf()` from `traverse_isr_handler()`
     - Added diagnostic counters (g_isr_call_count, g_chunks_loaded, etc.)
     - Added `print_diagnostics()` method
     - Added LED indicators (FAN1=heartbeat, FAN2=active)

2. **Replace move_queue.h:**
   - Copy contents of `move_queue_FIXED.h`
   - Added diagnostic method declarations
   - Added documentation

### Step 3: Update Winding Controller

In `winding_controller.cpp`, replace the `sync_traverse_to_spindle()` function with the fixed version:

```cpp
void WindingController::sync_traverse_to_spindle() {
    if (!spindle_motor) return;
    
    // ⭐ Check queue depth BEFORE generating chunks
    uint32_t queue_depth = move_queue->get_queue_depth();
    if (queue_depth >= 100) {  // Leave headroom
        static uint32_t skip_count = 0;
        if ((skip_count++ % 100) == 0) {
            printf("[SYNC] Queue depth %u - skipping\n", queue_depth);
        }
        return;
    }
    
    float current_rpm = spindle_motor->get_rpm();
    if (current_rpm < 10.0f) return;
    
    // Calculate steps
    float steps_per_sec = /* your calculation */;
    
    // ⭐ Reduce sync frequency
    static uint32_t last_sync = 0;
    uint32_t now = time_us_32();
    if ((now - last_sync) < 50000) return;  // 50ms min
    last_sync = now;
    
    // Generate and push chunks
    auto chunks = StepCompressor::compress_constant_velocity(/*...*/);
    
    // ⭐ Reduce printf frequency
    static uint32_t sync_count = 0;
    if ((sync_count++ % 20) == 0) {  // Only every 20th sync
        printf("[SYNC] RPM: %.1f, Steps: %d, Depth: %u\n",
               current_rpm, steps, queue_depth);
    }
    
    // Push chunks...
}
```

### Step 4: Update Main Loop

In `main.cpp`, add diagnostic monitoring:

```cpp
#include "diagnostic_monitor.h"

int main() {
    // ... existing initialization ...
    
    // Create diagnostic monitor
    DiagnosticMonitor* monitor = new DiagnosticMonitor(move_queue);
    
    printf("\nSYSTEM READY\n\n");
    
    // Main loop
    uint32_t last_diag_time = 0;
    
    while (true) {
        // Update winding controller
        if (winding_controller) {
            winding_controller->update();
        }
        
        // Update traverse (only when moving)
        if (traverse_controller && traverse_controller->is_moving()) {
            traverse_controller->generate_steps();
        }
        
        // Handle commands
        if (communication_handler) {
            communication_handler->update();
        }
        
        // ⭐ NEW: Monitor queue health every second
        monitor->update(1000);
        
        // ⭐ NEW: Full diagnostics every 10 seconds
        uint32_t now = time_us_32();
        if ((now - last_diag_time) > 10000000) {
            monitor->print_full_diagnostics();
            last_diag_time = now;
        }
        
        sleep_us(100);
    }
}
```

### Step 5: Rebuild and Flash

```bash
cd ~/pico_firmware/build
cmake ..
make -j4
# Flash the .uf2 file to your Pico
```

## 📊 Expected Results After Fix

### Startup Output:
```
[MoveQueue] Initialization complete
[MoveQueue] - Debug LED FAN1 (GPIO 17): ISR Heartbeat
[MoveQueue] - Debug LED FAN2 (GPIO 18): Active Indicator
[Scheduler] Hardware timer started at 20kHz
SYSTEM READY
```

### During Winding:
```
[Monitor] Depth: 45/128 Active:Y Paused:N E-Stop:N
[SYNC] RPM: 895.1, Steps: 58, Depth: 45
[Monitor] Depth: 52/128 Active:Y Paused:N E-Stop:N
[Monitor] Depth: 38/128 Active:Y Paused:N E-Stop:N
```

### Every 10 Seconds:
```
╔═══════════════════════════════════════════════════════════╗
║              MOVEQUEUE DIAGNOSTICS                        ║
╚═══════════════════════════════════════════════════════════╝
ISR Stats:
  - ISR calls:           200000
  - Chunks loaded:       1250
  - Steps executed:      45000
  - Feed paused hits:    0
  - E-stop hits:         0

Queue State:
  - Queue depth:         42 / 128
  - Head:                85
  - Tail:                43
  - Active running:      YES

Safety Flags:
  - Feeding paused:      ✓ NO
  - Emergency stop:      ✓ NO
```

## 🎯 What Each LED Means

- **FAN1 (GPIO 17)**: ISR Heartbeat
  - Blinks at 10Hz when ISR running
  - If not blinking → ISR not running (critical!)

- **FAN2 (GPIO 18)**: Active Chunk
  - ON when actively processing a chunk
  - OFF when waiting for new chunk
  - Should be ON most of the time during winding

## 🔧 Debugging Commands

### Check Queue Status:
Add a G-code command to check queue:
```cpp
// In gcode_interface.cpp
if (command == "M114") {  // Get current position + queue status
    move_queue->print_diagnostics();
    return true;
}
```

### Force Queue Clear (Emergency):
```cpp
if (command == "M999") {  // Reset
    move_queue->clear_queue();
    move_queue->resume_feeding();
    printf("✓ Queue cleared and feeding resumed\n");
    return true;
}
```

## ⚠️ Common Issues After Fix

### Issue 1: FAN1 Not Blinking
**Symptom:** FAN1 LED stays off
**Cause:** ISR not running
**Fix:** Check scheduler initialization

### Issue 2: FAN2 Never Turns On
**Symptom:** FAN2 LED stays off, Active always NO
**Cause:** Chunks not being loaded
**Fix:** Check safety flags (feeding_paused, emergency_stop)

### Issue 3: Queue Still Fills Up
**Symptom:** Depth reaches 100+ consistently
**Cause:** Sync frequency still too high
**Fix:** Increase MIN_SYNC_INTERVAL_US to 100ms

### Issue 4: Motion Too Jerky
**Symptom:** Traverse moves in jumps
**Cause:** Sync interval too long
**Fix:** Reduce MIN_SYNC_INTERVAL_US to 30ms

## 📝 Performance Tuning

### Optimal Values (Adjust Based on Your RPM):

```cpp
// In sync_traverse_to_spindle()
const uint32_t MIN_SYNC_INTERVAL_US = 50000;  // 50ms
const uint32_t MAX_QUEUE_DEPTH = 100;         // Leave 28 free
const uint32_t MIN_CHUNK_SIZE = 50;           // Minimum steps per chunk
```

### For High RPM (>1000):
```cpp
const uint32_t MIN_SYNC_INTERVAL_US = 30000;  // 30ms
const uint32_t MAX_QUEUE_DEPTH = 90;          // More aggressive
```

### For Low RPM (<500):
```cpp
const uint32_t MIN_SYNC_INTERVAL_US = 100000; // 100ms
const uint32_t MAX_QUEUE_DEPTH = 110;         // More conservative
```

## 🎉 Success Criteria

You'll know the fix is working when you see:
1. ✅ FAN1 blinking consistently (10Hz)
2. ✅ FAN2 turning on/off during winding
3. ✅ Queue depth staying between 20-80
4. ✅ "Active: Y" in monitor output
5. ✅ No "Queue FULL" warnings
6. ✅ Smooth traverse movement during winding

## 📞 Still Having Issues?

If problems persist after implementing all fixes:

1. **Capture full diagnostics:**
   ```
   Call move_queue->print_diagnostics() and share output
   ```

2. **Check LED states:**
   - Is FAN1 blinking?
   - Is FAN2 ever turning on?

3. **Monitor queue depth over time:**
   - Does it gradually fill up?
   - Does it stay at a steady level?
   - Does it drain and refill?

4. **Verify safety flags:**
   - Run `print_diagnostics()` during winding
   - Check if feed paused hits > 0
   - Check if e-stop hits > 0
