#!/usr/bin/env python3
"""
AI Documentation Generator
Purpose: Generate comprehensive documentation for the Pi Zero SKR Pico PUWinder project
"""

import os
import ast
from pathlib import Path
from datetime import datetime

def analyze_python_file(file_path):
    """Analyze a Python file and extract information"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        tree = ast.parse(content)
        
        classes = []
        functions = []
        imports = []
        
        for node in ast.walk(tree):
            if isinstance(node, ast.ClassDef):
                classes.append({
                    'name': node.name,
                    'docstring': ast.get_docstring(node) or '',
                    'methods': [n.name for n in node.body if isinstance(n, ast.FunctionDef)]
                })
            elif isinstance(node, ast.FunctionDef) and not isinstance(node.parent, ast.ClassDef):
                functions.append({
                    'name': node.name,
                    'docstring': ast.get_docstring(node) or ''
                })
            elif isinstance(node, ast.Import):
                imports.extend([n.name for n in node.names])
            elif isinstance(node, ast.ImportFrom):
                imports.extend([n.name for n in node.names])
        
        return {
            'classes': classes,
            'functions': functions,
            'imports': imports,
            'lines': len(content.splitlines())
        }
    except Exception as e:
        print(f"Error analyzing {file_path}: {e}")
        return None

def analyze_cpp_file(file_path):
    """Analyze a C++ file and extract information"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        classes = []
        functions = []
        
        # Simple regex-based extraction (could be improved)
        import re
        
        # Find class definitions
        class_pattern = r'class\s+(\w+)\s*\{'
        for match in re.finditer(class_pattern, content):
            classes.append({'name': match.group(1)})
        
        # Find function definitions
        func_pattern = r'(\w+)\s+(\w+)\s*\([^)]*\)\s*\{'
        for match in re.finditer(func_pattern, content):
            functions.append({'name': match.group(2)})
        
        return {
            'classes': classes,
            'functions': functions,
            'lines': len(content.splitlines())
        }
    except Exception as e:
        print(f"Error analyzing {file_path}: {e}")
        return None

def generate_project_overview():
    """Generate project overview documentation"""
    content = """# Project Overview

## 🤖 Pi Zero SKR Pico PUWinder

**AI-Generated Documentation** - Last Updated: """ + datetime.now().strftime('%Y-%m-%d %H:%M:%S') + """

### 📋 Project Description

The Pi Zero SKR Pico PUWinder is a precision winding machine controller that combines the computational power of a Raspberry Pi Zero with the real-time control capabilities of an SKR Pico microcontroller. This system provides industrial-grade precision for wire winding applications.

### 🏗️ Architecture

#### High-Level Controller (Pi Zero)
- **Role**: Brain of the system
- **Responsibilities**: 
  - G-code command processing
  - High-level winding logic
  - User interface and display
  - Safety monitoring
- **Communication**: UART to Pico

#### Low-Level Controller (SKR Pico)
- **Role**: Real-time hardware control
- **Responsibilities**:
  - Stepper motor control (traverse)
  - BLDC motor control (spindle)
  - Hall sensor feedback
  - ISR-driven step generation
- **Communication**: UART from Pi Zero

### 🔧 Key Components

#### Hardware
- **Spindle**: BLDC motor with Hall sensor feedback
- **Traverse**: Stepper motor with TMC2209 driver
- **Communication**: UART between Pi Zero and Pico
- **Safety**: Emergency stop, feed hold, quick stop

#### Software
- **Pi Zero**: Python-based G-code interpreter
- **Pico**: C++ firmware with real-time ISR
- **Protocol**: G-code compatible commands
- **Safety**: FluidNC-style safety features

### 🎯 Features

#### G-code Compatibility
- Standard G-codes: G0, G1, G28, G4
- Spindle control: M3, M4, M5, S
- Winding-specific: M6-M19
- Safety commands: M0, M1, M112, M410, M999

#### Safety Features
- **Feed Hold (M0)**: Pause all motion
- **Resume (M1)**: Resume from hold
- **Emergency Stop (M112)**: Immediate stop
- **Quick Stop (M410)**: Stop new moves, finish current
- **Reset (M999)**: Clear emergency state

#### Precision Control
- **Spindle RPM**: 0-3000 RPM with Hall sensor feedback
- **Traverse Position**: 0-200mm with 0.1mm precision
- **Synchronization**: Real-time spindle-traverse sync
- **Ramping**: Smooth acceleration/deceleration

### 📁 Project Structure

```
Pi-Zero-SKR-Pico-PUWinder/
├── pi_zero/                 # Pi Zero Python code
│   ├── main_controller.py   # Main G-code API
│   ├── uart_api.py         # UART communication
│   ├── winding_sync.py     # Winding synchronization
│   └── test_*.py           # Test suites
├── pico_firmware/          # Pico C++ firmware
│   ├── main.cpp           # Main application
│   ├── src/               # Source files
│   │   ├── spindle.cpp     # BLDC motor control
│   │   ├── traverse_controller.cpp  # Stepper control
│   │   ├── gcode_interface.cpp      # G-code parser
│   │   ├── move_queue.cpp           # Move queue
│   │   └── scheduler.cpp            # ISR scheduler
│   └── CMakeLists.txt     # Build configuration
├── docs/                  # Documentation
└── .github/workflows/     # CI/CD workflows
```

### 🚀 Getting Started

1. **Hardware Setup**: Connect Pi Zero to SKR Pico via UART
2. **Firmware**: Flash Pico with C++ firmware
3. **Software**: Install Python dependencies on Pi Zero
4. **Configuration**: Set up machine.cfg with your hardware
5. **Testing**: Run test suites to verify functionality

### 🔗 External Links

- **GitHub Repository**: [Pi-Zero-SKR-Pico-PUWinder](https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder)
- **CI/CD Status**: [GitHub Actions](https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder/actions)
- **Issues**: [GitHub Issues](https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder/issues)

### 📊 Statistics

- **Total Files**: """ + str(len(list(Path('.').rglob('*.py'))) + len(list(Path('.').rglob('*.cpp'))) + len(list(Path('.').rglob('*.h')))) + """
- **Python Files**: """ + str(len(list(Path('.').rglob('*.py')))) + """
- **C++ Files**: """ + str(len(list(Path('.').rglob('*.cpp'))) + len(list(Path('.').rglob('*.h')))) + """
- **Test Files**: """ + str(len(list(Path('.').rglob('test_*.py')))) + """
- **Documentation**: """ + str(len(list(Path('docs').rglob('*.md'))) if Path('docs').exists() else 0) + """

---
*This documentation is automatically generated by AI and updated on every push.*
"""
    return content

def generate_setup_guide():
    """Generate setup guide documentation"""
    content = """# Setup Guide

## 🛠️ Pi Zero SKR Pico PUWinder Setup

**AI-Generated Documentation** - Last Updated: """ + datetime.now().strftime('%Y-%m-%d %H:%M:%S') + """

### 📋 Prerequisites

#### Hardware Requirements
- Raspberry Pi Zero (or Zero W)
- SKR Pico v1.0 microcontroller
- BLDC motor with Hall sensor
- Stepper motor with TMC2209 driver
- UART connection between Pi Zero and Pico

#### Software Requirements
- Raspberry Pi OS (latest)
- Python 3.11+
- Pico SDK (for firmware compilation)
- CMake (for building firmware)

### 🔧 Hardware Setup

#### 1. Pi Zero Configuration
```bash
# Enable UART
sudo raspi-config
# Navigate to: Interfacing Options → Serial
# Enable: Would you like a login shell to be accessible over serial? → No
# Enable: Would you like the serial port hardware to be enabled? → Yes
```

#### 2. UART Connection
```
Pi Zero          SKR Pico
GPIO 14 (TXD) →  GPIO 1 (RX)
GPIO 15 (RXD) ←  GPIO 0 (TX)
GND           →  GND
```

#### 3. Motor Connections
```
Spindle (BLDC):
- PWM: GPIO 24
- Enable: GPIO 21
- Direction: GPIO 4
- Brake: GPIO 3
- Hall Sensor: GPIO 22

Traverse (Stepper):
- Step: GPIO 6
- Direction: GPIO 5
- Enable: GPIO 7
- Home Switch: GPIO 16
```

### 💻 Software Installation

#### 1. Pi Zero Setup
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Python dependencies
sudo apt install python3-serial python3-pip -y
pip3 install pyserial pytest

# Clone repository
git clone https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder.git
cd Pi-Zero-SKR-Pico-PUWinder
```

#### 2. Pico Firmware Setup
```bash
# Install Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Set environment variable
export PICO_SDK_PATH=/path/to/pico-sdk

# Build firmware
cd pico_firmware
mkdir build && cd build
cmake ..
make -j4
```

### ⚙️ Configuration

#### 1. Machine Configuration
Edit `pi_zero/machine.cfg` to match your hardware:

```ini
# Spindle Configuration
[spindle]
pin: gpio24          # PWM output
enable_pin: gpio21   # Enable pin
direction_pin: gpio4 # Direction control
brake_pin: gpio3     # Brake control
hall_pin: gpio22     # Hall sensor

# Traverse Configuration
[stepper_y]
step_pin: gpio6      # Step pin
dir_pin: gpio5       # Direction pin
enable_pin: !gpio7   # Enable pin
position_endstop: gpio16  # Home switch
```

#### 2. Test Configuration
```bash
# Test UART communication
python3 pi_zero/test_uart.py

# Test G-code commands
python3 pi_zero/test_gcode_safety.py

# Test comprehensive features
python3 pi_zero/test_comprehensive_features.py
```

### 🧪 Testing

#### 1. Basic Connectivity Test
```bash
python3 pi_zero/test_uart.py
```

Expected output:
```
✅ Opened /dev/serial0 @ 115200 baud
✅ Received: 'PONG'
✅ Received: Pico_Spindle_v1.0
```

#### 2. G-code Safety Test
```bash
python3 pi_zero/test_gcode_safety.py
```

Expected output:
```
✅ M0 (feed hold) - OK
✅ M1 (resume from hold) - OK
✅ M112 (emergency stop) - OK
✅ M999 (reset from emergency stop) - OK
```

#### 3. Comprehensive Feature Test
```bash
python3 pi_zero/test_comprehensive_features.py
```

### 🚀 Running the System

#### 1. Start the Main Controller
```bash
python3 pi_zero/main_controller.py
```

#### 2. Basic G-code Commands
```python
from main_controller import GCodeAPI

# Connect to Pico
api = GCodeAPI()
api.connect()

# Start spindle
api.set_spindle_rpm(500, 'CW')

# Move traverse
api.move_traverse(50.0, 1000.0)

# Stop spindle
api.stop_spindle()
```

### 🔧 Troubleshooting

#### Common Issues

1. **UART Communication Failed**
   - Check wiring connections
   - Verify UART is enabled in raspi-config
   - Check baud rate (115200)

2. **Firmware Not Responding**
   - Reflash Pico firmware
   - Check power supply
   - Verify UART pins

3. **Motor Not Moving**
   - Check motor connections
   - Verify enable pins
   - Check power supply

4. **Hall Sensor Not Working**
   - Check sensor wiring
   - Verify pull-up resistors
   - Check signal levels

### 📞 Support

- **GitHub Issues**: [Report Issues](https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder/issues)
- **Documentation**: [Project Docs](https://baztardo.github.io/Pi-Zero-SKR-Pico-PUWinder/)
- **CI/CD Status**: [GitHub Actions](https://github.com/baztardo/Pi-Zero-SKR-Pico-PUWinder/actions)

---
*This documentation is automatically generated by AI and updated on every push.*
"""
    return content

def generate_troubleshooting_guide():
    """Generate troubleshooting guide"""
    content = """# Troubleshooting Guide

## 🔧 Pi Zero SKR Pico PUWinder Troubleshooting

**AI-Generated Documentation** - Last Updated: """ + datetime.now().strftime('%Y-%m-%d %H:%M:%S') + """

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
"""
    return content

def generate_python_api():
    """Generate Python API documentation"""
    content = """# Python API Reference

## 🐍 Pi Zero SKR Pico PUWinder Python API

**AI-Generated Documentation** - Last Updated: """ + datetime.now().strftime('%Y-%m-%d %H:%M:%S') + """

### 📋 Overview

The Python API provides a high-level interface for controlling the winding machine from the Pi Zero. It handles G-code generation, UART communication, and safety features.

### 🔧 Core Classes

#### GCodeAPI
Main API class for G-code communication with the Pico.

```python
from main_controller import GCodeAPI

# Create API instance
api = GCodeAPI(port='/dev/serial0', baudrate=115200)

# Connect to Pico
if api.connect():
    print("Connected successfully")
```

#### WindingController
High-level winding process controller.

```python
from main_controller import WindingController, WindingParameters

# Create winding parameters
params = WindingParameters(
    target_turns=1000,
    spindle_rpm=300,
    wire_diameter_mm=0.064,
    winding_width_mm=50.0
)

# Create controller
controller = WindingController(api)
controller.set_parameters(params)
```

### 🎯 G-code Commands

#### Basic Movement
```python
# Rapid positioning
api.send_gcode("G0 Y50")

# Linear interpolation
api.send_gcode("G1 Y100 F1000")

# Home all axes
api.home_all_axes()
```

#### Spindle Control
```python
# Start spindle clockwise
api.set_spindle_rpm(500, 'CW')

# Start spindle counter-clockwise
api.set_spindle_rpm(1000, 'CCW')

# Stop spindle
api.stop_spindle()

# Set spindle speed
api.send_gcode("S1500")
```

#### Safety Commands
```python
# Feed hold
api.feed_hold()

# Resume from hold
api.resume_from_hold()

# Emergency stop
api.emergency_stop()

# Quick stop
api.quick_stop()

# Reset from emergency
api.reset_from_emergency()
```

### 🛡️ Safety Features

#### FluidNC-style Safety Commands
```python
# M0 - Feed hold
api.feed_hold()

# M1 - Resume from hold
api.resume_from_hold()

# M112 - Emergency stop
api.emergency_stop()

# M410 - Quick stop
api.quick_stop()

# M999 - Reset from emergency stop
api.reset_from_emergency()

# G4 - Dwell with planner sync
api.dwell_with_sync(1000.0)  # 1 second
api.dwell_with_sync(0.0)     # Sync planner
```

#### Winding Machine M-codes
```python
# M6 - Tool change (wire change)
api.set_wire_diameter(0.064)

# M7/M8 - Coolant control
api.enable_cooling(True)
api.enable_cooling(False)

# M10/M11 - Traverse brake
api.enable_traverse_brake(True)
api.enable_traverse_brake(False)

# M12/M13 - Spindle brake
api.send_gcode("M12")  # Enable brake
api.send_gcode("M13")  # Disable brake

# M14/M15 - Wire tension
api.enable_wire_tension(True)
api.enable_wire_tension(False)

# M16 - Home all axes
api.home_all_axes()

# M17/M18 - Stepper control
api.enable_steppers(True)
api.enable_steppers(False)

# M19 - Spindle orientation
api.send_gcode("M19")
```

### 📊 Status and Monitoring

#### Machine Status
```python
# Get comprehensive machine status
status = api.get_machine_status()
print(f"Spindle RPM: {status.get('spindle_rpm', 0)}")
print(f"Spindle Running: {status.get('spindle_running', False)}")
print(f"Traverse Position: {status.get('traverse_position', 0)} mm")
```

#### Move Queue Status
```python
# Get move queue information
queue_status = api.get_move_queue_status()
print(f"Spindle Queue Depth: {queue_status['spindle_queue_depth']}")
print(f"Traverse Queue Depth: {queue_status['traverse_queue_depth']}")
print(f"Spindle Active: {queue_status['spindle_active']}")
print(f"Traverse Active: {queue_status['traverse_active']}")
```

#### Scheduler Status
```python
# Get scheduler information
scheduler_status = api.get_scheduler_status()
print(f"Scheduler Running: {scheduler_status['is_running']}")
print(f"Tick Count: {scheduler_status['tick_count']}")
print(f"Interval: {scheduler_status['interval_us']} μs")
```

### 🔧 GPIO Control

#### Pin Control
```python
# Set GPIO pin state
api.set_gpio_pin(25, 1)  # Set GPIO 25 high
api.set_gpio_pin(25, 0)  # Set GPIO 25 low

# Set pin value
api.send_gcode("M42 P25 S1")  # Set GPIO 25 high
api.send_gcode("M47 P25 S0")  # Set GPIO 25 low
```

### 🧪 Testing and Validation

#### Basic Connectivity Test
```python
# Test UART communication
if api.connect():
    print("✅ Connected to Pico")
    
    # Test PING
    response = api.uart_api.send_command("PING")
    if response == "PONG":
        print("✅ PING/PONG working")
    
    # Test VERSION
    response = api.uart_api.send_command("VERSION")
    print(f"✅ Version: {response}")
```

#### Comprehensive Feature Test
```python
# Run comprehensive test
import test_comprehensive_features
test_comprehensive_features.test_comprehensive_features()
```

### 📈 Advanced Features

#### Enhanced Spindle Control
```python
# Set spindle ramp rate (placeholder)
api.set_spindle_ramp_rate(10.0)  # 10% per second

# Set spindle limits (placeholder)
api.set_spindle_max_rpm(3000.0)
api.set_spindle_min_rpm(10.0)

# Get ramp status (placeholder)
ramp_status = api.get_spindle_ramp_status()
print(f"Ramping: {ramp_status['is_ramping']}")
print(f"Progress: {ramp_status['progress']:.1%}")
```

#### Step Counting
```python
# Get step counts (placeholder)
step_counts = api.get_step_counts()
print(f"Spindle Steps: {step_counts['spindle_steps']}")
print(f"Traverse Steps: {step_counts['traverse_steps']}")
```

### 🔄 Error Handling

#### Connection Management
```python
# Safe connection handling
try:
    if api.connect():
        # Perform operations
        api.set_spindle_rpm(500, 'CW')
        time.sleep(5)
        api.stop_spindle()
    else:
        print("Failed to connect to Pico")
except Exception as e:
    print(f"Error: {e}")
finally:
    api.disconnect()
```

#### Command Validation
```python
# Validate commands before sending
def safe_spindle_control(api, rpm, direction):
    if not api.connected:
        print("Not connected")
        return False
    
    if rpm < 0 or rpm > 3000:
        print("RPM out of range (0-3000)")
        return False
    
    return api.set_spindle_rpm(rpm, direction)
```

### 📚 Examples

#### Complete Winding Process
```python
from main_controller import GCodeAPI, WindingController, WindingParameters

# Setup
api = GCodeAPI()
if not api.connect():
    print("Failed to connect")
    exit(1)

# Create winding parameters
params = WindingParameters(
    target_turns=1000,
    spindle_rpm=300,
    wire_diameter_mm=0.064,
    winding_width_mm=50.0
)

# Create controller
controller = WindingController(api)
controller.set_parameters(params)

# Start winding process
if controller.start():
    print("Winding process started")
    
    # Monitor progress
    while controller.running:
        status = controller.get_status()
        print(f"Layer: {status['current_layer']}, Turns: {status['turns_completed']}")
        time.sleep(1)
    
    # Stop process
    controller.stop()
    print("Winding process completed")

# Disconnect
api.disconnect()
```

---
*This documentation is automatically generated by AI and updated on every push.*
"""
    return content

def generate_cpp_api():
    """Generate C++ API documentation"""
    content = """# C++ API Reference

## ⚡ Pi Zero SKR Pico PUWinder C++ Firmware API

**AI-Generated Documentation** - Last Updated: """ + datetime.now().strftime('%Y-%m-%d %H:%M:%S') + """

### 📋 Overview

The C++ firmware runs on the SKR Pico and provides real-time control of the winding machine hardware. It includes ISR-driven step generation, G-code parsing, and safety features.

### 🏗️ Architecture

#### Main Components
- **main.cpp**: Application entry point and initialization
- **gcode_interface.cpp**: G-code command parser and executor
- **spindle.cpp**: BLDC motor control with Hall sensor feedback
- **traverse_controller.cpp**: Stepper motor control
- **move_queue.cpp**: Move queue with ISR-driven execution
- **scheduler.cpp**: Hardware timer ISR scheduler

### 🔧 Core Classes

#### BLDC_MOTOR
Controls the spindle BLDC motor with Hall sensor feedback.

```cpp
#include "spindle.h"

// Create spindle controller
BLDC_MOTOR* spindle_controller = new BLDC_MOTOR(SPINDLE_HALL_PIN);
spindle_controller->init();

// Get RPM
float rpm = spindle_controller->get_rpm();

// Set direction
spindle_controller->set_direction(DIRECTION_CW);

// Set brake
spindle_controller->set_brake(true);
```

#### TraverseController
Controls the traverse stepper motor.

```cpp
#include "traverse_controller.h"

// Create traverse controller
TraverseController* traverse_controller = new TraverseController();
traverse_controller->init();

// Move to position
traverse_controller->move_to_position(50.0f);

// Home axis
traverse_controller->home();

// Get current position
float position = traverse_controller->get_current_position();
```

#### GCodeInterface
Parses and executes G-code commands from the Pi Zero.

```cpp
#include "gcode_interface.h"

// Create G-code interface
GCodeInterface* gcode_interface = new GCodeInterface();

// Parse command
if (gcode_interface->parse_command("M3 S500")) {
    // Execute command
    gcode_interface->execute_command();
}
```

#### MoveQueue
Manages move chunks with ISR-driven execution.

```cpp
#include "move_queue.h"

// Create move queue
MoveQueue* move_queue = new MoveQueue();
move_queue->init();

// Push move chunk
StepChunk chunk;
// ... populate chunk ...
move_queue->push_chunk(AXIS_TRAVERSE, chunk);

// Check if axis is active
bool active = move_queue->is_active(AXIS_TRAVERSE);
```

#### Scheduler
Hardware timer ISR scheduler for real-time control.

```cpp
#include "scheduler.h"

// Create scheduler
Scheduler* scheduler = new Scheduler(move_queue);

// Start scheduler
scheduler->start(HEARTBEAT_US);

// Check if running
bool running = scheduler->is_running();
```

### 🎯 G-code Commands

#### Movement Commands
```cpp
// G0/G1 - Linear interpolation
gcode_interface->parse_command("G1 Y50 F1000");

// G28 - Home all axes
gcode_interface->parse_command("G28");

// G4 - Dwell with planner sync
gcode_interface->parse_command("G4 P1000");
```

#### Spindle Commands
```cpp
// M3 - Spindle clockwise
gcode_interface->parse_command("M3 S500");

// M4 - Spindle counter-clockwise
gcode_interface->parse_command("M4 S1000");

// M5 - Stop spindle
gcode_interface->parse_command("M5");

// S - Set spindle speed
gcode_interface->parse_command("S1500");
```

#### Safety Commands
```cpp
// M0 - Feed hold
gcode_interface->parse_command("M0");

// M1 - Resume from hold
gcode_interface->parse_command("M1");

// M112 - Emergency stop
gcode_interface->parse_command("M112");

// M410 - Quick stop
gcode_interface->parse_command("M410");

// M999 - Reset from emergency stop
gcode_interface->parse_command("M999");
```

### 🛡️ Safety Features

#### Move Queue Safety
```cpp
// Pause feeding
move_queue->pause_feeding();

// Resume feeding
move_queue->resume_feeding();

// Check if feeding is paused
bool paused = move_queue->is_feeding_paused();

// Emergency stop
move_queue->emergency_stop();

// Check if emergency stop is active
bool emergency = move_queue->is_emergency_stopped();
```

#### Spindle Safety
```cpp
// Set spindle brake
spindle_controller->set_brake(true);

// Check brake status
bool brake = spindle_controller->get_brake();

// Set direction
spindle_controller->set_direction(DIRECTION_CW);
```

### ⚡ ISR and Real-time Control

#### Hardware Timer ISR
```cpp
// Scheduler ISR handler
void scheduler_isr() {
    // Process move queues
    if (move_queue) {
        move_queue->handle_isr_tick();
    }
    
    // Update encoder
    if (spindle_controller) {
        spindle_controller->handle_pulse();
    }
}
```

#### Step Generation
```cpp
// Traverse step ISR
void traverse_step_isr() {
    if (traverse_controller) {
        traverse_controller->stepper_step();
    }
}
```

### 📊 Status and Monitoring

#### Machine Status
```cpp
// Get spindle status
float rpm = spindle_controller->get_rpm();
bool running = !spindle_controller->get_brake();

// Get traverse status
float position = traverse_controller->get_current_position();
bool homed = traverse_controller->is_homed();

// Get move queue status
uint32_t depth = move_queue->get_queue_depth(AXIS_TRAVERSE);
bool active = move_queue->is_active(AXIS_TRAVERSE);
```

#### Scheduler Status
```cpp
// Get scheduler information
bool running = scheduler->is_running();
uint32_t ticks = scheduler->get_tick_count();
```

### 🔧 Hardware Configuration

#### Pin Definitions
```cpp
// From config.h
#define SPINDLE_PWM_PIN 24
#define SPINDLE_ENABLE_PIN 21
#define SPINDLE_HALL_PIN 22
#define SPINDLE_DIR_PIN 4
#define SPINDLE_BRAKE_PIN 3

#define TRAVERSE_STEP_PIN 6
#define TRAVERSE_DIR_PIN 5
#define TRAVERSE_ENA_PIN 7
#define TRAVERSE_HOME_PIN 16
```

#### PWM Configuration
```cpp
// Initialize PWM
gpio_set_function(SPINDLE_PWM_PIN, GPIO_FUNC_PWM);
uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
uint chan = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
pwm_set_wrap(slice_num, 65535);
pwm_set_chan_level(slice_num, chan, 0);
pwm_set_enabled(slice_num, true);
```

### 🧪 Testing and Validation

#### Unit Testing
```cpp
// Test spindle control
void test_spindle() {
    BLDC_MOTOR* spindle = new BLDC_MOTOR(SPINDLE_HALL_PIN);
    spindle->init();
    
    // Test direction
    spindle->set_direction(DIRECTION_CW);
    assert(spindle->get_direction() == DIRECTION_CW);
    
    // Test brake
    spindle->set_brake(true);
    assert(spindle->get_brake() == true);
}
```

#### Integration Testing
```cpp
// Test G-code interface
void test_gcode() {
    GCodeInterface* gcode = new GCodeInterface();
    
    // Test parsing
    assert(gcode->parse_command("M3 S500"));
    assert(gcode->get_current_command() == TOKEN_M3);
    
    // Test execution
    assert(gcode->execute_command());
}
```

### 📈 Performance Optimization

#### ISR Optimization
```cpp
// Minimize ISR execution time
void scheduler_isr() {
    // Only essential operations in ISR
    move_queue->handle_isr_tick();
    
    // Defer non-critical operations
    if (user_callback) {
        user_callback(user_callback_data);
    }
}
```

#### Memory Management
```cpp
// Efficient move queue
class MoveQueue {
private:
    StepChunk queues[2][128];  // Fixed-size arrays
    volatile uint16_t head[2];
    volatile uint16_t tail[2];
};
```

### 🔄 Error Handling

#### Command Validation
```cpp
// Validate G-code parameters
bool GCodeInterface::validate_parameters() {
    if (params.has_S && (params.S < 0 || params.S > 3000)) {
        set_error("RPM out of range (0-3000)");
        return false;
    }
    return true;
}
```

#### Hardware Error Handling
```cpp
// Check hardware status
bool check_hardware() {
    if (!spindle_controller) {
        printf("ERROR: Spindle controller not initialized\n");
        return false;
    }
    return true;
}
```

### 📚 Examples

#### Complete Initialization
```cpp
int main() {{
    stdio_init_all();
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    // Initialize spindle
    spindle_controller = new BLDC_MOTOR(SPINDLE_HALL_PIN);
    spindle_controller->init();
    
    // Initialize traverse
    traverse_controller = new TraverseController();
    traverse_controller->init();
    
    // Initialize move queue
    move_queue = new MoveQueue();
    move_queue->init();
    
    // Initialize scheduler
    scheduler = new Scheduler(move_queue);
    scheduler->start(HEARTBEAT_US);
    
    // Initialize G-code interface
    gcode_interface = new GCodeInterface();
    
    printf("Pico Spindle Controller Ready\\n");
    
    // Main loop
    while (true) {{
        // Process UART commands
        if (uart_is_readable(PI_UART_ID)) {{
            char buffer[256];
            uart_read_blocking(PI_UART_ID, buffer, 1);
            // ... process command ...
        }}
    }}
}}
```

---
*This documentation is automatically generated by AI and updated on every push.*
"""
    return content

def main():
    """Generate all documentation"""
    print("🤖 Generating AI Documentation...")
    
    # Create docs directory if it doesn't exist
    os.makedirs('docs', exist_ok=True)
    
    # Generate documentation files
    docs = {
        'project_overview.md': generate_project_overview(),
        'setup_guide.md': generate_setup_guide(),
        'troubleshooting_guide.md': generate_troubleshooting_guide(),
        'python_api.md': generate_python_api(),
        'cpp_api.md': generate_cpp_api()
    }
    
    # Write documentation files
    for filename, content in docs.items():
        filepath = os.path.join('docs', filename)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"✅ Generated {filepath}")
    
    print(f"🎉 Generated {len(docs)} documentation files")
    print("📚 Documentation available in docs/ directory")

if __name__ == "__main__":
    main()