# Pi Zero Winding Controller

Touchscreen-optimized control system for the Pi Zero SKR Pico PUWinder project.

## 🎯 Features

- **Touchscreen Interface** - Optimized for 7" touchscreen displays
- **Web Interface** - Access from any device on the network
- **Command Line Interface** - Quick testing and debugging
- **Real-time Status** - Live updates of winding progress
- **Safety Controls** - Emergency stop and pause/resume
- **Parameter Control** - Adjustable winding parameters

## 🚀 Quick Start

### 1. Setup (Run once)
```bash
cd pi_zero
./setup.sh
```

### 2. Test UART Connection
```bash
python test_uart.py
```

### 3. Start Web Interface
```bash
python web_interface.py
```
Open browser to: `http://pi-zero-ip:5000`

### 4. Start Simple Control
```bash
python simple_control.py
```

## 📱 Touchscreen Interface

The web interface is optimized for touchscreen use with:
- **Large buttons** (80px+ height) for easy touch
- **Visual feedback** with animations and color coding
- **Status indicators** with real-time updates
- **Emergency stop** button (large, red, always visible)
- **Progress visualization** with progress bars

### Interface Layout:
```
┌─────────────────────────────────────┐
│ 🔄 Pi Zero Winding Controller      │
│ ● Connected | State: IDLE | RPM: 0 │
├─────────────────┬───────────────────┤
│ 🎮 Control      │ 📊 Progress       │
│ [Connect]       │ 0/1000 turns (0%) │
│ [Home]          │ ████████░░░░      │
│ [Start]         │ Layer: 0          │
│ [Pause]         │ Position: 0.0mm   │
│ [Stop]          │                   │
├─────────────────┴───────────────────┤
│ ⚙️ Settings                         │
│ Turns: [1000] RPM: [300] Wire: [0.064] │
└─────────────────────────────────────┘
│                    🚨 EMERGENCY STOP │
└─────────────────────────────────────┘
```

## 🎮 Controls

### Web Interface Commands:
- **Connect/Disconnect** - Establish UART connection to Pico
- **Home** - Home all axes (traverse and spindle)
- **Start** - Begin winding with current settings
- **Pause/Resume** - Pause and resume winding process
- **Stop** - Stop winding gracefully
- **Emergency Stop** - Immediate stop of all operations
- **Settings** - Adjust winding parameters

### Command Line Interface:
```bash
> home      # Home all axes
> start     # Start winding
> pause     # Pause winding
> resume    # Resume winding
> stop      # Stop winding
> emergency # Emergency stop
> status    # Show current status
> settings  # Change parameters
> test      # Test UART connection
> quit      # Exit program
```

## ⚙️ Winding Parameters

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| Target Turns | 1000 | 1-10000 | Total turns to wind |
| Spindle RPM | 300 | 50-2000 | Spindle rotation speed |
| Wire Diameter | 0.064mm | 0.01-1.0mm | 43 AWG wire diameter |
| Bobbin Width | 12.0mm | 1-50mm | Width of winding area |
| Start Offset | 22.0mm | 0-100mm | Distance from edge (20mm + 2mm bobbin) |

## 🔌 Hardware Connection

### UART Connection (Pi Zero ↔ Pico):
- **Pi Zero TX** (GPIO 14) → **Pico RX** (GPIO 1)
- **Pi Zero RX** (GPIO 15) → **Pico TX** (GPIO 0)
- **Ground** → **Ground**

### Touchscreen Connection:
- **HDMI** - Video output
- **USB** - Touch input (if USB touchscreen)

## 📊 Status Monitoring

The system provides real-time status updates:
- **Connection Status** - UART connection to Pico
- **Winding State** - IDLE, HOMING, WINDING, PAUSED, COMPLETE, ERROR
- **Progress** - Current turns vs target turns
- **RPM** - Current spindle speed
- **Layer** - Current winding layer
- **Position** - Traverse carriage position

## 🚨 Safety Features

- **Emergency Stop** - Large red button, always visible
- **Pause/Resume** - Graceful pause and resume
- **Status Monitoring** - Real-time error detection
- **UART Timeout** - Automatic reconnection on communication loss
- **Parameter Validation** - Prevents invalid settings

## 🔧 Troubleshooting

### Connection Issues:
```bash
# Test UART connection
python test_uart.py

# Check UART device
ls /dev/ttyUSB* /dev/ttyAMA*

# Manual connection test
python -c "import serial; print(serial.Serial('/dev/ttyUSB0', 115200).readline())"
```

### Permission Issues:
```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER
# Logout and login again
```

### Service Issues:
```bash
# Check service status
sudo systemctl status winding-controller

# View logs
sudo journalctl -u winding-controller -f

# Restart service
sudo systemctl restart winding-controller
```

## 📁 File Structure

```
pi_zero/
├── winding_controller.py    # Core control logic
├── web_interface.py         # Flask web server
├── simple_control.py        # Command line interface
├── requirements.txt         # Python dependencies
├── setup.sh                # Installation script
├── README.md               # This file
└── templates/
    └── index.html          # Web interface template
```

## 🎯 Usage Examples

### Basic Winding Sequence:
1. **Connect** to Pico
2. **Home** all axes
3. **Set parameters** (turns, RPM, wire diameter)
4. **Start** winding
5. **Monitor** progress
6. **Stop** when complete

### Emergency Procedure:
1. Press **🚨 EMERGENCY STOP** button
2. Check for errors
3. **Reset** emergency stop
4. **Home** axes if needed
5. **Restart** winding

## 📞 Support

For issues or questions:
1. Check the troubleshooting section
2. Review the logs: `sudo journalctl -u winding-controller`
3. Test with simple control: `python simple_control.py`
4. Verify hardware connections

---

**🎉 Happy Winding!** Your Pi Zero is now ready to control your precision winding system.
