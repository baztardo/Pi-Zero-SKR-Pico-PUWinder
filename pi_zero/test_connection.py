#!/usr/bin/env python3
"""
Connection Test for Pi Zero and Pico
Purpose: Test connection to both Pi Zero and Pico devices
"""

import serial
import time
import sys

def test_device(port, device_name):
    """Test connection to a specific device"""
    print(f"\n🔍 Testing {device_name} on {port}...")
    
    try:
        ser = serial.Serial(port, 115200, timeout=2)
        time.sleep(1)  # Wait for connection
        
        # Test 1: Send PING command
        print("  📤 Sending PING...")
        ser.write(b'PING\n')
        time.sleep(0.5)
        
        response = ser.read(100)
        decoded = response.decode('utf-8', errors='ignore').strip()
        
        if decoded:
            print(f"  📥 Response: {decoded}")
            
            # Check if it's a Pico (responds to PING)
            if 'PONG' in decoded or 'Pico' in decoded:
                print(f"  ✅ {device_name} is responding to PING - This looks like the Pico!")
                return True
            else:
                print(f"  ℹ️  {device_name} is sending data but not responding to PING")
                return False
        else:
            print(f"  ❌ No response from {device_name}")
            return False
            
    except Exception as e:
        print(f"  ❌ Error connecting to {device_name}: {e}")
        return False
    finally:
        try:
            ser.close()
        except:
            pass

def main():
    """Test all available devices"""
    print("🚀 Pi Zero & Pico Connection Test")
    print("=" * 50)
    
    # List of devices to test
    devices = [
        ('/dev/tty.usbmodem31442301', 'Pi Zero'),
        ('/dev/tty.usbmodem31442402', 'Device 2'),
        ('/dev/tty.usbmodem3144301', 'Device 3 (Pico?)')
    ]
    
    results = {}
    
    for port, name in devices:
        results[name] = test_device(port, name)
    
    print("\n📊 Test Results:")
    print("=" * 30)
    
    for name, success in results.items():
        status = "✅ Connected" if success else "❌ Failed"
        print(f"{name}: {status}")
    
    # Recommendations
    print("\n💡 Recommendations:")
    
    if results.get('Pi Zero'):
        print("✅ Pi Zero is connected and working")
        print("   - You can run your winding machine code")
        print("   - Try: python3 pi_zero/main_controller.py")
    
    if results.get('Device 3 (Pico?)'):
        print("✅ Pico is connected and responding")
        print("   - You can test G-code commands")
        print("   - Try: python3 pi_zero/test_gcode_safety.py")
    
    if not any(results.values()):
        print("❌ No devices responding properly")
        print("   - Check USB connections")
        print("   - Try different USB ports")
        print("   - Check if devices are powered on")

if __name__ == "__main__":
    main()
