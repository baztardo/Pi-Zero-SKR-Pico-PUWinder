#!/usr/bin/env python3
"""
Simple UART test for Pi Zero ↔ SKR Pico communication
Tests basic send/receive before full integration
"""

import serial
import time

# UART configuration
SERIAL_PORT = '/dev/serial0'
BAUD_RATE = 115200

def main():
    print("=" * 50)
    print("Pi Zero UART Test")
    print("=" * 50)
    
    try:
        # Open serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"✅ Opened {SERIAL_PORT} @ {BAUD_RATE} baud")
        time.sleep(0.1)
        
        # Send test command
        print("\n📤 Sending: PING")
        ser.write(b"PING\n")
        
        # Wait for response
        print("📥 Waiting for response...")
        response = ser.readline().decode('utf-8').strip()
        
        if response:
            print(f"✅ Received: {response}")
            
            if response == "PONG":
                print("\n🎉 SUCCESS! UART communication working!")
            else:
                print(f"\n⚠️  Unexpected response: {response}")
        else:
            print("❌ No response (timeout)")
            print("   Check:")
            print("   - Pico firmware uploaded?")
            print("   - Wiring correct? (TX→RX crossed)")
            print("   - Both powered on?")
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        print("   Check: Is /dev/serial0 enabled?")
        print("   Run: ls -l /dev/serial0")
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user")

if __name__ == "__main__":
    main()
