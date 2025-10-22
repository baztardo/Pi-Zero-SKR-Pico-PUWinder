#!/usr/bin/env python3
"""
Klipper-based Winding Controller for Pi Zero
Purpose: High-level control using Klipper's architecture and communication protocol
"""

import time
import threading
import json
import socket
from typing import Dict, Any, Optional
from dataclasses import dataclass
from enum import Enum

# =============================================================================
# Klipper Communication Protocol
# =============================================================================
class KlipperCommand:
    def __init__(self, cmd_id: int, command: str, params: Dict[str, Any] = None):
        self.cmd_id = cmd_id
        self.command = command
        self.params = params or {}
    
    def to_json(self) -> str:
        return json.dumps({
            'id': self.cmd_id,
            'method': self.command,
            'params': self.params
        })

class KlipperResponse:
    def __init__(self, response_data: str):
        try:
            data = json.loads(response_data)
            self.cmd_id = data.get('id', 0)
            self.result = data.get('result', {})
            self.error = data.get('error', None)
        except:
            self.cmd_id = 0
            self.result = {}
            self.error = "Invalid JSON response"

# =============================================================================
# Klipper Client for Pi Zero
# =============================================================================
class KlipperClient:
    def __init__(self, host='localhost', port=7125):
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.connected = False
        self.cmd_counter = 0
    
    def connect(self) -> bool:
        """Connect to Klipper host"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"✅ Connected to Klipper at {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect to Klipper: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from Klipper"""
        if self.socket:
            self.socket.close()
            self.connected = False
            print("✅ Disconnected from Klipper")
    
    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[KlipperResponse]:
        """Send command to Klipper"""
        if not self.connected:
            return None
        
        self.cmd_counter += 1
        cmd = KlipperCommand(self.cmd_counter, command, params)
        
        try:
            # Send command
            self.socket.send(cmd.to_json().encode() + b'\n')
            
            # Wait for response
            response_data = self.socket.recv(1024).decode().strip()
            return KlipperResponse(response_data)
            
        except Exception as e:
            print(f"❌ Command error: {e}")
            return None
    
    def get_printer_status(self) -> Optional[Dict[str, Any]]:
        """Get printer status from Klipper"""
        response = self.send_command("printer/objects/query", {
            "objects": {
                "toolhead": ["position", "extruder", "homed_axes"],
                "mcu": ["mcu_build_versions", "mcu_version"],
                "spindle": ["rpm", "target_rpm", "enabled"],
                "traverse": ["position", "homed_axes"]
            }
        })
        
        if response and not response.error:
            return response.result
        return None
    
    def set_spindle_rpm(self, rpm: float) -> bool:
        """Set spindle RPM"""
        response = self.send_command("printer/gcode/script", {
            "script": f"M3 S{rpm}"
        })
        return response and not response.error
    
    def stop_spindle(self) -> bool:
        """Stop spindle"""
        response = self.send_command("printer/gcode/script", {
            "script": "M5"
        })
        return response and not response.error
    
    def home_axes(self, axes: str = "XYZ") -> bool:
        """Home specified axes"""
        response = self.send_command("printer/gcode/script", {
            "script": f"G28 {axes}"
        })
        return response and not response.error
    
    def move_absolute(self, x: float = None, y: float = None, z: float = None, speed: float = None) -> bool:
        """Move to absolute position"""
        cmd = "G1"
        if x is not None:
            cmd += f" X{x}"
        if y is not None:
            cmd += f" Y{y}"
        if z is not None:
            cmd += f" Z{z}"
        if speed is not None:
            cmd += f" F{speed * 60}"  # Convert mm/s to mm/min
        
        response = self.send_command("printer/gcode/script", {
            "script": cmd
        })
        return response and not response.error

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
        self.klipper = KlipperClient()
        self.state = WindingState.IDLE
        self.params = WindingParams()
        
        # Progress tracking
        self.current_layer = 0
        self.turns_completed = 0
        self.turns_this_layer = 0
        self.current_rpm = 0.0
        self.current_position = {'x': 0.0, 'y': 0.0, 'z': 0.0}
        
        # Timing
        self.ramp_started = False
        self.ramp_start_time = 0.0
        self.last_update_time = 0.0
        
        # Threading
        self.running = False
        self.control_thread: Optional[threading.Thread] = None
    
    def connect(self) -> bool:
        """Connect to Klipper"""
        return self.klipper.connect()
    
    def disconnect(self):
        """Disconnect from Klipper"""
        self.klipper.disconnect()
    
    def set_parameters(self, params: WindingParams):
        """Set winding parameters"""
        self.params = params
        print(f"Parameters set: {params.target_turns} turns, {params.spindle_rpm} RPM")
        print(f"  Turns per layer: {params.turns_per_layer}, Total layers: {params.total_layers}")
    
    def start(self) -> bool:
        """Start winding process"""
        if self.state != WindingState.IDLE:
            print("❌ Cannot start: not idle")
            return False
        
        if not self.klipper.connected:
            print("❌ Not connected to Klipper")
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
        self.klipper.stop_spindle()
        
        if self.control_thread:
            self.control_thread.join()
    
    def emergency_stop(self):
        """Emergency stop"""
        print("🚨 EMERGENCY STOP!")
        self.running = False
        self.state = WindingState.IDLE
        
        # Emergency stop spindle
        self.klipper.stop_spindle()
        
        if self.control_thread:
            self.control_thread.join()
    
    def get_status(self) -> dict:
        """Get current status"""
        return {
            'state': self.state.value,
            'current_layer': self.current_layer,
            'turns_completed': self.turns_completed,
            'current_rpm': self.current_rpm,
            'progress_percent': (self.turns_completed / self.params.target_turns * 100) if self.params.target_turns > 0 else 0,
            'position': self.current_position
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
        """Home all axes"""
        print("🏠 Homing axes...")
        
        # Home all axes
        if self.klipper.home_axes("XYZ"):
            print("✅ Axes homed")
            self.current_position = {'x': 0.0, 'y': 0.0, 'z': 0.0}
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
        
        # Set spindle speed
        if self.klipper.set_spindle_rpm(current_target_rpm):
            self.current_rpm = current_target_rpm
        else:
            print("❌ Failed to set RPM")
            self.state = WindingState.ERROR
            return
        
        if ramp_progress >= 1.0:
            print("✅ Spindle ramp up complete")
            self.state = WindingState.WINDING
            self.ramp_started = False
    
    def _execute_winding(self):
        """Execute winding process"""
        # Check if we've reached target turns
        if self.turns_completed >= self.params.target_turns:
            print("🎯 Target turns reached! Stopping spindle.")
            self.state = WindingState.RAMPING_DOWN
            return
        
        # Get current status from Klipper
        status = self.klipper.get_printer_status()
        if status:
            # Update position
            if 'toolhead' in status and 'position' in status['toolhead']:
                pos = status['toolhead']['position']
                self.current_position = {'x': pos[0], 'y': pos[1], 'z': pos[2]}
            
            # Update RPM
            if 'spindle' in status and 'rpm' in status['spindle']:
                self.current_rpm = status['spindle']['rpm']
        
        # Simulate winding progress (in real implementation, this would be driven by encoder feedback)
        now = time.time()
        if now - self.last_update_time > 0.1:  # Update every 100ms
            self.turns_completed += 1
            self.turns_this_layer += 1
            self.last_update_time = now
            
            # Check if we need to move to next layer
            if self.turns_this_layer >= self.params.turns_per_layer:
                self.current_layer += 1
                self.turns_this_layer = 0
                print(f"📦 Starting layer {self.current_layer}/{self.params.total_layers}")
                
                # Move traverse to next layer position
                layer_position = self.params.start_position_mm + (self.current_layer * self.params.wire_diameter_mm)
                print(f"📍 Moving to layer position: {layer_position:.1f} mm")
                
                # Move traverse (Y-axis in this example)
                if self.klipper.move_absolute(y=layer_position, speed=10.0):
                    self.current_position['y'] = layer_position
                else:
                    print("❌ Failed to move traverse")
                    self.state = WindingState.ERROR
                    return
            
            # Progress update
            if self.turns_completed % 100 == 0:
                progress = (self.turns_completed / self.params.target_turns * 100)
                print(f"📊 Progress: {self.turns_completed}/{self.params.target_turns} turns ({progress:.1f}%)")
    
    def _ramp_down_spindle(self):
        """Ramp down spindle"""
        print("🛑 Ramping down spindle...")
        
        # Calculate ramp down progress
        elapsed = time.time() - self.ramp_start_time
        ramp_progress = min(elapsed / self.params.ramp_time_sec, 1.0)
        target_rpm = self.params.spindle_rpm * (1.0 - ramp_progress)
        
        if target_rpm > 0:
            if self.klipper.set_spindle_rpm(target_rpm):
                self.current_rpm = target_rpm
            else:
                print("❌ Failed to set RPM")
                self.state = WindingState.ERROR
                return
        else:
            if self.klipper.stop_spindle():
                print("✅ Spindle stopped")
                self.state = WindingState.COMPLETE
            else:
                print("❌ Failed to stop spindle")
                self.state = WindingState.ERROR

# =============================================================================
# Example Usage
# =============================================================================
if __name__ == "__main__":
    # Create winding controller
    controller = KlipperWindingController()
    
    # Connect to Klipper
    if not controller.connect():
        print("❌ Failed to connect to Klipper")
        exit(1)
    
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
    
    finally:
        controller.disconnect()
