#!/usr/bin/env python3
"""
winding_controller.py - Pi Zero Winding Controller (FIXED)
Fixed: send_command() now properly returns None on errors instead of strings
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
    ramp_time_sec: float = 3.0

class WindingController:
    """High-level winding controller for Pi Zero"""
    
    def __init__(self, port: str = '/dev/serial0', baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.serial_conn: Optional[serial.Serial] = None
        self.connected = False
        self.running = False
        
        # Status tracking
        self.current_rpm = 0.0
        self.traverse_position = 0.0
        self.status_thread: Optional[threading.Thread] = None
        self.status_lock = threading.Lock()
        
    def connect(self) -> bool:
        """Connect to Pico via UART"""
        try:
            print(f"🔌 Connecting to Pico on {self.port} @ {self.baudrate} baud...")
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=2.0,
                write_timeout=2.0
            )
            
            time.sleep(2)  # Wait for Pico to initialize
            
            # Clear buffers
            self.serial_conn.reset_input_buffer()
            self.serial_conn.reset_output_buffer()
            
            # Send dummy message to stabilize connection
            self._send_dummy_message()
            
            # Test connection with PING
            response = self.send_command("PING", timeout=3.0)
            if response == "PONG":
                print("✅ Connected to Pico successfully!")
                self.connected = True
                return True
            else:
                print(f"❌ Connection failed - unexpected response: {response}")
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
    
    # =============================================================================
    # FIXED: send_command() - Now properly returns None on errors
    # =============================================================================
    def send_command(self, command: str, timeout: float = 2.0) -> Optional[str]:
        """
        Send command to Pico and return response
        
        Returns:
            str: Response from Pico on success
            None: On error or no response  ✅ FIXED: Was returning strings like "NO_RESPONSE"
        """
        if not self.serial_conn or not self.serial_conn.is_open:
            print("❌ Not connected to Pico")
            return None  # ✅ FIXED: Return None, not "ERROR"
            
        try:
            # Clear input buffer before sending
            self.serial_conn.reset_input_buffer()
            
            # Send command
            self.serial_conn.write(f"{command}\n".encode())
            self.serial_conn.flush()
            
            # Wait for UART stability
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
                return None  # ✅ FIXED: Return None, not "NO_RESPONSE"
            
            # Parse STATUS responses automatically
            if response.startswith("STATUS:"):
                self._parse_status_response(response)
                
            return response
            
        except Exception as e:
            print(f"❌ Command error: {e}")
            return None  # ✅ FIXED: Return None, not "ERROR"
    
    def _send_dummy_message(self):
        """Send dummy message to stabilize UART connection"""
        try:
            self.serial_conn.write(b"PING\n")
            self.serial_conn.flush()
            time.sleep(0.1)
            self.serial_conn.reset_input_buffer()
        except Exception as e:
            print(f"⚠️ Dummy message failed: {e}")
    
    def _parse_status_response(self, response: str):
        """Parse STATUS: Spindle=0.1RPM(RUN) Traverse=0.00mm"""
        try:
            with self.status_lock:
                if "Spindle=" in response:
                    rpm_part = response.split("Spindle=")[1].split("RPM")[0]
                    self.current_rpm = float(rpm_part)
                
                if "Traverse=" in response:
                    pos_part = response.split("Traverse=")[1].split("mm")[0]
                    self.traverse_position = float(pos_part)
        except Exception as e:
            print(f"⚠️ Status parse error: {e}")
    
    def get_version(self) -> Optional[str]:
        """Get firmware version"""
        response = self.send_command("VERSION")
        return response if response else "Unknown"
    
    def get_status(self) -> Optional[Dict[str, Any]]:
        """Get current machine status"""
        response = self.send_command("STATUS")
        if response:
            with self.status_lock:
                return {
                    'rpm': self.current_rpm,
                    'position': self.traverse_position,
                    'response': response
                }
        return None
    
    def home_all_axes(self) -> bool:
        """Home all axes"""
        response = self.send_command("G28", timeout=30.0)  # Homing takes time
        return response == "OK"
    
    def set_spindle_rpm(self, rpm: float, direction: str = 'CW') -> bool:
        """
        Set spindle speed and direction
        
        Args:
            rpm: Spindle RPM (0-3000)
            direction: 'CW' or 'CCW'
        """
        if rpm < 0 or rpm > 3000:
            print(f"❌ RPM {rpm} out of range (0-3000)")
            return False
        
        command = "M3" if direction == 'CW' else "M4"
        response = self.send_command(f"{command} S{rpm}")
        return response is not None and "OK" in response
    
    def stop_spindle(self) -> bool:
        """Stop spindle"""
        response = self.send_command("M5")
        return response == "OK"
    
    def move_traverse(self, position_mm: float, feed_rate: float = 1000.0) -> bool:
        """Move traverse to position"""
        response = self.send_command(f"G1 Y{position_mm} F{feed_rate}")
        return response == "OK"
    
    def emergency_stop(self) -> bool:
        """Emergency stop all motion"""
        response = self.send_command("M112")
        return response is not None
    
    def start_status_monitor(self):
        """Start background status monitoring"""
        if self.running:
            return
            
        self.running = True
        self.status_thread = threading.Thread(target=self._status_monitor_loop, daemon=True)
        self.status_thread.start()
        print("📊 Status monitor started")
    
    def stop_status_monitor(self):
        """Stop background status monitoring"""
        self.running = False
        if self.status_thread:
            self.status_thread.join(timeout=2)
        print("📊 Status monitor stopped")
    
    def _status_monitor_loop(self):
        """Background thread for status monitoring"""
        while self.running:
            try:
                self.get_status()
                time.sleep(1)  # Update every second
            except Exception as e:
                print(f"⚠️ Status monitor error: {e}")
                time.sleep(1)

# =============================================================================
# Main function for testing
# =============================================================================
def main():
    """Test the winding controller"""
    controller = WindingController()
    
    if not controller.connect():
        print("❌ Failed to connect to Pico")
        return
    
    try:
        # Get version
        version = controller.get_version()
        print(f"📌 Firmware: {version}")
        
        # Get status
        status = controller.get_status()
        print(f"📊 Status: {status}")
        
        # Home axes
        print("🏠 Homing axes...")
        if controller.home_all_axes():
            print("✅ Homing complete")
        else:
            print("❌ Homing failed")
        
        # Test spindle
        print("🔄 Testing spindle at 500 RPM CW...")
        if controller.set_spindle_rpm(500, 'CW'):
            print("✅ Spindle started")
            time.sleep(5)
            controller.stop_spindle()
            print("✅ Spindle stopped")
        else:
            print("❌ Spindle control failed")
            
    except KeyboardInterrupt:
        print("\n⚠️ Interrupted by user")
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        controller.disconnect()

if __name__ == "__main__":
    main()
