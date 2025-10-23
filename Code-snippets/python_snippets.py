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
# Traverse Control Snippets
# =============================================================================

class TraverseController:
    """Traverse control with stepper motor"""
    
    def __init__(self, uart_connection):
        self.uart = uart_connection
        self.current_position = 0.0
        self.target_position = 0.0
        self.is_moving = False
        self.feed_rate = 1000.0  # mm/min
    
    def move_to(self, position: float, feed_rate: float = None) -> bool:
        """Move traverse to position"""
        try:
            self.target_position = position
            if feed_rate is not None:
                self.feed_rate = feed_rate
            
            # Send G-code command
            cmd = f"G1 Y{position} F{self.feed_rate}\n"
            self.uart.write(cmd.encode())
            self.is_moving = True
            
            return True
            
        except Exception as e:
            print(f"Error moving traverse: {e}")
            return False
    
    def home(self) -> bool:
        """Home traverse axis"""
        try:
            self.uart.write("G28 Y\n".encode())
            self.current_position = 0.0
            self.target_position = 0.0
            return True
        except Exception as e:
            print(f"Error homing traverse: {e}")
            return False
    
    def get_status(self) -> dict:
        """Get current traverse status"""
        return {
            'current_position': self.current_position,
            'target_position': self.target_position,
            'is_moving': self.is_moving,
            'feed_rate': self.feed_rate
        }

# =============================================================================
# Winding Control Snippets
# =============================================================================

class WindingController:
    """Main winding controller with synchronization"""
    
    def __init__(self, uart_connection):
        self.uart = uart_connection
        self.spindle = SpindleController(uart_connection)
        self.traverse = TraverseController(uart_connection)
        self.wire_diameter = 0.5  # mm
        self.turns_per_layer = 100
        self.current_layer = 0
        self.is_winding = False
    
    def start_winding(self, rpm: float, wire_diameter: float, turns: int) -> bool:
        """Start winding process"""
        try:
            self.wire_diameter = wire_diameter
            self.turns_per_layer = turns
            self.current_layer = 0
            self.is_winding = True
            
            # Start spindle
            self.spindle.set_rpm(rpm, 'CW')
            
            # Calculate traverse speed based on RPM and wire diameter
            traverse_speed = self.calculate_traverse_speed(rpm, wire_diameter)
            self.traverse.feed_rate = traverse_speed
            
            return True
            
        except Exception as e:
            print(f"Error starting winding: {e}")
            return False
    
    def calculate_traverse_speed(self, rpm: float, wire_diameter: float) -> float:
        """Calculate traverse speed based on spindle RPM and wire diameter"""
        import math
        
        # Calculate linear speed of wire
        wire_speed = (rpm * wire_diameter * math.pi) / 1000.0  # mm/min
        
        # Traverse speed should match wire speed for proper winding
        return wire_speed
    
    def stop_winding(self) -> bool:
        """Stop winding process"""
        try:
            self.is_winding = False
            self.spindle.stop()
            return True
        except Exception as e:
            print(f"Error stopping winding: {e}")
            return False
    
    def get_status(self) -> dict:
        """Get complete winding status"""
        return {
            'is_winding': self.is_winding,
            'wire_diameter': self.wire_diameter,
            'current_layer': self.current_layer,
            'turns_per_layer': self.turns_per_layer,
            'spindle': self.spindle.get_status(),
            'traverse': self.traverse.get_status()
        }

# =============================================================================
# Error Handling Snippets
# =============================================================================

def handle_uart_error(func):
    """Decorator for UART error handling"""
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except serial.SerialException as e:
            print(f"UART Error in {func.__name__}: {e}")
            return None
        except Exception as e:
            print(f"Error in {func.__name__}: {e}")
            return None
    return wrapper

def safe_uart_operation(operation_func, max_retries=3):
    """Safely execute UART operation with retries"""
    import time
    
    for attempt in range(max_retries):
        try:
            result = operation_func()
            if result is not None:
                return result
        except Exception as e:
            print(f"Attempt {attempt + 1} failed: {e}")
            if attempt < max_retries - 1:
                time.sleep(1.0)
    
    print(f"Operation failed after {max_retries} attempts")
    return None

# =============================================================================
# Testing Snippets
# =============================================================================

def test_uart_communication():
    """Test UART communication"""
    print("Testing UART communication...")
    
    # Test basic communication
    result = uart_with_retry()
    if result:
        print(f"✅ UART communication working: {result}")
        return True
    else:
        print("❌ UART communication failed")
        return False

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

def test_spindle_control():
    """Test spindle control"""
    print("Testing spindle control...")
    
    # This would require actual UART connection
    # For testing, we'll just show the interface
    print("Spindle control interface ready")
    print("Use SpindleController class for actual control")

# =============================================================================
# Main Execution
# =============================================================================

if __name__ == "__main__":
    print("Pi Zero SKR Pico PUWinder - Code Snippets")
    print("=" * 50)
    
    # Run tests
    test_gcode_parsing()
    test_spindle_control()
    
    print("\nCode snippets ready for use!")
    print("Import and use these functions in your main application.")
