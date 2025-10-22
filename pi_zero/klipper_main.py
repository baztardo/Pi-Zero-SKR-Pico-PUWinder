#!/usr/bin/env python3
"""
Klipper-based Main Controller for Pi Zero
Purpose: High-level control using Klipper's architecture and API
"""

import time
import threading
import json
import requests
from typing import Dict, Any, Optional
from dataclasses import dataclass
from enum import Enum
from winding_sync import WindingSyncController

# =============================================================================
# Klipper API Client
# =============================================================================
class KlipperAPI:
    def __init__(self, host='localhost', port=7125):
        self.base_url = f"http://{host}:{port}"
        self.session = requests.Session()
    
    def get_printer_status(self) -> Optional[Dict[str, Any]]:
        """Get printer status from Klipper API"""
        try:
            response = self.session.get(f"{self.base_url}/printer/objects/query", 
                                      params={"objects": "toolhead,spindle,traverse"})
            if response.status_code == 200:
                return response.json()
        except Exception as e:
            print(f"❌ API error: {e}")
        return None
    
    def send_gcode(self, gcode: str) -> bool:
        """Send G-code command to Klipper"""
        try:
            response = self.session.post(f"{self.base_url}/printer/gcode/script", 
                                       json={"script": gcode})
            return response.status_code == 200
        except Exception as e:
            print(f"❌ G-code error: {e}")
            return False
    
    def set_spindle_rpm(self, rpm: float) -> bool:
        """Set spindle RPM using M3 S command"""
        return self.send_gcode(f"M3 S{rpm}")
    
    def stop_spindle(self) -> bool:
        """Stop spindle using M5 command"""
        return self.send_gcode("M5")
    
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
# Klipper-based Winding Controller
# =============================================================================
class KlipperWindingController:
    def __init__(self):
        self.api = KlipperAPI()
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
    
    def emergency_stop(self):
        """Emergency stop"""
        print("🚨 EMERGENCY STOP!")
        self.running = False
        self.state = WindingState.IDLE
        
        # Emergency stop spindle
        self.api.stop_spindle()
        
        if self.control_thread:
            self.control_thread.join()
    
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
        
        # Get current status from Klipper
        status = self.api.get_printer_status()
        if status:
            # Update RPM from Klipper status
            # This would be populated from the actual Klipper response
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
    print("🚀 Starting Klipper-based Winding Controller")
    
    # Create winding controller
    controller = KlipperWindingController()
    
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
