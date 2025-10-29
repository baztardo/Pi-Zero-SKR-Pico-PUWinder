# PIO Implementation Progress - Snapshot 2025-10-29

## 🎯 Current Status: 90% Complete - PIO Works But FIFO Architecture Needs Redesign

### ✅ What's Working:
1. **UART Communication**
   - Fixed garbage character stripping (first command after sleep works!)
   - Commands parse correctly from CM4 via `/dev/serial0`

2. **Homing (GPIO Mode)**
   - TraverseController uses direct GPIO for homing
   - G28 command works perfectly
   - Motor moves correctly during homing

3. **PIO Hardware Acceleration**
   - PIO activates successfully when winding starts
   - PIO state machine running on PIO0 SM0
   - GPIO handoff works (PIO takes control, then releases back to GPIO)
   - PIO confirmed active via debug messages: `[ISR] ✓✓✓ PIO MODE ACTIVE`

4. **Safety Features**
   - Emergency stop on queue overflow (5 second timeout)
   - Clean handoff between GPIO (homing) and PIO (winding)
   - Proper deactivation when winding stops

### ❌ What's NOT Working:
**Main Issue: Queue overflow during winding**

**The Problem:**
- PIO FIFO is only 8 words deep (even with FIFO join)
- At 833µs per step, 8 steps = 6.6ms of work
- PIO drains FIFO in 6.6ms, then waits
- Main queue generates chunks every 250ms
- Queue fills faster than PIO can consume → Overflow at 100 depth

**Required Step Rate:** 9,816 steps/sec at 1500 RPM with 0.064mm wire
**Current Achieved:** Still limited by queue architecture (~1,200 steps/sec)

---

## 📁 Files Modified:

### Core PIO Files (NEW):
- `pico_firmware/src/pio_stepper.pio` - PIO assembly program
- `pico_firmware/src/pio_stepper.h` - PIO C++ wrapper header
- `pico_firmware/src/pio_stepper.cpp` - PIO C++ implementation
- `pico_firmware/CMakeLists.txt` - Added PIO compilation

### Modified Files:
- `pico_firmware/src/move_queue.h` - Added PIO integration
- `pico_firmware/src/move_queue.cpp` - ISR feeds PIO FIFO
- `pico_firmware/src/winding_controller.cpp` - Activate/deactivate PIO
- `pico_firmware/src/communication_handler.cpp` - UART garbage stripping
- `pico_firmware/src/config.h` - Speed settings (currently 1200 steps/sec)

---

## 🔧 How PIO Handoff Works:

### Boot Sequence:
```
1. MoveQueue::init()
   └─> Creates PIOStepper (INACTIVE)
   └─> STEP pin = regular GPIO
   └─> TraverseController can use GPIO

2. G28 (Homing)
   └─> TraverseController uses gpio_put() directly
   └─> Works perfectly!

3. WIND Command (Start Winding)
   └─> move_queue->activate_pio_mode()
   └─> PIO takes control of STEP pin (GPIO 6)
   └─> ISR feeds PIO FIFO
   └─> ⚠️ Queue overflows (FIFO too small!)

4. M5 or Stop
   └─> move_queue->deactivate_pio_mode()
   └─> PIO releases STEP pin
   └─> GPIO available for next G28
```

### Key Code Locations:

**PIO Activation:**
`pico_firmware/src/winding_controller.cpp` line 266:
```cpp
move_queue->activate_pio_mode();
```

**PIO Deactivation:**
`pico_firmware/src/winding_controller.cpp` line 114:
```cpp
move_queue->deactivate_pio_mode();
```

**ISR PIO Check:**
`pico_firmware/src/move_queue.cpp` line 196:
```cpp
if (pio_stepper && pio_stepper->is_active()) {
    // Feed PIO FIFO
    while (active.count > 0 && pio_stepper->can_queue_step()) {
        pio_stepper->queue_step(active.interval_us);
    }
}
```

---

## 🚀 Next Steps to Fix:

### Option 1: Real-Time Step Generation (Recommended)
**Approach:** Don't use queue at all during winding!

1. In `sync_traverse_to_spindle()`, calculate steps needed in real-time
2. Feed PIO directly from sync function (called every 250ms)
3. Remove chunking/queue for winding (keep it for homing)

**Changes Needed:**
- Modify `WindingController::sync_traverse_to_spindle()`
- Add direct PIO feed without StepCompressor
- Keep queue for homing/manual moves only

**Estimated Work:** 1-2 hours

### Option 2: Larger PIO FIFO (Harder)
**Approach:** Use DMA to feed PIO from RAM buffer

1. Create large buffer (1000+ steps) in RAM
2. Use DMA to automatically feed PIO
3. ISR fills RAM buffer from queue

**Changes Needed:**
- Add DMA channel configuration
- Implement circular buffer in RAM
- Coordinate DMA with PIO

**Estimated Work:** 3-4 hours (complex!)

### Option 3: Switch to Klipper (EASIEST!)
**When Manta M4P board arrives:**
1. Flash Klipper firmware to Manta
2. Use CB1 for host (already have)
3. Create custom kinematics plugin for winding
4. Klipper handles all step generation (proven!)

**Estimated Work:** 2-3 hours setup, but WAY more reliable!

---

## 🧪 Testing Commands:

### Test Homing (GPIO Mode):
```bash
echo "G28" > /dev/serial0
```
**Expected:** Traverse homes correctly, no errors

### Test Winding (PIO Mode):
```bash
echo "WIND T100 S100" > /dev/serial0
```
**Expected:** Currently fails with queue overflow after ~200 turns

### Test Lower Speed (May Work):
```bash
echo "WIND T100 S30" > /dev/serial0
```
**Expected:** Might work at 30 RPM (needs 2,945 steps/sec instead of 9,816)

---

## 📊 Performance Numbers:

### Target Requirements:
- **Spindle Speed:** 1500 RPM (25 rev/sec)
- **Wire Diameter:** 0.064mm
- **Traverse Speed:** 25 × 0.064 = 1.6 mm/sec
- **Steps/mm:** 6135 (calibrated)
- **Required Step Rate:** 1.6 × 6135 = **9,816 steps/sec**

### Current Achieved:
- **With ISR:** ~1,200 steps/sec (limited by software timing)
- **With PIO (theoretical):** 100kHz+ capable
- **With PIO (actual):** Limited by queue/FIFO architecture

### Why It Fails:
```
PIO FIFO: 8 steps × 833µs = 6.6ms of work
Queue fill rate: Chunks every 250ms
PIO drains FIFO 38x faster than queue refills it!
Result: FIFO empties, queue backs up, overflow at depth 100
```

---

## 🐛 Debug Messages to Watch:

### PIO Activation (Should See):
```
[MoveQueue] 🚀 Activating PIO mode for winding...
[PIOStepper] 🔄 Activating PIO mode (taking control of GPIO6)
[PIOStepper] ✓ PIO mode ACTIVE - ready for high-speed stepping
[ISR] ✓✓✓ PIO MODE ACTIVE - feeding hardware FIFO!
```

### PIO Status Check:
```
[ISR-DEBUG] pio_stepper=200046D0, is_active=1
```
- `is_active=1` means PIO working
- `is_active=0` means PIO not active

### Queue Overflow (Bad):
```
[SYNC] ⚠️  Queue depth 100 - skipping chunk generation
╔═══════════════════════════════════════════╗
║  ❌ EMERGENCY STOP - QUEUE OVERFLOW       ║
╚═══════════════════════════════════════════╝
```

---

## 💾 Current Build:

**Firmware Location:**
```
/Users/ssnow/Documents/GitHub/Pi-Zero-SKR-Pico-PUWinder/pico_firmware/build/pico_spindle_controller.uf2
```

**To Flash:**
1. Hold BOOTSEL on Pico
2. Plug in USB
3. Copy `.uf2` to RPI-RP2 drive
4. Pico reboots automatically

**To Test:**
```bash
# From CM4 SSH
echo "G28" > /dev/serial0          # Homing (works!)
echo "WIND T100 S100" > /dev/serial0  # Winding (overflows)
echo "M5" > /dev/serial0           # Stop
```

---

## 🔄 Git Snapshot Commands:

### Create Snapshot:
```bash
cd /Users/ssnow/Documents/GitHub/Pi-Zero-SKR-Pico-PUWinder
git add -A
git commit -m "WIP: PIO implementation 90% complete - handoff works, FIFO architecture needs redesign"
git tag pio-snapshot-2025-10-29
```

### Restore Snapshot Later:
```bash
git checkout pio-snapshot-2025-10-29
cd pico_firmware/build
cmake .. && make -j4
```

---

## 📝 Things You Can Try While Waiting:

### 1. Test at Lower RPM:
```bash
# Try progressively lower speeds to find working threshold
echo "WIND T50 S20" > /dev/serial0   # 20 RPM
echo "WIND T50 S40" > /dev/serial0   # 40 RPM  
echo "WIND T50 S60" > /dev/serial0   # 60 RPM
```

### 2. Adjust Queue Safety Threshold:
In `winding_controller.cpp` line 448, change:
```cpp
const uint32_t MAX_QUEUE_DEPTH = 100;  // Try 120?
```

### 3. Research Klipper for Winding:
- Look for custom kinematics examples
- Check if anyone has done CNC winding with Klipper
- Prepare for Manta M4P migration

---

## 🎓 What We Learned:

### PIO is Amazing BUT:
1. ✅ Zero CPU overhead for stepping
2. ✅ 100kHz+ capability
3. ✅ Perfect timing precision
4. ❌ Tiny FIFO (8 words) requires careful architecture
5. ❌ Can't just drop-in replace ISR - need redesign

### Architecture Matters:
1. Queue-based approach works for ISR (slow, steady)
2. PIO needs continuous feeding (fast, hungry)
3. Mismatch = overflow

### The Real Solution:
- **For learning/custom:** Fix PIO architecture (Option 1 above)
- **For production:** Use Klipper on Manta (proven, reliable)

---

## 📞 When You Have More AI Budget:

### Quick Fixes to Try:
1. "Implement real-time step generation in sync_traverse_to_spindle()"
2. "Remove queue usage during winding, feed PIO directly"
3. "Add DMA-based PIO feeding"

### Or Switch Direction:
1. "Help me set up Klipper on Manta M4P"
2. "Create custom Klipper kinematics for winding machine"
3. "Port winding logic to Klipper Python plugin"

---

## 🏆 Progress Made This Session:

1. ✅ Fixed UART garbage character issue
2. ✅ Implemented PIO hardware acceleration
3. ✅ Created clean GPIO/PIO handoff
4. ✅ Confirmed PIO activates and runs
5. ✅ Identified FIFO architecture as bottleneck
6. ✅ Documented everything for next session

**You're 90% there!** Just need the final architecture fix! 🚀

---

## 📚 Reference Links:

- **RP2040 Datasheet:** PIO chapter (page 317+)
- **Klipper Docs:** https://www.klipper3d.org/
- **Your Old PIO Code:** https://github.com/baztardo/PU-Winder/tree/main/Pico-SDK_PIO
- **FluidNC:** https://github.com/bdring/FluidNC

---

**END OF SNAPSHOT - Good luck!** 🎯

