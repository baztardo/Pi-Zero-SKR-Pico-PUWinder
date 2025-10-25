#!/usr/bin/env python3
"""
G-code Safety Commands Test for Pi Zero ↔ SKR Pico communication
Tests the new FluidNC-style safety commands
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
    print("=" * 60)
    print("Pi Zero G-code Safety Commands Test")
    print("=" * 60)
    
    try:
        # Open serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"✅ Opened {SERIAL_PORT} @ {BAUD_RATE} baud")
        time.sleep(0.1)
        
        # Flush any startup messages
        ser.reset_input_buffer()
        startup = ser.read_all()
        if startup:
            print(f"📥 Startup messages: {startup.decode('utf-8', errors='ignore')}")
        
        # Test basic commands
        print("\n🔍 Testing basic connectivity...")
        response = send_command(ser, "PING")
        if response == "PONG":
            print("✅ PING/PONG - OK")
        else:
            print(f"❌ PING failed: {response}")
        
        response = send_command(ser, "VERSION")
        if response:
            print(f"✅ VERSION: {response}")
        else:
            print("❌ VERSION failed")
        
        # Test STATUS command
        print("\n📊 Testing STATUS command...")
        response = send_command(ser, "STATUS")
        if response and response.startswith("STATUS:"):
            print(f"✅ STATUS: {response}")
        else:
            print(f"❌ STATUS failed: {response}")
        
        # ⭐ Test FluidNC-style Safety Commands
        print("\n🛡️ Testing FluidNC-style Safety Commands...")
        
        # Test M0 (feed hold)
        print("\n⏸️ Testing M0 (feed hold)...")
        response = send_command(ser, "M0")
        if response and "PAUSED" in response:
            print("✅ M0 (feed hold) - OK")
        else:
            print(f"❌ M0 failed: {response}")
        
        # Test M1 (resume from hold)
        print("\n▶️ Testing M1 (resume from hold)...")
        response = send_command(ser, "M1")
        if response and "RESUMED" in response:
            print("✅ M1 (resume from hold) - OK")
        else:
            print(f"❌ M1 failed: {response}")
        
        # Test M112 (emergency stop)
        print("\n🚨 Testing M112 (emergency stop)...")
        response = send_command(ser, "M112")
        if response and "EMERGENCY_STOP" in response:
            print("✅ M112 (emergency stop) - OK")
        else:
            print(f"❌ M112 failed: {response}")
        
        # Test M999 (reset from emergency stop)
        print("\n🔄 Testing M999 (reset from emergency stop)...")
        response = send_command(ser, "M999")
        if response and "RESET_OK" in response:
            print("✅ M999 (reset from emergency stop) - OK")
        else:
            print(f"❌ M999 failed: {response}")
        
        # Test M410 (quick stop)
        print("\n⏹️ Testing M410 (quick stop)...")
        response = send_command(ser, "M410")
        if response and "STOPPED" in response:
            print("✅ M410 (quick stop) - OK")
        else:
            print(f"❌ M410 failed: {response}")
        
        # Test G4 P0 (planner sync)
        print("\n🔄 Testing G4 P0 (planner sync)...")
        response = send_command(ser, "G4 P0")
        if response and "OK" in response:
            print("✅ G4 P0 (planner sync) - OK")
        else:
            print(f"❌ G4 P0 failed: {response}")
        
        # Test G4 P1000 (dwell 1 second)
        print("\n⏱️ Testing G4 P1000 (dwell 1 second)...")
        start_time = time.time()
        response = send_command(ser, "G4 P1000", timeout=3)
        elapsed = time.time() - start_time
        if response and "OK" in response:
            print(f"✅ G4 P1000 (dwell) - OK (took {elapsed:.1f}s)")
        else:
            print(f"❌ G4 P1000 failed: {response}")
        
        # Test spindle control with safety
        print("\n🔄 Testing spindle control with safety...")
        
        # Start spindle
        response = send_command(ser, "M3 S300")
        if response and "OK" in response:
            print("✅ M3 S300 (spindle CW) - OK")
        else:
            print(f"❌ M3 S300 failed: {response}")
        
        # Feed hold while running
        response = send_command(ser, "M0")
        if response and "PAUSED" in response:
            print("✅ Feed hold while spindle running - OK")
        else:
            print(f"❌ Feed hold failed: {response}")
        
        # Resume
        response = send_command(ser, "M1")
        if response and "RESUMED" in response:
            print("✅ Resume after feed hold - OK")
        else:
            print(f"❌ Resume failed: {response}")
        
        # Emergency stop
        response = send_command(ser, "M112")
        if response and "EMERGENCY_STOP" in response:
            print("✅ Emergency stop - OK")
        else:
            print(f"❌ Emergency stop failed: {response}")
        
        # Reset from emergency
        response = send_command(ser, "M999")
        if response and "RESET_OK" in response:
            print("✅ Reset from emergency - OK")
        else:
            print(f"❌ Reset from emergency failed: {response}")
        
        # Stop spindle
        response = send_command(ser, "M5")
        if response and "OK" in response:
            print("✅ M5 (stop spindle) - OK")
        else:
            print(f"❌ M5 failed: {response}")
        
        print("\n🎉 All safety command tests completed!")
        
    except serial.SerialException as e:
        print(f"❌ Serial error: {e}")
    except KeyboardInterrupt:
        print("\n⏹️ Test interrupted by user")
    finally:
        if 'ser' in locals():
            ser.close()
            print("🔌 Serial port closed")

if __name__ == "__main__":
    main()
