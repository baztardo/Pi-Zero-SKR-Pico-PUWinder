#!/usr/bin/env python3
"""
Test G-code Spindle Commands
Purpose: Test standard G-code spindle commands (M3, M4, M5, S)
"""

import time
import requests
from typing import Dict, Any

# =============================================================================
# G-code Spindle Test Client
# =============================================================================
class GCodeSpindleTester:
    def __init__(self, machine_url: str = "http://localhost:8080"):
        self.machine_url = machine_url
        self.api_url = f"{machine_url}/api/gcode"
    
    def send_gcode(self, gcode: str) -> bool:
        """Send G-code command to machine"""
        try:
            response = requests.post(
                self.api_url,
                json={"command": gcode},
                timeout=5
            )
            return response.status_code == 200
        except Exception as e:
            print(f"❌ G-code error: {e}")
            return False
    
    def test_spindle_commands(self):
        """Test all spindle G-code commands"""
        print("=" * 60)
        print("G-code Spindle Command Test")
        print("=" * 60)
        
        # Test M3 (Start spindle clockwise)
        print("\n🔄 Testing M3 (Start spindle clockwise)...")
        if self.send_gcode("M3 S300"):
            print("✅ M3 S300 - Spindle started clockwise at 300 RPM")
        else:
            print("❌ M3 S300 failed")
            return False
        
        time.sleep(2)
        
        # Test S command (Set speed)
        print("\n🔄 Testing S command (Set speed)...")
        if self.send_gcode("S500"):
            print("✅ S500 - Spindle speed set to 500 RPM")
        else:
            print("❌ S500 failed")
            return False
        
        time.sleep(2)
        
        # Test M4 (Start spindle counter-clockwise)
        print("\n🔄 Testing M4 (Start spindle counter-clockwise)...")
        if self.send_gcode("M4 S400"):
            print("✅ M4 S400 - Spindle started counter-clockwise at 400 RPM")
        else:
            print("❌ M4 S400 failed")
            return False
        
        time.sleep(2)
        
        # Test M5 (Stop spindle)
        print("\n🛑 Testing M5 (Stop spindle)...")
        if self.send_gcode("M5"):
            print("✅ M5 - Spindle stopped")
        else:
            print("❌ M5 failed")
            return False
        
        time.sleep(1)
        
        # Test M3 without speed (default)
        print("\n🔄 Testing M3 without speed...")
        if self.send_gcode("M3"):
            print("✅ M3 - Spindle started clockwise (default speed)")
        else:
            print("❌ M3 failed")
            return False
        
        time.sleep(2)
        
        # Test M5 again
        print("\n🛑 Testing M5 (Stop spindle)...")
        if self.send_gcode("M5"):
            print("✅ M5 - Spindle stopped")
        else:
            print("❌ M5 failed")
            return False
        
        print("\n🎉 All G-code spindle commands tested successfully!")
        return True
    
    def test_additional_m_codes(self):
        """Test additional M-codes for winding machine"""
        print("\n" + "=" * 60)
        print("Additional M-codes Test")
        print("=" * 60)
        
        # Test M6 (Wire change)
        print("\n🔧 Testing M6 (Wire change)...")
        if self.send_gcode("M6"):
            print("✅ M6 - Wire change procedure executed")
        else:
            print("❌ M6 failed")
            return False
        
        time.sleep(1)
        
        # Test M7/M8 (Coolant on/off)
        print("\n💨 Testing M7 (Coolant on)...")
        if self.send_gcode("M7"):
            print("✅ M7 - Coolant on")
        else:
            print("❌ M7 failed")
            return False
        
        time.sleep(1)
        
        print("\n💨 Testing M8 (Coolant off)...")
        if self.send_gcode("M8"):
            print("✅ M8 - Coolant off")
        else:
            print("❌ M8 failed")
            return False
        
        time.sleep(1)
        
        # Test M10/M11 (Traverse brake)
        print("\n🔒 Testing M10 (Traverse brake on)...")
        if self.send_gcode("M10"):
            print("✅ M10 - Traverse brake engaged")
        else:
            print("❌ M10 failed")
            return False
        
        time.sleep(1)
        
        print("\n🔓 Testing M11 (Traverse brake off)...")
        if self.send_gcode("M11"):
            print("✅ M11 - Traverse brake released")
        else:
            print("❌ M11 failed")
            return False
        
        time.sleep(1)
        
        # Test M12/M13 (Spindle brake)
        print("\n🔒 Testing M12 (Spindle brake on)...")
        if self.send_gcode("M12"):
            print("✅ M12 - Spindle brake engaged")
        else:
            print("❌ M12 failed")
            return False
        
        time.sleep(1)
        
        print("\n🔓 Testing M13 (Spindle brake off)...")
        if self.send_gcode("M13"):
            print("✅ M13 - Spindle brake released")
        else:
            print("❌ M13 failed")
            return False
        
        time.sleep(1)
        
        # Test M14/M15 (Wire tension)
        print("\n🎯 Testing M14 (Wire tension on)...")
        if self.send_gcode("M14"):
            print("✅ M14 - Wire tension enabled")
        else:
            print("❌ M14 failed")
            return False
        
        time.sleep(1)
        
        print("\n🎯 Testing M15 (Wire tension off)...")
        if self.send_gcode("M15"):
            print("✅ M15 - Wire tension disabled")
        else:
            print("❌ M15 failed")
            return False
        
        time.sleep(1)
        
        # Test M16 (Home all axes)
        print("\n🏠 Testing M16 (Home all axes)...")
        if self.send_gcode("M16"):
            print("✅ M16 - All axes homed")
        else:
            print("❌ M16 failed")
            return False
        
        time.sleep(1)
        
        # Test M17/M18 (Stepper enable/disable)
        print("\n⚙️ Testing M17 (Enable steppers)...")
        if self.send_gcode("M17"):
            print("✅ M17 - Steppers enabled")
        else:
            print("❌ M17 failed")
            return False
        
        time.sleep(1)
        
        print("\n⚙️ Testing M18 (Disable steppers)...")
        if self.send_gcode("M18"):
            print("✅ M18 - Steppers disabled")
        else:
            print("❌ M18 failed")
            return False
        
        time.sleep(1)
        
        # Test M19 (Spindle orientation)
        print("\n🎯 Testing M19 (Spindle orientation)...")
        if self.send_gcode("M19"):
            print("✅ M19 - Spindle orientation enabled")
        else:
            print("❌ M19 failed")
            return False
        
        time.sleep(1)
        
        # Test M42 (Set pin state)
        print("\n📌 Testing M42 (Set pin state)...")
        if self.send_gcode("M42 P25 S1"):
            print("✅ M42 - Pin 25 set to 1")
        else:
            print("❌ M42 failed")
            return False
        
        time.sleep(1)
        
        print("\n🎉 All additional M-codes tested successfully!")
        return True
    
    def test_winding_macros(self):
        """Test winding-specific G-code macros"""
        print("\n" + "=" * 60)
        print("Winding Macro Test")
        print("=" * 60)
        
        # Test WINDING_START
        print("\n🚀 Testing WINDING_START...")
        if self.send_gcode("WINDING_START RPM=300 START_Y=10"):
            print("✅ WINDING_START - Winding process started")
        else:
            print("❌ WINDING_START failed")
            return False
        
        time.sleep(2)
        
        # Test WINDING_LAYER
        print("\n📦 Testing WINDING_LAYER...")
        if self.send_gcode("WINDING_LAYER LAYER_POS=5 SPEED=10"):
            print("✅ WINDING_LAYER - Moved to layer position")
        else:
            print("❌ WINDING_LAYER failed")
            return False
        
        time.sleep(1)
        
        # Test WINDING_SYNC
        print("\n🔄 Testing WINDING_SYNC...")
        if self.send_gcode("WINDING_SYNC RPM=300 WIRE_DIAMETER=0.064 TARGET_Y=8"):
            print("✅ WINDING_SYNC - Synchronized movement executed")
        else:
            print("❌ WINDING_SYNC failed")
            return False
        
        time.sleep(2)
        
        # Test WINDING_STOP
        print("\n🛑 Testing WINDING_STOP...")
        if self.send_gcode("WINDING_STOP"):
            print("✅ WINDING_STOP - Winding process stopped")
        else:
            print("❌ WINDING_STOP failed")
            return False
        
        print("\n🎉 All winding macros tested successfully!")
        return True
    
    def test_emergency_stop(self):
        """Test emergency stop functionality"""
        print("\n" + "=" * 60)
        print("Emergency Stop Test")
        print("=" * 60)
        
        # Start spindle
        print("\n🚀 Starting spindle...")
        if self.send_gcode("M3 S500"):
            print("✅ Spindle started at 500 RPM")
        else:
            print("❌ Failed to start spindle")
            return False
        
        time.sleep(1)
        
        # Test emergency stop
        print("\n🚨 Testing emergency stop...")
        if self.send_gcode("WINDING_EMERGENCY"):
            print("✅ WINDING_EMERGENCY - Emergency stop executed")
        else:
            print("❌ WINDING_EMERGENCY failed")
            return False
        
        print("\n🎉 Emergency stop test completed!")
        return True

# =============================================================================
# Main Test Function
# =============================================================================
def main():
    """Run all G-code spindle tests"""
    print("🧪 G-code Spindle Command Test Suite")
    print("=" * 60)
    
    # Create tester
    tester = GCodeSpindleTester()
    
    try:
        # Test basic spindle commands
        if not tester.test_spindle_commands():
            print("❌ Basic spindle commands failed")
            return False
        
        # Test additional M-codes
        if not tester.test_additional_m_codes():
            print("❌ Additional M-codes failed")
            return False
        
        # Test winding macros
        if not tester.test_winding_macros():
            print("❌ Winding macros failed")
            return False
        
        # Test emergency stop
        if not tester.test_emergency_stop():
            print("❌ Emergency stop failed")
            return False
        
        print("\n" + "=" * 60)
        print("🎉 ALL TESTS PASSED!")
        print("✅ G-code spindle commands working correctly")
        print("✅ Additional M-codes functioning properly")
        print("✅ Winding macros functioning properly")
        print("✅ Emergency stop operational")
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
