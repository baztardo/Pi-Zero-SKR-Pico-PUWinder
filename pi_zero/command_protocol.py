#!/usr/bin/env python3
"""
Command Protocol - Advanced command system
Purpose: Robust command protocol for Pi Zero ↔ Pico communication
"""

import serial
import time
import threading
import queue
from typing import Optional, Dict, Any, Callable
from enum import Enum
from dataclasses import dataclass

class CommandStatus(Enum):
    PENDING = "pending"
    EXECUTING = "executing"
    COMPLETED = "completed"
    ERROR = "error"
    TIMEOUT = "timeout"

@dataclass
class Command:
    id: int
    command: str
    status: CommandStatus
    response: Optional[str] = None
    timestamp: float = 0.0
    timeout: float = 5.0

class CommandProtocol:
    """
    Advanced command protocol with queue management
    """
    
    def __init__(self, port: str, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        self.connected = False
        
        # Command management
        self.command_queue = queue.Queue()
        self.pending_commands: Dict[int, Command] = {}
        self.command_id = 0
        self.response_handlers: Dict[str, Callable] = {}
        
        # Threading
        self.receive_thread = None
        self.running = False
        
        # Statistics
        self.commands_sent = 0
        self.commands_completed = 0
        self.commands_failed = 0
        
    def connect(self) -> bool:
        """Connect to Pico with retry logic"""
        max_retries = 3
        retry_delay = 1.0
        
        for attempt in range(max_retries):
            try:
                print(f"🔍 Connecting to {self.port} (attempt {attempt + 1}/{max_retries})...")
                self.ser = serial.Serial(self.port, self.baudrate, timeout=2)
                time.sleep(0.5)
                
                # Test connection
                if self._test_connection():
                    self.connected = True
                    self._start_receive_thread()
                    print(f"✅ Connected to {self.port}")
                    return True
                else:
                    self.ser.close()
                    print(f"❌ Connection test failed")
                    
            except Exception as e:
                print(f"❌ Connection error: {e}")
                if attempt < max_retries - 1:
                    time.sleep(retry_delay)
                    
        print("❌ Could not connect to Pico")
        return False
    
    def disconnect(self):
        """Disconnect from Pico"""
        self.running = False
        if self.receive_thread:
            self.receive_thread.join(timeout=1.0)
        if self.ser:
            self.ser.close()
        self.connected = False
        print("🔌 Disconnected from Pico")
    
    def _test_connection(self) -> bool:
        """Test connection with PING command"""
        try:
            self.ser.write(b'PING\n')
            time.sleep(0.5)
            response = self.ser.read(100)
            decoded = response.decode('utf-8', errors='ignore').strip()
            return 'PONG' in decoded or decoded != ''
        except:
            return False
    
    def _start_receive_thread(self):
        """Start background thread for receiving responses"""
        self.running = True
        self.receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
        self.receive_thread.start()
    
    def _receive_loop(self):
        """Background thread for receiving responses"""
        while self.running:
            try:
                if self.ser and self.ser.in_waiting:
                    response = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if response:
                        self._handle_response(response)
                time.sleep(0.01)
            except Exception as e:
                print(f"❌ Receive error: {e}")
                time.sleep(0.1)
    
    def _handle_response(self, response: str):
        """Handle response from Pico"""
        print(f"📥 Received: {response}")
        
        # Handle different response types
        if response.startswith("ok"):
            self._handle_ok_response(response)
        elif response.startswith("error"):
            self._handle_error_response(response)
        elif response.startswith("STATUS"):
            self._handle_status_response(response)
        elif response == "PONG":
            self._handle_pong_response()
        else:
            self._handle_generic_response(response)
    
    def _handle_ok_response(self, response: str):
        """Handle 'ok' response (Klipper style)"""
        # Find the oldest pending command and mark as completed
        for cmd_id, cmd in self.pending_commands.items():
            if cmd.status == CommandStatus.EXECUTING:
                cmd.status = CommandStatus.COMPLETED
                cmd.response = response
                self.commands_completed += 1
                print(f"✅ Command {cmd_id} completed: {cmd.command}")
                break
    
    def _handle_error_response(self, response: str):
        """Handle error response"""
        # Find the oldest pending command and mark as error
        for cmd_id, cmd in self.pending_commands.items():
            if cmd.status == CommandStatus.EXECUTING:
                cmd.status = CommandStatus.ERROR
                cmd.response = response
                self.commands_failed += 1
                print(f"❌ Command {cmd_id} failed: {cmd.command} - {response}")
                break
    
    def _handle_status_response(self, response: str):
        """Handle status response"""
        print(f"📊 Status: {response}")
    
    def _handle_pong_response(self):
        """Handle PONG response"""
        print("🏓 PONG received")
    
    def _handle_generic_response(self, response: str):
        """Handle generic response"""
        print(f"📥 Generic response: {response}")
    
    def send_command(self, command: str, timeout: float = 5.0) -> Optional[str]:
        """Send command and wait for response (FluidNC/Klipper style)"""
        if not self.connected:
            print("❌ Not connected to Pico")
            return None
        
        # Create command
        self.command_id += 1
        cmd = Command(
            id=self.command_id,
            command=command,
            status=CommandStatus.PENDING,
            timestamp=time.time(),
            timeout=timeout
        )
        
        # Add to pending commands
        self.pending_commands[cmd.id] = cmd
        
        try:
            # Send command
            print(f"📤 Sending command {cmd.id}: {command}")
            self.ser.write(f"{command}\n".encode())
            cmd.status = CommandStatus.EXECUTING
            self.commands_sent += 1
            
            # Wait for response
            start_time = time.time()
            while cmd.status == CommandStatus.EXECUTING:
                if time.time() - start_time > timeout:
                    cmd.status = CommandStatus.TIMEOUT
                    print(f"⏰ Command {cmd.id} timed out: {command}")
                    break
                time.sleep(0.01)
            
            # Clean up
            response = cmd.response
            del self.pending_commands[cmd.id]
            
            return response
            
        except Exception as e:
            print(f"❌ Error sending command: {e}")
            cmd.status = CommandStatus.ERROR
            del self.pending_commands[cmd.id]
            return None
    
    def send_command_async(self, command: str, callback: Optional[Callable] = None):
        """Send command asynchronously (Klipper style)"""
        if not self.connected:
            print("❌ Not connected to Pico")
            return None
        
        # Create command
        self.command_id += 1
        cmd = Command(
            id=self.command_id,
            command=command,
            status=CommandStatus.PENDING,
            timestamp=time.time()
        )
        
        # Add to pending commands
        self.pending_commands[cmd.id] = cmd
        
        try:
            # Send command
            print(f"📤 Sending async command {cmd.id}: {command}")
            self.ser.write(f"{command}\n".encode())
            cmd.status = CommandStatus.EXECUTING
            self.commands_sent += 1
            
            # Store callback
            if callback:
                self.response_handlers[str(cmd.id)] = callback
            
            return cmd.id
            
        except Exception as e:
            print(f"❌ Error sending async command: {e}")
            cmd.status = CommandStatus.ERROR
            del self.pending_commands[cmd.id]
            return None
    
    def get_status(self) -> Dict[str, Any]:
        """Get protocol status"""
        return {
            'connected': self.connected,
            'commands_sent': self.commands_sent,
            'commands_completed': self.commands_completed,
            'commands_failed': self.commands_failed,
            'pending_commands': len(self.pending_commands),
            'success_rate': self.commands_completed / max(self.commands_sent, 1) * 100
        }
    
    def emergency_stop(self):
        """Emergency stop"""
        print("🚨 Emergency stop!")
        self.send_command("M112", timeout=1.0)
    
    def reset(self):
        """Reset from emergency stop"""
        print("🔄 Resetting...")
        self.send_command("M999", timeout=2.0)

def main():
    """Test the command protocol"""
    # Try to find Pico device
    devices = ['/dev/tty.usbmodem3144301', '/dev/tty.usbmodem31442402']
    
    for device in devices:
        print(f"\n🔍 Testing {device}...")
        protocol = CommandProtocol(device)
        
        if protocol.connect():
            print("✅ Connected! Testing commands...")
            
            # Test basic commands
            commands = ['PING', 'VERSION', 'STATUS', 'M3 S100', 'M5']
            
            for cmd in commands:
                response = protocol.send_command(cmd)
                if response:
                    print(f"✅ {cmd}: {response}")
                else:
                    print(f"❌ {cmd}: No response")
                time.sleep(0.5)
            
            # Show status
            status = protocol.get_status()
            print(f"\n📊 Protocol Status:")
            print(f"  Commands sent: {status['commands_sent']}")
            print(f"  Commands completed: {status['commands_completed']}")
            print(f"  Success rate: {status['success_rate']:.1f}%")
            
            protocol.disconnect()
            break
        else:
            print(f"❌ Could not connect to {device}")

if __name__ == "__main__":
    main()
