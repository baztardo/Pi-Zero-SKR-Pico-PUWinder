# 📚 Code Snippets for Pi Zero SKR Pico PUWinder

This directory contains useful code snippets, examples, and patterns for developing with the Pi Zero SKR Pico PUWinder project.

## 📁 Files Overview

### **Python Snippets** (`python_snippets.py`)
- **UART Communication** - Basic and retry-based UART operations
- **G-code Processing** - Parse and execute G-code commands
- **Spindle Control** - PWM-based spindle speed control
- **Error Handling** - Decorators and safe operation patterns
- **Testing Functions** - Unit tests and validation

## 🚀 Quick Start

### **Python Development**
```python
# Import and use Python snippets
from python_snippets import SpindleController, parse_gcode_command

# Parse G-code command
parsed = parse_gcode_command("M3 S1500")
print(parsed)

# Initialize spindle controller
spindle = SpindleController(uart_connection)
spindle.set_rpm(1500, 'CW')
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

## 🧪 Testing and Validation

### **Python Testing**
```python
# Run snippet tests
python3 python_snippets.py

# Test specific functions
from python_snippets import test_gcode_parsing
test_gcode_parsing()
```

---

**🎉 Use these code snippets to accelerate your development and build robust, reliable winding machine control systems!**

The snippets provide a solid foundation for Python development with comprehensive G-code examples for complete system integration.
