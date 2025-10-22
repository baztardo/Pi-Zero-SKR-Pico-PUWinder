#!/usr/bin/env python3
"""
UART API Client for Pi Zero
Purpose: Direct UART communication with Pico (replaces HTTP API)
"""

import serial
import time
import threading
from typing import Dict, Any, Optional

# =============================================================================
# UART API Client
# =============================================================================
class UARTAPI:
    def __init__(self, port='/dev/serial0', baudrate=115200, timeout=2):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_conn = None
        self.connected = False
        
    def connect(self) -> bool:
        """Connect to Pico via UART"""
        try:
            self.serial_conn = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            time.sleep(0.1)  # Allow connection to stabilize
            
            # Flush any startup messages
            self.serial_conn.reset_input_buffer()
            startup_data = self.serial_conn.read_all()
            if startup_data:
                print(f"Flushed startup data: {startup_data.decode('utf-8', errors='ignore').strip()}")
            
            self.connected = True
            print(f"✅ Connected to {self.port} @ {self.baudrate} baud")
            return True
            
        except serial.SerialException as e:
            print(f"❌ UART connection failed: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from Pico"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
        self.connected = False
        print("🔌 Disconnected from Pico")
    
    def send_command(self, command: str) -> Optional[str]:
        """Send command to Pico and return response"""
        if not self.connected or not self.serial_conn:
            print("❌ Not connected to Pico")
            return None
        
        try:
            # Send command
            cmd_bytes = f"{command}\n".encode('utf-8')
            self.serial_conn.write(cmd_bytes)
            
            # Read response
            response = self.serial_conn.readline()
            if response:
                return response.decode('utf-8').strip()
            else:
                print(f"❌ No response to command: {command}")
                return None
                
        except Exception as e:
            print(f"❌ Command error: {e}")
            return None
    
    def get_machine_status(self) -> Optional[Dict[str, Any]]:
        """Get machine status from Pico"""
        # Get RPM
        rpm_response = self.send_command("GET_SPINDLE_RPM")
        rpm = 0.0
        if rpm_response and "RPM:" in rpm_response:
            try:
                rpm = float(rpm_response.split(":")[1])
            except (ValueError, IndexError):
                pass
        
        return {
            'spindle_rpm': rpm,
            'connected': self.connected,
            'port': self.port
        }
    
    def send_gcode(self, gcode: str) -> bool:
        """Send G-code command to Pico"""
        if not self.connected or not self.serial_conn:
            print("❌ Not connected to Pico")
            return False
        
        try:
            # Send G-code directly to Pico (now supports G-code interface)
            cmd_bytes = f"{gcode}\n".encode('utf-8')
            self.serial_conn.write(cmd_bytes)
            
            # Read response
            response = self.serial_conn.readline()
            if response:
                response_str = response.decode('utf-8').strip()
                print(f"📥 Response: {response_str}")
                
                # Check for error responses
                if response_str.startswith("ERROR_"):
                    print(f"❌ Pico error: {response_str}")
                    return False
                elif response_str in ["OK", "PONG", "STOPPED"]:
                    return True
                elif "RPM:" in response_str:
                    return True
                else:
                    return True  # Assume success for other responses
            else:
                print(f"❌ No response to command: {gcode}")
                return False
                
        except Exception as e:
            print(f"❌ Command error: {e}")
            return False
    
    def set_spindle_rpm(self, rpm: float) -> bool:
        """Set spindle RPM"""
        response = self.send_command(f"SET_SPINDLE_RPM {rpm}")
        return response == "OK"
    
    def stop_spindle(self) -> bool:
        """Stop spindle"""
        response = self.send_command("STOP_SPINDLE")
        return response == "STOPPED"
    
    def get_spindle_rpm(self) -> float:
        """Get current spindle RPM"""
        response = self.send_command("GET_SPINDLE_RPM")
        if response and "RPM:" in response:
            try:
                return float(response.split(":")[1])
            except (ValueError, IndexError):
                pass
        return 0.0
    
    def ping(self) -> bool:
        """Test connection to Pico"""
        response = self.send_command("PING")
        return response == "PONG"
    
    def get_version(self) -> Optional[str]:
        """Get Pico firmware version"""
        return self.send_command("VERSION")

# =============================================================================
# Test Functions
# =============================================================================
def test_uart_connection():
    """Test UART connection to Pico"""
    print("🧪 Testing UART connection...")
    
    api = UARTAPI()
    
    if not api.connect():
        print("❌ Failed to connect to Pico")
        return False
    
    # Test ping
    if not api.ping():
        print("❌ Ping failed")
        api.disconnect()
        return False
    
    print("✅ Ping successful")
    
    # Test version
    version = api.get_version()
    if version:
        print(f"✅ Version: {version}")
    else:
        print("❌ Version request failed")
    
    # Test spindle control
    print("🧪 Testing spindle control...")
    
    if api.set_spindle_rpm(100):
        print("✅ Set RPM to 100")
        time.sleep(1)
        
        rpm = api.get_spindle_rpm()
        print(f"✅ Current RPM: {rpm}")
        
        if api.stop_spindle():
            print("✅ Spindle stopped")
        else:
            print("❌ Failed to stop spindle")
    else:
        print("❌ Failed to set RPM")
    
    api.disconnect()
    print("🎉 UART test completed")
    return True

if __name__ == "__main__":
    test_uart_connection()
