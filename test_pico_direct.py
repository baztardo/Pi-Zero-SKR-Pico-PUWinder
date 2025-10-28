#!/usr/bin/env python3
"""
Direct Pico Firmware Test Script
Tests basic commands without web interface
"""

import serial
import time
import sys

def test_pico_commands():
    """Test basic Pico commands directly"""
    
    # Try different serial ports
    ports_to_try = ['/dev/ttyUSB0', '/dev/ttyUSB1', '/dev/ttyACM0', '/dev/ttyACM1']
    
    for port in ports_to_try:
        try:
            print(f"\n🔍 Trying port: {port}")
            ser = serial.Serial(port, 115200, timeout=2)
            time.sleep(1)  # Let it settle
            
            # Clear any existing data
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            print(f"✅ Connected to {port}")
            
            # Test 1: PING
            print("\n1. Testing PING...")
            ser.write(b"PING\n")
            time.sleep(0.5)
            response = ser.readline().decode().strip()
            print(f"   Response: {response}")
            
            # Test 2: VERSION
            print("\n2. Testing VERSION...")
            ser.write(b"VERSION\n")
            time.sleep(0.5)
            response = ser.readline().decode().strip()
            print(f"   Response: {response}")
            
            # Test 3: STATUS
            print("\n3. Testing STATUS...")
            ser.write(b"STATUS\n")
            time.sleep(0.5)
            response = ser.readline().decode().strip()
            print(f"   Response: {response}")
            
            # Test 4: G28 (Home)
            print("\n4. Testing G28 (Home)...")
            ser.write(b"G28\n")
            time.sleep(2)  # Give time for homing
            response = ser.readline().decode().strip()
            print(f"   Response: {response}")
            
            # Test 5: M5 (Stop)
            print("\n5. Testing M5 (Stop)...")
            ser.write(b"M5\n")
            time.sleep(0.5)
            response = ser.readline().decode().strip()
            print(f"   Response: {response}")
            
            # Test 6: M112 (Emergency Stop)
            print("\n6. Testing M112 (Emergency Stop)...")
            ser.write(b"M112\n")
            time.sleep(0.5)
            response = ser.readline().decode().strip()
            print(f"   Response: {response}")
            
            ser.close()
            print(f"\n✅ All tests completed on {port}")
            return True
            
        except Exception as e:
            print(f"   ❌ Failed: {e}")
            continue
    
    print("\n❌ No working serial port found")
    return False

if __name__ == "__main__":
    print("🔧 Pico Firmware Direct Test")
    print("=" * 40)
    test_pico_commands()
