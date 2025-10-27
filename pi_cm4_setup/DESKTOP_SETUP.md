# 🖥️ Pi CM4 Desktop OS Setup Guide

**Using Raspberry Pi OS Desktop (64-bit) for easier testing and development**

## 🎯 Why Desktop OS for Testing?

### ✅ **Advantages:**
- **Easy setup** - GUI makes everything simpler
- **Visual feedback** - See what's happening on screen
- **Easy file management** - Drag & drop, file browser
- **Multiple windows** - Terminal + browser + file manager
- **Easy debugging** - Visual error messages
- **Touchscreen friendly** - Full desktop environment
- **Easy network setup** - GUI network manager
- **Easy UART testing** - Visual serial terminal tools

### ❌ **Disadvantages:**
- **Larger image** (~2GB vs ~500MB)
- **More RAM usage** (~300MB vs ~100MB)
- **Slower boot** (~30 seconds vs ~10 seconds)

## 🚀 Quick Setup with Desktop OS

### 1. 📱 Flash the Image
```
1. Download Raspberry Pi Imager
2. Choose OS: Raspberry Pi OS Desktop (64-bit)
3. Choose Storage: Your microSD card
4. Click gear icon (⚙️) for advanced options:
   - Enable SSH: ✅
   - Username: pi
   - Password: (your choice)
   - Configure WiFi: ❌ (you're using Ethernet)
   - Set static IP: ✅
     - IP: 192.168.1.100
     - Gateway: 192.168.1.1
     - DNS: 8.8.8.8
5. Click "Write"
```

### 2. 🔌 Connect Hardware
```
Pi CM4:
├── Ethernet cable → Router
├── Touchscreen → HDMI + USB
├── Power supply → USB-C
└── MicroSD card → Inserted
```

### 3. 🖥️ First Boot
```
1. Pi boots to desktop
2. Connect to network (if not using static IP)
3. Open terminal
4. SSH from your computer: ssh pi@192.168.1.100
```

### 4. 🚀 Run Setup Script
```bash
# SSH into Pi
ssh pi@192.168.1.100

# Clone project (if not already done)
git clone https://github.com/your-repo/Pi-Zero-SKR-Pico-PUWinder.git
cd Pi-Zero-SKR-Pico-PUWinder/pi_cm4_setup

# Run setup script
chmod +x setup_cm4.sh
./setup_cm4.sh

# Reboot
sudo reboot
```

## 🎮 Desktop-Specific Features

### 1. 🖥️ Touchscreen Desktop
- **Full desktop environment** - Easy to navigate
- **File manager** - Drag & drop files
- **Multiple windows** - Terminal + browser + file manager
- **Easy configuration** - GUI tools for everything

### 2. 🌐 Network Setup
- **GUI network manager** - Easy WiFi/Ethernet setup
- **Visual connection status** - See if connected
- **Easy IP configuration** - GUI static IP setup

### 3. 🔌 UART Testing
- **Visual serial terminal** - See UART data visually
- **Multiple terminal windows** - Test while running controller
- **Easy debugging** - Visual error messages

### 4. 🎯 Development Tools
- **Code editor** - Built-in text editor
- **File browser** - Easy file management
- **Multiple monitors** - If you have multiple displays

## 🚀 Running the Controller

### 1. 🖥️ On the Touchscreen
```bash
# Open terminal on touchscreen
# Navigate to project
cd Pi-Zero-SKR-Pico-PUWinder/pi_zero

# Test UART connection
python test_uart.py

# Start web interface
python web_interface.py

# Open browser
chromium-browser http://localhost:5000
```

### 2. 💻 From Your Computer
```bash
# SSH into Pi
ssh pi@192.168.1.100

# Start controller
cd pi_zero
python web_interface.py

# Open browser on your computer
# Go to: http://192.168.1.100:5000
```

### 3. 🎮 Kiosk Mode (Full Screen)
```bash
# Launch in kiosk mode on touchscreen
chromium-browser --kiosk --disable-infobars http://localhost:5000
```

## 🔧 Desktop-Specific Configuration

### 1. 🖥️ Touchscreen Setup
```bash
# Check if touchscreen is detected
xinput list

# Test touch
xinput test <device-id>

# Rotate display if needed
xrandr --output HDMI-1 --rotate left
```

### 2. 🌐 Network Configuration
```bash
# GUI method:
# Click network icon in taskbar
# Configure connection

# Command line method:
sudo nano /etc/dhcpcd.conf
# Add:
# interface eth0
# static ip_address=192.168.1.100/24
# static routers=192.168.1.1
# static domain_name_servers=8.8.8.8
```

### 3. 🔌 UART Configuration
```bash
# Enable UART
sudo raspi-config
# Interface Options → Serial Port → Login shell: No, Serial port: Yes

# Add user to dialout group
sudo usermod -a -G dialout $USER

# Reboot
sudo reboot
```

## 🎯 Testing Workflow

### 1. 🖥️ Visual Testing
```
1. Open terminal on touchscreen
2. Run: python test_uart.py
3. See results visually
4. Run: python web_interface.py
5. Open browser: http://localhost:5000
6. Test touch interface
```

### 2. 💻 Remote Testing
```
1. SSH from your computer
2. Start controller remotely
3. Open browser on your computer
4. Control from your computer
5. Monitor on touchscreen
```

### 3. 🔄 Development Workflow
```
1. Edit code on your computer
2. Copy to Pi via SCP/SFTP
3. Test on Pi touchscreen
4. Debug visually
5. Repeat
```

## 🚀 Auto-Start Configuration

### 1. 🖥️ Desktop Auto-Start
```bash
# Create desktop shortcut
cat > ~/Desktop/winding-controller.desktop << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Winding Controller
Comment=Pi Zero Winding Controller
Exec=chromium-browser --kiosk --disable-infobars http://localhost:5000
Icon=applications-electronics
Terminal=false
Categories=Electronics;
EOF

chmod +x ~/Desktop/winding-controller.desktop
```

### 2. 🔄 System Service
```bash
# Create systemd service
sudo nano /etc/systemd/system/winding-controller.service

# Add:
[Unit]
Description=Winding Controller
After=graphical.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/Pi-Zero-SKR-Pico-PUWinder/pi_zero
Environment=DISPLAY=:0
ExecStart=/home/pi/Pi-Zero-SKR-Pico-PUWinder/pi_zero/venv/bin/python web_interface.py
Restart=always

[Install]
WantedBy=graphical.target

# Enable service
sudo systemctl enable winding-controller.service
```

## 🔧 Troubleshooting

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

# Rotate if needed
xrandr --output HDMI-1 --rotate left
```

### UART Issues:
```bash
# Check UART device
ls /dev/ttyUSB* /dev/ttyAMA*

# Test connection
python test_uart.py
```

### Network Issues:
```bash
# Check IP
ip addr show

# Test connectivity
ping google.com
```

## 🎯 Performance Optimization

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

### Disable Desktop (if needed):
```bash
sudo systemctl set-default multi-user.target
sudo reboot
```

## 🔄 Migration to Pi Zero

When ready to move to Pi Zero:

1. **Document what works** on CM4 Desktop
2. **Note any issues** and solutions
3. **Flash Pi Zero** with OS Lite (production)
4. **Copy working code**
5. **Test on Pi Zero**

---

**🎉 Desktop OS is perfect for testing!** You get all the visual tools and easy setup while developing your winding controller.
