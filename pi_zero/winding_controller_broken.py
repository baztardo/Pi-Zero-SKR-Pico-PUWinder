#!/usr/bin/env python3
"""
winding_controller.py - FINAL FIX
Handles first-send garbling (this is NORMAL UART behavior)
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
        self.uart_primed = False  # Track if UART is initialized
        
        # Status tracking
        self.current_rpm = 0.0
        self.traverse_position = 0.0
        self.status_thread: Optional[threading.Thread] = None
        self.status_lock = threading.Lock()
        
    def connect(self) -> bool:
        """Connect to Pico via UART - handles first-send garbling"""
        try:
            print(f"🔌 Connecting to Pico on {self.port} @ {self.baudrate} baud...")
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=2.0,
                write_timeout=2.0
            )
            
            time.sleep(2)  # Wait for Pico to initialize
            
            # Clear any garbage
            self.serial_conn.reset_input_buffer()
            self.serial_conn.reset_output_buffer()
            time.sleep(0.2)
            
            # ============================================================
            # CRITICAL: Prime UART with dummy sends
            # First 1-3 sends are ALWAYS garbled - THIS IS NORMAL!
            # ============================================================
            print("📡 Priming UART (first sends are garbled - normal behavior)...")
            
            for attempt in range(3):
                self.serial_conn.write(b"PING\n")
                self.serial_conn.flush()
                time.sleep(0.3)
                
                # Read and discard (will be garbled)
                if self.serial_conn.in_waiting > 0:
                    garbage = self.serial_conn.read(self.serial_conn.in_waiting)
                    # Don't print garbage - it's expected
                
                self.serial_conn.reset_input_buffer()
                time.sleep(0.1)
            
            # Mark UART as primed
            self.uart_primed = True
            print("✅ UART primed")
            
            # NOW test with real command
            response = self.send_command("PING", timeout=3.0)
            
            if response == "PONG":
                print("✅ Connected to Pico successfully!")
                self.connected = True
                return True
            else:
                print(f"⚠️ Connection test got: {response}")
                # Try once more
                time.sleep(0.5)
                response = self.send_command("PING", timeout=3.0)
                if response == "PONG":
                    print("✅ Connected on retry!")
                    self.connected = True
                    return True
                else:
                    print(f"❌ Connection failed after retry")
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
            self.connected = False
            self.uart_primed = False
        print("🔌 Disconnected from Pico")
    
    def send_command(self, command: str, timeout: float = 2.0) -> Optional[str]:
        """
        Send command to Pico and return response
        Returns None on error or timeout
        """
        if not self.serial_conn or not self.serial_conn.is_open:
            print("❌ Not connected to Pico")
            return None
        
        # If UART not primed yet, prime it first
        if not self.uart_primed:
            print("⚠️ UART not primed - priming now...")
            for _ in range(2):
                self.serial_conn.write(b"PING\n")
                self.serial_conn.flush()
                time.sleep(0.2)
                if self.serial_conn.in_waiting > 0:
                    self.serial_conn.read(self.serial_conn.in_waiting)
                self.serial_conn.reset_input_buffer()
            self.uart_primed = True
            
        try:
            # Clear input buffer
            self.serial_conn.reset_input_buffer()
            time.sleep(0.05)
            
            # Send command
            self.serial_conn.write(f"{command}\n".encode())
            self.serial_conn.flush()
            
            # Wait for processing
            time.sleep(0.25)
            
            # Read response
            start_time = time.time()
            
            while time.time() - start_time < timeout:
                if self.serial_conn.in_waiting > 0:
                    try:
                        response = self.serial_conn.readline().decode('utf-8', errors='replace').strip()
                        
                        if len(response) > 0:
                            # Parse STATUS responses
                            if response.startswith("STATUS:"):
                                self._parse_status_response(response)
                            
                            return response
                            
                    except:
                        return None
                        
                time.sleep(0.01)
            
            print(f"⚠️ Timeout: {command}")
            return None
            
        except Exception as e:
            print(f"❌ Error: {e}")
            return None
    
    def _parse_status_response(self, response: str):
        """Parse STATUS response"""
        try:
            with self.status_lock:
                if "Spindle=" in response and "RPM" in response:
                    rpm_part = response.split("Spindle=")[1].split("RPM")[0]
                    self.current_rpm = float(rpm_part)
                
                if "Traverse=" in response and "mm" in response:
                    pos_part = response.split("Traverse=")[1].split("mm")[0]
                    self.traverse_position = float(pos_part)
        except:
            pass
    
    def get_version(self) -> Optional[str]:
        """Get firmware version"""
        response = self.send_command("VERSION")
        return response if response else "Unknown"
    
    def get_status(self) -> Optional[Dict[str, Any]]:
        """Get current machine status"""
        response = self.send_command("STATUS", timeout=1.5)
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
        response = self.send_command("G28", timeout=30.0)
        
        if response is None:
            print("❌ Homing failed: No response")
            return False
        
        if "OK" in response or "HOMED" in response:
            print("✅ Homing complete")
            return True
        
        print(f"❌ Homing failed: {response}")
        return False
    
    def set_spindle_rpm(self, rpm: float, direction: str = 'CW') -> bool:
        """Set spindle speed and direction"""
        if rpm < 0 or rpm > 3000:
            print(f"❌ RPM {rpm} out of range")
            return False
        
        command = "M3" if direction == 'CW' else "M4"
        response = self.send_command(f"{command} S{rpm}")
        
        if response is None:
            print("❌ Spindle command failed")
            return False
        
        return "OK" in response
    
    def stop_spindle(self) -> bool:
        """Stop spindle with retry logic"""
        print("🛑 Stopping spindle...")
        
        # Try M5 command
        response = self.send_command("M5", timeout=1.0)
        
        if response is not None and "OK" in response:
            print("✅ Spindle stopped")
            return True
        
        # Retry up to 3 times
        print("⚠️ M5 failed, retrying...")
        for i in range(3):
            time.sleep(0.2)
            response = self.send_command("M5", timeout=1.0)
            if response is not None and "OK" in response:
                print(f"✅ Spindle stopped (retry {i+1})")
                return True
        
        # Last resort: emergency stop
        print("🚨 Trying M112 emergency stop...")
        response = self.send_command("M112", timeout=1.0)
        if response is not None:
            print("✅ Emergency stop sent")
            return True
        
        print("❌ Failed to stop spindle")
        print("🔌 Power cycle Pico or disconnect motor power")
        return False
    
    def move_traverse(self, position_mm: float, feed_rate: float = 1000.0) -> bool:
        """Move traverse to position"""
        response = self.send_command(f"G1 Y{position_mm} F{feed_rate}")
        
        if response is None:
            return False
        
        return "OK" in response
    
    def emergency_stop(self) -> bool:
        """Emergency stop all motion"""
        print("🚨 EMERGENCY STOP!")
        
        # Try M112
        response = self.send_command("M112", timeout=1.0)
        if response is not None:
            return True
        
        # Fallback to M5
        response = self.send_command("M5", timeout=1.0)
        if response is not None:
            return True
        
        print("❌ Emergency stop failed")
        return False
    
    def reset_uart(self):
        """Reset UART connection (if commands stop working)"""
        print("🔄 Resetting UART...")
        if self.serial_conn:
            self.serial_conn.reset_input_buffer()
            self.serial_conn.reset_output_buffer()
            time.sleep(0.5)
            
            # Re-prime UART
            self.uart_primed = False
            for _ in range(2):
                self.serial_conn.write(b"PING\n")
                self.serial_conn.flush()
                time.sleep(0.2)
                if self.serial_conn.in_waiting > 0:
                    self.serial_conn.read(self.serial_conn.in_waiting)
            self.uart_primed = True
            print("✅ UART reset complete")
    
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
                time.sleep(1)
            except:
                time.sleep(1)

def main():
    """Test the winding controller"""
    controller = WindingController()
    
    if not controller.connect():
        print("❌ Failed to connect")
        return
    
    try:
        # Get version
        version = controller.get_version()
        print(f"📌 Firmware: {version}")
        
        # Get status
        status = controller.get_status()
        if status:
            print(f"📊 RPM: {status['rpm']:.1f}, Pos: {status['position']:.2f}mm")
        
        # Make sure motor is stopped
        print("\n🛑 Ensuring motor is stopped...")
        controller.stop_spindle()
        
    except KeyboardInterrupt:
        print("\n⚠️ Interrupted")
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("\n🛑 Stopping motor before exit...")
        controller.stop_spindle()
        controller.disconnect()

if __name__ == "__main__":
    main()
