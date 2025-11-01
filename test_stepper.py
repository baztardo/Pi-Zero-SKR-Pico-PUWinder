#!/usr/bin/env python3
"""
Test stepper motor movement directly
"""

import serial
import time

def test_stepper_movement():
    """Test basic stepper movement"""
    print("=== STEPPER MOVEMENT TEST ===")

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Get initial position
    ser.write(b'STATUS\n')
    time.sleep(0.5)
    response = ser.read(500).decode('utf-8', errors='ignore')
    print(f"Initial status: {response.strip()}")

    # Extract initial position
    if "Traverse=" in response:
        initial_pos = float(response.split("Traverse=")[1].split("mm")[0])
        print(f"Initial position: {initial_pos}mm")
    else:
        initial_pos = 0

    # Try small moves
    moves = [1, -1, 5, -5]  # mm

    for move in moves:
        print(f"\nMoving {move}mm...")
        ser.write(f'G1 X{move} F500\n'.encode())  # 500mm/min
        time.sleep(2)  # Give time to move

        # Check position
        ser.write(b'STATUS\n')
        time.sleep(0.5)
        response = ser.read(500).decode('utf-8', errors='ignore')

        if "Traverse=" in response:
            current_pos = float(response.split("Traverse=")[1].split("mm")[0])
            actual_move = current_pos - initial_pos
            print(f"Position after move: {current_pos}mm (moved {actual_move}mm)")
            initial_pos = current_pos
        else:
            print(f"Status response: {response.strip()}")

    # Return to zero
    print("
Returning to 0mm...")
    ser.write(b'G1 X0 F500\n')
    time.sleep(3)

    ser.write(b'STATUS\n')
    time.sleep(0.5)
    response = ser.read(500).decode('utf-8', errors='ignore')
    print(f"Final status: {response.strip()}")

    ser.close()

def test_manual_step_pulses():
    """Test manual step pulses"""
    print("\n=== MANUAL STEP PULSE TEST ===")

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Send some manual commands
    commands = [
        "G1 X1 F100",   # 1mm at 100mm/min
        "G1 X-1 F100",  # -1mm at 100mm/min
        "G1 X0 F100"    # back to 0
    ]

    for cmd in commands:
        print(f"Sending: {cmd}")
        ser.write(f'{cmd}\n'.encode())
        time.sleep(3)  # Wait for movement

        ser.write(b'STATUS\n')
        time.sleep(0.5)
        response = ser.read(500).decode('utf-8', errors='ignore')
        print(f"Result: {response.strip()}")

    ser.close()

if __name__ == "__main__":
    print("Stepper Motor Diagnostic Tool")
    print("=" * 30)

    try:
        test_stepper_movement()
        test_manual_step_pulses()
    except Exception as e:
        print(f"Error: {e}")

    print("\nDiagnostic complete!")
