#!/usr/bin/env python3
"""
Pico Controller - Easy G-code Command Interface
Purpose: Simple interface to send G-code commands to Pico
Usage: python3 pico_controller.py
"""

import serial
import time
import sys
from typing import Optional, List

class PicoController:
    def __init__(self):
        self.ser = None
        self.connected = False
        self.device_path = None
        
    def connect(self) -> bool:
        """Connect to Pico"""
        devices = ['/dev/tty.usbmodem31442301', '/dev/tty.usbmodem3144301', '/dev/tty.usbmodem31442402']
        
        for device in devices:
            try:
                print(f"🔍 Trying to connect to {device}...")
                self.ser = serial.Serial(device, 115200, timeout=2)
                time.sleep(0.5)
                
                # Test connection with PING
                self.ser.write(b'PING\n')
                time.sleep(0.5)
                
                response = self.ser.read(100)
                decoded = response.decode('utf-8', errors='ignore').strip()
                
                if decoded and 'PONG' in decoded:
                    print(f"✅ Connected to Pico on {device}")
                    self.connected = True
                    self.device_path = device
                    return True
                elif decoded and 'Status' in decoded:
                    print(f"✅ Found Pico on {device} (status reporting)")
                    self.connected = True
                    self.device_path = device
                    return True
                else:
                    print(f"📥 Response: {decoded}")
                    if decoded:
                        print(f"✅ Connected to device on {device}")
                        self.connected = True
                        self.device_path = device
                        return True
                    else:
                        print(f"❌ No response from {device}")
                        self.ser.close()
                        
            except Exception as e:
                print(f"❌ Error connecting to {device}: {e}")
                continue
        
        print("❌ Could not connect to Pico")
        return False
    
    def disconnect(self):
        """Disconnect from Pico"""
        if self.ser:
            self.ser.close()
            self.connected = False
            print("🔌 Disconnected from Pico")
    
    def send_command(self, command: str) -> Optional[str]:
        """Send a command to Pico and return response"""
        if not self.connected:
            print("❌ Not connected to Pico")
            return None
        
        try:
            print(f"📤 Sending: {command}")
            self.ser.write(f"{command}\n".encode())
            time.sleep(0.5)
            
            response = self.ser.read(200)
            decoded = response.decode('utf-8', errors='ignore').strip()
            
            if decoded:
                print(f"📥 Response: {decoded}")
                return decoded
            else:
                print("📥 No response")
                return None
                
        except Exception as e:
            print(f"❌ Error sending command: {e}")
            return None
    
    def interactive_mode(self):
        """Interactive command mode"""
        print("\n🎮 Interactive Pico Controller")
        print("=" * 40)
        print("Type G-code commands or 'help' for available commands")
        print("Type 'quit' to exit")
        print()
        
        while True:
            try:
                command = input("Pico> ").strip()
                
                if command.lower() == 'quit':
                    break
                elif command.lower() == 'help':
                    self.show_help()
                elif command.lower() == 'status':
                    self.send_command('STATUS')
                elif command.lower() == 'ping':
                    self.send_command('PING')
                elif command.lower() == 'version':
                    self.send_command('VERSION')
                elif command.lower() == 'home':
                    self.send_command('G28')
                elif command.lower() == 'stop':
                    self.send_command('M5')
                elif command.lower() == 'emergency':
                    self.send_command('M112')
                elif command.lower() == 'reset':
                    self.send_command('M999')
                elif command:
                    self.send_command(command)
                else:
                    print("Enter a command or 'help'")
                    
            except KeyboardInterrupt:
                print("\n⚠️  Interrupted by user")
                break
            except EOFError:
                break
        
        print("\n👋 Goodbye!")
    
    def show_help(self):
        """Show available commands"""
        print("\n📋 Available Commands:")
        print("=" * 30)
        print("Basic Commands:")
        print("  ping          - Test connection")
        print("  version       - Get firmware version")
        print("  status        - Get machine status")
        print("  home          - Home all axes (G28)")
        print("  stop          - Stop spindle (M5)")
        print("  emergency     - Emergency stop (M112)")
        print("  reset         - Reset from emergency (M999)")
        print()
        print("G-code Commands:")
        print("  M3 S500       - Start spindle CW at 500 RPM")
        print("  M4 S1000      - Start spindle CCW at 1000 RPM")
        print("  M5            - Stop spindle")
        print("  G1 Y50 F1000  - Move traverse to 50mm at 1000mm/min")
        print("  G0 Y100       - Rapid move to 100mm")
        print("  G4 P1000      - Dwell for 1000ms")
        print()
        print("Winding Commands:")
        print("  M6 S0.064     - Set wire diameter")
        print("  M7            - Coolant on")
        print("  M8            - Coolant off")
        print("  M10           - Traverse brake on")
        print("  M11           - Traverse brake off")
        print("  M12           - Spindle brake on")
        print("  M13           - Spindle brake off")
        print("  M14           - Wire tension on")
        print("  M15           - Wire tension off")
        print("  M16           - Home all axes")
        print("  M17           - Enable steppers")
        print("  M18           - Disable steppers")
        print()
        print("GPIO Commands:")
        print("  M42 P25 S1    - Set GPIO 25 HIGH")
        print("  M42 P25 S0    - Set GPIO 25 LOW")
        print()
        print("Control Commands:")
        print("  help          - Show this help")
        print("  quit          - Exit controller")
        print()
    
    def run_sequence(self, commands: List[str]):
        """Run a sequence of commands"""
        print(f"🎬 Running sequence of {len(commands)} commands...")
        
        for i, command in enumerate(commands, 1):
            print(f"\n[{i}/{len(commands)}] {command}")
            response = self.send_command(command)
            if response is None:
                print(f"❌ Command failed: {command}")
                return False
            time.sleep(1)  # Wait between commands
        
        print("✅ Sequence completed!")
        return True

def main():
    """Main function"""
    controller = PicoController()
    
    # Check if command line arguments provided
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
            controller.run_sequence(commands)
        
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
