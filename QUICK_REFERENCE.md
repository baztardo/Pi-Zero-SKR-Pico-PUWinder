# Quick Reference Card - Pickup Winder Project

## 🚀 Quick Start Commands

### Flash New Firmware:
```bash
# 1. Hold BOOTSEL button on Pico
# 2. Plug in USB
# 3. Copy file:
cp pico_firmware/build/pico_spindle_controller.uf2 /Volumes/RPI-RP2/
```

### Test from CM4:
```bash
ssh cm4
echo "G28" > /dev/serial0              # Home
echo "WIND T100 S100" > /dev/serial0   # Wind 100 turns at 100 RPM
echo "M5" > /dev/serial0                # Stop
```

### Monitor Serial Output:
```bash
# From Mac:
screen /dev/tty.usbmodem* 115200
# Press Ctrl-A then K to exit
```

---

## 📂 Important Files

### Main Code:
- `pico_firmware/src/move_queue.cpp` - ISR and PIO integration
- `pico_firmware/src/winding_controller.cpp` - Winding logic
- `pico_firmware/src/pio_stepper.cpp` - PIO wrapper
- `pico_firmware/src/config.h` - All settings

### Build:
- `pico_firmware/build/pico_spindle_controller.uf2` - Flash this!
- `pico_firmware/CMakeLists.txt` - Build configuration

### Documentation:
- `PIO_PROGRESS_SNAPSHOT.md` - Full status and next steps
- `QUICK_REFERENCE.md` - This file!

---

## 🔧 Build Commands

```bash
cd /Users/ssnow/Documents/GitHub/Pi-Zero-SKR-Pico-PUWinder/pico_firmware/build
cmake ..
make -j4
```

---

## 🐛 Debug

### Check PIO Status:
Look for these in serial output:
```
[PIOStepper] ✓ PIO mode ACTIVE           ← Good!
[ISR] ✓✓✓ PIO MODE ACTIVE                ← Good!
[ISR-DEBUG] pio_stepper=..., is_active=1  ← Good! (1=active)
```

### Queue Overflow (Bad):
```
[SYNC] ⚠️  Queue depth 100
❌ EMERGENCY STOP - QUEUE OVERFLOW
```

---

## 🎯 Current Status

| Feature | Status | Notes |
|---------|--------|-------|
| UART Commands | ✅ Working | Garbage stripping fixed |
| Homing (G28) | ✅ Working | Uses GPIO directly |
| PIO Activation | ✅ Working | Handoff works! |
| Winding | ❌ Overflows | FIFO too small |

**Required:** 9,816 steps/sec at 1500 RPM  
**Achieved:** ~1,200 steps/sec (queue bottleneck)

---

## 📌 Key Numbers

```
Spindle: 1500 RPM max = 25 rev/sec
Wire: 0.064mm diameter
Steps/mm: 6135 (traverse calibration)
Speed needed: 1.6 mm/s = 9,816 steps/sec
```

---

## 🔄 Git Snapshot

### Restore This Version:
```bash
git checkout pio-snapshot-2025-10-29
cd pico_firmware/build
cmake .. && make -j4
```

### See What Changed:
```bash
git log --oneline
git diff HEAD~1
```

---

## 📞 Next Session Tasks

1. **Option A:** Fix PIO architecture (real-time stepping)
2. **Option B:** Switch to Klipper on Manta M4P
3. **Option C:** Test at lower RPM to find working threshold

See `PIO_PROGRESS_SNAPSHOT.md` for details!

---

## 💡 Quick Experiments

### Test Lower Speed:
```bash
# Find max working RPM:
echo "WIND T50 S20" > /dev/serial0
echo "WIND T50 S40" > /dev/serial0
echo "WIND T50 S60" > /dev/serial0
```

### Adjust Queue Limit:
Edit `winding_controller.cpp` line 448:
```cpp
const uint32_t MAX_QUEUE_DEPTH = 120;  // Was 100
```

---

**Last Updated:** 2025-10-29  
**Commit:** pio-snapshot-2025-10-29  
**Firmware:** pico_spindle_controller.uf2

