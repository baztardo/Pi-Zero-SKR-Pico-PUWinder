#!/usr/bin/env python3
"""
Python Code Snippets for Pi Zero SKR Pico PUWinder
Useful code patterns and examples for development
"""

# =============================================================================
# UART Communication Snippets
# =============================================================================

def uart_communication_example():
    """Basic UART communication with error handling"""
    import serial
    import time
    
    try:
        # Open serial connection
        ser = serial.Serial('/dev/serial0', 115200, timeout=2.0)
        
        # Send command
        command = "PING\n"
        ser.write(command.encode())
        
        # Read response
        response = ser.readline().decode().strip()
        print(f"Sent: {command.strip()}")
        print(f"Received: {response}")
        
        # Close connection
        ser.close()
        
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except Exception as e:
        print(f"Error: {e}")

def uart_with_retry():
    """UART communication with retry logic"""
    import serial
    import time
    
    max_retries = 3
    retry_delay = 1.0
    
    for attempt in range(max_retries):
        try:
            ser = serial.Serial('/dev/serial0', 115200, timeout=2.0)
            ser.write("VERSION\n".encode())
            response = ser.readline().decode().strip()
            ser.close()
            return response
            
        except serial.SerialException:
            if attempt < max_retries - 1:
                print(f"Attempt {attempt + 1} failed, retrying in {retry_delay}s...")
                time.sleep(retry_delay)
            else:
                print("All retry attempts failed")
                return None

# =============================================================================
# G-code Processing Snippets
# =============================================================================

def parse_gcode_command(command: str) -> dict:
    """Parse G-code command into components"""
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

def execute_gcode_command(parsed_cmd: dict) -> str:
    """Execute parsed G-code command"""
    cmd_type = parsed_cmd['type']
    cmd_num = parsed_cmd['number']
    params = parsed_cmd['parameters']
    
    if cmd_type == 'G':
        if cmd_num == '0' or cmd_num == '1':
            # Linear movement
            x = params.get('X', 0)
            y = params.get('Y', 0)
            f = params.get('F', 1000)  # Feed rate
            return f"Moving to X{x} Y{y} at {f} mm/min"
            
        elif cmd_num == '28':
            # Home all axes
            return "Homing all axes"
    
    elif cmd_type == 'M':
        if cmd_num == '3':
            # Spindle clockwise
            s = params.get('S', 1000)  # RPM
            return f"Starting spindle clockwise at {s} RPM"
            
        elif cmd_num == '4':
            # Spindle counter-clockwise
            s = params.get('S', 1000)
            return f"Starting spindle counter-clockwise at {s} RPM"
            
        elif cmd_num == '5':
            # Stop spindle
            return "Stopping spindle"
    
    return f"Unknown command: {cmd_type}{cmd_num}"

# =============================================================================
# Spindle Control Snippets
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

# =============================================================================
# Testing Snippets
# =============================================================================

def test_gcode_parsing():
    """Test G-code parsing"""
    print("Testing G-code parsing...")
    
    test_commands = [
        "G1 X10 Y20 F1000",
        "M3 S1500",
        "M4 S2000",
        "M5",
        "G28"
    ]
    
    for cmd in test_commands:
        parsed = parse_gcode_command(cmd)
        executed = execute_gcode_command(parsed)
        print(f"Command: {cmd}")
        print(f"Parsed: {parsed}")
        print(f"Executed: {executed}")
        print()

if __name__ == "__main__":
    print("Pi Zero SKR Pico PUWinder - Python Code Snippets")
    print("=" * 50)
    
    # Run tests
    test_gcode_parsing()
    
    print("\nPython code snippets ready for use!")
    print("Import and use these functions in your main application.")
