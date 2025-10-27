#!/usr/bin/env python3
"""
UART Connection Test Script
Tests basic UART communication with Pico
"""

import serial
import time
import sys

def test_uart_connection(port="/dev/ttyUSB0", baudrate=115200):
    """Test UART connection with detailed diagnostics"""
    print("🔍 UART Connection Test")
    print("=" * 40)
    print(f"Port: {port}")
    print(f"Baudrate: {baudrate}")
    print()
    
    try:
        # Open serial connection
        print("1. Opening serial connection...")
        ser = serial.Serial(port, baudrate, timeout=2.0)
        time.sleep(2)  # Wait for Pico to initialize
        print("   ✅ Serial port opened")
        
        # Clear buffers
        print("2. Clearing UART buffers...")
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("   ✅ Buffers cleared")
        
        # Send dummy message
        print("3. Sending dummy message...")
        ser.write(b"PING\n")
        ser.flush()
        time.sleep(0.1)
        ser.reset_input_buffer()
        print("   ✅ Dummy message sent")
        
        # Test ping command
        print("4. Testing PING command...")
        for attempt in range(3):
            try:
                ser.write(b"PING\n")
                ser.flush()
                time.sleep(0.2)
                
                if ser.in_waiting > 0:
                    response = ser.readline().decode().strip()
                    print(f"   Attempt {attempt + 1}: {response}")
                    if response == "PONG":
                        print("   ✅ PING successful!")
                        break
                else:
                    print(f"   Attempt {attempt + 1}: No response")
                    
            except Exception as e:
                print(f"   Attempt {attempt + 1}: Error - {e}")
            
            time.sleep(0.5)
        else:
            print("   ❌ PING failed after 3 attempts")
            return False
        
        # Test version command
        print("5. Testing VERSION command...")
        ser.write(b"VERSION\n")
        ser.flush()
        time.sleep(0.2)
        
        if ser.in_waiting > 0:
            response = ser.readline().decode().strip()
            print(f"   ✅ Version: {response}")
        else:
            print("   ❌ No version response")
        
        # Test status command
        print("6. Testing STATUS command...")
        ser.write(b"STATUS\n")
        ser.flush()
        time.sleep(0.2)
        
        if ser.in_waiting > 0:
            response = ser.readline().decode().strip()
            print(f"   ✅ Status: {response}")
        else:
            print("   ❌ No status response")
        
        # Close connection
        ser.close()
        print("\n✅ UART connection test completed successfully!")
        return True
        
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        print("   Check if Pico is connected and UART is enabled")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

def main():
    """Main test function"""
    print("🔄 Pi Zero UART Test")
    print("=" * 30)
    
    # Try different common UART ports
    ports_to_try = [
        "/dev/ttyUSB0",
        "/dev/ttyUSB1", 
        "/dev/ttyAMA0",
        "/dev/serial0"
    ]
    
    success = False
    for port in ports_to_try:
        print(f"\n🔍 Trying port: {port}")
        try:
            if test_uart_connection(port):
                success = True
                print(f"\n🎉 Success! Use port: {port}")
                break
        except Exception as e:
            print(f"   ❌ Port {port} failed: {e}")
    
    if not success:
        print("\n❌ No working UART port found")
        print("   Check:")
        print("   - Pico is connected via UART")
        print("   - UART is enabled: sudo raspi-config")
        print("   - User is in dialout group: sudo usermod -a -G dialout $USER")
        print("   - Available ports: ls /dev/tty*")
        sys.exit(1)
    else:
        print("\n🎉 UART connection is working!")
        print("   You can now run the winding controller")

if __name__ == "__main__":
    main()
