# Pi CM4 Touchscreen Testing Setup

Setup guide for using Pi CM4 with touchscreen to test the winding controller before moving to Pi Zero.

## 🎯 What You Need

### Hardware:
- **Pi CM4** (any variant)
- **Touchscreen** (7" recommended)
- **Ethernet cable**
- **MicroSD card** (32GB+ recommended)
- **Power supply** (5V 3A)
- **UART adapter** (USB-to-Serial or direct GPIO)

### Software:
- **Raspberry Pi OS Lite** (64-bit)
- **Ethernet connection** to network

## 🚀 Quick Setup

### 1. Flash OS Image
```bash
# Download Raspberry Pi Imager
# https://www.raspberrypi.org/downloads/

# Flash Raspberry Pi OS Lite (64-bit)
# Enable SSH, set username/password
# Set static IP if desired
```

### 2. Initial Setup
```bash
# SSH into Pi CM4
ssh pi@your-pi-ip

# Update system
sudo apt update && sudo apt upgrade -y

# Install required packages
sudo apt install -y python3-pip python3-venv git
```

### 3. Clone Project
```bash
# Clone the project
git clone https://github.com/your-repo/Pi-Zero-SKR-Pico-PUWinder.git
cd Pi-Zero-SKR-Pico-PUWinder/pi_zero
```

### 4. Install Dependencies
```bash
# Create virtual environment
python3 -m venv venv
source venv/bin/activate

# Install Python packages
pip install -r requirements.txt
```

### 5. Configure UART
```bash
# Enable UART
sudo raspi-config
# Interface Options → Serial Port → Login shell: No, Serial port: Yes

# Add user to dialout group
sudo usermod -a -G dialout $USER

# Reboot
sudo reboot
```

## 🖥️ Touchscreen Configuration

### For USB Touchscreen:
```bash
# Usually works automatically
# Check if detected:
lsusb
# Should show touchscreen device
```

### For DSI Touchscreen:
```bash
# Enable DSI in config
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
# Usually works automatically
# Check display:
xrandr
# Should show connected display
```

## 🔌 UART Connection Options

### Option 1: USB-to-Serial Adapter
```
Pi CM4 USB → USB-Serial Adapter → Pico UART
```

### Option 2: Direct GPIO (if available)
```
Pi CM4 GPIO 14 (TX) → Pico GPIO 1 (RX)
Pi CM4 GPIO 15 (RX) → Pico GPIO 0 (TX)
Pi CM4 Ground → Pico Ground
```

### Option 3: USB-C (if Pico supports)
```
Pi CM4 USB → Pico USB-C
```

## 🚀 Running the Controller

### 1. Test UART Connection
```bash
cd pi_zero
python test_uart.py
```

### 2. Start Web Interface
```bash
python web_interface.py
```

### 3. Access from Touchscreen
- Open browser to: `http://localhost:5000`
- Or: `http://pi-cm4-ip:5000`

### 4. Start Simple Control
```bash
python simple_control.py
```

## 📱 Touchscreen Optimization

### Browser Settings:
```bash
# Install Chromium for better touch support
sudo apt install -y chromium-browser

# Launch in kiosk mode
chromium-browser --kiosk --disable-infobars http://localhost:5000
```

### Auto-start on Boot:
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
ExecStart=/usr/bin/chromium-browser --kiosk --disable-infobars http://localhost:5000
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
xrandr --output HDMI-1 --mode 1920x1080

# Rotate display (if needed)
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

## 🎯 Testing Workflow

### 1. Basic Test:
```bash
# Test UART
python test_uart.py

# Test web interface
python web_interface.py
# Open browser to http://localhost:5000
```

### 2. Full Test:
```bash
# Start controller
python simple_control.py

# Test commands:
> test      # Test UART
> home      # Home axes
> start     # Start winding
> status    # Check progress
```

### 3. Touchscreen Test:
```bash
# Launch in kiosk mode
chromium-browser --kiosk http://localhost:5000

# Test touch interface
# - Connect button
# - Home button
# - Start winding
# - Emergency stop
```

## 📊 Performance Notes

### Pi CM4 vs Pi Zero:
- **Much faster** - Better for development
- **More RAM** - Can handle complex interfaces
- **Better GPU** - Smoother touchscreen experience
- **Ethernet** - Reliable network connection

### Optimization:
```bash
# Disable unnecessary services
sudo systemctl disable bluetooth
sudo systemctl disable wifi-powersave

# Increase GPU memory
sudo nano /boot/config.txt
# Add: gpu_mem=128
```

## 🔄 Migration to Pi Zero

When ready to move to Pi Zero:
1. **Test everything** on CM4 first
2. **Document working config**
3. **Flash Pi Zero** with same setup
4. **Copy working code**
5. **Test on Pi Zero**

---

**🎉 Your Pi CM4 is now ready for testing!** This gives you a powerful development platform before moving to the Pi Zero.
