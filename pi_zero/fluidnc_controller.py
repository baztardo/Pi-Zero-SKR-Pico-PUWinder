#!/usr/bin/env python3
"""
Advanced Command Controller
Purpose: High-level command controller with protocol management
"""

import time
import sys
from command_protocol import CommandProtocol, CommandStatus

class AdvancedController:
    """
    Advanced command controller with protocol management
    """
    
    def __init__(self):
        self.protocol = None
        self.connected = False
        
        # Machine state
        self.spindle_rpm = 0.0
        self.spindle_direction = "CW"
        self.traverse_position = 0.0
        self.emergency_stop = False
        self.feed_hold = False
        
        # Command sequences
        self.command_sequences = {
            'startup': ['PING', 'VERSION', 'STATUS', 'M17', 'G28'],
            'shutdown': ['M5', 'M18', 'M112'],
            'emergency': ['M112'],
            'reset': ['M999', 'M17']
        }
    
    def connect(self) -> bool:
        """Connect to Pico using command protocol"""
        devices = ['/dev/tty.usbmodem3144301', '/dev/tty.usbmodem31442402', '/dev/tty.usbmodem31442301']
        
        for device in devices:
            print(f"🔍 Trying to connect to {device}...")
            self.protocol = CommandProtocol(device)
            
            if self.protocol.connect():
                self.connected = True
                print(f"✅ Connected to Pico on {device}")
                return True
            else:
                print(f"❌ Could not connect to {device}")
        
        print("❌ Could not connect to any device")
        return False
    
    def disconnect(self):
        """Disconnect from Pico"""
        if self.protocol:
            self.protocol.disconnect()
        self.connected = False
        print("🔌 Disconnected from Pico")
    
    def send_command(self, command: str, timeout: float = 5.0) -> bool:
        """Send command with proper error handling"""
        if not self.connected:
            print("❌ Not connected to Pico")
            return False
        
        response = self.protocol.send_command(command, timeout)
        
        if response:
            print(f"✅ Command successful: {command}")
            return True
        else:
            print(f"❌ Command failed: {command}")
            return False
    
    def run_sequence(self, sequence_name: str) -> bool:
        """Run a predefined command sequence"""
        if sequence_name not in self.command_sequences:
            print(f"❌ Unknown sequence: {sequence_name}")
            return False
        
        commands = self.command_sequences[sequence_name]
        print(f"🎬 Running sequence '{sequence_name}': {commands}")
        
        success = True
        for i, command in enumerate(commands, 1):
            print(f"\n[{i}/{len(commands)}] {command}")
            if not self.send_command(command):
                success = False
                break
            time.sleep(0.5)
        
        if success:
            print(f"✅ Sequence '{sequence_name}' completed successfully")
        else:
            print(f"❌ Sequence '{sequence_name}' failed")
        
        return success
    
    def startup(self) -> bool:
        """Startup sequence"""
        print("🚀 Starting up machine...")
        return self.run_sequence('startup')
    
    def shutdown(self) -> bool:
        """Shutdown sequence"""
        print("🛑 Shutting down machine...")
        return self.run_sequence('shutdown')
    
    def emergency_stop(self) -> bool:
        """Emergency stop"""
        print("🚨 EMERGENCY STOP!")
        self.emergency_stop = True
        return self.send_command("M112", timeout=1.0)
    
    def reset_from_emergency(self) -> bool:
        """Reset from emergency stop"""
        print("🔄 Resetting from emergency stop...")
        self.emergency_stop = False
        return self.run_sequence('reset')
    
    def feed_hold(self) -> bool:
        """Feed hold"""
        print("⏸️  Feed hold...")
        self.feed_hold = True
        return self.send_command("M0")
    
    def resume_from_hold(self) -> bool:
        """Resume from feed hold"""
        print("▶️  Resuming from hold...")
        self.feed_hold = False
        return self.send_command("M1")
    
    def start_spindle(self, rpm: float, direction: str = "CW") -> bool:
        """Start spindle"""
        if self.emergency_stop:
            print("❌ Cannot start spindle - emergency stop active")
            return False
        
        if direction.upper() == "CW":
            command = f"M3 S{rpm}"
        else:
            command = f"M4 S{rpm}"
        
        if self.send_command(command):
            self.spindle_rpm = rpm
            self.spindle_direction = direction.upper()
            print(f"✅ Spindle started: {rpm} RPM {direction}")
            return True
        return False
    
    def stop_spindle(self) -> bool:
        """Stop spindle"""
        if self.send_command("M5"):
            self.spindle_rpm = 0.0
            print("✅ Spindle stopped")
            return True
        return False
    
    def move_traverse(self, position: float, feed_rate: float = 1000.0) -> bool:
        """Move traverse"""
        if self.emergency_stop:
            print("❌ Cannot move - emergency stop active")
            return False
        
        if self.feed_hold:
            print("❌ Cannot move - feed hold active")
            return False
        
        command = f"G1 Y{position} F{feed_rate}"
        if self.send_command(command):
            self.traverse_position = position
            print(f"✅ Traverse moved to {position}mm")
            return True
        return False
    
    def home_all_axes(self) -> bool:
        """Home all axes"""
        if self.emergency_stop:
            print("❌ Cannot home - emergency stop active")
            return False
        
        return self.send_command("G28")
    
    def set_wire_diameter(self, diameter: float) -> bool:
        """Set wire diameter"""
        command = f"M6 S{diameter}"
        return self.send_command(command)
    
    def enable_coolant(self, enable: bool = True) -> bool:
        """Enable/disable coolant"""
        command = "M7" if enable else "M8"
        return self.send_command(command)
    
    def enable_brake(self, enable: bool = True) -> bool:
        """Enable/disable traverse brake"""
        command = "M10" if enable else "M11"
        return self.send_command(command)
    
    def enable_tension(self, enable: bool = True) -> bool:
        """Enable/disable wire tension"""
        command = "M14" if enable else "M15"
        return self.send_command(command)
    
    def get_status(self) -> dict:
        """Get machine status"""
        if not self.connected:
            return {'connected': False}
        
        # Send status command
        response = self.protocol.send_command("STATUS", timeout=2.0)
        
        status = {
            'connected': self.connected,
            'emergency_stop': self.emergency_stop,
            'feed_hold': self.feed_hold,
            'spindle_rpm': self.spindle_rpm,
            'spindle_direction': self.spindle_direction,
            'traverse_position': self.traverse_position,
            'protocol_status': self.protocol.get_status() if self.protocol else {}
        }
        
        if response:
            status['raw_response'] = response
        
        return status
    
    def interactive_mode(self):
        """Interactive mode with advanced commands"""
        print("\n🎮 Advanced Command Controller")
        print("=" * 50)
        print("Available commands:")
        print("  startup, shutdown, emergency, reset")
        print("  feed_hold, resume, spindle_start, spindle_stop")
        print("  move, home, set_wire, coolant, brake, tension")
        print("  status, help, quit")
        print()
        
        while True:
            try:
                command = input("FluidNC> ").strip().lower()
                
                if command == 'quit':
                    break
                elif command == 'help':
                    self.show_help()
                elif command == 'startup':
                    self.startup()
                elif command == 'shutdown':
                    self.shutdown()
                elif command == 'emergency':
                    self.emergency_stop()
                elif command == 'reset':
                    self.reset_from_emergency()
                elif command == 'feed_hold':
                    self.feed_hold()
                elif command == 'resume':
                    self.resume_from_hold()
                elif command.startswith('spindle_start'):
                    parts = command.split()
                    rpm = float(parts[1]) if len(parts) > 1 else 500.0
                    direction = parts[2] if len(parts) > 2 else "CW"
                    self.start_spindle(rpm, direction)
                elif command == 'spindle_stop':
                    self.stop_spindle()
                elif command.startswith('move'):
                    parts = command.split()
                    position = float(parts[1]) if len(parts) > 1 else 50.0
                    feed_rate = float(parts[2]) if len(parts) > 2 else 1000.0
                    self.move_traverse(position, feed_rate)
                elif command == 'home':
                    self.home_all_axes()
                elif command.startswith('set_wire'):
                    parts = command.split()
                    diameter = float(parts[1]) if len(parts) > 1 else 0.064
                    self.set_wire_diameter(diameter)
                elif command.startswith('coolant'):
                    enable = 'on' in command
                    self.enable_coolant(enable)
                elif command.startswith('brake'):
                    enable = 'on' in command
                    self.enable_brake(enable)
                elif command.startswith('tension'):
                    enable = 'on' in command
                    self.enable_tension(enable)
                elif command == 'status':
                    status = self.get_status()
                    print(f"📊 Status: {status}")
                else:
                    print("❌ Unknown command. Type 'help' for available commands.")
                    
            except KeyboardInterrupt:
                print("\n⚠️  Interrupted by user")
                break
            except Exception as e:
                print(f"❌ Error: {e}")
        
        print("\n👋 Goodbye!")
    
    def show_help(self):
        """Show help information"""
        print("\n📋 Available Commands:")
        print("=" * 30)
        print("Machine Control:")
        print("  startup        - Start up machine")
        print("  shutdown       - Shutdown machine")
        print("  emergency      - Emergency stop")
        print("  reset          - Reset from emergency")
        print()
        print("Motion Control:")
        print("  feed_hold      - Feed hold")
        print("  resume         - Resume from hold")
        print("  move <pos> [feed] - Move traverse")
        print("  home           - Home all axes")
        print()
        print("Spindle Control:")
        print("  spindle_start <rpm> [direction] - Start spindle")
        print("  spindle_stop   - Stop spindle")
        print()
        print("Winding Control:")
        print("  set_wire <diameter> - Set wire diameter")
        print("  coolant on/off - Enable/disable coolant")
        print("  brake on/off   - Enable/disable brake")
        print("  tension on/off - Enable/disable tension")
        print()
        print("Status:")
        print("  status         - Get machine status")
        print("  help           - Show this help")
        print("  quit           - Exit controller")
        print()

def main():
    """Main function"""
    controller = AdvancedController()
    
    if len(sys.argv) > 1:
        # Command line mode
        if not controller.connect():
            sys.exit(1)
        
        if len(sys.argv) == 2:
            # Single command
            command = sys.argv[1]
            controller.send_command(command)
        else:
            # Multiple commands
            commands = sys.argv[1:]
            for cmd in commands:
                controller.send_command(cmd)
        
        controller.disconnect()
    else:
        # Interactive mode
        if not controller.connect():
            sys.exit(1)
        
        try:
            controller.interactive_mode()
        finally:
            controller.disconnect()

if __name__ == "__main__":
    main()
