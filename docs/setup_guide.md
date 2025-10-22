# 🚀 Setup Guide - Pi Zero SKR Pico PUWinder

## 🤖 AI-Generated Setup Instructions

This guide is automatically generated to ensure it matches your current codebase.

## 📋 Prerequisites

### Hardware Requirements
- **Raspberry Pi Zero W** (or Pi Zero 2 W)
- **SKR Pico v1.0** (or compatible)
- **BLDC Motor** with Hall sensors
- **Stepper Motor** for traverse
- **Power Supply** 5V 3A minimum
- **SD Card** 32GB Class 10 minimum

### Software Requirements
- **Python 3.11+** on Pi Zero
- **Pico SDK** for firmware development
- **Git** for version control
- **GitHub account** for CI/CD

## 🔧 Hardware Setup

### 1. Pi Zero Configuration
```bash
# Enable UART
sudo raspi-config
# Navigate to: Interfacing Options → Serial
# Select: Yes (login shell disabled, serial enabled)

# Update system
sudo apt update && sudo apt upgrade -y

# Install dependencies
sudo apt install -y python3-pip git cmake
```

### 2. SKR Pico Configuration
```bash
# Install Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Set environment variable
echo 'export PICO_SDK_PATH=$PWD' >> ~/.bashrc
source ~/.bashrc
```

### 3. Wiring Connections
```
Pi Zero          SKR Pico
GPIO 14 (TXD) →  GPIO 0 (RX)
GPIO 15 (RXD) →  GPIO 1 (TX)
GND           →  GND
5V            →  VIN
```

## 💻 Software Setup

### 1. Clone Repository
```bash
git clone https://github.com/your-username/Pi-Zero-SKR-Pico-PUWinder.git
cd Pi-Zero-SKR-Pico-PUWinder
```

### 2. Pi Zero Setup
```bash
cd pi_zero
pip install -r requirements.txt

# Test UART communication
python test_uart.py

# Test spindle control
python test_spindle.py
```

### 3. Pico Firmware Setup
```bash
cd pico_firmware
mkdir build && cd build
cmake ..
make

# Flash firmware to Pico
cp pico_spindle_controller.uf2 /media/your-username/RPI-RP2/
```

## 🧪 Testing Setup

### 1. Basic Communication Test
```bash
# On Pi Zero
cd pi_zero
python test_uart.py

# Expected output:
# ✅ Opened /dev/serial0 @ 115200 baud
# 📤 Sending: PING
# 📥 Waiting for response...
# ✅ Received: 'PONG'
# 🎉 SUCCESS! UART communication working!
```

### 2. Spindle Control Test
```bash
# Test spindle commands
python test_spindle.py

# Expected output:
# ✅ Spindle control test passed
# ✅ RPM control working
# ✅ Direction control working
```

### 3. G-code Interface Test
```bash
# Test G-code processing
python test_gcode_interface.py

# Expected output:
# ✅ G-code parsing working
# ✅ Command execution working
# ✅ Error handling working
```

## 🔄 Development Setup

### 1. GitHub Integration
```bash
# Configure Git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Set up GitHub CLI
gh auth login
```

### 2. Cursor IDE Setup
```bash
# Install Cursor
# Download from: https://cursor.sh

# Open project
cursor .

# Install extensions:
# - Python
# - C/C++
# - GitHub Copilot
# - GitHub Copilot Chat
```

### 3. GitHub Codespaces (Alternative)
```bash
# Go to GitHub repository
# Click "Code" → "Codespaces"
# Click "Create codespace on main"
# Wait for environment setup
```

## 🚀 Quick Start

### 1. Start the System
```bash
# On Pi Zero
cd pi_zero
python main_controller.py
```

### 2. Send Test Commands
```bash
# In another terminal
python test_uart.py

# Send G-code commands
echo "M3 S1000" | python uart_api.py
echo "G1 Y10 F1000" | python uart_api.py
echo "M5" | python uart_api.py
```

### 3. Monitor Performance
```bash
# Check system status
python test_github_integration.py

# Monitor logs
tail -f logs/winder.log
```

## 🔧 Troubleshooting

### Common Issues

#### UART Communication Fails
```bash
# Check UART is enabled
sudo raspi-config
# Interfacing Options → Serial → Yes

# Check permissions
sudo usermod -a -G dialout $USER
# Logout and login again
```

#### Pico Not Responding
```bash
# Check firmware is flashed
ls /media/your-username/RPI-RP2/

# Reflash firmware
cp pico_firmware/build/pico_spindle_controller.uf2 /media/your-username/RPI-RP2/
```

#### Python Dependencies Missing
```bash
# Install missing packages
pip install pyserial pytest

# Or install all requirements
pip install -r requirements.txt
```

### Getting Help

#### Check Logs
```bash
# System logs
sudo journalctl -u your-service

# Application logs
tail -f logs/winder.log
```

#### GitHub Issues
- Use the issue templates in the repository
- Include hardware configuration
- Provide error logs
- Describe steps to reproduce

## 📚 Next Steps

### 1. Explore Examples
- Check `examples/` directory
- Run sample programs
- Modify parameters

### 2. Read Documentation
- API documentation in `docs/`
- Configuration guide
- Troubleshooting tips

### 3. Join Community
- GitHub Discussions
- Issue tracker
- Pull requests

---

*This setup guide is automatically generated and updated by AI to ensure accuracy.*
