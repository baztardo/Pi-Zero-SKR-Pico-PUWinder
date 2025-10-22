# Pick-up Winder - G-code Compatible Architecture

## 🏗️ System Architecture

This winding machine uses **G-code compatible architecture** inspired by open-source 3D printing firmware:

### 🧠 **Pi Zero (Host/Brain)**
- **High-level control logic**
- **Motion planning and coordination**
- **User interface and display**
- **Winding process management**
- **G-code host software**

### ⚙️ **Pico (MCU/Hardware Slave)**
- **Real-time hardware control**
- **BLDC motor PWM control**
- **Hall sensor feedback**
- **Stepper motor control**
- **Hardware interface**

## 📁 Project Structure

```
Pi-Zero-SKR-Pico-PUWinder/
├── pi_zero/                    # Pi Zero (Host) code
│   ├── main_controller.py     # Main G-code controller
│   ├── winding_controller.py  # Winding process logic
│   ├── machine.cfg           # Machine configuration
│   ├── requirements.txt       # Python dependencies
│   └── test_*.py             # Test scripts
│
└── pico_firmware/             # Pico (MCU) code
    ├── main.cpp              # Hardware interface
    ├── src/
    │   ├── spindle.h/.cpp    # BLDC motor control
    │   ├── config.h          # Hardware configuration
    │   └── ...
    └── CMakeLists.txt        # Build configuration
```

## 🔧 Hardware Configuration

### **Pi Zero (Host)**
- **Raspberry Pi Zero W** with WiFi
- **LCD Display** (20x4 character)
- **Control buttons**
- **UART communication** to Pico

### **Pico (MCU)**
- **Raspberry Pi Pico**

## 🎮 G-code Spindle Commands

The system uses **standard G-code commands** for spindle control:

> **Note**: This system uses G-code compatible architecture inspired by open-source 3D printing firmware but is a standalone implementation for winding machines.

### **Basic Spindle Commands:**
- **`M3 S<rpm>`** - Start spindle clockwise at specified RPM
- **`M4 S<rpm>`** - Start spindle counter-clockwise at specified RPM  
- **`M5`** - Stop spindle
- **`S<rpm>`** - Set spindle speed (RPM)

### **Winding Machine M-codes:**
- **`M6`** - Wire change procedure
- **`M7`** - Coolant on (air blast for wire cleaning)
- **`M8`** - Coolant off
- **`M10`** - Enable traverse brake
- **`M11`** - Disable traverse brake
- **`M12`** - Enable spindle brake
- **`M13`** - Disable spindle brake
- **`M14`** - Enable wire tension
- **`M15`** - Disable wire tension
- **`M16`** - Home all axes
- **`M17`** - Enable steppers
- **`M18`** - Disable steppers
- **`M19`** - Spindle orientation
- **`M42 P<pin> S<value>`** - Set pin state
- **`M47 P<pin> S<value>`** - Set pin value

### **Winding-Specific Macros:**
- **`WINDING_START RPM=300 START_Y=10`** - Start winding process
- **`WINDING_LAYER LAYER_POS=5 SPEED=10`** - Move to layer position
- **`WINDING_SYNC RPM=300 WIRE_DIAMETER=0.064 TARGET_Y=8`** - Synchronized movement
- **`WINDING_STOP`** - Stop winding process
- **`WINDING_EMERGENCY`** - Emergency stop

### **Movement Commands:**
- **`G28 Y`** - Home Y-axis (traverse)
- **`G1 Y<position> F<speed>`** - Move traverse to position
- **`G1 Y<position> F<speed>`** - Synchronized traverse movement
- **BLDC motor** with PWM control
- **Hall sensor** for RPM feedback
- **Stepper motor** for traverse
- **TMC2209** stepper driver

## 🚀 Getting Started

### **1. Install Dependencies on Pi Zero**

```bash
# Install Python dependencies
pip3 install -r requirements.txt

# Install system packages
sudo apt update
sudo apt install python3-serial python3-requests
```

### **2. Configure Machine**

Copy `machine.cfg` to your config directory:
```bash
cp machine.cfg ~/machine.cfg
```

### **3. Build and Flash Pico Firmware**

```bash
cd pico_firmware
mkdir build && cd build
cmake ..
make -j4
# Flash to Pico using picotool or similar
```

### **4. Start the System**

```bash
# Start winding controller
python3 main_controller.py
```

## 🎯 Key Features

### **G-code Compatible Architecture**
- **Proven architecture** inspired by 3D printing
- **Real-time motion control**
- **Hardware abstraction**
- **Extensible configuration**

### **Winding Process**
- **Multi-layer winding**
- **Precise traverse positioning**
- **Spindle speed control**
- **Progress tracking**

### **Safety Features**
- **Emergency stop**
- **Watchdog timer**
- **Error handling**
- **Status monitoring**

## 📊 Communication Flow

```
Pi Zero (Host)          Pico (MCU)
     │                      │
     │─── UART Commands ───▶│
     │                      │
     │◀── Status/Feedback ──│
     │                      │
     │─── G-code Commands ─▶│
     │                      │
     │◀── Position/RPM ─────│
```

## 🔧 Configuration

### **Hardware Pins (config.h)**
```c
// UART communication
#define PI_UART_TX 0
#define PI_UART_RX 1

// BLDC motor control
#define SPINDLE_PWM_PIN 24
#define SPINDLE_HALL_PIN 22

// Traverse stepper
#define TRAVERSE_STEP_PIN 6
#define TRAVERSE_DIR_PIN 5
```

### **Winding Parameters**
```python
params = WindingParams(
    target_turns=1000,
    spindle_rpm=300.0,
    wire_diameter_mm=0.064,  # 43 AWG
    layer_width_mm=50.0,
    start_position_mm=20.0,
    ramp_time_sec=3.0
)
```

## 🧪 Testing

### **Test UART Communication**
```bash
python3 test_uart.py
```

### **Test Spindle Control**
```bash
python3 test_spindle.py
```

### **Test Full System**
```bash
python3 main_controller.py
```

## 🎯 Benefits of G-code Compatible Architecture

1. **Proven Technology**: Based on battle-tested 3D printing architecture
2. **Real-time Control**: Pico handles hardware timing, Pi Zero handles planning
3. **Extensible**: Easy to add new features and hardware
4. **Maintainable**: Clear separation of concerns
5. **Reliable**: Robust error handling and safety features

## 🔧 Troubleshooting

### **UART Communication Issues**
- Check wiring (TX→RX, RX→TX)
- Verify baud rate (115200)
- Check `/dev/serial0` permissions

### **BLDC Motor Issues**
- Verify PWM pin configuration
- Check Hall sensor wiring
- Test with `SET_SPINDLE_RPM` command

### **Stepper Motor Issues**
- Check TMC2209 configuration
- Verify step/dir pins
- Test with `G1` commands

## 📚 References

- [Raspberry Pi Pico SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf)
- [TMC2209 Datasheet](https://www.trinamic.com/fileadmin/assets/Products/ICs_Documents/TMC2209_Datasheet_V103.pdf)
- [G-code Standards](https://en.wikipedia.org/wiki/G-code)

## 🙏 Credits

This project uses G-code compatible architecture inspired by open-source 3D printing firmware. The G-code command structure and M-code definitions are based on industry standards and open-source implementations.

**Inspiration and References:**
- G-code standards and open-source 3D printing firmware
- Community-driven motion control systems
- Open-source hardware and software principles

**Note**: This is a standalone implementation specifically designed for winding machines and does not use any proprietary or trademarked software.