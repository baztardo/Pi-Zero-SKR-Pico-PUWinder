# 📱 Pi CM4 Image Setup Guide

**Step-by-step guide to flash and configure your Pi CM4 for touchscreen testing**

## 🎯 Recommended Image

### **Raspberry Pi OS Lite (64-bit)** - **RECOMMENDED**
- **Why**: Minimal, fast, perfect for embedded use
- **Size**: ~500MB
- **Boot time**: ~10 seconds
- **Memory usage**: ~100MB

### Alternative: **Raspberry Pi OS Desktop (64-bit)**
- **Why**: If you want GUI for easier setup
- **Size**: ~2GB
- **Boot time**: ~30 seconds
- **Memory usage**: ~300MB

## 🚀 Flashing the Image

### 1. Download Raspberry Pi Imager
```bash
# Download from: https://www.raspberrypi.org/downloads/
# Available for Windows, macOS, Linux
```

### 2. Flash the Image
```
1. Open Raspberry Pi Imager
2. Choose OS: Raspberry Pi OS Lite (64-bit)
3. Choose Storage: Your microSD card
4. Click the gear icon (⚙️) for advanced options
5. Configure:
   - Enable SSH: ✅
   - Username: pi
   - Password: (your choice)
   - Configure WiFi: ❌ (you're using Ethernet)
   - Set static IP: ✅ (recommended)
     - IP: 192.168.1.100
     - Gateway: 192.168.1.1
     - DNS: 8.8.8.8
6. Click "Write"
```

### 3. Static IP Configuration
**Recommended settings:**
```
IP Address: 192.168.1.100
Gateway: 192.168.1.1
DNS: 8.8.8.8
```

**Or use your network range:**
- If your router is 192.168.0.x, use 192.168.0.100
- If your router is 10.0.0.x, use 10.0.0.100

## 🔌 First Boot Setup

### 1. Connect Hardware
```
Pi CM4:
├── Ethernet cable → Router
├── Touchscreen → HDMI + USB
├── Power supply → USB-C
└── MicroSD card → Inserted
```

### 2. Boot and SSH
```bash
# Wait for Pi to boot (LED should stop blinking)
# SSH into your Pi
ssh pi@192.168.1.100

# First time setup
sudo raspi-config
```

### 3. Essential Configuration
```bash
# In raspi-config:
1. System Options → Password (change if needed)
2. Interface Options → SSH (enable)
3. Interface Options → Serial Port (enable)
4. Advanced Options → Expand Filesystem
5. Finish and reboot
```

## 🖥️ Touchscreen Configuration

### For USB Touchscreen:
```bash
# Usually works automatically
# Check if detected:
lsusb
# Should show touchscreen device

# Test touch:
xinput list
xinput test <device-id>
```

### For DSI Touchscreen:
```bash
# Edit config
sudo nano /boot/config.txt

# Add these lines:
dtoverlay=vc4-kms-v3d
dtoverlay=vc4-kms-v3d-pi4
dtoverlay=vc4-kms-v3d-pi4,2xhdmi
dtoverlay=vc4-kms-v3d-pi4,2xhdmi,audio=off

# Reboot
sudo reboot
```

### For HDMI + USB Touchscreen:
```bash
# Check display
xrandr

# If display is wrong size:
xrandr --output HDMI-1 --mode 1920x1080

# If display is rotated:
xrandr --output HDMI-1 --rotate left
```

## 🔌 UART Configuration

### Enable UART:
```bash
# Already done in raspi-config, but verify:
sudo raspi-config
# Interface Options → Serial Port → Login shell: No, Serial port: Yes
```

### Add user to dialout group:
```bash
sudo usermod -a -G dialout $USER
# Logout and login again for changes to take effect
```

### Test UART:
```bash
# Check available ports
ls /dev/ttyUSB* /dev/ttyAMA*

# Test connection (after setting up the project)
python test_uart.py
```

## 🚀 Project Setup

### 1. Clone Project
```bash
# SSH into Pi
ssh pi@192.168.1.100

# Clone project
git clone https://github.com/your-repo/Pi-Zero-SKR-Pico-PUWinder.git
cd Pi-Zero-SKR-Pico-PUWinder/pi_cm4_setup
```

### 2. Run Setup Script
```bash
# Make executable
chmod +x setup_cm4.sh

# Run setup
./setup_cm4.sh

# Reboot
sudo reboot
```

### 3. Test Everything
```bash
# After reboot, SSH back in
ssh pi@192.168.1.100

# Test UART
cd pi_zero
python test_uart.py

# Start web interface
python web_interface.py
```

## 🎮 Touchscreen Testing

### 1. Launch Browser
```bash
# On the touchscreen, open browser
chromium-browser --kiosk http://localhost:5000
```

### 2. Test Interface
- **Connect button** - Should connect to Pico
- **Home button** - Should home all axes
- **Start button** - Should start winding
- **Emergency stop** - Should stop everything

### 3. Auto-start (Optional)
```bash
# Enable auto-start service
sudo systemctl start winding-controller.service

# Check status
sudo systemctl status winding-controller.service
```

## 🔧 Troubleshooting

### Can't SSH:
```bash
# Check if Pi is on network
ping 192.168.1.100

# Check if SSH is enabled
nmap -p 22 192.168.1.100
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

# List available modes
xrandr --output HDMI-1 --mode 1920x1080
```

### UART Issues:
```bash
# Check UART device
ls /dev/ttyUSB* /dev/ttyAMA*

# Test connection
python test_uart.py
```

## 📊 Performance Optimization

### Disable Unnecessary Services:
```bash
sudo systemctl disable bluetooth
sudo systemctl disable wifi-powersave
sudo systemctl disable hciuart
```

### Increase GPU Memory:
```bash
sudo nano /boot/config.txt
# Add: gpu_mem=128
```

### Disable Desktop (if using Lite):
```bash
sudo systemctl set-default multi-user.target
sudo reboot
```

## 🎯 Quick Verification

### 1. Network:
```bash
ping google.com
```

### 2. UART:
```bash
python test_uart.py
```

### 3. Touchscreen:
```bash
xinput list
```

### 4. Web Interface:
```bash
python web_interface.py
# Open browser to http://localhost:5000
```

---

**🎉 Your Pi CM4 is now ready for testing!** This gives you a powerful development platform before moving to the Pi Zero.
