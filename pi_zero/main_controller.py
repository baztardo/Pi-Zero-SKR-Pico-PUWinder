#!/usr/bin/env python3
"""
Pi Zero Main Controller - The Brain of the Winding System
Purpose: High-level control, user interface, and coordination
"""

import time
import threading
from typing import Optional
from winding_controller import PiZeroWindingController, WindingParams, WindingState
from lcd_display import LCDDisplay

# =============================================================================
# Main Controller Class
# =============================================================================
class MainController:
    def __init__(self):
        # Initialize components
        self.winding_controller = PiZeroWindingController()
        self.lcd = LCDDisplay()
        
        # UI state
        self.running = False
        self.display_thread: Optional[threading.Thread] = None
        
        # Menu state
        self.menu_state = "MAIN"
        self.selected_param = 0
        
        # Default parameters
        self.params = WindingParams(
            target_turns=1000,
            spindle_rpm=300.0,
            wire_diameter_mm=0.064,
            layer_width_mm=50.0,
            start_position_mm=20.0,
            ramp_time_sec=3.0
        )
    
    def start(self):
        """Start the main controller"""
        print("🚀 Starting Pi Zero Main Controller")
        
        # Initialize LCD
        self.lcd.clear()
        self.lcd.write("Pi Zero Winding")
        self.lcd.write("Controller v1.0")
        self.lcd.write("Initializing...")
        self.lcd.write("Please wait")
        
        # Connect to Pico
        if not self.winding_controller.connect():
            self.lcd.display_error("Pico connection failed")
            return False
        
        # Set parameters
        self.winding_controller.set_parameters(self.params)
        
        # Start display thread
        self.running = True
        self.display_thread = threading.Thread(target=self._display_loop)
        self.display_thread.start()
        
        # Show main menu
        self._show_main_menu()
        
        return True
    
    def stop(self):
        """Stop the main controller"""
        print("🛑 Stopping Pi Zero Main Controller")
        
        self.running = False
        
        # Stop winding if running
        if self.winding_controller.state != WindingState.IDLE:
            self.winding_controller.stop()
        
        # Stop display thread
        if self.display_thread:
            self.display_thread.join()
        
        # Disconnect from Pico
        self.winding_controller.disconnect()
        
        # Clear LCD
        self.lcd.clear()
        self.lcd.write("System stopped")
        self.lcd.write("Goodbye!")
    
    def _display_loop(self):
        """Display update loop (runs in separate thread)"""
        while self.running:
            try:
                if self.winding_controller.state == WindingState.IDLE:
                    # Show main menu
                    self._show_main_menu()
                else:
                    # Show winding status
                    status = self.winding_controller.get_status()
                    self.lcd.display_winding_status(status)
                
                time.sleep(0.5)  # Update every 500ms
                
            except Exception as e:
                print(f"❌ Display loop error: {e}")
                self.lcd.display_error("Display error")
                time.sleep(1.0)
    
    def _show_main_menu(self):
        """Show main menu on LCD"""
        self.lcd.clear()
        self.lcd.write("Winding Controller")
        self.lcd.write(f"Turns: {self.params.target_turns}")
        self.lcd.write(f"RPM: {self.params.spindle_rpm}")
        self.lcd.write("Press START")
    
    def start_winding(self):
        """Start winding process"""
        if self.winding_controller.state != WindingState.IDLE:
            print("❌ Cannot start: winding already in progress")
            return False
        
        print("🚀 Starting winding process")
        
        # Update parameters
        self.winding_controller.set_parameters(self.params)
        
        # Start winding
        if self.winding_controller.start():
            print("✅ Winding process started")
            return True
        else:
            print("❌ Failed to start winding")
            return False
    
    def stop_winding(self):
        """Stop winding process"""
        if self.winding_controller.state == WindingState.IDLE:
            print("❌ No winding process to stop")
            return False
        
        print("🛑 Stopping winding process")
        self.winding_controller.stop()
        return True
    
    def emergency_stop(self):
        """Emergency stop"""
        print("🚨 EMERGENCY STOP!")
        self.winding_controller.emergency_stop()
        self.lcd.display_error("EMERGENCY STOP!")
    
    def set_parameter(self, param_name: str, value):
        """Set winding parameter"""
        if param_name == "target_turns":
            self.params.target_turns = int(value)
        elif param_name == "spindle_rpm":
            self.params.spindle_rpm = float(value)
        elif param_name == "wire_diameter_mm":
            self.params.wire_diameter_mm = float(value)
        elif param_name == "layer_width_mm":
            self.params.layer_width_mm = float(value)
        elif param_name == "start_position_mm":
            self.params.start_position_mm = float(value)
        elif param_name == "ramp_time_sec":
            self.params.ramp_time_sec = float(value)
        else:
            print(f"❌ Unknown parameter: {param_name}")
            return False
        
        # Recalculate derived parameters
        self.params.calculate_layers()
        print(f"✅ Set {param_name} = {value}")
        return True
    
    def get_status(self) -> dict:
        """Get system status"""
        return {
            'winding_status': self.winding_controller.get_status(),
            'parameters': {
                'target_turns': self.params.target_turns,
                'spindle_rpm': self.params.spindle_rpm,
                'wire_diameter_mm': self.params.wire_diameter_mm,
                'layer_width_mm': self.params.layer_width_mm,
                'start_position_mm': self.params.start_position_mm,
                'ramp_time_sec': self.params.ramp_time_sec,
                'turns_per_layer': self.params.turns_per_layer,
                'total_layers': self.params.total_layers
            }
        }

# =============================================================================
# Example Usage
# =============================================================================
if __name__ == "__main__":
    # Create main controller
    controller = MainController()
    
    try:
        # Start controller
        if not controller.start():
            print("❌ Failed to start controller")
            exit(1)
        
        print("✅ Pi Zero Main Controller started")
        print("Commands:")
        print("  start - Start winding process")
        print("  stop - Stop winding process")
        print("  emergency - Emergency stop")
        print("  status - Show status")
        print("  quit - Exit")
        
        # Simple command interface
        while True:
            try:
                command = input("> ").strip().lower()
                
                if command == "start":
                    controller.start_winding()
                
                elif command == "stop":
                    controller.stop_winding()
                
                elif command == "emergency":
                    controller.emergency_stop()
                
                elif command == "status":
                    status = controller.get_status()
                    print(f"Winding Status: {status['winding_status']}")
                    print(f"Parameters: {status['parameters']}")
                
                elif command == "quit":
                    break
                
                else:
                    print("Unknown command")
                
            except KeyboardInterrupt:
                print("\n⚠️  Interrupted by user")
                break
    
    except Exception as e:
        print(f"❌ Controller error: {e}")
    
    finally:
        controller.stop()
        print("✅ Controller stopped")
