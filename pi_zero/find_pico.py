#!/usr/bin/env python3
"""
Find Pico Device
Purpose: Identify which device is actually the Pico
"""

import serial
import time
import sys

def test_device(device_path):
    """Test a device to see if it's the Pico"""
    try:
        print(f"🔍 Testing {device_path}...")
        ser = serial.Serial(device_path, 115200, timeout=2)
        time.sleep(0.5)
        
        # Send PING command
        ser.write(b'PING\n')
        time.sleep(0.5)
        
        # Read response
        response = ser.read(200)
        decoded = response.decode('utf-8', errors='ignore').strip()
        
        ser.close()
        
        print(f"  Response: {repr(decoded)}")
        
        # Check if this looks like a Pico response
        if decoded and decoded != '.' and decoded != '\x00':
            if 'PONG' in decoded or 'Status' in decoded or 'OK' in decoded:
                print(f"  ✅ This looks like a working Pico!")
                return True
            else:
                print(f"  ⚠️  This device responds but doesn't look like Pico")
                return False
        else:
            print(f"  ❌ This device is stuck or not responding properly")
            return False
            
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def main():
    """Find the working Pico device"""
    print("🔍 Searching for Pico device...")
    print("=" * 40)
    
    # List all USB devices
    import subprocess
    try:
        result = subprocess.run(['ls', '/dev/tty.usbmodem*'], capture_output=True, text=True)
        devices = result.stdout.strip().split('\n')
        devices = [d for d in devices if d]  # Remove empty strings
    except:
        devices = []
    
    if not devices:
        print("❌ No USB devices found")
        return
    
    print(f"Found {len(devices)} USB devices:")
    for device in devices:
        print(f"  - {device}")
    print()
    
    # Test each device
    working_devices = []
    for device in devices:
        if test_device(device):
            working_devices.append(device)
        print()
    
    if working_devices:
        print(f"✅ Found {len(working_devices)} working Pico device(s):")
        for device in working_devices:
            print(f"  - {device}")
        
        print(f"\n🎯 Recommended device: {working_devices[0]}")
        print(f"   Update your controllers to use: {working_devices[0]}")
    else:
        print("❌ No working Pico devices found")
        print("   The Pico may need to be reset or re-flashed")

if __name__ == "__main__":
    main()
