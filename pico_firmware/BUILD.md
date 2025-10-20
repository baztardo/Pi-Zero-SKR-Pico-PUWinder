# Building Pico Firmware

## Prerequisites

1. **Pico SDK** - Install from: https://github.com/raspberrypi/pico-sdk
   ```bash
   # On macOS (if not already installed):
   brew install cmake
   brew install --cask gcc-arm-embedded
   
   # Clone SDK
   cd ~/
   git clone https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   ```

2. **Set environment variable:**
   ```bash
   export PICO_SDK_PATH=~/pico-sdk
   # Add to ~/.zshrc to make permanent:
   echo 'export PICO_SDK_PATH=~/pico-sdk' >> ~/.zshrc
   ```

## Build Steps

```bash
# Navigate to firmware directory
cd pico_firmware

# Create build directory
mkdir -p build
cd build

# Configure
cmake ..

# Build
make -j4
```

## Output

The build creates:
- `pico_uart_test.uf2` - Flash this to the Pico
- `pico_uart_test.elf` - Debug symbols
- `pico_uart_test.bin` - Raw binary

## Upload to SKR Pico

1. Hold **BOOTSEL** button on the SKR Pico
2. Plug in USB cable (or press RESET while holding BOOTSEL)
3. Pico appears as USB drive named "RPI-RP2"
4. Drag `pico_uart_test.uf2` onto the drive
5. Pico automatically reboots and runs the firmware

## Verify It's Running

**Option 1: USB Serial (debug output)**
```bash
# macOS
screen /dev/tty.usbmodem* 115200

# You should see:
# Pico UART Test Ready
# Listening on UART0 (GPIO 0/1) @ 115200 baud
```

**Option 2: LED indicator**
The onboard LED should indicate the Pico is running.

## Troubleshooting

### CMAKE Error: PICO_SDK_PATH not set
```bash
export PICO_SDK_PATH=~/pico-sdk
```

### arm-none-eabi-gcc not found
```bash
# macOS
brew install --cask gcc-arm-embedded

# Linux
sudo apt install gcc-arm-none-eabi
```

### No BOOTSEL drive appears
- Try different USB cable (must be data cable, not charge-only)
- Try different USB port
- Make sure you're holding BOOTSEL before plugging in

