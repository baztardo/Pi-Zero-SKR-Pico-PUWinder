#!/usr/bin/env python3
"""
Simple test for the motion planner connection
"""

import serial
import time
import glob

def test_connection():
    """Test basic connection to Pico"""
    print("Testing Pico connection...")

    # Find Pico
    devices = glob.glob('/dev/ttyACM*')
    if not devices:
        devices = glob.glob('/dev/ttyUSB*')

    if not devices:
        print("❌ No serial devices found")
        return False

    device = devices[0]
    print(f"Found device: {device}")

    try:
        # Connect
        ser = serial.Serial(device, 115200, timeout=2)
        print("✓ Connected")

        # Test commands
        commands = [
            ("VERSION", "Pico_Spindle"),
            ("PING", "PONG"),
            ("STATUS", "STATUS:")
        ]

        for cmd, expected in commands:
            ser.write(f"{cmd}\n".encode())
            ser.flush()

            time.sleep(0.2)
            response = ""
            while ser.in_waiting:
                response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')

            print(f"  {cmd}: {'✓' if expected in response else '✗'}")
            print(f"    Response: {response.strip()[:100]}...")

        ser.close()
        print("✓ Test completed")
        return True

    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == "__main__":
    print("Motion Planner Connection Test")
    print("=" * 30)
    success = test_connection()
    print("\n" + ("✅ Ready for motion planning!" if success else "❌ Connection issues"))
