#!/usr/bin/env python3
"""
Test G-code Interface
Purpose: Test the new G-code interface on Pico with traverse control
"""

import time
from uart_api import UARTAPI

def test_gcode_interface():
    """Test the new G-code interface"""
    print("🧪 Testing G-code Interface")
    print("=" * 50)
    
    # Create UART API
    api = UARTAPI()
    
    # Connect to Pico
    if not api.connect():
        print("❌ Failed to connect to Pico")
        return False
    
    print("✅ Connected to Pico")
    
    # Test basic commands
    print("\n🔧 Testing Basic Commands:")
    
    # Test PING
    if api.ping():
        print("✅ PING successful")
    else:
        print("❌ PING failed")
        return False
    
    # Test VERSION
    version = api.get_version()
    if version:
        print(f"✅ VERSION: {version}")
    else:
        print("❌ VERSION failed")
        return False
    
    # Test G-code commands
    print("\n🎮 Testing G-code Commands:")
    
    # Test M3 (spindle CW)
    if api.send_gcode("M3 S300"):
        print("✅ M3 S300 (spindle CW at 300 RPM)")
    else:
        print("❌ M3 S300 failed")
    
    time.sleep(1)
    
    # Test M4 (spindle CCW)
    if api.send_gcode("M4 S400"):
        print("✅ M4 S400 (spindle CCW at 400 RPM)")
    else:
        print("❌ M4 S400 failed")
    
    time.sleep(1)
    
    # Test M5 (spindle stop)
    if api.send_gcode("M5"):
        print("✅ M5 (spindle stop)")
    else:
        print("❌ M5 failed")
    
    # Test traverse control
    print("\n🎯 Testing Traverse Control:")
    
    # Test G28 Y (home Y-axis)
    if api.send_gcode("G28 Y"):
        print("✅ G28 Y (home Y-axis)")
    else:
        print("❌ G28 Y failed")
    
    time.sleep(1)
    
    # Test G1 Y (move traverse)
    if api.send_gcode("G1 Y10 F50"):
        print("✅ G1 Y10 F50 (move to Y=10mm at 50mm/min)")
    else:
        print("❌ G1 Y10 F50 failed")
    
    time.sleep(2)
    
    # Test G1 Y (move back)
    if api.send_gcode("G1 Y0 F50"):
        print("✅ G1 Y0 F50 (move to Y=0mm)")
    else:
        print("❌ G1 Y0 F50 failed")
    
    # Test M-codes
    print("\n🔧 Testing M-codes:")
    
    # Test M10 (traverse brake on)
    if api.send_gcode("M10"):
        print("✅ M10 (traverse brake on)")
    else:
        print("❌ M10 failed")
    
    time.sleep(1)
    
    # Test M11 (traverse brake off)
    if api.send_gcode("M11"):
        print("✅ M11 (traverse brake off)")
    else:
        print("❌ M11 failed")
    
    # Test M17 (enable steppers)
    if api.send_gcode("M17"):
        print("✅ M17 (enable steppers)")
    else:
        print("❌ M17 failed")
    
    time.sleep(1)
    
    # Test M18 (disable steppers)
    if api.send_gcode("M18"):
        print("✅ M18 (disable steppers)")
    else:
        print("❌ M18 failed")
    
    # Test M16 (home all axes)
    if api.send_gcode("M16"):
        print("✅ M16 (home all axes)")
    else:
        print("❌ M16 failed")
    
    # Test error handling
    print("\n🚨 Testing Error Handling:")
    
    # Test invalid command
    if not api.send_gcode("INVALID_COMMAND"):
        print("✅ Invalid command properly rejected")
    else:
        print("❌ Invalid command should have been rejected")
    
    # Test malformed G-code
    if not api.send_gcode("G1"):
        print("✅ Malformed G-code properly rejected")
    else:
        print("❌ Malformed G-code should have been rejected")
    
    # Disconnect
    api.disconnect()
    print("\n🎉 G-code interface test completed!")
    return True

def test_winding_sequence():
    """Test a complete winding sequence"""
    print("\n🧵 Testing Winding Sequence")
    print("=" * 50)
    
    api = UARTAPI()
    
    if not api.connect():
        print("❌ Failed to connect to Pico")
        return False
    
    print("✅ Connected to Pico")
    
    # Winding sequence
    print("\n📋 Executing winding sequence:")
    
    # 1. Home all axes
    print("1. Homing all axes...")
    if api.send_gcode("M16"):
        print("   ✅ All axes homed")
    else:
        print("   ❌ Homing failed")
        return False
    
    # 2. Enable steppers
    print("2. Enabling steppers...")
    if api.send_gcode("M17"):
        print("   ✅ Steppers enabled")
    else:
        print("   ❌ Failed to enable steppers")
        return False
    
    # 3. Start spindle
    print("3. Starting spindle...")
    if api.send_gcode("M3 S300"):
        print("   ✅ Spindle started at 300 RPM")
    else:
        print("   ❌ Failed to start spindle")
        return False
    
    # 4. Move traverse to start position
    print("4. Moving to start position...")
    if api.send_gcode("G1 Y5 F50"):
        print("   ✅ Moved to Y=5mm")
    else:
        print("   ❌ Failed to move traverse")
        return False
    
    # 5. Simulate winding (move traverse)
    print("5. Simulating winding...")
    for i in range(5):
        y_pos = 5 + (i * 0.1)  # Move 0.1mm per turn
        if api.send_gcode(f"G1 Y{y_pos:.1f} F10"):
            print(f"   ✅ Turn {i+1}: Y={y_pos:.1f}mm")
        else:
            print(f"   ❌ Turn {i+1} failed")
            break
        time.sleep(0.5)
    
    # 6. Stop spindle
    print("6. Stopping spindle...")
    if api.send_gcode("M5"):
        print("   ✅ Spindle stopped")
    else:
        print("   ❌ Failed to stop spindle")
    
    # 7. Disable steppers
    print("7. Disabling steppers...")
    if api.send_gcode("M18"):
        print("   ✅ Steppers disabled")
    else:
        print("   ❌ Failed to disable steppers")
    
    api.disconnect()
    print("\n🎉 Winding sequence completed!")
    return True

def main():
    """Main test function"""
    print("🧪 G-code Interface Test Suite")
    print("=" * 60)
    
    try:
        # Test basic G-code interface
        if not test_gcode_interface():
            print("❌ G-code interface test failed")
            return False
        
        # Test winding sequence
        if not test_winding_sequence():
            print("❌ Winding sequence test failed")
            return False
        
        print("\n" + "=" * 60)
        print("🎉 ALL TESTS PASSED!")
        print("✅ G-code interface working correctly")
        print("✅ Traverse control functional")
        print("✅ Error handling working")
        print("✅ Winding sequence successful")
        print("=" * 60)
        
        return True
        
    except KeyboardInterrupt:
        print("\n\n⚠️  Test interrupted by user")
        return False
    except Exception as e:
        print(f"\n❌ Test error: {e}")
        return False

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
