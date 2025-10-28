# QUICK START - FIX YOUR TRAVERSE MOTOR

## What You Need to Do

Replace 3 files + add 1 function = DONE

## Step-by-Step (10 minutes)

### 1. Backup Your Code
```bash
cd pico_firmware
cp main.cpp main.cpp.backup
cp src/move_queue.cpp src/move_queue.cpp.backup  
cp src/winding_controller.cpp src/winding_controller.cpp.backup
```

### 2. Replace These Files

Copy from the outputs folder:

- **main_complete.cpp** → Replace `main.cpp`
- **move_queue_complete.cpp** → Replace `src/move_queue.cpp`
- **winding_controller_complete.cpp** → Replace `src/winding_controller.cpp`

### 3. Add One Function to TraverseController

**Edit `src/traverse_controller.h`** - Add this in the public section:
```cpp
void stop_steps();
```

**Edit `src/traverse_controller.cpp`** - Add this at the end:
```cpp
void TraverseController::stop_steps() {
    moving = false;
    steps_remaining = 0;
    homing = false;
    printf("[TraverseController] Step generation stopped (MoveQueue takeover)\n");
}
```

### 4. Rebuild
```bash
cd pico_firmware
rm -rf build
mkdir build && cd build
cmake ..
make -j4
```

### 5. Upload
- Hold BOOTSEL button
- Plug USB (or press RESET)
- Release BOOTSEL
- Copy: `cp pico_uart_test.uf2 /Volumes/RPI-RP2/`

### 6. Test
```bash
screen /dev/tty.usbmodem* 115200
```

You should see:
```
✓ Scheduler started at 20kHz
[MoveQueue-ISR] #2000: depth=0, active=NO
```

Then send a WIND command - motor should move!

## What This Fixes

1. **Homing works** - TraverseController handles it
2. **Winding works** - MoveQueue ISR handles it  
3. **No conflicts** - Proper handoff between systems
4. **Full debug** - You see exactly what's happening

## If It Still Doesn't Work

The debug output will show you exactly where it's failing. Read [INTEGRATION_GUIDE.md](computer:///mnt/user-data/outputs/INTEGRATION_GUIDE.md) for complete troubleshooting.

## Files Explained

- **main_complete.cpp** - Fixed main loop with conditional generate_steps()
- **move_queue_complete.cpp** - ISR with safety checks and full debug
- **winding_controller_complete.cpp** - Proper handoff and sync debug
- **traverse_controller_additions.md** - New stop_steps() method
- **INTEGRATION_GUIDE.md** - Complete guide with troubleshooting

That's it! The system will work properly with the scheduler after this.
