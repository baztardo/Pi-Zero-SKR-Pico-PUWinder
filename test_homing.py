#!/usr/bin/env python3
"""
Test homing setup and limit switch positioning
"""

import serial
import time

def test_homing_detailed():
    """Detailed homing test with monitoring"""
    print("=== HOMING DIAGNOSTIC TEST ===")

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Start homing
    print("\n1. Starting homing sequence...")
    ser.reset_input_buffer()
    ser.write(b'G28\n')

    # Monitor progress
    start_time = time.time()
    while time.time() - start_time < 10:  # Monitor for 10 seconds
        if ser.in_waiting:
            data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"Progress: {data.strip()}")

            # Check for completion or timeout messages
            if "HOMING" in data and "complete" in data.lower():
                print("✓ Homing completed!")
                break
            if "timeout" in data.lower():
                print("✗ Homing timed out!")
                break

        time.sleep(0.1)

    # Check final status
    print("\n2. Checking final status...")
    ser.write(b'STATUS\n')
    time.sleep(0.5)
    response = ser.read(1000).decode('utf-8', errors='ignore')
    print(f"Final status: {response.strip()}")

    ser.close()

def test_manual_movement():
    """Test manual movement in both directions"""
    print("\n=== MANUAL MOVEMENT TEST ===")

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    directions = [
        ("Away from home (+X)", b'G1 X50 F100\n'),
        ("Toward home (-X)", b'G1 X-50 F100\n')
    ]

    for direction_name, command in directions:
        print(f"\nTesting: {direction_name}")
        ser.write(command)
        time.sleep(3)  # Give time to move

        # Check position
        ser.write(b'STATUS\n')
        time.sleep(0.5)
        response = ser.read(1000).decode('utf-8', errors='ignore')
        print(f"Position after {direction_name}: {response.strip()}")

        # Ask user if switch was triggered
        input(f"Was the limit switch triggered during {direction_name}? (press Enter)")

    ser.close()

def test_switch_detection():
    """Test if limit switch is properly detected"""
    print("\n=== LIMIT SWITCH DETECTION TEST ===")

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    print("Press and hold the limit switch, then press Enter...")
    input()

    ser.write(b'STATUS\n')
    time.sleep(0.5)
    response = ser.read(1000).decode('utf-8', errors='ignore')
    print(f"Status with switch pressed: {response.strip()}")

    print("Release the limit switch, then press Enter...")
    input()

    ser.write(b'STATUS\n')
    time.sleep(0.5)
    response = ser.read(1000).decode('utf-8', errors='ignore')
    print(f"Status with switch released: {response.strip()}")

    ser.close()

if __name__ == "__main__":
    print("Traverse Homing Diagnostic Tool")
    print("=" * 40)

    while True:
        print("\nSelect test:")
        print("1. Detailed homing test")
        print("2. Manual movement test")
        print("3. Switch detection test")
        print("4. Exit")

        choice = input("Enter choice (1-4): ").strip()

        if choice == '1':
            test_homing_detailed()
        elif choice == '2':
            test_manual_movement()
        elif choice == '3':
            test_switch_detection()
        elif choice == '4':
            break
        else:
            print("Invalid choice")

    print("Diagnostic complete!")
