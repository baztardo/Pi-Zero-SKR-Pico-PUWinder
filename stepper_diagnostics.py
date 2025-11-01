#!/usr/bin/env python3
"""
Comprehensive stepper diagnostics
"""

import serial
import time

def full_diagnostic():
    """Run complete stepper diagnostic"""
    print("Stepper Motor Full Diagnostic")
    print("=" * 30)

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Test TMC2209 communication
    print("1. Testing TMC2209 UART...")
    ser.write(b'TEST_TMC2209\n')
    time.sleep(1)
    if ser.in_waiting:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"   TMC2209: {response.strip()}")
    else:
        print("   TMC2209: No response")

    # Test enable pin
    print("2. Testing Enable Pin...")
    ser.write(b'TEST_ENABLE\n')
    time.sleep(1)
    if ser.in_waiting:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"   Enable: {response.strip()}")
    else:
        print("   Enable: No response")

    # Test GPIO pins
    pins = [4, 5, 6]  # DIR, STEP, EN
    pin_names = ["DIR", "STEP", "EN"]

    for i, pin in enumerate(pins):
        print(f"3. Testing GPIO {pin} ({pin_names[i]})...")
        ser.write(f'TEST_GPIO_OUT P{pin}\n'.encode())
        time.sleep(1)
        if ser.in_waiting:
            response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"   GPIO {pin}: {response.strip()}")
        else:
            print(f"   GPIO {pin}: No response")

    # Test basic movement
    print("4. Testing Basic Movement...")
    ser.write(b'STATUS\n')
    time.sleep(0.5)
    if ser.in_waiting:
        start_pos = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"   Start position: {start_pos.strip()}")

    ser.write(b'G0 X5 F100\n')  # Small move
    time.sleep(2)

    ser.write(b'STATUS\n')
    time.sleep(0.5)
    if ser.in_waiting:
        end_pos = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        print(f"   End position: {end_pos.strip()}")

    ser.close()

def power_supply_check():
    """Instructions for checking power supply"""
    print("\nStepper Power Supply Check:")
    print("=" * 28)
    print("1. What voltage is your stepper motor rated for?")
    print("   - NEMA 17: Usually 12V or 24V")
    print("   - NEMA 23: Usually 24V or 48V")
    print()
    print("2. Check your power supply:")
    print("   - Voltage output with multimeter")
    print("   - Current capacity (minimum 1A)")
    print("   - Connected to TMC2209 VM and GND pins")
    print()
    print("3. Common issues:")
    print("   - Wrong voltage (12V motor on 24V supply = screeching)")
    print("   - Insufficient current")
    print("   - Power supply not turned on")

def motor_check():
    """Instructions for checking motor"""
    print("\nStepper Motor Check:")
    print("=" * 20)
    print("1. Can you manually turn the motor shaft?")
    print("   - Should turn freely with light resistance")
    print("   - If stuck, motor is faulty")
    print()
    print("2. Check motor connections:")
    print("   - Wires connected to TMC2209 A1/A2/B1/B2")
    print("   - Correct coil pairing")
    print()
    print("3. Motor specs:")
    print("   - Step angle (usually 1.8°)")
    print("   - Holding torque")
    print("   - Voltage/current rating")

if __name__ == "__main__":
    full_diagnostic()
    power_supply_check()
    motor_check()

    print("\nMost likely issues:")
    print("1. Wrong stepper power supply voltage")
    print("2. TMC2209 EN pin not working")
    print("3. Stepper motor faulty or stuck")
    print("4. Microstepping configuration wrong")
