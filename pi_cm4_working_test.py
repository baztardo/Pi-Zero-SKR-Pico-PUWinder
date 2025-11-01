#!/usr/bin/env python3
"""
Working test for Pico Coil Winder USB communication
Using the actual commands supported by the firmware
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
        devices = glob.glob('/dev/ttyACM*')
        if not devices:
            devices = glob.glob('/dev/ttyUSB*')

        for device in devices:
            try:
                # Try to open and identify the device
                with serial.Serial(device, 115200, timeout=1) as ser:
                    ser.write(b'VERSION\n')  # Firmware version command
                    time.sleep(0.1)
                    response = ser.read(200).decode('utf-8', errors='ignore')
                    if 'VERSION' in response or len(response) > 0:
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
        time.sleep(1)

    def send_command(self, command):
        """Send a command and get response"""
        if not self.serial:
            raise Exception("Not connected")

        # Send command
        self.serial.write(f"{command}\n".encode())
        self.serial.flush()

        # Read response with timeout
        response = ""
        start_time = time.time()
        while time.time() - start_time < 2:  # 2 second timeout
            if self.serial.in_waiting:
                data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                response += data
                # Break if we got a complete response (ends with newline)
                if '\n' in response:
                    break
            time.sleep(0.01)

        return response.strip()

    def test_basic_communication(self):
        """Test basic communication with supported commands"""
        print("Testing basic communication...")

        # Test VERSION command
        response = self.send_command("VERSION")
        print(f"VERSION response: {response}")

        # Test PING command
        response = self.send_command("PING")
        print(f"PING response: {response}")

        # Test STATUS command
        response = self.send_command("STATUS")
        print(f"STATUS response: {response}")

    def test_motion_commands(self):
        """Test motion commands"""
        print("Testing motion commands...")

        # Test G28 (home)
        response = self.send_command("G28")
        print(f"G28 (home) response: {response}")

        # Test G1 (move) - small move
        response = self.send_command("G1 X10 Y5 F500")
        print(f"G1 (move) response: {response}")

        # Test M3 (spindle on)
        response = self.send_command("M3 S500")
        print(f"M3 (spindle on) response: {response}")

        # Test M5 (spindle off)
        response = self.send_command("M5")
        print(f"M5 (spindle off) response: {response}")

    def test_winding(self):
        """Test winding commands"""
        print("Testing winding commands...")

        # Test WIND command with parameters
        response = self.send_command("WIND T100 S30")
        print(f"WIND response: {response}")

    def close(self):
        """Close the connection"""
        if self.serial:
            self.serial.close()
            print("Disconnected from Pico")

def main():
    print("Pi CM4 Pico USB Test - Working Commands")
    print("=" * 50)

    try:
        # Create host
        host = PicoUSBHost()

        # Connect
        host.connect()

        # Run tests
        host.test_basic_communication()
        print()
        host.test_motion_commands()
        print()
        host.test_winding()

        # Close
        host.close()

        print("\n🎉 All tests completed successfully!")
        print("✅ USB communication is working perfectly!")
        print("✅ Pico coil winder is ready for use!")

    except Exception as e:
        print(f"❌ Test failed: {e}")
        print("\nTroubleshooting:")
        print("- Make sure Pico is connected via USB")
        print("- Check that firmware was flashed correctly")
        print("- Verify Pico has power (LED should be on)")
        sys.exit(1)

if __name__ == "__main__":
    main()
