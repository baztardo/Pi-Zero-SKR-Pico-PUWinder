# 🚀 QUICK START: Queue Overflow Fix

## 📦 Files Created for You

I've created 5 files to fix your queue overflow issue:

### Core Files (MUST REPLACE):
1. **move_queue_FIXED.cpp** - ISR without printf, LED indicators, diagnostics
2. **move_queue_FIXED.h** - Updated header with diagnostic methods

### Helper Files (NEW):
3. **diagnostic_monitor.h** - Real-time queue monitoring
4. **winding_controller_FIXED.cpp** - Improved sync with backpressure
5. **QUEUE_OVERFLOW_FIX_GUIDE.md** - Complete documentation

## ⚡ Critical Changes Summary

### 1. ISR Printf Removal (CRITICAL)
**Why:** Printf takes 200-500µs, your ISR runs every 50µs
**Fix:** Replaced with LED indicators and counters

**OLD CODE (traverse_isr_handler):**
```cpp
printf("[MoveQueue-ISR] Loading chunk: %u steps\n", active.count);  // ❌ BLOCKING!
```

**NEW CODE:**
```cpp
g_chunks_loaded++;   // ✅ Fast counter
gpio_put(18, 1);     // ✅ LED indicator
// Printf moved to main loop instead
```

### 2. Queue Backpressure (CRITICAL)
**Why:** Sync generates chunks faster than ISR can process
**Fix:** Check queue depth before generating chunks

**NEW CODE in sync_traverse_to_spindle():**
```cpp
// Check queue depth FIRST
uint32_t depth = move_queue->get_queue_depth();
if (depth >= 100) {
    return;  // Skip sync - let queue drain
}

// Reduce sync frequency
static uint32_t last_sync = 0;
if ((time_us_32() - last_sync) < 50000) {
    return;  // 50ms minimum between syncs
}
```

### 3. Diagnostic Monitoring (IMPORTANT)
**Why:** Need visibility into queue health
**Fix:** Monitor from main loop

**NEW CODE in main loop:**
```cpp
#include "diagnostic_monitor.h"

DiagnosticMonitor* monitor = new DiagnosticMonitor(move_queue);

while (true) {
    // ... existing code ...
    
    monitor->update(1000);  // Print every second
}
```

## 🔧 Step-by-Step Implementation

### STEP 1: Backup Your Files
```bash
cd ~/pico_firmware/src
cp move_queue.cpp move_queue.cpp.backup
cp move_queue.h move_queue.h.backup
```

### STEP 2: Replace MoveQueue Files
```bash
# Download from Claude (or copy from project folder)
# Then:
cp move_queue_FIXED.cpp move_queue.cpp
cp move_queue_FIXED.h move_queue.h
```

### STEP 3: Add Diagnostic Monitor
```bash
# Copy diagnostic_monitor.h to your include directory
cp diagnostic_monitor.h ../include/
```

### STEP 4: Update winding_controller.cpp
Find the `sync_traverse_to_spindle()` function and add these lines at the start:

```cpp
void WindingController::sync_traverse_to_spindle() {
    if (!spindle_motor) return;
    
    // ⭐⭐⭐ ADD THESE LINES ⭐⭐⭐
    uint32_t queue_depth = move_queue->get_queue_depth();
    if (queue_depth >= 100) {
        return;  // Queue too full - skip sync
    }
    
    float current_rpm = spindle_motor->get_rpm();
    if (current_rpm < 10.0f) return;
    
    // ⭐⭐⭐ ADD THIS ⭐⭐⭐
    static uint32_t last_sync = 0;
    uint32_t now = time_us_32();
    if ((now - last_sync) < 50000) return;  // 50ms minimum
    last_sync = now;
    
    // ... rest of function ...
    
    // ⭐⭐⭐ REDUCE PRINTF FREQUENCY ⭐⭐⭐
    static uint32_t sync_count = 0;
    if ((sync_count++ % 20) == 0) {  // Only every 20th call
        printf("[SYNC] RPM: %.1f, Depth: %u\n", current_rpm, queue_depth);
    }
}
```

### STEP 5: Update main.cpp
Add diagnostic monitoring to your main loop:

```cpp
#include "diagnostic_monitor.h"  // ⭐ ADD THIS

int main() {
    // ... existing initialization ...
    
    // ⭐⭐⭐ ADD THIS ⭐⭐⭐
    DiagnosticMonitor* monitor = new DiagnosticMonitor(move_queue);
    uint32_t last_full_diag = 0;
    
    printf("\nSYSTEM READY\n\n");
    
    while (true) {
        // ... existing update calls ...
        
        // ⭐⭐⭐ ADD THESE ⭐⭐⭐
        monitor->update(1000);  // Status every 1 sec
        
        uint32_t now = time_us_32();
        if ((now - last_full_diag) > 10000000) {
            monitor->print_full_diagnostics();
            last_full_diag = now;
        }
        
        sleep_us(100);
    }
}
```

### STEP 6: Rebuild
```bash
cd ~/pico_firmware/build
cmake ..
make -j4
# Flash the generated .uf2 file
```

## 🎯 What You'll See After Fix

### Good Output (Working):
```
[Monitor] Depth: 45/128 Active:Y Paused:N E-Stop:N
[SYNC] RPM: 895.1, Depth: 45
[Monitor] Depth: 52/128 Active:Y Paused:N E-Stop:N
[Monitor] Depth: 38/128 Active:Y Paused:N E-Stop:N
```

### Bad Output (Still Broken):
```
[Monitor] Depth:127/128 Active:N Paused:N E-Stop:N ⚠️ QUEUE FULL!
[Monitor] Depth:127/128 Active:N Paused:N E-Stop:N ⚠️ QUEUE FULL!
```

## 🔴 LED Indicators

Watch these LEDs on your board:

- **FAN1 (GPIO 17)**: ISR Heartbeat
  - Should blink at 10Hz
  - If not blinking → ISR not running!

- **FAN2 (GPIO 18)**: Active Processing
  - ON when processing chunks
  - OFF when waiting
  - Should be ON during winding

## 🐛 Quick Troubleshooting

### Problem: Queue still fills to 127/127
**Quick Fix:** Increase sync interval
```cpp
// In sync_traverse_to_spindle()
if ((now - last_sync) < 100000) return;  // Change 50ms to 100ms
```

### Problem: FAN1 never blinks
**Quick Fix:** ISR not running - check scheduler
```cpp
// In main.cpp, verify:
scheduler->start(HEARTBEAT_US);
printf("Scheduler running: %s\n", scheduler->is_running() ? "YES" : "NO");
```

### Problem: Motion too jerky
**Quick Fix:** Reduce sync interval
```cpp
// In sync_traverse_to_spindle()
if ((now - last_sync) < 30000) return;  // Change 50ms to 30ms
```

## 📞 Need Help?

If still broken after implementing these fixes:

1. Run `move_queue->print_diagnostics()` and share the output
2. Tell me what the LEDs are doing (FAN1 blinking? FAN2 on?)
3. Share the Monitor output during winding

## 🎉 Success Checklist

After implementing fixes, verify:
- [ ] FAN1 blinking at 10Hz
- [ ] FAN2 turns on during winding
- [ ] Queue depth stays 20-80 (not 127!)
- [ ] "Active: Y" in monitor output
- [ ] No "Queue FULL" warnings
- [ ] Smooth traverse movement

## ⏱️ Expected Time

- File replacement: 5 minutes
- Code updates: 10 minutes
- Build and flash: 5 minutes
- Testing: 5 minutes
- **Total: ~25 minutes**

---

**Read QUEUE_OVERFLOW_FIX_GUIDE.md for complete details and troubleshooting!**
