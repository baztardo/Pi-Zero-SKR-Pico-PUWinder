#!/usr/bin/env python3
"""
Simple Command Line Interface for Pi Zero Winding Controller
Quick testing and control without web interface
"""

import sys
import time
from winding_controller import WindingController, WindingParameters

def print_status(controller):
    """Print current status"""
    progress = controller.get_progress()
    print(f"\n📊 Status: {progress['state'].upper()}")
    print(f"🔄 Turns: {progress['current_turns']}/{progress['target_turns']} ({progress['progress_percent']:.1f}%)")
    print(f"⚡ RPM: {progress['current_rpm']:.1f}")
    print(f"📦 Layer: {progress['current_layer']}")
    print(f"📍 Position: {progress['traverse_position']:.2f}mm")
    print(f"🔗 Connected: {controller.serial_conn is not None and controller.serial_conn.is_open}")

def main():
    print("🔄 Pi Zero Winding Controller - Simple Interface")
    print("=" * 50)
    
    # Initialize controller
    controller = WindingController()
    
    # Add status callback
    def status_callback(status_dict):
        # Update controller state from status
        if 'status' in status_dict:
            controller._parse_status_response(status_dict['status'])
        print_status(controller)
    
    controller.add_status_callback(status_callback)
    
    # Connect to Pico
    print("🔌 Connecting to Pico...")
    if not controller.connect():
        print("❌ Failed to connect to Pico. Check UART connection.")
        return
    
    print("✅ Connected to Pico!")
    print(f"📋 Firmware: {controller.get_version()}")
    
    # Start status monitoring
    controller.start_status_monitor()
    
    try:
        while True:
            print("\n" + "=" * 50)
            print("🎮 Available Commands:")
            print("  home     - Home all axes")
            print("  start    - Start winding with current settings")
            print("  pause    - Pause winding")
            print("  resume   - Resume winding")
            print("  stop     - Stop winding")
            print("  emergency - Emergency stop")
            print("  reset    - Reset emergency stop")
            print("  status   - Show current status")
            print("  settings - Change winding parameters")
            print("  test     - Test UART connection")
            print("  quit     - Exit program")
            print("=" * 50)
            
            cmd = input("> ").strip().lower()
            
            if cmd == "quit":
                break
            elif cmd == "home":
                print("🏠 Homing all axes...")
                if controller.home_all_axes():
                    print("✅ Homing completed")
                else:
                    print("❌ Homing failed")
            elif cmd == "start":
                print("🔄 Starting winding...")
                if controller.start_winding():
                    print("✅ Winding started")
                else:
                    print("❌ Failed to start winding")
            elif cmd == "pause":
                if controller.pause_winding():
                    print("⏸️ Winding paused")
                else:
                    print("❌ Failed to pause winding")
            elif cmd == "resume":
                if controller.resume_winding():
                    print("▶️ Winding resumed")
                else:
                    print("❌ Failed to resume winding")
            elif cmd == "stop":
                if controller.stop_winding():
                    print("⏹️ Winding stopped")
                else:
                    print("❌ Failed to stop winding")
            elif cmd == "emergency":
                if controller.emergency_stop():
                    print("🚨 EMERGENCY STOP ACTIVATED")
                else:
                    print("❌ Emergency stop failed")
            elif cmd == "reset":
                if controller.reset_emergency_stop():
                    print("🔄 Emergency stop reset")
                else:
                    print("❌ Failed to reset emergency stop")
            elif cmd == "status":
                print_status(controller)
            elif cmd == "settings":
                change_settings(controller)
            elif cmd == "test":
                test_connection(controller)
            else:
                print("❌ Unknown command")
                
    except KeyboardInterrupt:
        print("\n🛑 Shutting down...")
    finally:
        controller.stop_status_monitor()
        controller.disconnect()

def change_settings(controller):
    """Change winding parameters"""
    print("\n⚙️ Current Settings:")
    print(f"  Target Turns: {controller.params.target_turns}")
    print(f"  Spindle RPM: {controller.params.spindle_rpm}")
    print(f"  Wire Diameter: {controller.params.wire_diameter_mm}mm")
    print(f"  Bobbin Width: {controller.params.bobbin_width_mm}mm")
    print(f"  Start Offset: {controller.params.offset_mm}mm")
    
    print("\n📝 Enter new values (press Enter to keep current):")
    
    try:
        turns = input(f"Target Turns [{controller.params.target_turns}]: ").strip()
        if turns:
            controller.params.target_turns = int(turns)
        
        rpm = input(f"Spindle RPM [{controller.params.spindle_rpm}]: ").strip()
        if rpm:
            controller.params.spindle_rpm = float(rpm)
        
        wire_dia = input(f"Wire Diameter (mm) [{controller.params.wire_diameter_mm}]: ").strip()
        if wire_dia:
            controller.params.wire_diameter_mm = float(wire_dia)
        
        bobbin_width = input(f"Bobbin Width (mm) [{controller.params.bobbin_width_mm}]: ").strip()
        if bobbin_width:
            controller.params.bobbin_width_mm = float(bobbin_width)
        
        offset = input(f"Start Offset (mm) [{controller.params.offset_mm}]: ").strip()
        if offset:
            controller.params.offset_mm = float(offset)
        
        print("✅ Settings updated")
        
    except ValueError:
        print("❌ Invalid input. Settings not changed.")

def test_connection(controller):
    """Test UART connection with detailed diagnostics"""
    print("\n🔍 Testing UART Connection...")
    print("=" * 40)
    
    # Test basic connection
    print("1. Testing basic connection...")
    if controller.serial_conn and controller.serial_conn.is_open:
        print("   ✅ Serial port is open")
    else:
        print("   ❌ Serial port is not open")
        return
    
    # Test ping
    print("2. Testing PING command...")
    response = controller.send_command("PING")
    if response == "PONG":
        print("   ✅ PING successful")
    else:
        print(f"   ❌ PING failed: {response}")
        return
    
    # Test version
    print("3. Testing VERSION command...")
    version = controller.get_version()
    if version and version != "Unknown":
        print(f"   ✅ Version: {version}")
    else:
        print("   ❌ Version command failed")
    
    # Test status
    print("4. Testing STATUS command...")
    status = controller.get_status()
    if status:
        print(f"   ✅ Status: {status}")
    else:
        print("   ❌ Status command failed")
    
    print("\n✅ UART connection test completed")

if __name__ == "__main__":
    main()
