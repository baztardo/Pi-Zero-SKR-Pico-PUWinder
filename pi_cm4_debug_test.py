#!/usr/bin/env python3
"""
Debug test for Pico USB communication
"""

import serial
import time

def test_basic_connection():
    """Test basic serial connection"""
    print("Testing basic serial connection...")

    try:
        # Open serial port
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
        print("✓ Serial port opened successfully")

        # Send a simple command
        ser.write(b'M115\n')
        ser.flush()
        print("✓ Sent M115 command")

        # Read response
        time.sleep(0.5)
        if ser.in_waiting:
            response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"✓ Response: {repr(response)}")
        else:
            print("✗ No response received")

        ser.close()
        print("✓ Serial port closed")

    except Exception as e:
        print(f"✗ Error: {e}")

def test_manual_commands():
    """Test manual command sending"""
    print("\nTesting manual commands...")

    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)

        commands = ['M115', 'M105', 'HELP']

        for cmd in commands:
            print(f"\n--- Testing {cmd} ---")
            ser.write(f'{cmd}\n'.encode())
            ser.flush()

            time.sleep(0.2)

            response = ""
            while ser.in_waiting:
                response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                time.sleep(0.1)

            if response:
                print(f"Response: {repr(response)}")
            else:
                print("No response")

        ser.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    print("Pico USB Debug Test")
    print("=" * 30)

    test_basic_connection()
    test_manual_commands()

    print("\nDebug complete!")
