# 🚀 Pi CM4 Quick Start Guide

**For testing the winding controller before moving to Pi Zero**

## 📋 What You Need Right Now

### Hardware:
- ✅ **Pi CM4** (any variant)
- ✅ **Touchscreen** (7" recommended)
- ✅ **Ethernet cable** (since no WiFi)
- ✅ **MicroSD card** (32GB+)
- ✅ **Power supply** (5V 3A)
- ✅ **UART adapter** (USB-to-Serial or direct GPIO)

### Software:
- ✅ **Raspberry Pi OS Lite** (64-bit) - **RECOMMENDED**
- ✅ **Ethernet connection** to your network

## 🎯 Step-by-Step Setup

### 1. 📱 Flash the OS
```bash
# Download Raspberry Pi Imager
# https://www.raspberrypi.org/downloads/

# Choose: Raspberry Pi OS Lite (64-bit)
# Enable SSH
# Set username: pi
# Set password: (your choice)
# Set static IP: 192.168.1.100 (or your network range)
```

### 2. 🔌 Connect Hardware
```
Pi CM4:
├── Ethernet cable → Router
├── Touchscreen → HDMI + USB
├── Power supply → USB-C
└── UART adapter → USB (for Pico connection)
```

### 3. 🚀 First Boot
```bash
# SSH into your Pi CM4
ssh pi@192.168.1.100

# Run the setup script
cd Pi-Zero-SKR-Pico-PUWinder/pi_cm4_setup
./setup_cm4.sh

# Reboot
sudo reboot
```

### 4. 🧪 Test Everything
```bash
# After reboot, SSH back in
ssh pi@192.168.1.100

# Test UART connection
cd pi_zero
python test_uart.py

# Start web interface
python web_interface.py
```

### 5. 🖥️ Access Touchscreen
- **On the touchscreen**: Open browser to `http://localhost:5000`
- **From your computer**: Open browser to `http://192.168.1.100:5000`

## 🔌 UART Connection Options

### Option 1: USB-to-Serial Adapter (EASIEST)
```
Pi CM4 USB → USB-Serial Adapter → Pico UART
```
- **Pros**: Easy, reliable
- **Cons**: Need adapter

### Option 2: Direct GPIO (if you have access)
```
Pi CM4 GPIO 14 (TX) → Pico GPIO 1 (RX)
Pi CM4 GPIO 15 (RX) → Pico GPIO 0 (TX)
Pi CM4 Ground → Pico Ground
```
- **Pros**: Direct connection
- **Cons**: Need to access GPIO pins

### Option 3: USB-C (if Pico supports)
```
Pi CM4 USB → Pico USB-C
```
- **Pros**: Simple
- **Cons**: May not work with all Pico variants

## 🎮 Testing Workflow

### 1. Basic Connection Test
```bash
python test_uart.py
```
**Expected output:**
```
🔍 UART Connection Test
========================================
Port: /dev/ttyUSB0
Baudrate: 115200

1. Opening serial connection...
   ✅ Serial port opened
2. Clearing UART buffers...
   ✅ Buffers cleared
3. Sending dummy message...
   ✅ Dummy message sent
4. Testing PING command...
   Attempt 1: PONG
   ✅ PING successful!
5. Testing VERSION command...
   ✅ Version: Pico Spindle Controller v1.0
6. Testing STATUS command...
   ✅ Status: {"state":"idle","turns":0}

✅ UART connection test completed successfully!
```

### 2. Web Interface Test
```bash
python web_interface.py
```
**Then on touchscreen:**
1. Open browser to `http://localhost:5000`
2. Click "🔌 Connect" button
3. Should show "✅ Connected to Pico"
4. Click "🏠 Home" button
5. Click "▶️ Start" button
6. Watch real-time progress

### 3. Command Line Test
```bash
python simple_control.py
```
**Test commands:**
```
> test      # Test UART connection
> home      # Home all axes
> start     # Start winding
> status    # Check progress
> emergency # Emergency stop
> quit      # Exit
```

## 🚨 Troubleshooting

### UART Not Working:
```bash
# Check available ports
ls /dev/ttyUSB* /dev/ttyAMA*

# Test with different port
python test_uart.py
# It will try multiple ports automatically
```

### Touchscreen Not Working:
```bash
# Check if detected
lsusb
xinput list

# Test touch
xinput test <device-id>
```

### Display Issues:
```bash
# Check display
xrandr

# Rotate if needed
xrandr --output HDMI-1 --rotate left
```

### Network Issues:
```bash
# Check IP
ip addr show

# Test connectivity
ping google.com
```

## 🎯 What to Test

### 1. ✅ UART Communication
- PING command works
- VERSION command works
- STATUS command works

### 2. ✅ Touchscreen Interface
- Large buttons are touchable
- Status updates in real-time
- Emergency stop button works

### 3. ✅ Winding Sequence
- Home all axes
- Start winding
- Pause/resume
- Emergency stop
- Progress tracking

### 4. ✅ Safety Features
- Emergency stop works
- Pause/resume works
- Status monitoring works

## 🔄 Migration to Pi Zero

When you're ready to move to Pi Zero:

1. **Document what works** on CM4
2. **Note any issues** and solutions
3. **Flash Pi Zero** with same OS
4. **Copy working code**
5. **Test on Pi Zero**

## 📞 Quick Help

### If Something Doesn't Work:
1. **Check UART**: `python test_uart.py`
2. **Check network**: `ping google.com`
3. **Check touchscreen**: `xinput list`
4. **Check logs**: `sudo journalctl -u winding-controller`

### Common Issues:
- **First command fails**: Normal, dummy message fixes this
- **Touchscreen not responsive**: Check USB connection
- **Display wrong size**: Use `xrandr` to adjust
- **UART not found**: Check adapter connection

---

**🎉 You're ready to test!** Your Pi CM4 with touchscreen is now set up for testing the winding controller.
