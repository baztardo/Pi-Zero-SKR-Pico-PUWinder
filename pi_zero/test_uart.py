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
        
        # Flush any startup messages from Pico
        ser.reset_input_buffer()
        startup = ser.read_all()
        if startup:
            print(f"   Flushed startup data: {startup}")
        time.sleep(0.1)
        
        # Send test command
        print("\n📤 Sending: PING")
        data = b"PING\n"
        print(f"   Raw bytes: {data} (length: {len(data)})")
        ser.write(data)
        
        # Wait for response
        print("📥 Waiting for response...")
        raw_response = ser.readline()
        print(f"   Raw bytes: {raw_response} (length: {len(raw_response)})")
        response = raw_response.decode('utf-8').strip()
        
        if response:
            print(f"✅ Received: '{response}'")
            print(f"   After strip: '{response}' (length: {len(response)})")
            
            if response == "PONG":
                print("\n🎉 SUCCESS! UART communication working!")
            else:
                print(f"\n⚠️  Unexpected response: '{response}'")
        
        # Try VERSION command too
        print("\n📤 Sending: VERSION")
        ser.write(b"VERSION\n")
        
        print("📥 Waiting for response...")
        response2 = ser.readline().decode('utf-8').strip()
        if response2:
            print(f"✅ Received: {response2}")
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
