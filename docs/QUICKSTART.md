# Quick Start Guide

## Overview
This guide will help you test the UART communication between your Pi Zero and SKR Pico.

---

## Part 1: Build & Upload Pico Firmware

### Prerequisites
- Pico SDK installed
- CMake and build tools

### Steps

1. **Navigate to the firmware directory:**
   ```bash
   cd pico_firmware
   ```

2. **Create build directory:**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake:**
   ```bash
   cmake ..
   ```

4. **Build the firmware:**
   ```bash
   make -j4
   ```
   This creates `pico_uart_test.uf2`

5. **Upload to Pico:**
   - Hold BOOTSEL button on SKR Pico
   - Plug in USB (or press reset while holding BOOTSEL)
   - Pico appears as USB drive
   - Drag `pico_uart_test.uf2` to the drive
   - Pico automatically reboots

6. **Verify (optional):**
   - Connect to USB serial: `screen /dev/ttyACM0 115200`
   - You should see: "Pico UART Test Ready"
   - Press `Ctrl+A` then `K` to exit screen

---

## Part 2: Wire Pi Zero ↔ Pico

Follow the wiring guide in `docs/WIRING.md`:

```
Pi Zero          SKR Pico
--------         ---------
GPIO 14 (TX) --> GPIO 1 (RX)
GPIO 15 (RX) <-- GPIO 0 (TX)
GND          --> GND
```

**⚠️ CRITICAL: Do NOT connect 5V wires!**

---

## Part 3: Configure Pi Zero UART

1. **Enable serial port:**
   ```bash
   sudo raspi-config
   ```
   - Go to: Interface Options → Serial Port
   - "Login shell over serial?" → **No**
   - "Serial port hardware enabled?" → **Yes**
   - Reboot: `sudo reboot`

2. **Verify serial port exists:**
   ```bash
   ls -l /dev/serial0
   ```
   Should show: `lrwxrwxrwx ... /dev/serial0 -> ttyAMA0`

---

## Part 4: Install Python Dependencies

```bash
cd ~/pi_zero
pip3 install -r requirements.txt
```

Or install directly:
```bash
pip3 install pyserial
```

---

## Part 5: Run the Test!

1. **Make sure both devices are powered:**
   - Pi Zero: via USB
   - SKR Pico: via 24V PSU (or USB)

2. **Run the test script:**
   ```bash
   cd ~/pi_zero
   python3 test_uart.py
   ```

3. **Expected output:**
   ```
   ==================================================
   Pi Zero UART Test
   ==================================================
   ✅ Opened /dev/serial0 @ 115200 baud
   
   📤 Sending: PING
   📥 Waiting for response...
   ✅ Received: PONG
   
   🎉 SUCCESS! UART communication working!
   ```

---

## Troubleshooting

### ❌ No response (timeout)
**Check:**
- Is Pico firmware uploaded? (LED should blink or USB serial shows output)
- Is wiring correct? TX→RX must be crossed!
- Are both devices powered?
- Is `/dev/serial0` enabled? Run `ls -l /dev/serial0`

### ❌ Serial port not found
**Fix:**
- Run `sudo raspi-config` and enable serial hardware
- Check: `ls -l /dev/tty*` to see available ports
- Reboot Pi Zero

### ❌ Permission denied
**Fix:**
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### ⚠️ Garbage characters
**Check:**
- Baud rate matches on both sides (115200)
- TX/RX not swapped
- GND connected

---

## Next Steps

Once UART communication works:
1. Add motor control commands
2. Add encoder reading
3. Build the full winding interface

🎉 You're ready to start building the full pickup winder!

