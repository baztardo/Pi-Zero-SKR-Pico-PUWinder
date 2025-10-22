#!/usr/bin/env python3
"""
Spindle Control Test for Pi Zero ↔ SKR Pico communication
Tests spindle RPM control commands
"""

try:
    import serial
    import time
except ImportError as e:
    print(f"❌ Missing dependency: {e}")
    print("   Install with: sudo apt install python3-serial")
    exit(1)

# UART configuration
SERIAL_PORT = '/dev/serial0'
BAUD_RATE = 115200

def send_command(ser, command, timeout=2):
    """Send command and wait for response"""
    print(f"📤 Sending: {command}")
    ser.write(f"{command}\n".encode())
    
    # Wait for response
    start_time = time.time()
    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            response = ser.readline().decode('utf-8').strip()
            print(f"📥 Received: {response}")
            return response
        time.sleep(0.01)
    
    print("❌ Timeout - no response")
    return None

def main():
    print("=" * 50)
    print("Pi Zero Spindle Control Test")
    print("=" * 50)
    
    try:
        # Open serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"✅ Opened {SERIAL_PORT} @ {BAUD_RATE} baud")
        time.sleep(0.1)
        
        # Flush any startup messages
        ser.reset_input_buffer()
        startup = ser.read_all()
        if startup:
            print(f"   Flushed startup data: {startup}")
        time.sleep(0.1)
        
        # Test basic communication
        print("\n🔍 Testing basic communication...")
        response = send_command(ser, "PING")
        if response != "PONG":
            print("❌ Basic communication failed!")
            return
        
        # Test version
        print("\n🔍 Testing version...")
        send_command(ser, "VERSION")
        
        # Test spindle control
        print("\n🔍 Testing spindle control...")
        
        # Set spindle to 100 RPM
        print("\n📤 Setting spindle to 100 RPM...")
        response = send_command(ser, "SET_SPINDLE_RPM 100")
        if response == "OK":
            print("✅ Spindle set to 100 RPM")
        else:
            print(f"❌ Failed to set RPM: {response}")
        
        # Get current RPM
        print("\n📤 Getting current RPM...")
        response = send_command(ser, "GET_SPINDLE_RPM")
        if response and response.startswith("RPM:"):
            rpm = response.split(":")[1]
            print(f"✅ Current RPM: {rpm}")
        else:
            print(f"❌ Failed to get RPM: {response}")
        
        # Test higher RPM
        print("\n📤 Setting spindle to 500 RPM...")
        response = send_command(ser, "SET_SPINDLE_RPM 500")
        if response == "OK":
            print("✅ Spindle set to 500 RPM")
        
        # Get RPM again
        print("\n📤 Getting current RPM...")
        response = send_command(ser, "GET_SPINDLE_RPM")
        if response and response.startswith("RPM:"):
            rpm = response.split(":")[1]
            print(f"✅ Current RPM: {rpm}")
        
        # Stop spindle
        print("\n📤 Stopping spindle...")
        response = send_command(ser, "STOP_SPINDLE")
        if response == "STOPPED":
            print("✅ Spindle stopped")
        
        # Test error handling
        print("\n🔍 Testing error handling...")
        send_command(ser, "SET_SPINDLE_RPM 5000")  # Too high
        send_command(ser, "SET_SPINDLE_RPM -100")  # Negative
        send_command(ser, "UNKNOWN_COMMAND")      # Invalid command
        
        print("\n🎉 Spindle control test completed!")
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
        print("   Check: Is /dev/serial0 enabled?")
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user")

if __name__ == "__main__":
    main()
