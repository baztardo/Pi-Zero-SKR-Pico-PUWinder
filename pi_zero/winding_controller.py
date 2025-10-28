#!/usr/bin/env python3
"""
winding_controller.py - FIXED VERSION
Handles UART communication with Pico firmware
"""

import serial
import time
import threading
from typing import Optional, Dict, Any
from dataclasses import dataclass

@dataclass
class WindingParameters:
    """Winding process parameters"""
    target_turns: int = 1000
    spindle_rpm: float = 300.0
    wire_diameter_mm: float = 0.064
    winding_width_mm: float = 50.0
    start_position_mm: float = 20.0

class WindingState:
    IDLE = "IDLE"
    WINDING = "WINDING"
    PAUSED = "PAUSED"
    ERROR = "ERROR"

class WindingController:
    def __init__(self, port: str = "/dev/serial0", baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn = None
        self.state = WindingState.IDLE
        self.params = WindingParameters()
        self.current_rpm = 0.0
        self.current_turns = 0
        self.traverse_position = 0.0
        self.connected = False
        
        # Status monitoring
        self.status_thread = None
        self.status_running = False
        self.status_callbacks = []

    def connect(self) -> bool:
        """Connect to Pico via UART"""
        try:
            self.serial_conn = serial.Serial(self.port, self.baudrate, timeout=2)
            time.sleep(0.5)  # Let it settle
            
            # Send dummy message to stabilize UART
            self._send_dummy_message()
            
            # Test connection
            if self._test_connection():
                self.connected = True
                print(f"✅ Connected to Pico on {self.port}")
                return True
            else:
                self.serial_conn.close()
                self.serial_conn = None
                return False
                
        except Exception as e:
            print(f"❌ Connection error: {e}")
            return False

    def disconnect(self):
        """Disconnect from Pico"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
        self.connected = False
        print("🔌 Disconnected from Pico")

    def _send_dummy_message(self):
        """Send dummy message to stabilize UART connection"""
        try:
            if self.serial_conn and self.serial_conn.is_open:
                self.serial_conn.write(b"PING\n")
                self.serial_conn.flush()
                time.sleep(0.3)
                self.serial_conn.reset_input_buffer()
        except:
            pass

    def _test_connection(self) -> bool:
        """Test connection with multiple ping attempts"""
        for attempt in range(3):
            try:
                response = self.send_command("PING")
                if response and "PONG" in str(response):
                    print(f"✅ Connection test successful (attempt {attempt + 1})")
                    return True
                else:
                    print(f"⚠️ Ping attempt {attempt + 1} failed: {response}")
                    time.sleep(0.5)
            except:
                time.sleep(0.5)
        
        print("❌ Failed to connect to Pico")
        return False

    def send_command(self, command: str, timeout: float = 2.0) -> Optional[str]:
        """Send command and get response"""
        if not self.serial_conn or not self.serial_conn.is_open:
            return None
        
        try:
            # Clear input buffer
            self.serial_conn.reset_input_buffer()
            
            # Send command
            self.serial_conn.write(f"{command}\n".encode())
            self.serial_conn.flush()
            
            # Wait for response
            time.sleep(0.3)
            
            # Read response with timeout
            start_time = time.time()
            response = ""
            
            while time.time() - start_time < timeout:
                if self.serial_conn.in_waiting > 0:
                    response = self.serial_conn.read(self.serial_conn.in_waiting).decode().strip()
                    break
                time.sleep(0.01)
            
            if not response:
                print(f"⚠️ No response to command: {command}")
                return None
                
            # Parse STATUS responses automatically
            if response.startswith("STATUS:"):
                self._parse_status_response(response)
                
            return response
            
        except Exception as e:
            print(f"❌ Command error: {e}")
            return None

    def _parse_status_response(self, response: str):
        """Parse STATUS: Spindle=0.1RPM(RUN) Traverse=0.00mm Turns=123"""
        try:
            if "Spindle=" in response:
                rpm_part = response.split("Spindle=")[1].split("RPM")[0]
                self.current_rpm = float(rpm_part)
            
            if "Traverse=" in response:
                pos_part = response.split("Traverse=")[1].split("mm")[0]
                self.traverse_position = float(pos_part)
            
            if "Turns=" in response:
                turns_part = response.split("Turns=")[1].split()[0]  # Get first word after Turns=
                self.current_turns = int(turns_part)
        except:
            pass

    def home_all_axes(self) -> bool:
        """Home all axes (traverse and spindle)"""
        print("🏠 Homing all axes...")
        
        # Stop spindle first if running
        self.send_command("M5")
        time.sleep(0.2)
        
        # Send home command
        response = self.send_command("G28")
        
        # Accept any response as success if we got one
        if response:
            self.state = WindingState.IDLE
            print("✅ Homing initiated")
            time.sleep(2)  # Give time to home
            return True
        else:
            print(f"❌ No response from Pico")
            self.state = WindingState.ERROR
            return False

    def start_winding(self, params: Optional[WindingParameters] = None) -> bool:
        """Start winding process with given parameters"""
        if params:
            self.params = params
        
        # Send simplified winding command
        cmd = f"WIND T{self.params.target_turns} S{self.params.spindle_rpm}"
        print(f"🔄 Starting winding: {self.params.target_turns} turns, {self.params.spindle_rpm} RPM")
        response = self.send_command(cmd)
        
        # Accept any response as success
        if response and str(response).strip():
            self.state = WindingState.WINDING
            print("✅ Winding started")
            return True
        else:
            print(f"❌ Failed to start winding: {response}")
            return False

    def stop_winding(self) -> bool:
        """Stop winding process"""
        print("⏹️ Stopping winding...")
        
        # ALWAYS stop spindle first
        self.send_command("M5")
        time.sleep(0.1)
        
        # Then stop winding
        self.send_command("STOP_WIND")
        
        # Force state
        self.state = WindingState.IDLE
        self.current_rpm = 0.0
        print("✅ Stopped")
        return True

    def emergency_stop(self) -> bool:
        """Emergency stop all operations"""
        print("🚨 Emergency stop...")
        
        # Send emergency stop
        self.send_command("M112")
        time.sleep(0.1)
        
        # Force stop spindle
        self.send_command("M5")
        
        # Force state reset
        self.state = WindingState.ERROR
        self.current_rpm = 0.0
        print("🚨 EMERGENCY STOP")
        return True

    def reset_emergency_stop(self) -> bool:
        """Reset from emergency stop and recover from errors"""
        print("🔄 Resetting emergency stop...")
        
        # Stop spindle first
        self.send_command("M5")
        time.sleep(0.1)
        
        # Send reset commands
        for cmd in ["M999", "STATUS"]:
            self.send_command(cmd)
            time.sleep(0.1)
        
        # FORCE reset state
        self.state = WindingState.IDLE
        self.current_turns = 0
        self.current_rpm = 0.0
        print("✅ Reset complete")
        return True

    def get_status(self) -> Dict[str, Any]:
        """Get current system status"""
        response = self.send_command("STATUS")
        if response:
            try:
                return {"status": response}
            except:
                return {"status": response}
        return {"status": "unknown"}

    def get_version(self) -> str:
        """Get firmware version"""
        response = self.send_command("VERSION")
        return response if response else "Unknown"

    def add_status_callback(self, callback):
        """Add status update callback"""
        self.status_callbacks.append(callback)

    def start_status_monitor(self):
        """Start status monitoring thread"""
        if self.status_running:
            return
        
        self.status_running = True
        self.status_thread = threading.Thread(target=self._status_monitor_loop, daemon=True)
        self.status_thread.start()
        print("📊 Status monitoring started")

    def _status_monitor_loop(self):
        """Status monitoring loop"""
        while self.status_running and self.connected:
            try:
                status = self.get_status()
                for callback in self.status_callbacks:
                    try:
                        callback(status)
                    except:
                        pass
                time.sleep(1)
            except:
                time.sleep(1)

    def stop_status_monitor(self):
        """Stop status monitoring"""
        self.status_running = False
        if self.status_thread:
            self.status_thread.join(timeout=1)

    def get_progress(self) -> Dict[str, Any]:
        """Get current winding progress"""
        progress_percent = 0.0
        if self.params.target_turns > 0:
            progress_percent = (self.current_turns / self.params.target_turns) * 100.0
        
        return {
            'state': self.state,
            'current_turns': self.current_turns,
            'target_turns': self.params.target_turns,
            'progress_percent': progress_percent,
            'current_rpm': self.current_rpm,
            'current_layer': 1,  # Placeholder - could be calculated based on turns
            'traverse_position': self.traverse_position
        }

    def pause_winding(self) -> bool:
        """Pause winding process"""
        print("⏸️ Pausing winding...")
        response = self.send_command("PAUSE_WIND")
        
        if response:
            self.state = WindingState.PAUSED
            print("✅ Winding paused")
            return True
        else:
            print(f"❌ Failed to pause winding: {response}")
            return False

    def resume_winding(self) -> bool:
        """Resume winding process"""
        print("▶️ Resuming winding...")
        response = self.send_command("RESUME_WIND")
        
        if response:
            self.state = WindingState.WINDING
            print("✅ Winding resumed")
            return True
        else:
            print(f"❌ Failed to resume winding: {response}")
            return False
