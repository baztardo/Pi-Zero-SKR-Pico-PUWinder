# 🔧 Troubleshooting Guide - Pi Zero SKR Pico PUWinder

## 🤖 AI-Generated Troubleshooting

This guide is automatically generated based on common issues and solutions.

## 🚨 Common Issues

### 1. UART Communication Problems

#### Issue: "Serial port not found"
```bash
# Error: SerialException: could not open port /dev/serial0
```

#### Solutions:
```bash
# 1. Check UART is enabled
sudo raspi-config
# Navigate to: Interfacing Options → Serial
# Select: Yes (login shell disabled, serial enabled)

# 2. Check permissions
sudo usermod -a -g dialout $USER
# Logout and login again

# 3. Check device exists
ls -la /dev/serial*
# Should show: /dev/serial0 -> ttyAMA0

# 4. Test with minicom
sudo apt install minicom
sudo minicom -D /dev/serial0 -b 115200
```

#### Debug Commands:
```bash
# Check UART status
sudo dmesg | grep -i uart

# Check permissions
ls -la /dev/serial0

# Test communication
echo "PING" > /dev/serial0
cat /dev/serial0
```

### 2. Pico Firmware Issues

#### Issue: "Pico not responding"
```bash
# Error: No response from Pico
```

#### Solutions:
```bash
# 1. Check firmware is flashed
ls /media/your-username/RPI-RP2/
# Should show: pico_spindle_controller.uf2

# 2. Reflash firmware
cd pico_firmware
mkdir build && cd build
cmake ..
make
cp pico_spindle_controller.uf2 /media/your-username/RPI-RP2/

# 3. Check Pico is in bootloader mode
# Hold BOOTSEL button while connecting USB
# Should appear as RPI-RP2 drive
```

#### Debug Commands:
```bash
# Check Pico connection
lsusb | grep -i pico

# Check mount points
ls /media/your-username/

# Test with picotool
pip install picotool
picotool info
```

### 3. Python Dependencies

#### Issue: "ModuleNotFoundError"
```bash
# Error: ModuleNotFoundError: No module named 'serial'
```

#### Solutions:
```bash
# 1. Install missing packages
pip install pyserial pytest

# 2. Install all requirements
cd pi_zero
pip install -r requirements.txt

# 3. Use virtual environment
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

#### Debug Commands:
```bash
# Check installed packages
pip list | grep serial

# Check Python path
python -c "import sys; print(sys.path)"

# Test imports
python -c "import serial; print('pyserial OK')"
```

### 4. Build Issues

#### Issue: "CMake failed"
```bash
# Error: CMake Error: Could not find PICO_SDK_PATH
```

#### Solutions:
```bash
# 1. Set PICO_SDK_PATH
export PICO_SDK_PATH=/path/to/pico-sdk
echo 'export PICO_SDK_PATH=/path/to/pico-sdk' >> ~/.bashrc

# 2. Install dependencies
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi

# 3. Clean build
rm -rf build
mkdir build && cd build
cmake ..
make
```

#### Debug Commands:
```bash
# Check environment
echo $PICO_SDK_PATH

# Check CMake version
cmake --version

# Check toolchain
arm-none-eabi-gcc --version
```

### 5. GitHub Actions Failures

#### Issue: "Workflow failed"
```bash
# Error: Job failed in GitHub Actions
```

#### Solutions:
```bash
# 1. Check workflow logs
# Go to: GitHub → Actions → Failed workflow
# Click on failed job to see details

# 2. Test locally
cd pi_zero
python test_github_integration.py

# 3. Check dependencies
pip install -r requirements.txt

# 4. Use simple CI
# Rename: .github/workflows/simple-ci.yml → ci.yml
```

#### Debug Commands:
```bash
# Test Python syntax
python -m py_compile *.py

# Test imports
python -c "import sys; sys.path.append('.'); import test_github_integration"

# Check file permissions
ls -la *.py
```

## 🔍 Diagnostic Tools

### 1. System Information
```bash
# Check Pi Zero specs
cat /proc/cpuinfo
cat /proc/meminfo

# Check Python version
python --version
pip --version

# Check installed packages
pip list
```

### 2. Hardware Status
```bash
# Check GPIO
gpio readall

# Check UART
sudo dmesg | grep -i uart

# Check USB devices
lsusb
```

### 3. Network Status
```bash
# Check WiFi
iwconfig

# Check IP address
ip addr show

# Check GitHub connection
ping github.com
```

### 4. Log Analysis
```bash
# System logs
sudo journalctl -f

# Application logs
tail -f logs/winder.log

# Error logs
grep -i error logs/winder.log
```

## 🛠️ Advanced Troubleshooting

### 1. Performance Issues
```bash
# Check CPU usage
top

# Check memory usage
free -h

# Check disk usage
df -h

# Check temperature
vcgencmd measure_temp
```

### 2. Communication Debugging
```bash
# Monitor UART traffic
sudo cat /dev/serial0 | hexdump -C

# Test with different baud rates
stty -F /dev/serial0 9600
stty -F /dev/serial0 115200
```

### 3. Firmware Debugging
```bash
# Check firmware version
picotool info

# Read firmware
picotool read -o firmware.uf2

# Verify firmware
picotool verify firmware.uf2
```

## 📞 Getting Help

### 1. GitHub Issues
- Use issue templates
- Include system information
- Provide error logs
- Describe steps to reproduce

### 2. Community Support
- GitHub Discussions
- Stack Overflow
- Raspberry Pi forums
- Pico SDK documentation

### 3. Professional Support
- Contact maintainers
- Hire consultants
- Training courses
- Custom development

## 🔄 Prevention

### 1. Regular Maintenance
```bash
# Update system
sudo apt update && sudo apt upgrade

# Update Python packages
pip install --upgrade pip
pip install --upgrade -r requirements.txt

# Clean build files
rm -rf build/
```

### 2. Backup Strategy
```bash
# Backup configuration
cp -r pi_zero/config/ backup/

# Backup firmware
cp pico_firmware/build/*.uf2 backup/

# Backup code
git push origin main
```

### 3. Monitoring
```bash
# Set up log rotation
sudo logrotate -f /etc/logrotate.conf

# Monitor disk space
df -h | grep -E "(Filesystem|/dev/root)"

# Monitor system health
vcgencmd get_throttled
```

---

*This troubleshooting guide is automatically generated and updated by AI to ensure accuracy and completeness.*
