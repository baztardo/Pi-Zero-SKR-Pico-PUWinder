#!/usr/bin/env python3
"""
Pi Zero Winding Controller
Controls Pico-based winding system via UART
"""

import serial
import time
import threading
import json
from typing import Dict, Optional, Callable
from dataclasses import dataclass
from enum import Enum

class WindingState(Enum):
    IDLE = "idle"
    HOMING = "homing"
    WINDING = "winding"
    PAUSED = "paused"
    COMPLETE = "complete"
    ERROR = "error"

@dataclass
class WindingParams:
    target_turns: int = 1000
    spindle_rpm: float = 300.0
    wire_diameter_mm: float = 0.064  # 43 AWG
    bobbin_width_mm: float = 12.0
    offset_mm: float = 22.0  # 20mm + 2mm bobbin thickness
    ramp_time_sec: float = 3.0

class WindingController:
    def __init__(self, port: str = "/dev/ttyUSB0", baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn: Optional[serial.Serial] = None
        self.state = WindingState.IDLE
        self.params = WindingParams()
        self.status_callbacks: list[Callable] = []
        self.running = False
        self.status_thread: Optional[threading.Thread] = None
        
        # Status tracking
        self.current_turns = 0
        self.current_rpm = 0.0
        self.current_layer = 0
        self.traverse_position = 0.0
        self.last_status_time = 0
        
    def connect(self) -> bool:
        """Connect to Pico via UART"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1.0,
                write_timeout=1.0
            )
            time.sleep(2)  # Wait for Pico to initialize
            
            # Clear any garbage data from UART buffer
            self.serial_conn.reset_input_buffer()
            self.serial_conn.reset_output_buffer()
            
            # Send dummy message to stabilize connection
            print("🔄 Stabilizing UART connection...")
            self._send_dummy_message()
            
            # Test connection with multiple ping attempts
            if self._test_connection():
                print(f"✅ Connected to Pico on {self.port}")
                self.start_status_monitor()
                return True
            else:
                print(f"❌ Failed to connect to Pico on {self.port}")
                return False
                
        except Exception as e:
            print(f"❌ Connection error: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from Pico"""
        self.running = False
        if self.status_thread:
            self.status_thread.join(timeout=2)
        if self.serial_conn:
            self.serial_conn.close()
            self.serial_conn = None
        print("🔌 Disconnected from Pico")
    
    def send_command(self, command: str, timeout: float = 2.0) -> Optional[str]:
        """Send command to Pico and return response"""
        if not self.serial_conn or not self.serial_conn.is_open:
            print("❌ Not connected to Pico")
            return None
            
        try:
            # Clear input buffer before sending
            self.serial_conn.reset_input_buffer()
            
            # Send command
            self.serial_conn.write(f"{command}\n".encode())
            self.serial_conn.flush()
            
            # Wait longer for UART stability
            time.sleep(0.3)
            
            # Read response with timeout
            start_time = time.time()
            response = ""
            
            while time.time() - start_time < timeout:
                if self.serial_conn.in_waiting > 0:
                    response = self.serial_conn.readline().decode().strip()
                    break
                time.sleep(0.01)
            
            if not response:
                print(f"⚠️ No response to command: {command}")
                return "NO_RESPONSE"  # Return string instead of None
                
            return response
            
        except Exception as e:
            print(f"❌ Command error: {e}")
            return "ERROR"  # Return string instead of None
    
    def _send_dummy_message(self):
        """Send dummy message to stabilize UART connection"""
        try:
            # Send a simple command that should always work
            self.serial_conn.write(b"PING\n")
            self.serial_conn.flush()
            time.sleep(0.3)  # Wait longer for UART to stabilize
            
            # Read and discard any response
            self.serial_conn.reset_input_buffer()
            time.sleep(0.1)
            
        except Exception as e:
            print(f"⚠️ Dummy message error (expected): {e}")
    
    def _test_connection(self) -> bool:
        """Test connection with multiple ping attempts"""
        for attempt in range(3):
            try:
                response = self.send_command("PING")
                if response == "PONG":
                    print(f"✅ Connection test successful (attempt {attempt + 1})")
                    return True
                else:
                    print(f"⚠️ Ping attempt {attempt + 1} failed: {response}")
                    time.sleep(0.5)
            except Exception as e:
                print(f"⚠️ Ping attempt {attempt + 1} error: {e}")
                time.sleep(0.5)
        
        return False
    
    def ping(self) -> bool:
        """Test connection to Pico"""
        response = self.send_command("PING")
        return response == "PONG"
    
    def get_version(self) -> str:
        """Get Pico firmware version"""
        response = self.send_command("VERSION")
        return response if response else "Unknown"
    
    def get_status(self) -> Dict:
        """Get current system status"""
        response = self.send_command("STATUS")
        if response:
            try:
                return json.loads(response)
            except:
                return {"status": response}
        return {"status": "unknown"}
    
    def home_all_axes(self) -> bool:
        """Home all axes (traverse and spindle)"""
        print("🏠 Homing all axes...")
        response = self.send_command("G28")
        if response and ("OK" in str(response) or "HOMED" in str(response)):
            self.state = WindingState.IDLE
            print("✅ All axes homed")
            return True
        else:
            print(f"❌ Homing failed: {response}")
            self.state = WindingState.ERROR
            return False
    
    def enable_motors(self) -> bool:
        """Enable all motors"""
        response = self.send_command("M17")
        return response and ("OK" in str(response) or "ENABLED" in str(response))
    
    def disable_motors(self) -> bool:
        """Disable all motors"""
        response = self.send_command("M18")
        return response and ("OK" in str(response) or "DISABLED" in str(response))
    
    def set_spindle_speed(self, rpm: float) -> bool:
        """Set spindle speed in RPM"""
        response = self.send_command(f"S{rpm}")
        return response and ("OK" in str(response) or "SET" in str(response))
    
    def start_spindle(self, direction: str = "CW") -> bool:
        """Start spindle rotation"""
        command = "M3" if direction.upper() == "CW" else "M4"
        response = self.send_command(command)
        return response and ("OK" in str(response) or "STARTED" in str(response))
    
    def stop_spindle(self) -> bool:
        """Stop spindle rotation"""
        response = self.send_command("M5")
        return response and ("OK" in str(response) or "STOPPED" in str(response))
    
    def start_winding(self, params: Optional[WindingParams] = None) -> bool:
        """Start winding sequence"""
        if params:
            self.params = params
        
        # Build command with parameters
        cmd = f"WIND T{self.params.target_turns} S{self.params.spindle_rpm} W{self.params.wire_diameter_mm} B{self.params.bobbin_width_mm} O{self.params.offset_mm}"
        
        print(f"🔄 Starting winding: {self.params.target_turns} turns, {self.params.spindle_rpm} RPM")
        response = self.send_command(cmd)
        
        if response and "WINDING_STARTED" in str(response):
            self.state = WindingState.WINDING
            print("✅ Winding started")
            return True
        else:
            print(f"❌ Failed to start winding: {response}")
            self.state = WindingState.ERROR
            return False
    
    def pause_winding(self) -> bool:
        """Pause winding process"""
        response = self.send_command("PAUSE_WIND")
        if response and "WINDING_PAUSED" in str(response):
            self.state = WindingState.PAUSED
            print("⏸️ Winding paused")
            return True
        return False
    
    def resume_winding(self) -> bool:
        """Resume winding process"""
        response = self.send_command("RESUME_WIND")
        if response and "WINDING_RESUMED" in str(response):
            self.state = WindingState.WINDING
            print("▶️ Winding resumed")
            return True
        return False
    
    def stop_winding(self) -> bool:
        """Stop winding process"""
        # Try multiple stop commands
        commands = ["STOP_WIND", "M5", "M112"]
        for cmd in commands:
            response = self.send_command(cmd)
            if response and ("WINDING_STOPPED" in str(response) or "STOPPED" in str(response) or "STOP" in str(response)):
                self.state = WindingState.IDLE
                print(f"⏹️ Winding stopped with {cmd}")
                return True
        print("❌ Failed to stop winding")
        return False
    
    def emergency_stop(self) -> bool:
        """Emergency stop all operations"""
        response = self.send_command("M112")
        if response and ("EMERGENCY_STOP" in str(response) or "STOPPED" in str(response)):
            self.state = WindingState.ERROR
            print("🚨 EMERGENCY STOP ACTIVATED")
            return True
        return False
    
    def reset_emergency_stop(self) -> bool:
        """Reset from emergency stop and recover from errors"""
        # Try multiple reset commands
        commands = ["M999", "M5", "STATUS"]
        for cmd in commands:
            response = self.send_command(cmd)
            if response and ("RESET" in str(response) or "OK" in str(response)):
                self.state = WindingState.IDLE
                print(f"🔄 System reset with {cmd}")
                return True
        
        # Force reset state even if commands fail
        self.state = WindingState.IDLE
        print("🔄 Forced system reset")
        return True
    
    def add_status_callback(self, callback: Callable):
        """Add callback for status updates"""
        self.status_callbacks.append(callback)
    
    def start_status_monitor(self):
        """Start background status monitoring"""
        self.running = True
        self.status_thread = threading.Thread(target=self._status_monitor_loop, daemon=True)
        self.status_thread.start()
        print("📊 Status monitoring started")
    
    def _status_monitor_loop(self):
        """Background status monitoring loop"""
        while self.running:
            try:
                # Get status from Pico
                status = self.get_status()
                
                # Update local status
                if "turns" in status:
                    self.current_turns = status.get("turns", 0)
                if "rpm" in status:
                    self.current_rpm = status.get("rpm", 0.0)
                if "layer" in status:
                    self.current_layer = status.get("layer", 0)
                if "position" in status:
                    self.traverse_position = status.get("position", 0.0)
                
                # Call status callbacks
                for callback in self.status_callbacks:
                    try:
                        callback(self)
                    except Exception as e:
                        print(f"❌ Status callback error: {e}")
                
                self.last_status_time = time.time()
                time.sleep(0.5)  # Update every 500ms
                
            except Exception as e:
                print(f"❌ Status monitor error: {e}")
                time.sleep(1.0)
    
    def get_progress(self) -> Dict:
        """Get winding progress information"""
        return {
            "state": self.state.value,
            "current_turns": self.current_turns,
            "target_turns": self.params.target_turns,
            "progress_percent": (self.current_turns / self.params.target_turns * 100) if self.params.target_turns > 0 else 0,
            "current_rpm": self.current_rpm,
            "current_layer": self.current_layer,
            "traverse_position": self.traverse_position,
            "last_update": self.last_status_time
        }

def main():
    """Test the winding controller"""
    controller = WindingController()
    
    def status_callback(ctrl):
        progress = ctrl.get_progress()
        print(f"📊 Status: {progress['state']} | Turns: {progress['current_turns']}/{progress['target_turns']} | RPM: {progress['current_rpm']:.1f}")
    
    controller.add_status_callback(status_callback)
    
    if not controller.connect():
        return
    
    try:
        print("🎮 Winding Controller Ready")
        print("Commands: start, pause, resume, stop, emergency, status, quit")
        
        while True:
            cmd = input("> ").strip().lower()
            
            if cmd == "quit":
                break
            elif cmd == "start":
                controller.start_winding()
            elif cmd == "pause":
                controller.pause_winding()
            elif cmd == "resume":
                controller.resume_winding()
            elif cmd == "stop":
                controller.stop_winding()
            elif cmd == "emergency":
                controller.emergency_stop()
            elif cmd == "status":
                print(json.dumps(controller.get_progress(), indent=2))
            elif cmd == "home":
                controller.home_all_axes()
            else:
                print("Unknown command")
                
    except KeyboardInterrupt:
        print("\n🛑 Shutting down...")
    finally:
        controller.disconnect()

if __name__ == "__main__":
    main()
