# 📚 Code Snippets for Pi Zero SKR Pico PUWinder

This directory contains useful code snippets, examples, and patterns for developing with the Pi Zero SKR Pico PUWinder project.

## 📁 Files Overview

### **Python Snippets** (`python_snippets.py`)
- **UART Communication** - Basic and retry-based UART operations
- **G-code Processing** - Parse and execute G-code commands
- **Spindle Control** - PWM-based spindle speed control
- **Traverse Control** - Stepper motor position control
- **Winding Control** - Synchronized spindle and traverse control
- **Error Handling** - Decorators and safe operation patterns
- **Testing Functions** - Unit tests and validation

### **C++ Snippets** (`cpp_snippets.cpp`)
- **UART Communication** - Serial communication with Pi Zero
- **PWM Control** - Spindle motor speed control
- **Spindle Control** - Complete spindle management
- **Stepper Control** - Traverse motor positioning
- **G-code Processing** - Command parsing and execution
- **Interrupt Handlers** - Real-time response functions
- **Main Control Loop** - Primary firmware logic

### **G-code Examples** (`gcode_examples.txt`)
- **Basic Movement** - Linear and rapid movements
- **Spindle Control** - Start, stop, and speed control
- **Winding Commands** - Machine-specific M-codes
- **Complete Programs** - Full winding sequences
- **Advanced Patterns** - Zigzag, spiral, variable speed
- **Error Handling** - Safe startup and emergency procedures
- **Testing Commands** - System validation and diagnostics

## 🚀 Quick Start

### **Python Development**
```python
# Import and use Python snippets
from python_snippets import SpindleController, TraverseController, WindingController

# Initialize controllers
spindle = SpindleController(uart_connection)
traverse = TraverseController(uart_connection)
winder = WindingController(uart_connection)

# Start winding process
winder.start_winding(rpm=1500, wire_diameter=0.5, turns=100)
```

### **C++ Development**
```cpp
// Include C++ snippets in your firmware
#include "cpp_snippets.cpp"

// Initialize hardware
spindle_init();
stepper_init();

// Set spindle speed
set_spindle_rpm(1500.0f, true);  // 1500 RPM clockwise

// Move traverse
stepper_move_to(10.0f, 1000.0f);  // Move 10mm at 1000 mm/min
```

### **G-code Testing**
```bash
# Test basic commands
echo "PING" | python3 uart_api.py
echo "M3 S1500" | python3 uart_api.py
echo "G1 Y10 F1000" | python3 uart_api.py
echo "M5" | python3 uart_api.py
```

## 🎯 Use Cases

### **1. Development**
- **Copy snippets** into your main code
- **Modify parameters** for your specific needs
- **Combine patterns** to create complex functionality
- **Use as templates** for new features

### **2. Testing**
- **Unit testing** with provided test functions
- **Hardware validation** with diagnostic commands
- **Integration testing** with complete examples
- **Error handling** with safe sequences

### **3. Learning**
- **Study patterns** to understand the architecture
- **Learn G-code** with comprehensive examples
- **Understand hardware** control with C++ snippets
- **Practice integration** with Python examples

## 🔧 Customization

### **Python Snippets**
```python
# Modify for your specific hardware
class CustomSpindleController(SpindleController):
    def __init__(self, uart_connection, custom_params):
        super().__init__(uart_connection)
        self.custom_params = custom_params
    
    def set_rpm(self, rpm, direction='CW'):
        # Add your custom logic here
        return super().set_rpm(rpm, direction)
```

### **C++ Snippets**
```cpp
// Modify for your specific hardware configuration
#define CUSTOM_SPINDLE_PIN 10
#define CUSTOM_TRAVERSE_PIN 11

// Update pin definitions
void custom_hardware_init() {
    gpio_init(CUSTOM_SPINDLE_PIN);
    gpio_set_dir(CUSTOM_SPINDLE_PIN, GPIO_OUT);
    // Add your custom initialization
}
```

### **G-code Commands**
```
; Add your custom M-codes
M100 S500          ; Custom command with parameter
M101               ; Custom command without parameters
M102 P2 S1         ; Custom GPIO control
```

## 📊 Snippet Categories

### **Communication**
- UART setup and configuration
- Command sending and receiving
- Error handling and retry logic
- Response parsing and validation

### **Hardware Control**
- PWM generation and control
- GPIO configuration and control
- Interrupt handling and timing
- Sensor reading and processing

### **Motion Control**
- Spindle speed and direction
- Traverse positioning and homing
- Synchronized movement patterns
- Speed and acceleration control

### **G-code Processing**
- Command parsing and validation
- Parameter extraction and conversion
- Command execution and response
- Error reporting and handling

### **Winding Logic**
- Layer calculation and planning
- Speed synchronization algorithms
- Pattern generation and execution
- Quality control and monitoring

## 🧪 Testing and Validation

### **Python Testing**
```python
# Run snippet tests
python3 python_snippets.py

# Test specific functions
from python_snippets import test_uart_communication, test_gcode_parsing
test_uart_communication()
test_gcode_parsing()
```

### **C++ Testing**
```cpp
// Compile and test C++ snippets
gcc -o test_snippets cpp_snippets.cpp
./test_snippets

// Test specific functions
spindle_init();
set_spindle_rpm(1000.0f, true);
stop_spindle();
```

### **G-code Testing**
```bash
# Test G-code commands
echo "G1 Y10 F1000" | python3 uart_api.py
echo "M3 S1500" | python3 uart_api.py
echo "M5" | python3 uart_api.py
```

## 🔄 Integration Examples

### **Complete Winding Program**
```python
# Python integration example
from python_snippets import WindingController
import serial

# Setup
uart = serial.Serial('/dev/serial0', 115200)
winder = WindingController(uart)

# Execute winding
winder.start_winding(rpm=1500, wire_diameter=0.5, turns=100)
# ... winding logic ...
winder.stop_winding()
```

### **Firmware Integration**
```cpp
// C++ integration example
#include "cpp_snippets.cpp"

int main() {
    // Initialize hardware
    spindle_init();
    stepper_init();
    
    // Main control loop
    while (true) {
        // Process commands
        // Update status
        // Handle interrupts
    }
}
```

## 📚 Documentation

### **Code Comments**
- All snippets include detailed comments
- Parameter descriptions and usage examples
- Error handling and edge cases
- Performance considerations and optimizations

### **Usage Examples**
- Complete working examples
- Integration patterns
- Best practices and recommendations
- Troubleshooting guides

### **API Reference**
- Function signatures and parameters
- Return values and error codes
- Dependencies and requirements
- Configuration options

## 🎯 Best Practices

### **Code Organization**
- Keep snippets focused and single-purpose
- Use clear naming conventions
- Include error handling
- Document parameters and usage

### **Testing**
- Test snippets individually
- Validate integration points
- Check error conditions
- Verify performance requirements

### **Maintenance**
- Keep snippets up-to-date
- Update documentation
- Test with new hardware
- Refactor for improvements

---

**🎉 Use these code snippets to accelerate your development and build robust, reliable winding machine control systems!**

The snippets provide a solid foundation for both Python and C++ development, with comprehensive G-code examples for complete system integration.
