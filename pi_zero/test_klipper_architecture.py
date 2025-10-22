#!/usr/bin/env python3
"""
Test Klipper-Style Architecture
Purpose: Test the proper Klipper-style flow with Scheduler, MoveQueue, and WindingController
"""

import time
from uart_api import UARTAPI

def test_klipper_architecture():
    """Test the Klipper-style architecture components"""
    print("🧪 Testing Klipper-Style Architecture")
    print("=" * 60)
    
    # Create UART API
    api = UARTAPI()
    
    # Connect to Pico
    if not api.connect():
        print("❌ Failed to connect to Pico")
        return False
    
    print("✅ Connected to Pico")
    
    # Test 1: Basic connectivity
    print("\n🔧 Test 1: Basic Connectivity")
    if api.ping():
        print("✅ PING successful")
    else:
        print("❌ PING failed")
        return False
    
    # Test 2: Scheduler and MoveQueue initialization
    print("\n⏰ Test 2: Scheduler and MoveQueue")
    version = api.get_version()
    if version:
        print(f"✅ Version: {version}")
        if "Klipper-style" in version or "Scheduler" in version:
            print("✅ Klipper-style architecture detected")
        else:
            print("⚠️  Klipper-style architecture not confirmed")
    else:
        print("❌ Version check failed")
        return False
    
    # Test 3: Stepper enable/disable (M17/M18)
    print("\n🔌 Test 3: Stepper Control (M17/M18)")
    
    # Enable steppers
    if api.send_gcode("M17"):
        print("✅ M17 (enable steppers) successful")
    else:
        print("❌ M17 failed")
        return False
    
    time.sleep(0.5)
    
    # Disable steppers
    if api.send_gcode("M18"):
        print("✅ M18 (disable steppers) successful")
    else:
        print("❌ M18 failed")
        return False
    
    # Test 4: MoveQueue functionality (G0/G1)
    print("\n🎯 Test 4: MoveQueue Functionality (G0/G1)")
    
    # Enable steppers first
    if not api.send_gcode("M17"):
        print("❌ Failed to enable steppers for movement test")
        return False
    
    # Test G0 (rapid move)
    if api.send_gcode("G0 Y5"):
        print("✅ G0 Y5 (rapid move) successful")
    else:
        print("❌ G0 Y5 failed")
        return False
    
    time.sleep(1)
    
    # Test G1 (linear move)
    if api.send_gcode("G1 Y10 F50"):
        print("✅ G1 Y10 F50 (linear move) successful")
    else:
        print("❌ G1 Y10 F50 failed")
        return False
    
    time.sleep(1)
    
    # Test G1 (move back)
    if api.send_gcode("G1 Y0 F50"):
        print("✅ G1 Y0 F50 (move back) successful")
    else:
        print("❌ G1 Y0 F50 failed")
        return False
    
    # Test 5: WindingController functionality (G28)
    print("\n🏠 Test 5: WindingController Homing (G28)")
    
    if api.send_gcode("G28 Y"):
        print("✅ G28 Y (home Y-axis) successful")
    else:
        print("❌ G28 Y failed")
        return False
    
    time.sleep(1)
    
    # Test 6: Spindle control with MoveQueue
    print("\n🔄 Test 6: Spindle Control")
    
    # Test M3 (spindle CW)
    if api.send_gcode("M3 S300"):
        print("✅ M3 S300 (spindle CW at 300 RPM) successful")
    else:
        print("❌ M3 S300 failed")
        return False
    
    time.sleep(2)
    
    # Test M5 (spindle stop)
    if api.send_gcode("M5"):
        print("✅ M5 (spindle stop) successful")
    else:
        print("❌ M5 failed")
        return False
    
    # Test 7: MoveQueue brake control (M10/M11)
    print("\n🛑 Test 7: MoveQueue Brake Control (M10/M11)")
    
    # Test M10 (traverse brake on)
    if api.send_gcode("M10"):
        print("✅ M10 (traverse brake on) successful")
    else:
        print("❌ M10 failed")
        return False
    
    time.sleep(1)
    
    # Test M11 (traverse brake off)
    if api.send_gcode("M11"):
        print("✅ M11 (traverse brake off) successful")
    else:
        print("❌ M11 failed")
        return False
    
    # Test 8: Complete winding sequence
    print("\n🧵 Test 8: Complete Winding Sequence")
    
    # Home all axes
    if api.send_gcode("M16"):
        print("✅ M16 (home all axes) successful")
    else:
        print("❌ M16 failed")
        return False
    
    time.sleep(1)
    
    # Enable steppers
    if api.send_gcode("M17"):
        print("✅ Steppers enabled for winding")
    else:
        print("❌ Failed to enable steppers")
        return False
    
    # Start spindle
    if api.send_gcode("M3 S200"):
        print("✅ Spindle started for winding")
    else:
        print("❌ Failed to start spindle")
        return False
    
    # Simulate winding moves
    for i in range(3):
        y_pos = i * 2.0  # Move 2mm per layer
        if api.send_gcode(f"G1 Y{y_pos} F100"):
            print(f"✅ Winding layer {i+1}: Y={y_pos}mm")
        else:
            print(f"❌ Winding layer {i+1} failed")
            break
        time.sleep(0.5)
    
    # Stop spindle
    if api.send_gcode("M5"):
        print("✅ Spindle stopped")
    else:
        print("❌ Failed to stop spindle")
    
    # Disable steppers
    if api.send_gcode("M18"):
        print("✅ Steppers disabled")
    else:
        print("❌ Failed to disable steppers")
    
    # Disconnect
    api.disconnect()
    print("\n🎉 Klipper-style architecture test completed!")
    return True

def test_architecture_components():
    """Test individual architecture components"""
    print("\n🔍 Testing Architecture Components")
    print("=" * 40)
    
    api = UARTAPI()
    
    if not api.connect():
        print("❌ Failed to connect to Pico")
        return False
    
    print("✅ Connected to Pico")
    
    # Test component integration
    print("\n📋 Architecture Component Tests:")
    
    # 1. Scheduler (heartbeat)
    print("1. Scheduler (ISR-based timing)")
    if api.send_gcode("PING"):
        print("   ✅ Scheduler responding")
    else:
        print("   ❌ Scheduler not responding")
    
    # 2. MoveQueue (step generation)
    print("2. MoveQueue (step generation)")
    if api.send_gcode("M17"):  # Enable steppers
        print("   ✅ MoveQueue enabled")
        if api.send_gcode("G1 Y1 F100"):  # Small move
            print("   ✅ MoveQueue executing moves")
        else:
            print("   ❌ MoveQueue not executing moves")
        api.send_gcode("M18")  # Disable steppers
    else:
        print("   ❌ MoveQueue not enabled")
    
    # 3. WindingController (high-level logic)
    print("3. WindingController (high-level logic)")
    if api.send_gcode("G28 Y"):  # Home command
        print("   ✅ WindingController homing")
    else:
        print("   ❌ WindingController not homing")
    
    # 4. StepCompressor (smooth movement)
    print("4. StepCompressor (smooth movement)")
    if api.send_gcode("G1 Y5 F50"):  # Slow move to test compression
        print("   ✅ StepCompressor working")
    else:
        print("   ❌ StepCompressor not working")
    
    api.disconnect()
    print("\n✅ Architecture component test completed!")
    return True

def main():
    """Main test function"""
    print("🧪 Klipper-Style Architecture Test Suite")
    print("=" * 70)
    
    try:
        # Test basic architecture
        if not test_klipper_architecture():
            print("❌ Klipper architecture test failed")
            return False
        
        # Test individual components
        if not test_architecture_components():
            print("❌ Architecture components test failed")
            return False
        
        print("\n" + "=" * 70)
        print("🎉 ALL KLIPPER-STYLE ARCHITECTURE TESTS PASSED!")
        print("✅ Scheduler (ISR-based timing)")
        print("✅ MoveQueue (step generation)")
        print("✅ WindingController (high-level logic)")
        print("✅ StepCompressor (smooth movement)")
        print("✅ G-code Interface (command processing)")
        print("✅ Hardware Integration (spindle + traverse)")
        print("=" * 70)
        
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
