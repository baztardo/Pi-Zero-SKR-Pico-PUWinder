---
name: Hardware Issue
about: Report problems with Pi Zero, SKR Pico, or hardware connections
title: '[HW] '
labels: 'hardware'
assignees: ''

---

**Hardware Problem**
Describe the hardware issue you're experiencing.

**Hardware Setup**
- **Pi Zero Model**: [e.g., Pi Zero W, Pi Zero 2 W]
- **SKR Pico Version**: [e.g., v1.0, v1.1]
- **Power Supply**: [e.g., 5V 2A, 5V 3A]
- **SD Card**: [e.g., 32GB Class 10, 64GB Class 10]

**Connection Issues**
- **UART Connection**: [Working/Not Working]
- **GPIO Pins**: [List any custom GPIO usage]
- **Power Issues**: [Stable/Unstable/Overheating]
- **Physical Damage**: [Yes/No - describe if yes]

**Symptoms**
- [ ] System won't boot
- [ ] UART communication fails
- [ ] Spindle motor not responding
- [ ] Traverse stepper not moving
- [ ] Random crashes/reboots
- [ ] Overheating
- [ ] Power supply issues
- [ ] SD card corruption

**Error Messages**
```
# Paste any error messages here
# Include dmesg output if relevant
dmesg | grep -i error
```

**Hardware Logs**
```
# Pi Zero system logs
sudo journalctl -u your-service-name

# Hardware detection
lsusb
lsmod
```

**Wiring Diagram**
```
# Describe or attach wiring diagram
# Include pin connections:
# - UART: GPIO 14/15
# - Spindle: PWM pin, Hall sensor pin
# - Traverse: Step/Dir/Enable pins
```

**Troubleshooting Attempted**
- [ ] Checked power supply voltage
- [ ] Verified UART connections
- [ ] Tested with different SD card
- [ ] Checked for loose connections
- [ ] Verified GPIO pin assignments
- [ ] Tested with minimal configuration

**Additional Context**
- When did this start?
- Any recent hardware changes?
- Environmental conditions?
- Previous working state?
