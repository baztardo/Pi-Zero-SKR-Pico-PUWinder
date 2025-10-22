#!/usr/bin/env python3
"""
Pi Zero Winding Controller - Main Controller/Brain
Purpose: High-level winding process control and coordination
"""

import serial
import time
import threading
from enum import Enum
from dataclasses import dataclass
from typing import Optional

# =============================================================================
# Winding States
# =============================================================================
class WindingState(Enum):
    IDLE = "IDLE"
    HOMING_SPINDLE = "HOMING_SPINDLE"
    HOMING_TRAVERSE = "HOMING_TRAVERSE"
    MOVING_TO_START = "MOVING_TO_START"
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
# Pi Zero Winding Controller
# =============================================================================
class PiZeroWindingController:
    def __init__(self, serial_port='/dev/serial0', baud_rate=115200):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.ser: Optional[serial.Serial] = None
        
        # Winding state
        self.state = WindingState.IDLE
        self.params = WindingParams()
        
        # Progress tracking
        self.current_layer = 0
        self.turns_completed = 0
        self.turns_this_layer = 0
        self.current_rpm = 0.0
        self.current_traverse_position_mm = 0.0
        
        # Timing
        self.ramp_started = False
        self.ramp_start_time = 0.0
        self.last_update_time = 0.0
        
        # Threading
        self.running = False
        self.control_thread: Optional[threading.Thread] = None
        
    def connect(self) -> bool:
        """Connect to Pico via UART"""
        try:
            self.ser = serial.Serial(self.serial_port, self.baud_rate, timeout=1)
            time.sleep(0.1)
            
            # Test connection
            response = self.send_command("PING", timeout=2)
            if response != "PONG":
                print("❌ Failed to connect to Pico")
                return False
            
            print("✅ Connected to Pico")
            return True
            
        except serial.SerialException as e:
            print(f"❌ Serial error: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from Pico"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("✅ Disconnected from Pico")
    
    def send_command(self, command: str, timeout: float = 2.0) -> Optional[str]:
        """Send command to Pico and wait for response"""
        if not self.ser or not self.ser.is_open:
            return None
        
        try:
            # Send command
            self.ser.write(f"{command}\n".encode())
            
            # Wait for response
            start_time = time.time()
            while time.time() - start_time < timeout:
                if self.ser.in_waiting > 0:
                    response = self.ser.readline().decode('utf-8').strip()
                    return response
                time.sleep(0.01)
            
            return None
            
        except Exception as e:
            print(f"❌ Command error: {e}")
            return None
    
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
        
        if not self.ser or not self.ser.is_open:
            print("❌ Not connected to Pico")
            return False
        
        print("🚀 Starting winding process")
        self.state = WindingState.HOMING_SPINDLE
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
        self.send_command("STOP_SPINDLE")
        
        if self.control_thread:
            self.control_thread.join()
    
    def emergency_stop(self):
        """Emergency stop"""
        print("🚨 EMERGENCY STOP!")
        self.running = False
        self.state = WindingState.IDLE
        
        # Emergency stop spindle
        self.send_command("STOP_SPINDLE")
        
        if self.control_thread:
            self.control_thread.join()
    
    def get_status(self) -> dict:
        """Get current status"""
        return {
            'state': self.state.value,
            'current_layer': self.current_layer,
            'turns_completed': self.turns_completed,
            'current_rpm': self.current_rpm,
            'progress_percent': (self.turns_completed / self.params.target_turns * 100) if self.params.target_turns > 0 else 0
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
        
        elif self.state == WindingState.HOMING_SPINDLE:
            self._home_spindle()
        
        elif self.state == WindingState.HOMING_TRAVERSE:
            self._home_traverse()
        
        elif self.state == WindingState.MOVING_TO_START:
            self._move_to_start()
        
        elif self.state == WindingState.RAMPING_UP:
            self._ramp_up_spindle()
        
        elif self.state == WindingState.WINDING:
            self._execute_winding()
        
        elif self.state == WindingState.RAMPING_DOWN:
            self._ramp_down_spindle()
        
        elif self.state == WindingState.COMPLETE:
            print("🎉 Winding complete!")
            self.running = False
    
    def _home_spindle(self):
        """Home spindle to Z index"""
        print("🏠 Homing spindle...")
        
        # Start spindle at slow speed for homing
        response = self.send_command("SET_SPINDLE_RPM 50")
        if response != "OK":
            print(f"❌ Failed to start spindle: {response}")
            self.state = WindingState.ERROR
            return
        
        # Simulate homing (in real implementation, wait for Z-index)
        time.sleep(2.0)  # 2 seconds for homing
        
        print("✅ Spindle homed")
        self.state = WindingState.HOMING_TRAVERSE
    
    def _home_traverse(self):
        """Home traverse axis"""
        print("🏠 Homing traverse...")
        
        # TODO: Implement traverse homing
        # For now, just simulate
        time.sleep(1.0)
        
        print("✅ Traverse homed")
        self.current_traverse_position_mm = 0.0
        self.state = WindingState.MOVING_TO_START
    
    def _move_to_start(self):
        """Move traverse to start position"""
        print(f"📍 Moving to start position: {self.params.start_position_mm} mm")
        
        # TODO: Implement traverse movement
        # For now, just simulate
        time.sleep(0.5)
        
        self.current_traverse_position_mm = self.params.start_position_mm
        self.state = WindingState.RAMPING_UP
    
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
        response = self.send_command(f"SET_SPINDLE_RPM {current_target_rpm:.1f}")
        if response != "OK":
            print(f"❌ Failed to set RPM: {response}")
            self.state = WindingState.ERROR
            return
        
        self.current_rpm = current_target_rpm
        
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
        
        # Update RPM from Pico
        response = self.send_command("GET_SPINDLE_RPM")
        if response and response.startswith("RPM:"):
            try:
                self.current_rpm = float(response.split(":")[1])
            except:
                pass
        
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
                
                # TODO: Move traverse to next layer position
                layer_position = self.params.start_position_mm + (self.current_layer * self.params.wire_diameter_mm)
                print(f"📍 Moving to layer position: {layer_position:.1f} mm")
            
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
            response = self.send_command(f"SET_SPINDLE_RPM {target_rpm:.1f}")
            if response == "OK":
                self.current_rpm = target_rpm
        else:
            response = self.send_command("STOP_SPINDLE")
            if response == "STOPPED":
                print("✅ Spindle stopped")
                self.state = WindingState.COMPLETE

# =============================================================================
# Example Usage
# =============================================================================
if __name__ == "__main__":
    # Create winding controller
    controller = PiZeroWindingController()
    
    # Connect to Pico
    if not controller.connect():
        print("❌ Failed to connect to Pico")
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
