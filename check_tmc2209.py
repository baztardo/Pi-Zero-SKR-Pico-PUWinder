#!/usr/bin/env python3
"""
Check TMC2209 status and configuration
"""

import serial
import time

def check_tmc2209():
    """Check TMC2209 configuration"""
    print("TMC2209 Status Check")
    print("=" * 20)

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    print("Testing TMC2209 communication...")
    ser.write(b'TEST_TMC2209\n')
    time.sleep(1)

    if ser.in_waiting:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"TMC2209 Response: {response.strip()}")
    else:
        print("No TMC2209 response")

    print("\nTesting enable pin...")
    ser.write(b'TEST_ENABLE\n')
    time.sleep(1)

    if ser.in_waiting:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"Enable Response: {response.strip()}")
    else:
        print("No enable response")

    ser.close()

def manual_stepper_test():
    """Manual stepper control test"""
    print("\nManual Stepper Control")
    print("=" * 22)

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Enable stepper
    print("Enabling stepper...")
    ser.write(b'M17\n')
    time.sleep(0.5)

    # Manual direction control
    print("Setting direction forward...")
    ser.write(b'M18\n')  # This might be wrong command
    time.sleep(0.5)

    # Try to pulse STEP pin manually
    print("Testing STEP pin directly...")
    ser.write(b'TEST_GPIO_OUT P5\n')
    time.sleep(1)

    if ser.in_waiting:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"STEP pin test: {response.strip()}")

    ser.close()

if __name__ == "__main__":
    print("TMC2209 & Stepper Diagnostics")
    print("=" * 30)

    check_tmc2209()
    manual_stepper_test()

    print("\nIf TMC2209 responds but stepper doesn't move:")
    print("1. TMC2209 EN pin not connected (GPIO 6)")
    print("2. Stepper power supply not connected")
    print("3. Stepper motor faulty")
    print("4. Microstepping/current wrong")
