#!/usr/bin/env python3
"""
G-code Compatible Main Controller for Pi Zero
Purpose: High-level control using G-code compatible architecture
"""

import time
import threading
import json
from typing import Dict, Any, Optional
from dataclasses import dataclass
from enum import Enum
from winding_sync import WindingSyncController
from uart_api import UARTAPI
import re

# =============================================================================
# G-code API Client (UART-based)
# =============================================================================
class GCodeAPI:
    def __init__(self, port='/dev/serial0', baudrate=115200):
        self.uart_api = UARTAPI(port=port, baudrate=baudrate)
        self.connected = False
    
    def connect(self) -> bool:
        """Connect to Pico via UART"""
        self.connected = self.uart_api.connect()
        return self.connected
    
    def disconnect(self):
        """Disconnect from Pico"""
        self.uart_api.disconnect()
        self.connected = False
    
    def get_machine_status(self) -> Optional[Dict[str, Any]]:
        """Get machine status from Pico using STATUS command"""
        if not self.connected:
            return None
        
        response = self.uart_api.send_command("STATUS")
        if response and response.startswith("STATUS:"):
            # Parse enhanced status format: "STATUS: Spindle=120.5RPM(RUN) Traverse=25.3mm"
            status_data = {}
            
            # Extract spindle info
            if "Spindle=" in response:
                spindle_part = response.split("Spindle=")[1].split()[0]
                if "RPM" in spindle_part:
                    rpm_str = spindle_part.split("RPM")[0]
                    status_data['spindle_rpm'] = float(rpm_str)
                if "RUN" in spindle_part:
                    status_data['spindle_running'] = True
                elif "STOP" in spindle_part:
                    status_data['spindle_running'] = False
            
            # Extract traverse position
            if "Traverse=" in response:
                traverse_part = response.split("Traverse=")[1].split()[0]
                if "mm" in traverse_part:
                    pos_str = traverse_part.split("mm")[0]
                    status_data['traverse_position'] = float(pos_str)
            
            return status_data
        return None
    
    def parse_gcode_command(self, command: str) -> dict:
        """Parse G-code command into components (from Code-snippets)"""
        command = command.strip().upper()
        
        # Extract command type
        if command.startswith('G'):
            cmd_type = 'G'
            cmd_num = command[1:].split()[0]
        elif command.startswith('M'):
            cmd_type = 'M'
            cmd_num = command[1:].split()[0]
        else:
            return {'type': 'UNKNOWN', 'command': command}
        
        # Extract parameters
        params = {}
        parts = command.split()
        for part in parts[1:]:
            if len(part) > 1 and part[0].isalpha():
                param_name = part[0]
                try:
                    param_value = float(part[1:])
                    params[param_name] = param_value
                except ValueError:
                    params[param_name] = part[1:]
        
        return {
            'type': cmd_type,
            'number': cmd_num,
            'command': command,
            'parameters': params
        }
    
    def send_gcode(self, gcode: str) -> bool:
        """Send G-code command to Pico with improved parsing"""
        if not self.connected:
            return False
        
        # Parse and validate command
        parsed = self.parse_gcode_command(gcode)
        if parsed['type'] == 'UNKNOWN':
            print(f"Unknown G-code command: {gcode}")
            return False
        
        # Send the command
        return self.uart_api.send_command(gcode) is not None
    
    def set_spindle_rpm(self, rpm: float, direction: str = 'CW') -> bool:
        """Set spindle RPM with direction (from Code-snippets improvement)"""
        # Clamp RPM to reasonable range
        rpm = max(0, min(3000, rpm))
        
        if rpm > 0:
            cmd = f"M{3 if direction.upper() == 'CW' else 4} S{rpm}"
        else:
            cmd = "M5"  # Stop spindle
        
        return self.send_gcode(cmd)
    
    def stop_spindle(self) -> bool:
        """Stop spindle using M5 command"""
        return self.send_gcode("M5")
    
    def move_traverse(self, position: float, feed_rate: float = 1000.0) -> bool:
        """Move traverse to position (from Code-snippets improvement)"""
        # Clamp feed rate to reasonable range
        feed_rate = max(10.0, min(5000.0, feed_rate))
        return self.send_gcode(f"G1 Y{position} F{feed_rate}")
    
    def home_traverse(self) -> bool:
        """Home traverse axis (from Code-snippets improvement)"""
        return self.send_gcode("G28 Y")
    
    def home_all_axes(self) -> bool:
        """Home all axes (from Code-snippets improvement)"""
        return self.send_gcode("G28")
    
    # Winding machine specific commands (from Code-snippets)
    def set_wire_diameter(self, diameter: float) -> bool:
        """Set wire diameter (M6 S command)"""
        return self.send_gcode(f"M6 S{diameter}")
    
    def enable_traverse_brake(self) -> bool:
        """Enable traverse brake (M10)"""
        return self.send_gcode("M10")
    
    def disable_traverse_brake(self) -> bool:
        """Disable traverse brake (M11)"""
        return self.send_gcode("M11")
    
    def enable_wire_tension(self) -> bool:
        """Enable wire tension (M12)"""
        return self.send_gcode("M12")
    
    def disable_wire_tension(self) -> bool:
        """Disable wire tension (M13)"""
        return self.send_gcode("M13")
    
    def enable_cooling(self) -> bool:
        """Enable cooling (M14)"""
        return self.send_gcode("M14")
    
    def disable_cooling(self) -> bool:
        """Disable cooling (M15)"""
        return self.send_gcode("M15")
    
    def emergency_stop(self) -> bool:
        """Emergency stop all systems (M16)"""
        return self.send_gcode("M16")
    
    def enable_steppers(self) -> bool:
        """Enable all steppers (M17)"""
        return self.send_gcode("M17")
    
    def disable_steppers(self) -> bool:
        """Disable all steppers (M18)"""
        return self.send_gcode("M18")
    
    def set_gpio_pin(self, pin: int, state: int) -> bool:
        """Set GPIO pin state (M42 P S)"""
        return self.send_gcode(f"M42 P{pin} S{state}")
    
    # ⭐ NEW: FluidNC-style Safety Commands
    def feed_hold(self) -> bool:
        """Feed hold (M0) - pause all motion"""
        return self.send_gcode("M0")
    
    def resume_from_hold(self) -> bool:
        """Resume from hold (M1) - resume motion"""
        return self.send_gcode("M1")
    
    def emergency_stop(self) -> bool:
        """Emergency stop (M112) - immediate stop all systems"""
        return self.send_gcode("M112")
    
    def quick_stop(self) -> bool:
        """Quick stop (M410) - stop new moves, finish current"""
        return self.send_gcode("M410")
    
    def reset_from_emergency(self) -> bool:
        """Reset from emergency stop (M999) - clear emergency state"""
        return self.send_gcode("M999")
    
    def dwell_with_sync(self, milliseconds: float = 0.0) -> bool:
        """Dwell with planner sync (G4 P) - wait for all moves to complete"""
        if milliseconds == 0.0:
            return self.send_gcode("G4 P0")  # Special case: sync planner
        else:
            return self.send_gcode(f"G4 P{milliseconds}")
    
    def set_spindle_direction_cw(self) -> bool:
        """Set spindle clockwise using M3"""
        return self.send_gcode("M3")
    
    def set_spindle_direction_ccw(self) -> bool:
        """Set spindle counter-clockwise using M4"""
        return self.send_gcode("M4")
    
    def set_spindle_speed(self, speed: float) -> bool:
        """Set spindle speed using S command"""
        return self.send_gcode(f"S{speed}")
    
    def enable_spindle(self) -> bool:
        """Enable spindle using M3"""
        return self.send_gcode("M3")
    
    def disable_spindle(self) -> bool:
        """Disable spindle using M5"""
        return self.send_gcode("M5")
    
    def start_spindle_cw(self, rpm: float) -> bool:
        """Start spindle clockwise at specified RPM"""
        return self.send_gcode(f"M3 S{rpm}")
    
    def start_spindle_ccw(self, rpm: float) -> bool:
        """Start spindle counter-clockwise at specified RPM"""
        return self.send_gcode(f"M4 S{rpm}")
    
    def set_spindle_brake(self, enable: bool) -> bool:
        """Set spindle brake"""
        if enable:
            return self.send_gcode("M12")  # Enable spindle brake
        else:
            return self.send_gcode("M13")  # Disable spindle brake
    
    def set_traverse_brake(self, enable: bool) -> bool:
        """Set traverse brake"""
        if enable:
            return self.send_gcode("M10")  # Enable traverse brake
        else:
            return self.send_gcode("M11")  # Disable traverse brake
    
    def set_wire_tension(self, enable: bool) -> bool:
        """Set wire tension"""
        if enable:
            return self.send_gcode("M14")  # Enable wire tension
        else:
            return self.send_gcode("M15")  # Disable wire tension
    
    def set_coolant(self, enable: bool) -> bool:
        """Set coolant (air blast)"""
        if enable:
            return self.send_gcode("M7")   # Coolant on
        else:
            return self.send_gcode("M8")   # Coolant off
    
    def wire_change(self) -> bool:
        """Wire change procedure"""
        return self.send_gcode("M6")  # Wire change
    
    def home_all_axes(self) -> bool:
        """Home all axes"""
        return self.send_gcode("M16")  # Home all axes
    
    def enable_steppers(self) -> bool:
        """Enable steppers"""
        return self.send_gcode("M17")  # Enable steppers
    
    def disable_steppers(self) -> bool:
        """Disable steppers"""
        return self.send_gcode("M18")  # Disable steppers
    
    def set_spindle_orientation(self, enable: bool) -> bool:
        """Set spindle orientation"""
        if enable:
            return self.send_gcode("M19")  # Enable spindle orientation
        else:
            return self.send_gcode("M5")   # Stop spindle
    
    def set_pin_state(self, pin: int, value: int) -> bool:
        """Set pin state"""
        return self.send_gcode(f"M42 P{pin} S{value}")
    
    def set_pin_value(self, pin: int, value: int) -> bool:
        """Set pin value"""
        return self.send_gcode(f"M47 P{pin} S{value}")
    
    def home_y_axis(self) -> bool:
        """Home Y-axis only (traverse)"""
        return self.send_gcode("G28 Y")
    
    def move_traverse(self, y: float, speed: float = None) -> bool:
        """Move traverse to Y position"""
        cmd = f"G1 Y{y}"
        if speed is not None:
            cmd += f" F{speed * 60}"  # Convert mm/s to mm/min
        
        return self.send_gcode(cmd)
    
    def sync_traverse_to_spindle(self, rpm: float, wire_diameter: float, target_y: float) -> bool:
        """Synchronize traverse movement with spindle RPM"""
        # Calculate traverse speed based on spindle RPM and wire diameter
        traverse_speed = (rpm * wire_diameter) / 60.0  # mm/s
        
        # Send synchronized movement command
        cmd = f"G1 Y{target_y} F{traverse_speed * 60}"  # Convert to mm/min
        return self.send_gcode(cmd)

# =============================================================================
# Winding States
# =============================================================================
class WindingState(Enum):
    IDLE = "IDLE"
    HOMING = "HOMING"
    RAMPING_UP = "RAMPING_UP"
    WINDING = "WINDING"
    RAMPING_DOWN = "RAMPING_DOWN"
    COMPLETE = "COMPLETE"
    ERROR = "ERROR"

# =============================================================================
# Winding Parameters
# =============================================================================
@dataclass
class WindingParams:
    target_turns: int = 1000
    spindle_rpm: float = 300.0
    wire_diameter_mm: float = 0.064  # 43 AWG
    layer_width_mm: float = 50.0
    start_position_mm: float = 20.0
    ramp_time_sec: float = 3.0
    
    # Calculated values
    wire_pitch_mm: float = 0.0
    turns_per_layer: int = 0
    total_layers: int = 0
    
    def __post_init__(self):
        self.calculate_layers()
    
    def calculate_layers(self):
        self.wire_pitch_mm = self.wire_diameter_mm  # Tight winding
        self.turns_per_layer = int(self.layer_width_mm / self.wire_pitch_mm)
        if self.turns_per_layer == 0:
            self.turns_per_layer = 1
        self.total_layers = (self.target_turns + self.turns_per_layer - 1) // self.turns_per_layer

# =============================================================================
# Controller Classes (from Code-snippets improvement)
# =============================================================================
class SpindleController:
    """Spindle control with PWM and feedback"""
    
    def __init__(self, uart_connection):
        self.uart = uart_connection
        self.current_rpm = 0
        self.target_rpm = 0
        self.is_running = False
        self.direction = 'CW'  # CW or CCW
    
    def set_rpm(self, rpm: float, direction: str = 'CW') -> bool:
        """Set spindle RPM and direction"""
        try:
            self.target_rpm = max(0, min(3000, rpm))  # Clamp to 0-3000 RPM
            self.direction = direction.upper()
            
            # Send command to Pico
            if self.target_rpm > 0:
                cmd = f"M{3 if direction.upper() == 'CW' else 4} S{self.target_rpm}\n"
                self.uart.write(cmd.encode())
                self.is_running = True
            else:
                self.stop()
            
            return True
            
        except Exception as e:
            print(f"Error setting RPM: {e}")
            return False
    
    def stop(self) -> bool:
        """Stop spindle"""
        try:
            self.uart.write("M5\n".encode())
            self.is_running = False
            self.current_rpm = 0
            self.target_rpm = 0
            return True
        except Exception as e:
            print(f"Error stopping spindle: {e}")
            return False
    
    def get_status(self) -> dict:
        """Get current spindle status"""
        return {
            'current_rpm': self.current_rpm,
            'target_rpm': self.target_rpm,
            'is_running': self.is_running,
            'direction': self.direction
        }

class TraverseController:
    """Traverse control with stepper motor"""
    
    def __init__(self, uart_connection):
        self.uart = uart_connection
        self.current_position = 0.0
        self.target_position = 0.0
        self.is_moving = False
        self.feed_rate = 1000.0  # mm/min
    
    def move_to(self, position: float, feed_rate: float = None) -> bool:
        """Move traverse to position"""
        try:
            self.target_position = position
            if feed_rate is not None:
                self.feed_rate = feed_rate
            
            # Send G-code command
            cmd = f"G1 Y{position} F{self.feed_rate}\n"
            self.uart.write(cmd.encode())
            self.is_moving = True
            
            return True
            
        except Exception as e:
            print(f"Error moving traverse: {e}")
            return False
    
    def home(self) -> bool:
        """Home traverse axis"""
        try:
            self.uart.write("G28 Y\n".encode())
            self.current_position = 0.0
            self.target_position = 0.0
            return True
        except Exception as e:
            print(f"Error homing traverse: {e}")
            return False
    
    def get_status(self) -> dict:
        """Get current traverse status"""
        return {
            'current_position': self.current_position,
            'target_position': self.target_position,
            'is_moving': self.is_moving,
            'feed_rate': self.feed_rate
        }

# =============================================================================
# G-code Compatible Winding Controller
# =============================================================================
class WindingController:
    def __init__(self):
        self.api = GCodeAPI()
        self.state = WindingState.IDLE
        self.params = WindingParams()
        
        # Progress tracking
        self.current_layer = 0
        self.turns_completed = 0
        self.turns_this_layer = 0
        self.current_rpm = 0.0
        self.current_traverse_position = 0.0  # Y-axis only
        
        # Timing
        self.ramp_started = False
        self.ramp_start_time = 0.0
        self.last_update_time = 0.0
        
        # Threading
        self.running = False
        self.control_thread: Optional[threading.Thread] = None
        
        # Synchronization controller
        self.sync_controller = WindingSyncController()
    
    def set_parameters(self, params: WindingParams):
        """Set winding parameters"""
        self.params = params
        
        # Configure synchronization controller
        self.sync_controller.set_parameters(
            wire_diameter_mm=params.wire_diameter_mm,
            spindle_rpm=params.spindle_rpm
        )
        
        print(f"Parameters set: {params.target_turns} turns, {params.spindle_rpm} RPM")
        print(f"  Turns per layer: {params.turns_per_layer}, Total layers: {params.total_layers}")
        print(f"  Wire diameter: {params.wire_diameter_mm:.3f} mm")
        print(f"  Sync speed: {self.sync_controller.sync_params.traverse_speed_mm_per_sec:.3f} mm/s")
    
    def start(self) -> bool:
        """Start winding process"""
        if self.state != WindingState.IDLE:
            print("❌ Cannot start: not idle")
            return False
        
        # Connect to Pico
        if not self.api.connect():
            print("❌ Failed to connect to Pico")
            return False
        
        print("🚀 Starting winding process")
        self.state = WindingState.HOMING
        self.current_layer = 0
        self.turns_completed = 0
        self.turns_this_layer = 0
        self.current_rpm = 0.0
        
        # Start control thread
        self.running = True
        self.control_thread = threading.Thread(target=self._control_loop)
        self.control_thread.start()
        
        return True
    
    def stop(self):
        """Stop winding process"""
        print("🛑 Stopping winding process")
        self.running = False
        self.state = WindingState.IDLE
        
        # Stop spindle
        self.api.stop_spindle()
        
        if self.control_thread:
            self.control_thread.join()
        
        # Disconnect from Pico
        self.api.disconnect()
    
    def emergency_stop(self):
        """Emergency stop using M112 command"""
        print("🚨 EMERGENCY STOP!")
        self.running = False
        self.state = WindingState.IDLE
        
        # Use M112 emergency stop command
        self.api.emergency_stop()
        
        if self.control_thread:
            self.control_thread.join()
    
    def reset_from_emergency(self) -> bool:
        """Reset from emergency stop using M999"""
        print("🔄 Resetting from emergency stop")
        return self.api.reset_from_emergency()
    
    def feed_hold(self) -> bool:
        """Feed hold using M0"""
        print("⏸️ Feed hold")
        return self.api.feed_hold()
    
    def resume_from_hold(self) -> bool:
        """Resume from hold using M1"""
        print("▶️ Resume from hold")
        return self.api.resume_from_hold()
    
    def quick_stop(self) -> bool:
        """Quick stop using M410"""
        print("⏹️ Quick stop")
        return self.api.quick_stop()
    
    def get_status(self) -> dict:
        """Get current status"""
        # Get synchronization status
        sync_status = self.sync_controller.get_sync_status()
        
        return {
            'state': self.state.value,
            'current_layer': self.current_layer,
            'turns_completed': self.turns_completed,
            'current_rpm': self.current_rpm,
            'progress_percent': (self.turns_completed / self.params.target_turns * 100) if self.params.target_turns > 0 else 0,
            'traverse_position': self.current_traverse_position,
            'wire_diameter': self.params.wire_diameter_mm,
            'turns_per_layer': self.params.turns_per_layer,
            'total_layers': self.params.total_layers,
            'sync_status': sync_status
        }
    
    def _control_loop(self):
        """Main control loop (runs in separate thread)"""
        print("🔄 Starting control loop")
        
        while self.running:
            try:
                self._update()
                time.sleep(0.1)  # 100ms update rate
                
            except Exception as e:
                print(f"❌ Control loop error: {e}")
                self.state = WindingState.ERROR
                break
        
        print("🔄 Control loop stopped")
    
    def _update(self):
        """Update winding process"""
        if not self.running:
            return
        
        if self.state == WindingState.IDLE:
            return
        
        elif self.state == WindingState.HOMING:
            self._home_axes()
        
        elif self.state == WindingState.RAMPING_UP:
            self._ramp_up_spindle()
        
        elif self.state == WindingState.WINDING:
            self._execute_winding()
        
        elif self.state == WindingState.RAMPING_DOWN:
            self._ramp_down_spindle()
        
        elif self.state == WindingState.COMPLETE:
            print("🎉 Winding complete!")
            self.running = False
    
    def _home_axes(self):
        """Home Y-axis (traverse) only"""
        print("🏠 Homing traverse (Y-axis)...")
        
        # Home Y-axis only
        if self.api.home_y_axis():
            print("✅ Traverse homed")
            self.current_traverse_position = 0.0
            self.state = WindingState.RAMPING_UP
        else:
            print("❌ Homing failed")
            self.state = WindingState.ERROR
    
    def _ramp_up_spindle(self):
        """Ramp up spindle to target RPM"""
        if not self.ramp_started:
            print(f"🚀 Starting spindle ramp up to {self.params.spindle_rpm} RPM over {self.params.ramp_time_sec} seconds")
            self.ramp_started = True
            self.ramp_start_time = time.time()
        
        # Calculate current target RPM based on ramp progress
        elapsed = time.time() - self.ramp_start_time
        ramp_progress = min(elapsed / self.params.ramp_time_sec, 1.0)
        current_target_rpm = self.params.spindle_rpm * ramp_progress
        
        # Set spindle speed using G-code
        if self.api.start_spindle_cw(current_target_rpm):
            self.current_rpm = current_target_rpm
            print(f"🔄 Spindle RPM: {self.current_rpm:.1f} (M3 S{self.current_rpm:.0f})")
        else:
            print("❌ Failed to set spindle RPM")
            self.state = WindingState.ERROR
            return
        
        if ramp_progress >= 1.0:
            print("✅ Spindle ramp up complete")
            self.state = WindingState.WINDING
            self.ramp_started = False
            
            # Start synchronization
            self.sync_controller.start_sync()
    
    def _execute_winding(self):
        """Execute winding process with spindle-traverse synchronization"""
        # Check if we've reached target turns
        if self.turns_completed >= self.params.target_turns:
            print("🎯 Target turns reached! Stopping spindle.")
            self.state = WindingState.RAMPING_DOWN
            return
        
        # Get current status from machine
        status = self.api.get_machine_status()
        if status:
            # Update RPM from machine status
            # This would be populated from the actual machine response
            pass
        
        # Update synchronization controller with current RPM
        self.sync_controller.update_spindle_rpm(self.current_rpm)
        
        # Simulate winding progress (in real implementation, this would be driven by encoder feedback)
        now = time.time()
        if now - self.last_update_time > 0.1:  # Update every 100ms
            self.turns_completed += 1
            self.turns_this_layer += 1
            self.last_update_time = now
            
            # CRITICAL: Get synchronized movement commands
            sync_commands = self.sync_controller.get_sync_commands()
            if sync_commands:
                # Execute synchronized movement
                if self.api.send_gcode(sync_commands['gcode']):
                    self.current_traverse_position = sync_commands['target_y']
                    self.sync_controller.update_traverse_position(self.current_traverse_position)
                else:
                    print("❌ Failed to execute sync movement")
                    self.state = WindingState.ERROR
                    return
            
            # Check if we need to move to next layer
            if self.turns_this_layer >= self.params.turns_per_layer:
                self.current_layer += 1
                self.turns_this_layer = 0
                print(f"📦 Starting layer {self.current_layer}/{self.params.total_layers}")
                
                # Calculate layer position using sync controller
                layer_position = self.sync_controller.calculate_layer_position(self.current_layer)
                print(f"📍 Moving to layer position: {layer_position:.3f} mm")
                
                # Rapid move to layer position (not synchronized)
                if self.api.move_traverse(y=layer_position, speed=10.0):
                    self.current_traverse_position = layer_position
                    self.sync_controller.update_traverse_position(layer_position)
                else:
                    print("❌ Failed to move traverse to layer")
                    self.state = WindingState.ERROR
                    return
            
            # Progress update
            if self.turns_completed % 100 == 0:
                progress = (self.turns_completed / self.params.target_turns * 100)
                print(f"📊 Progress: {self.turns_completed}/{self.params.target_turns} turns ({progress:.1f}%)")
                print(f"   Traverse position: {self.current_traverse_position:.2f} mm")
                print(f"   Spindle RPM: {self.current_rpm:.1f}")
    
    def _ramp_down_spindle(self):
        """Ramp down spindle"""
        print("🛑 Ramping down spindle...")
        
        # Stop synchronization
        self.sync_controller.stop_sync()
        
        # Calculate ramp down progress
        elapsed = time.time() - self.ramp_start_time
        ramp_progress = min(elapsed / self.params.ramp_time_sec, 1.0)
        target_rpm = self.params.spindle_rpm * (1.0 - ramp_progress)
        
        if target_rpm > 0:
            if self.api.start_spindle_cw(target_rpm):
                self.current_rpm = target_rpm
                print(f"🔄 Ramping down: {self.current_rpm:.1f} RPM (M3 S{self.current_rpm:.0f})")
            else:
                print("❌ Failed to set spindle RPM")
                self.state = WindingState.ERROR
                return
        else:
            if self.api.stop_spindle():
                print("✅ Spindle stopped (M5)")
                self.state = WindingState.COMPLETE
            else:
                print("❌ Failed to stop spindle")
                self.state = WindingState.ERROR

# =============================================================================
# Main Application
# =============================================================================
def main():
    """Main application"""
    print("🚀 Starting G-code Compatible Winding Controller")
    
    # Create winding controller
    controller = WindingController()
    
    try:
        # Set winding parameters
        params = WindingParams(
            target_turns=1000,
            spindle_rpm=300.0,
            wire_diameter_mm=0.064,
            layer_width_mm=50.0,
            start_position_mm=20.0,
            ramp_time_sec=3.0
        )
        controller.set_parameters(params)
        
        # Start winding process
        if controller.start():
            print("🚀 Winding process started")
            
            # Monitor progress
            while controller.running:
                status = controller.get_status()
                print(f"Status: {status['state']}, Progress: {status['progress_percent']:.1f}%")
                time.sleep(1.0)
            
            print("✅ Winding process completed")
        
    except KeyboardInterrupt:
        print("\n⚠️  Interrupted by user")
        controller.emergency_stop()
    
    except Exception as e:
        print(f"❌ Error: {e}")
        controller.emergency_stop()

if __name__ == "__main__":
    main()
