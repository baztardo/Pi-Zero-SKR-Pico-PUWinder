#!/usr/bin/env python3
"""
Hardware connectivity test - check if stepper signals are reaching TMC2209
"""

import serial
import time

def test_stepper_signals():
    """Test if stepper control signals are working"""
    print("Stepper Signal Test")
    print("=" * 30)

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    print("\n1. Testing stepper enable (ENA pin)...")
    ser.write(b'STATUS\n')
    time.sleep(0.5)
    response = ser.read(500).decode('utf-8', errors='ignore')
    print(f"Status: {response.strip()}")

    print("\n2. Testing manual stepper pulses...")
    # Try to move a small amount
    ser.write(b'G1 X1 F100\n')  # 1mm at 100mm/min
    time.sleep(2)

    ser.write(b'STATUS\n')
    time.sleep(0.5)
    response = ser.read(500).decode('utf-8', errors='ignore')
    print(f"After move: {response.strip()}")

    ser.close()

def check_power():
    """Check if stepper has power"""
    print("\nStepper Power Check:")
    print("- Is the stepper motor power supply connected?")
    print("- Is the power supply turned ON?")
    print("- Is the TMC2209 VM/GND connected to power supply?")
    print("- Does the stepper motor get warm when powered?")
    input("Press Enter when you've checked power...")

def check_connections():
    """Check TMC2209 connections"""
    print("\nTMC2209 Connection Check:")
    print("GPIO Connections:")
    print("- GPIO 5 (STEP) → TMC2209 STEP pin?")
    print("- GPIO 4 (DIR) → TMC2209 DIR pin?")
    print("- GPIO 6 (ENA) → TMC2209 EN pin?")
    print("- GPIO 8 (UART TX) → TMC2209 PDN/UART?")
    print("- GPIO 9 (UART RX) → TMC2209 PDN/UART?")

    print("\nStepper Motor:")
    print("- Stepper wires → TMC2209 A1/A2/B1/B2?")
    print("- Correct stepper motor type/voltage?")

    print("\nPower:")
    print("- TMC2209 VM/GND → Stepper power supply?")
    print("- Power supply voltage correct for stepper?")
    input("Press Enter when you've verified connections...")

if __name__ == "__main__":
    print("Hardware Troubleshooting Guide")
    print("=" * 35)

    check_power()
    check_connections()

    print("\nRunning stepper signal test...")
    test_stepper_signals()

    print("\nIf stepper still doesn't move:")
    print("1. Try a different stepper motor")
    print("2. Check TMC2209 with multimeter (is it getting power?)")
    print("3. Try direct GPIO test (bypass TMC2209)")
    print("4. Check if stepper power supply is working")
