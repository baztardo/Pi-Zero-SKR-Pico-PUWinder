# Command Controllers

## 🎯 **Available Controllers**

### **1. Simple Controller (`pico_controller.py`)**
- **Purpose**: Basic command sending with interactive mode
- **Usage**: `python3 pi_zero/pico_controller.py`
- **Features**: 
  - Interactive mode with help
  - Single command mode
  - Auto-detection of Pico device

### **2. Advanced Controller (`advanced_controller.py`)**
- **Purpose**: High-level command management with protocol handling
- **Usage**: `python3 pi_zero/advanced_controller.py`
- **Features**:
  - Command sequences (startup, shutdown, emergency)
  - State management (spindle, traverse, emergency stop)
  - Protocol statistics
  - Async command support

### **3. Command Protocol (`command_protocol.py`)**
- **Purpose**: Low-level protocol management
- **Features**:
  - Command queue management
  - Response handling
  - Timeout management
  - Statistics tracking

### **4. Simple Wrapper (`send`)**
- **Purpose**: Super simple command sending
- **Usage**: `./pi_zero/send "COMMAND"`
- **Features**: One-line command execution

## 🚀 **Quick Start**

### **Interactive Mode (Recommended)**
```bash
python3 pi_zero/pico_controller.py
```

### **Single Commands**
```bash
python3 pi_zero/pico_controller.py "PING"
python3 pi_zero/pico_controller.py "M3 S500"
python3 pi_zero/pico_controller.py "G1 Y50 F1000"
```

### **Super Simple**
```bash
./pi_zero/send "PING"
./pi_zero/send "M3 S500"
```

## 📋 **Available Commands**

### **Basic Commands**
- `PING` - Test connection
- `VERSION` - Get firmware version
- `STATUS` - Get machine status
- `G28` - Home all axes
- `M5` - Stop spindle
- `M112` - Emergency stop
- `M999` - Reset from emergency

### **G-code Commands**
- `M3 S500` - Start spindle CW at 500 RPM
- `M4 S1000` - Start spindle CCW at 1000 RPM
- `G1 Y50 F1000` - Move traverse to 50mm at 1000mm/min
- `G0 Y100` - Rapid move to 100mm
- `G4 P1000` - Dwell for 1000ms

### **Winding Commands**
- `M6 S0.064` - Set wire diameter
- `M7` - Coolant on
- `M8` - Coolant off
- `M10` - Traverse brake on
- `M11` - Traverse brake off
- `M12` - Spindle brake on
- `M13` - Spindle brake off
- `M14` - Wire tension on
- `M15` - Wire tension off
- `M16` - Home all axes
- `M17` - Enable steppers
- `M18` - Disable steppers

### **GPIO Commands**
- `M42 P25 S1` - Set GPIO 25 HIGH
- `M42 P25 S0` - Set GPIO 25 LOW

## 🔧 **Current Status**

### **✅ What's Working**
- Controllers connect successfully to Pico
- Commands are being sent properly
- Interface is much simpler than long Python one-liners
- Multiple controller options for different use cases

### **❌ What's Still Wrong**
- Pico is responding with just dots (`.`)
- Pico is not processing commands properly
- Commands are timing out

## 🚨 **Next Steps**

1. **Reset the Pico** (unplug USB, wait 5 seconds, plug back in)
2. **Test the controllers** again
3. **Verify Pico firmware** is loaded correctly
4. **Check UART connections** between Pi Zero and Pico

## 📊 **Controller Comparison**

| Feature | Simple Controller | Advanced Controller | Command Protocol |
|---------|------------------|-------------------|------------------|
| Ease of Use | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐ |
| Features | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| State Management | ❌ | ✅ | ✅ |
| Command Sequences | ❌ | ✅ | ✅ |
| Statistics | ❌ | ✅ | ✅ |
| Async Support | ❌ | ✅ | ✅ |

Choose the controller that best fits your needs! 🚀
