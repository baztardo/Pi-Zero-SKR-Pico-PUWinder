#!/usr/bin/env python3
"""
Pi CM4 USB Test Script for Pico Coil Winder
Based on Klipper's USB communication approach
"""

import serial
import time
import sys
import glob

class PicoUSBHost:
    def __init__(self):
        self.serial_port = None
        self.find_pico_device()

    def find_pico_device(self):
        """Find the Pico device by VID/PID"""
        # USB VID/PID from our descriptors
        vid = "1d50"
        pid = "614e"

        # Look for serial devices (CDC ACM)
        devices = glob.glob('/dev/ttyACM*')
        if not devices:
            devices = glob.glob('/dev/ttyUSB*')

        for device in devices:
            try:
                # Try to open and read device info
                with serial.Serial(device, 115200, timeout=1) as ser:
                    ser.write(b'M115\n')  # Klipper identification command
                    time.sleep(0.1)
                    response = ser.read(1000).decode('utf-8', errors='ignore')
                    if 'Pi-Zero-SKR-Pico-PUWinder' in response:
                        print(f"Found Pico device: {device}")
                        self.serial_port = device
                        return
            except:
                continue

        raise Exception("Pico device not found. Make sure it's connected via USB.")

    def connect(self):
        """Connect to the Pico"""
        if not self.serial_port:
            self.find_pico_device()

        self.serial = serial.Serial(
            self.serial_port,
            baudrate=115200,
            timeout=1,
            write_timeout=1
        )
        print(f"Connected to Pico on {self.serial_port}")

        # Wait for device to be ready
        time.sleep(1)

    def send_command(self, command):
        """Send a command and get response"""
        if not self.serial:
            raise Exception("Not connected")

        # Send command
        self.serial.write(f"{command}\n".encode())
        self.serial.flush()

        # Read response
        response = ""
        start_time = time.time()
        while time.time() - start_time < 2:  # 2 second timeout
            if self.serial.in_waiting:
                data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                response += data
                if '\n' in response:
                    break
            time.sleep(0.01)

        return response.strip()

    def test_basic_communication(self):
        """Test basic communication"""
        print("Testing basic communication...")

        # Test identification
        response = self.send_command("M115")
        print(f"M115 response: {response}")

        # Test ping
        response = self.send_command("M105")
        print(f"M105 response: {response}")

        # Test help
        response = self.send_command("HELP")
        print(f"HELP response: {response}")

    def test_motion_commands(self):
        """Test motion commands"""
        print("Testing motion commands...")

        # Home command
        response = self.send_command("G28")
        print(f"G28 response: {response}")

        # Move to position
        response = self.send_command("G1 X100 Y50 F1000")
        print(f"G1 response: {response}")

        # Spindle on
        response = self.send_command("M3 S1000")
        print(f"M3 response: {response}")

        # Spindle off
        response = self.send_command("M5")
        print(f"M5 response: {response}")

    def close(self):
        """Close the connection"""
        if self.serial:
            self.serial.close()
            print("Disconnected from Pico")

def main():
    print("Pi CM4 Pico USB Test")
    print("=" * 40)

    try:
        # Create host
        host = PicoUSBHost()

        # Connect
        host.connect()

        # Run tests
        host.test_basic_communication()
        print()
        host.test_motion_commands()

        # Close
        host.close()

        print("\nAll tests completed successfully!")

    except Exception as e:
        print(f"Test failed: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
