# Troubleshooting Guide

## 🔧 Pi Zero SKR Pico PUWinder Troubleshooting

**AI-Generated Documentation** - Last Updated: 2025-10-25 03:51:01

### 🚨 Common Issues and Solutions

#### 1. UART Communication Issues

**Problem**: Pi Zero cannot communicate with Pico
```
❌ Failed to connect to Pico
❌ Timeout - no response
```

**Solutions**:
1. **Check Wiring**:
   ```
   Pi Zero TX (GPIO 14) → Pico RX (GPIO 1)
   Pi Zero RX (GPIO 15) → Pico TX (GPIO 0)
   GND → GND
   ```

2. **Enable UART**:
   ```bash
   sudo raspi-config
   # Interfacing Options → Serial
   # Enable serial port hardware
   ```

3. **Check Baud Rate**:
   ```python
   # Should be 115200
   api = GCodeAPI(baudrate=115200)
   ```

4. **Test Connection**:
   ```bash
   python3 pi_zero/test_uart.py
   ```

#### 2. Firmware Issues

**Problem**: Pico firmware not responding
```
❌ PING failed: None
❌ VERSION failed
```

**Solutions**:
1. **Reflash Firmware**:
   ```bash
   cd pico_firmware
   mkdir build && cd build
   cmake ..
   make -j4
   # Flash to Pico
   ```

2. **Check Power Supply**:
   - Ensure stable 5V supply
   - Check current capacity

3. **Reset Pico**:
   - Press and hold BOOTSEL button
   - Power on Pico
   - Release BOOTSEL

#### 3. Motor Control Issues

**Problem**: Motors not moving
```
❌ M3 S500 failed
❌ G1 Y50 F1000 failed
```

**Solutions**:
1. **Check Enable Pins**:
   ```python
   # Enable steppers
   api.enable_steppers(True)
   ```

2. **Check Motor Connections**:
   ```
   Spindle: PWM=GPIO24, Enable=GPIO21
   Traverse: Step=GPIO6, Dir=GPIO5, Enable=GPIO7
   ```

3. **Check Power Supply**:
   - Spindle: 12V/24V supply
   - Stepper: 12V/24V supply

4. **Test Individual Motors**:
   ```python
   # Test spindle
   api.set_spindle_rpm(100, 'CW')
   
   # Test traverse
   api.move_traverse(10.0, 100.0)
   ```

#### 4. Hall Sensor Issues

**Problem**: Spindle RPM not reading correctly
```
❌ Hall sensor not working
❌ RPM always 0
```

**Solutions**:
1. **Check Wiring**:
   ```
   Hall Sensor → GPIO 22
   VCC → 3.3V
   GND → GND
   ```

2. **Check Pull-up Resistors**:
   - Add 10kΩ pull-up to 3.3V
   - Check signal levels with oscilloscope

3. **Test Sensor**:
   ```python
   # Check if sensor is working
   status = api.get_machine_status()
   print(f"Spindle RPM: {status.get('spindle_rpm', 0)}")
   ```

#### 5. Safety System Issues

**Problem**: Safety commands not working
```
❌ M112 (emergency stop) failed
❌ M0 (feed hold) failed
```

**Solutions**:
1. **Check Move Queue**:
   ```python
   # Ensure move queue is initialized
   queue_status = api.get_move_queue_status()
   ```

2. **Test Safety Commands**:
   ```bash
   python3 pi_zero/test_gcode_safety.py
   ```

3. **Check Emergency Stop**:
   ```python
   # Test emergency stop
   api.emergency_stop()
   api.reset_from_emergency()
   ```

### 🔍 Diagnostic Commands

#### 1. System Status Check
```python
from main_controller import GCodeAPI

api = GCodeAPI()
api.connect()

# Check system status
status = api.get_machine_status()
print(f"Status: {status}")

# Check move queue
queue_status = api.get_move_queue_status()
print(f"Queue: {queue_status}")

# Check scheduler
scheduler_status = api.get_scheduler_status()
print(f"Scheduler: {scheduler_status}")
```

#### 2. Hardware Test
```python
# Test all GPIO pins
for pin in [3, 4, 5, 6, 7, 21, 22, 24, 25]:
    api.set_gpio_pin(pin, 1)
    time.sleep(0.1)
    api.set_gpio_pin(pin, 0)
```

#### 3. Communication Test
```python
# Test all G-code commands
commands = [
    "PING", "VERSION", "STATUS",
    "M3 S100", "M5", "G28", "M17", "M18"
]

for cmd in commands:
    response = api.uart_api.send_command(cmd)
    print(f"{cmd}: {response}")
```

### 📊 Performance Monitoring

#### 1. System Metrics
```python
# Monitor system performance
import time

start_time = time.time()
for i in range(100):
    status = api.get_machine_status()
    time.sleep(0.1)

elapsed = time.time() - start_time
print(f"Average response time: {elapsed/100:.3f}s per command")
```

#### 2. Memory Usage
```bash
# Check Pi Zero memory usage
free -h

# Check Python memory usage
ps aux | grep python
```

#### 3. CPU Usage
```bash
# Monitor CPU usage
top -p $(pgrep -f python)
```

### 🆘 Emergency Procedures

#### 1. Complete System Reset
```bash
# Stop all processes
sudo pkill -f python

# Reset GPIO pins
sudo gpio reset

# Restart system
sudo reboot
```

#### 2. Firmware Recovery
```bash
# Enter bootloader mode
# Hold BOOTSEL button while powering on

# Flash firmware
sudo picotool load pico_firmware/build/pico_spindle_controller.uf2
```

#### 3. Configuration Reset
```bash
# Backup current config
cp pi_zero/machine.cfg pi_zero/machine.cfg.backup

# Reset to defaults
git checkout pi_zero/machine.cfg
```

### 📞 Getting Help

#### 1. Check Logs
```bash
# System logs
sudo journalctl -f

# Python logs
tail -f /var/log/python.log
```

#### 2. GitHub Issues
- **Report Issues**: [GitHub Issues](https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder/issues)
- **Check Existing**: Search for similar issues
- **Provide Details**: Include logs, configuration, hardware setup

#### 3. Community Support
- **GitHub Discussions**: [Discussions](https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder/discussions)
- **Documentation**: [Project Docs](https://baztardo.github.io/Pi-Zero-SKR-Pico-PUWinder/)

---
*This documentation is automatically generated by AI and updated on every push.*
