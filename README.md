# Pi Zero + SKR Pico Guitar Pickup Winder

Dual-processor architecture for high-performance winding:
- **Pi Zero 2 W**: High-level control, UI, data logging (Python)
- **SKR Pico**: Real-time motor control, encoder reading (C++)

## Features
- 🚀 1500-3000 RPM capable (BLDC motor)
- 📊 Real-time encoder feedback
- 🔄 Synchronized traverse & spindle
- 📡 UART communication (Pi ↔ Pico)
- 🐍 Python control (easy to modify!)
- ⚡ Hardware timing (Pico PIO)

## Quick Start

**Complete testing guide in 3 steps:**

1. **Setup Pi Zero** → `docs/PI_ZERO_SETUP.md`
2. **Build Pico Firmware** → `pico_firmware/BUILD.md`
3. **Test Communication** → `docs/QUICKSTART.md`

## Hardware
- Raspberry Pi Zero 2 W
- BTT SKR Pico V1.0
- BLDC motor + EP-0172 controller
- Rotary encoder (360 PPR)
