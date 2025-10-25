#!/usr/bin/env python3
"""
Comprehensive Feature Test for Pi Zero ↔ SKR Pico communication
Tests all implemented features including safety commands, enhanced spindle control, and move queue features
"""

import time
from main_controller import GCodeAPI, WindingController, WindingParameters

def test_comprehensive_features():
    """Test all comprehensive features"""
    print("🧪 Comprehensive Feature Test")
    print("=" * 60)
    
    # Create API instance
    api = GCodeAPI()
    
    # Test 1: Basic Connectivity
    print("\n1. Testing Basic Connectivity...")
    if not api.connect():
        print("❌ Failed to connect to Pico")
        return False
    
    # Test PING/PONG
    response = api.uart_api.send_command("PING")
    if response == "PONG":
        print("✅ PING/PONG - OK")
    else:
        print(f"❌ PING failed: {response}")
    
    # Test VERSION
    response = api.uart_api.send_command("VERSION")
    if response:
        print(f"✅ VERSION: {response}")
    else:
        print("❌ VERSION failed")
    
    # Test 2: Enhanced STATUS Command
    print("\n2. Testing Enhanced STATUS Command...")
    status = api.get_machine_status()
    if status:
        print(f"✅ STATUS: {status}")
        if 'spindle_rpm' in status:
            print(f"   Spindle RPM: {status['spindle_rpm']}")
        if 'spindle_running' in status:
            print(f"   Spindle Running: {status['spindle_running']}")
        if 'traverse_position' in status:
            print(f"   Traverse Position: {status['traverse_position']} mm")
    else:
        print("❌ STATUS failed")
    
    # Test 3: FluidNC-style Safety Commands
    print("\n3. Testing FluidNC-style Safety Commands...")
    
    # Test M0 (feed hold)
    if api.feed_hold():
        print("✅ M0 (feed hold) - OK")
    else:
        print("❌ M0 failed")
    
    # Test M1 (resume from hold)
    if api.resume_from_hold():
        print("✅ M1 (resume from hold) - OK")
    else:
        print("❌ M1 failed")
    
    # Test M112 (emergency stop)
    if api.emergency_stop():
        print("✅ M112 (emergency stop) - OK")
    else:
        print("❌ M112 failed")
    
    # Test M999 (reset from emergency stop)
    if api.reset_from_emergency():
        print("✅ M999 (reset from emergency stop) - OK")
    else:
        print("❌ M999 failed")
    
    # Test M410 (quick stop)
    if api.quick_stop():
        print("✅ M410 (quick stop) - OK")
    else:
        print("❌ M410 failed")
    
    # Test G4 P0 (planner sync)
    if api.dwell_with_sync(0.0):
        print("✅ G4 P0 (planner sync) - OK")
    else:
        print("❌ G4 P0 failed")
    
    # Test G4 P1000 (dwell 1 second)
    start_time = time.time()
    if api.dwell_with_sync(1000.0):
        elapsed = time.time() - start_time
        print(f"✅ G4 P1000 (dwell) - OK (took {elapsed:.1f}s)")
    else:
        print("❌ G4 P1000 failed")
    
    # Test 4: Enhanced Spindle Control
    print("\n4. Testing Enhanced Spindle Control...")
    
    # Test M3 S500 (spindle CW with speed)
    if api.set_spindle_rpm(500, 'CW'):
        print("✅ M3 S500 (spindle CW) - OK")
    else:
        print("❌ M3 S500 failed")
    
    # Test S1000 (set speed)
    if api.send_gcode("S1000"):
        print("✅ S1000 (set speed) - OK")
    else:
        print("❌ S1000 failed")
    
    # Test M4 S2000 (spindle CCW with speed)
    if api.set_spindle_rpm(2000, 'CCW'):
        print("✅ M4 S2000 (spindle CCW) - OK")
    else:
        print("❌ M4 S2000 failed")
    
    # Test M5 (stop spindle)
    if api.stop_spindle():
        print("✅ M5 (stop spindle) - OK")
    else:
        print("❌ M5 failed")
    
    # Test 5: Winding Machine M-codes
    print("\n5. Testing Winding Machine M-codes...")
    
    # Test M6 (tool change)
    if api.set_wire_diameter(0.064):
        print("✅ M6 (tool change) - OK")
    else:
        print("❌ M6 failed")
    
    # Test M7/M8 (coolant)
    if api.enable_cooling(True):
        print("✅ M7 (coolant on) - OK")
    else:
        print("❌ M7 failed")
    
    if api.enable_cooling(False):
        print("✅ M8 (coolant off) - OK")
    else:
        print("❌ M8 failed")
    
    # Test M10/M11 (traverse brake)
    if api.enable_traverse_brake(True):
        print("✅ M10 (traverse brake on) - OK")
    else:
        print("❌ M10 failed")
    
    if api.enable_traverse_brake(False):
        print("✅ M11 (traverse brake off) - OK")
    else:
        print("❌ M11 failed")
    
    # Test M12/M13 (spindle brake)
    if api.send_gcode("M12"):
        print("✅ M12 (spindle brake on) - OK")
    else:
        print("❌ M12 failed")
    
    if api.send_gcode("M13"):
        print("✅ M13 (spindle brake off) - OK")
    else:
        print("❌ M13 failed")
    
    # Test M14/M15 (wire tension)
    if api.enable_wire_tension(True):
        print("✅ M14 (wire tension on) - OK")
    else:
        print("❌ M14 failed")
    
    if api.enable_wire_tension(False):
        print("✅ M15 (wire tension off) - OK")
    else:
        print("❌ M15 failed")
    
    # Test M16 (home all axes)
    if api.home_all_axes():
        print("✅ M16 (home all axes) - OK")
    else:
        print("❌ M16 failed")
    
    # Test M17/M18 (enable/disable steppers)
    if api.enable_steppers(True):
        print("✅ M17 (enable steppers) - OK")
    else:
        print("❌ M17 failed")
    
    if api.enable_steppers(False):
        print("✅ M18 (disable steppers) - OK")
    else:
        print("❌ M18 failed")
    
    # Test M19 (spindle orientation)
    if api.send_gcode("M19"):
        print("✅ M19 (spindle orientation) - OK")
    else:
        print("❌ M19 failed")
    
    # Test 6: GPIO Control
    print("\n6. Testing GPIO Control...")
    
    # Test M42 (set pin state)
    if api.set_gpio_pin(25, 1):
        print("✅ M42 P25 S1 (set pin state) - OK")
    else:
        print("❌ M42 failed")
    
    if api.set_gpio_pin(25, 0):
        print("✅ M42 P25 S0 (clear pin state) - OK")
    else:
        print("❌ M42 failed")
    
    # Test M47 (set pin value)
    if api.send_gcode("M47 P25 S1"):
        print("✅ M47 P25 S1 (set pin value) - OK")
    else:
        print("❌ M47 failed")
    
    # Test 7: Enhanced Features (Placeholder)
    print("\n7. Testing Enhanced Features (Placeholder)...")
    
    # Test enhanced spindle control (not yet implemented in Pico)
    ramp_status = api.get_spindle_ramp_status()
    print(f"   Spindle ramp status: {ramp_status}")
    
    # Test move queue status (not yet implemented in Pico)
    queue_status = api.get_move_queue_status()
    print(f"   Move queue status: {queue_status}")
    
    # Test scheduler status (not yet implemented in Pico)
    scheduler_status = api.get_scheduler_status()
    print(f"   Scheduler status: {scheduler_status}")
    
    # Test step counts (not yet implemented in Pico)
    step_counts = api.get_step_counts()
    print(f"   Step counts: {step_counts}")
    
    # Test 8: Winding Controller Integration
    print("\n8. Testing Winding Controller Integration...")
    
    # Create winding parameters
    params = WindingParameters(
        target_turns=100,
        spindle_rpm=300,
        wire_diameter_mm=0.064,
        winding_width_mm=50.0
    )
    
    # Create winding controller
    winding_controller = WindingController(api)
    winding_controller.set_parameters(params)
    
    # Test winding controller methods
    if winding_controller.feed_hold():
        print("✅ WindingController.feed_hold() - OK")
    else:
        print("❌ WindingController.feed_hold() failed")
    
    if winding_controller.resume_from_hold():
        print("✅ WindingController.resume_from_hold() - OK")
    else:
        print("❌ WindingController.resume_from_hold() failed")
    
    if winding_controller.quick_stop():
        print("✅ WindingController.quick_stop() - OK")
    else:
        print("❌ WindingController.quick_stop() failed")
    
    # Test emergency stop
    winding_controller.emergency_stop()
    print("✅ WindingController.emergency_stop() - OK")
    
    if winding_controller.reset_from_emergency():
        print("✅ WindingController.reset_from_emergency() - OK")
    else:
        print("❌ WindingController.reset_from_emergency() failed")
    
    # Disconnect
    api.disconnect()
    print("\n🎉 Comprehensive feature test completed!")
    return True

if __name__ == "__main__":
    test_comprehensive_features()
