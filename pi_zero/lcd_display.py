#!/usr/bin/env python3
"""
Pi Zero LCD Display Interface
Purpose: Display winding status and progress on LCD
"""

import time
from typing import Dict, Any

# =============================================================================
# LCD Display Interface
# =============================================================================
class LCDDisplay:
    def __init__(self, width=20, height=4):
        self.width = width
        self.height = height
        self.current_line = 0
        
        # Initialize display (placeholder - replace with actual LCD library)
        print("🖥️  LCD Display initialized")
        print(f"   Size: {width}x{height}")
    
    def clear(self):
        """Clear display"""
        print("🖥️  LCD: Clear display")
        self.current_line = 0
    
    def set_cursor(self, col: int, row: int):
        """Set cursor position"""
        self.current_line = row
        print(f"🖥️  LCD: Cursor set to ({col}, {row})")
    
    def write(self, text: str):
        """Write text to display"""
        print(f"🖥️  LCD Line {self.current_line}: {text}")
        self.current_line = (self.current_line + 1) % self.height
    
    def display_winding_status(self, status: Dict[str, Any]):
        """Display winding status"""
        self.clear()
        
        # Line 1: State
        state_text = f"State: {status['state']}"
        self.write(state_text)
        
        # Line 2: Progress
        progress_text = f"Progress: {status['progress_percent']:.1f}%"
        self.write(progress_text)
        
        # Line 3: Layer info
        layer_text = f"Layer: {status['current_layer']}"
        self.write(layer_text)
        
        # Line 4: RPM
        rpm_text = f"RPM: {status['current_rpm']:.1f}"
        self.write(rpm_text)
    
    def display_error(self, error_msg: str):
        """Display error message"""
        self.clear()
        self.write("ERROR!")
        self.write(error_msg[:self.width])
        self.write("Check system")
        self.write("Press reset")
    
    def display_complete(self):
        """Display completion message"""
        self.clear()
        self.write("WINDING COMPLETE!")
        self.write("Success!")
        self.write("Remove wire")
        self.write("Press reset")

# =============================================================================
# Example Usage
# =============================================================================
if __name__ == "__main__":
    # Create LCD display
    lcd = LCDDisplay()
    
    # Test display
    lcd.clear()
    lcd.write("Pi Zero Winding")
    lcd.write("Controller v1.0")
    lcd.write("Ready to start")
    lcd.write("Press button")
    
    time.sleep(2)
    
    # Test status display
    status = {
        'state': 'WINDING',
        'progress_percent': 45.2,
        'current_layer': 3,
        'current_rpm': 285.5
    }
    lcd.display_winding_status(status)
    
    time.sleep(2)
    
    # Test error display
    lcd.display_error("Connection lost")
    
    time.sleep(2)
    
    # Test completion display
    lcd.display_complete()
