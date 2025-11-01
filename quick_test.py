#!/usr/bin/env python3
"""
Quick stepper test - move and check if position changes
"""

import serial
import time

print("Quick Stepper Test")
print("==================")

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

# Get starting position
ser.write(b'STATUS\n')
time.sleep(0.5)
start_response = ser.read(500).decode('utf-8', errors='ignore')
print(f"START: {start_response.strip()}")

# Move 10mm
print("Moving 10mm...")
ser.write(b'G1 X10 F500\n')
time.sleep(3)  # Wait for move

# Get ending position
ser.write(b'STATUS\n')
time.sleep(0.5)
end_response = ser.read(500).decode('utf-8', errors='ignore')
print(f"END: {end_response.strip()}")

ser.close()

print("\nDid the 'Traverse=' value change? If yes, stepper IS moving!")