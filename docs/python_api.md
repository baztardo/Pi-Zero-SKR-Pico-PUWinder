# Python API Reference

## 🐍 Pi Zero SKR Pico PUWinder Python API

**AI-Generated Documentation** - Last Updated: 2025-10-25 03:51:01

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
