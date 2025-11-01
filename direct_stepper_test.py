#!/usr/bin/env python3
"""
Direct stepper test - bypass homing and MoveQueue
"""

import serial
import time

def test_direct_movement():
    """Test stepper movement without homing"""
    print("Direct Stepper Movement Test")
    print("=" * 30)

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Enable stepper first
    print("Enabling stepper...")
    ser.write(b'M17\n')  # Enable all steppers
    time.sleep(0.5)

    # Try small absolute moves
    moves = [10, -10, 50, -50]  # mm

    for move in moves:
        print(f"\nMoving to {move}mm...")
        ser.write(f'G0 X{move} F300\n'.encode())  # Rapid move at 300mm/min
        time.sleep(3)  # Wait for move

        # Check position
        ser.write(b'STATUS\n')
        time.sleep(0.5)
        response = ser.read(500).decode('utf-8', errors='ignore')
        print(f"Position after move: {response.strip()}")

        time.sleep(1)  # Pause between moves

    # Disable stepper
    print("\nDisabling stepper...")
    ser.write(b'M18\n')  # Disable all steppers

    ser.close()

def test_manual_pulses():
    """Test very basic manual commands"""
    print("\nManual Pulse Test")
    print("=" * 15)

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Simple commands
    commands = [
        "M17",      # Enable
        "G0 X5 F100",  # Move 5mm
        "STATUS",   # Check position
        "G0 X0 F100",  # Return to 0
        "STATUS",   # Check position
        "M18"       # Disable
    ]

    for cmd in commands:
        print(f"Sending: {cmd}")
        ser.write(f'{cmd}\n'.encode())
        time.sleep(2)

        if ser.in_waiting:
            response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"Response: {response.strip()}")

    ser.close()

if __name__ == "__main__":
    print("Stepper Motor Direct Test")
    print("Bypasses homing and MoveQueue issues")
    print("=" * 40)

    try:
        test_direct_movement()
        test_manual_pulses()
    except Exception as e:
        print(f"Error: {e}")

    print("\nIf stepper screeches but doesn't move:")
    print("1. Stepper is powered but not getting step pulses")
    print("2. TMC2209 STEP pin not connected")
    print("3. Stepper motor mechanically stuck")
    print("4. Limit switch triggered immediately")
