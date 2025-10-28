#!/bin/bash
# Pico Probe Debug Script
# This script starts OpenOCD for Pico Probe debugging

echo "🔧 Starting Pico Probe Debug Session"
echo "=================================="
echo ""
echo "SWD Connections:"
echo "  Pico Probe SWDIO → Target Pico GPIO 2"
echo "  Pico Probe SWCLK → Target Pico GPIO 3"
echo "  Pico Probe GND   → Target Pico GND"
echo ""
echo "Starting OpenOCD..."
echo "Connect GDB to: localhost:3333"
echo ""

# Start OpenOCD with Pico Probe configuration
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg
