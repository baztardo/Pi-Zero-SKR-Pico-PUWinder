# Pi CM4 Setup Guide

This directory contains everything needed to set up a Raspberry Pi CM4 for testing the Pi-Zero-SKR-Pico-PUWinder project.

## Quick Start

1. **Flash Raspberry Pi OS Desktop** (see `IMAGE_SETUP.md`)
2. **Clone the project:**
   ```bash
   git clone <your-repo-url>
   cd Pi-Zero-SKR-Pico-PUWinder
   ```
3. **Run the setup script:**
   ```bash
   ./pi_cm4_setup/setup.sh
   ```
4. **Test the connection:**
   ```bash
   cd pi_zero
   source venv/bin/activate
   python test_uart.py
   ```

## Files Overview

| File | Purpose |
|------|---------|
| `setup.sh` | Main setup script - installs dependencies, configures UART, creates virtual environment |
| `IMAGE_SETUP.md` | Detailed guide for flashing Raspberry Pi OS Desktop on CM4 |
| `DESKTOP_SETUP.md` | Desktop-specific configuration and touchscreen setup |
| `QUICK_START.md` | Condensed setup steps for rapid deployment |

## What the Setup Script Does

- Updates system packages
- Installs Python 3, pip, venv, git
- Installs Firefox browser (or skips if unavailable)
- Enables UART communication
- Adds user to dialout group for serial access
- Creates Python virtual environment
- Installs Flask and PySerial
- Makes Python scripts executable

## After Setup

1. **Connect your Pico** via UART (GPIO 14/15)
2. **Test connection:**
   ```bash
   cd pi_zero
   source venv/bin/activate
   python test_uart.py
   ```
3. **Start web interface:**
   ```bash
   python web_interface.py
   ```
4. **Open browser:** http://localhost:5000

## Troubleshooting

- **UART not working:** Check `/dev/ttyAMA0` permissions
- **Python errors:** Make sure virtual environment is activated
- **Browser issues:** Use any web browser, not just Firefox
- **Pico not responding:** Check wiring and power

## Hardware Requirements

- Raspberry Pi CM4
- MicroSD card (32GB+ recommended)
- Pico board with firmware uploaded
- UART connection (GPIO 14/15)
- Optional: Touchscreen display