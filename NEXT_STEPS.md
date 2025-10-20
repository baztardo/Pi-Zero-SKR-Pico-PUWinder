# 🚀 NEXT STEPS - BLDC Integration

## ✅ COMPLETED:
- Pi Zero setup ✅
- UART working ✅  
- Basic PING/PONG test ✅

## 🎯 NEXT: Add BLDC Control

### Files to Create in Other Session:

**1. pico_firmware/src/bldc_control.h**
```cpp
#pragma once
#include "pico/stdlib.h"

class BLDCControl {
public:
    BLDCControl(uint pwm_pin, uint speed_pin);
    void init();
    void set_rpm(float rpm);
    void stop();
    float get_measured_rpm();
private:
    uint pwm_pin;
    uint speed_pin;
    float target_rpm;
};
```

**2. pico_firmware/src/bldc_control.cpp**
- Copy from: /PU-Winder/EP-0172_BLDC_test/bldc_speed_pulse.cpp
- Adapt for RPM control via PWM

**3. Update pico_firmware/main.cpp**
Add commands:
- SET_BLDC_RPM <rpm>
- GET_BLDC_RPM
- STOP_BLDC

### Pi Zero Test:
```python
# pi_zero/test_bldc.py
ser.write(b"SET_BLDC_RPM 1500\n")
response = ser.readline()  # "ok"

ser.write(b"GET_BLDC_RPM\n") 
rpm = ser.readline()  # "1523.4"
```

### Pins to Use:
- BLDC PWM: GPIO 20 (PWM output to EP-0172 SC pin)
- Speed pulse: GPIO 21 (read from EP-0172 speed output)

## 🔧 Implementation Order:
1. Add BLDC files (copy from EP-0172 code)
2. Add to CMakeLists.txt
3. Update main.cpp with BLDC commands
4. Compile and test!

Target: 1500-3000 RPM! 🚀
