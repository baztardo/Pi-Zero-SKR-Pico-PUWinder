#!/usr/bin/env python3
"""
Simple Pico Command Sender
Purpose: Send G-code commands to Pico easily
Usage: python3 send_pico.py "M3 S500"
"""

import sys
import serial
import time

def send_command(command):
    """Send a command to the Pico"""
    # Try to find the Pico device
    devices = ['/dev/tty.usbmodem3144301', '/dev/tty.usbmodem31442402', '/dev/tty.usbmodem31442301']
    
    for device in devices:
        try:
            print(f"🔍 Trying {device}...")
            ser = serial.Serial(device, 115200, timeout=2)
            time.sleep(0.5)
            
            # Send the command
            print(f"📤 Sending: {command}")
            ser.write(f"{command}\n".encode())
            time.sleep(0.5)
            
            # Read response
            response = ser.read(200)
            decoded = response.decode('utf-8', errors='ignore').strip()
            
            if decoded:
                print(f"📥 Response: {decoded}")
                ser.close()
                return True
            else:
                print("📥 No response")
                ser.close()
                
        except Exception as e:
            print(f"❌ Error with {device}: {e}")
            continue
    
    print("❌ Could not connect to Pico")
    return False

def main():
    """Main function"""
    if len(sys.argv) != 2:
        print("Usage: python3 send_pico.py \"COMMAND\"")
        print("Examples:")
        print("  python3 send_pico.py \"PING\"")
        print("  python3 send_pico.py \"M3 S500\"")
        print("  python3 send_pico.py \"G1 Y50 F1000\"")
        print("  python3 send_pico.py \"M5\"")
        sys.exit(1)
    
    command = sys.argv[1]
    send_command(command)

if __name__ == "__main__":
    main()
