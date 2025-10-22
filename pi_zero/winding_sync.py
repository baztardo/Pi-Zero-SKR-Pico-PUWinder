#!/usr/bin/env python3
"""
Winding Synchronization Module
Purpose: Core logic for synchronizing traverse movement with spindle RPM
"""

import time
import math
from typing import Tuple, Dict, Any
from dataclasses import dataclass

# =============================================================================
# Winding Synchronization Parameters
# =============================================================================
@dataclass
class SyncParams:
    wire_diameter_mm: float = 0.064  # 43 AWG
    traverse_pitch_mm: float = 0.064  # Same as wire diameter for tight winding
    spindle_rpm: float = 300.0
    traverse_speed_mm_per_sec: float = 0.0  # Calculated
    sync_tolerance: float = 0.01  # 1% tolerance for synchronization
    
    def __post_init__(self):
        self.calculate_sync_speed()
    
    def calculate_sync_speed(self):
        """Calculate traverse speed based on spindle RPM and wire diameter"""
        # Traverse speed = (RPM * wire_diameter) / 60.0  # mm/s
        self.traverse_speed_mm_per_sec = (self.spindle_rpm * self.wire_diameter_mm) / 60.0

# =============================================================================
# Winding Synchronization Controller
# =============================================================================
class WindingSyncController:
    def __init__(self):
        self.sync_params = SyncParams()
        self.is_syncing = False
        self.sync_start_time = 0.0
        self.last_sync_time = 0.0
        self.sync_error = 0.0
        
        # Traverse position tracking
        self.traverse_position = 0.0
        self.target_traverse_position = 0.0
        
        # Spindle tracking
        self.spindle_rpm = 0.0
        self.spindle_revolutions = 0.0
        
    def set_parameters(self, wire_diameter_mm: float, spindle_rpm: float):
        """Set synchronization parameters"""
        self.sync_params.wire_diameter_mm = wire_diameter_mm
        self.sync_params.spindle_rpm = spindle_rpm
        self.sync_params.calculate_sync_speed()
        
        print(f"Sync parameters set:")
        print(f"  Wire diameter: {wire_diameter_mm:.3f} mm")
        print(f"  Spindle RPM: {spindle_rpm:.1f}")
        print(f"  Traverse speed: {self.sync_params.traverse_speed_mm_per_sec:.3f} mm/s")
    
    def start_sync(self):
        """Start synchronization process"""
        self.is_syncing = True
        self.sync_start_time = time.time()
        self.last_sync_time = time.time()
        self.sync_error = 0.0
        
        print("🔄 Starting winding synchronization")
        print(f"   Target traverse speed: {self.sync_params.traverse_speed_mm_per_sec:.3f} mm/s")
    
    def stop_sync(self):
        """Stop synchronization process"""
        self.is_syncing = False
        print("🛑 Stopping winding synchronization")
    
    def update_spindle_rpm(self, rpm: float):
        """Update spindle RPM and recalculate sync speed"""
        self.spindle_rpm = rpm
        self.sync_params.spindle_rpm = rpm
        self.sync_params.calculate_sync_speed()
        
        if self.is_syncing:
            print(f"🔄 Spindle RPM updated: {rpm:.1f} -> Traverse speed: {self.sync_params.traverse_speed_mm_per_sec:.3f} mm/s")
    
    def calculate_traverse_movement(self, spindle_revolutions: float) -> float:
        """Calculate how much traverse should move based on spindle revolutions"""
        # Each spindle revolution should move traverse by wire diameter
        traverse_movement = spindle_revolutions * self.sync_params.wire_diameter_mm
        return traverse_movement
    
    def get_sync_commands(self) -> Dict[str, Any]:
        """Get G-code commands for synchronized movement"""
        if not self.is_syncing:
            return {}
        
        # Calculate target traverse position
        self.target_traverse_position = self.traverse_position + self.sync_params.traverse_speed_mm_per_sec * 0.1  # 100ms update
        
        # Generate G-code command for synchronized movement
        traverse_speed_mm_per_min = self.sync_params.traverse_speed_mm_per_sec * 60.0
        
        return {
            'gcode': f"G1 Y{self.target_traverse_position:.3f} F{traverse_speed_mm_per_min:.1f}",
            'target_y': self.target_traverse_position,
            'speed': traverse_speed_mm_per_min,
            'sync_speed_mm_per_sec': self.sync_params.traverse_speed_mm_per_sec
        }
    
    def update_traverse_position(self, new_position: float):
        """Update traverse position after movement"""
        self.traverse_position = new_position
        
        if self.is_syncing:
            # Calculate sync error
            expected_position = self.calculate_traverse_movement(self.spindle_revolutions)
            self.sync_error = abs(self.traverse_position - expected_position)
            
            # Check if sync error is within tolerance
            if self.sync_error > self.sync_params.sync_tolerance:
                print(f"⚠️  Sync error: {self.sync_error:.3f} mm (tolerance: {self.sync_params.sync_tolerance:.3f} mm)")
    
    def get_sync_status(self) -> Dict[str, Any]:
        """Get synchronization status"""
        return {
            'is_syncing': self.is_syncing,
            'spindle_rpm': self.spindle_rpm,
            'traverse_position': self.traverse_position,
            'target_traverse_position': self.target_traverse_position,
            'sync_speed_mm_per_sec': self.sync_params.traverse_speed_mm_per_sec,
            'sync_error': self.sync_error,
            'wire_diameter': self.sync_params.wire_diameter_mm,
            'tolerance': self.sync_params.sync_tolerance
        }
    
    def calculate_layer_position(self, layer: int) -> float:
        """Calculate traverse position for a specific layer"""
        # Each layer is one wire diameter higher
        return layer * self.sync_params.wire_diameter_mm
    
    def calculate_turns_per_layer(self, layer_width_mm: float) -> int:
        """Calculate how many turns fit in a layer"""
        return int(layer_width_mm / self.sync_params.wire_diameter_mm)
    
    def calculate_total_layers(self, target_turns: int, layer_width_mm: float) -> int:
        """Calculate total layers needed"""
        turns_per_layer = self.calculate_turns_per_layer(layer_width_mm)
        return (target_turns + turns_per_layer - 1) // turns_per_layer

# =============================================================================
# Example Usage
# =============================================================================
if __name__ == "__main__":
    # Create synchronization controller
    sync_controller = WindingSyncController()
    
    # Set parameters
    sync_controller.set_parameters(
        wire_diameter_mm=0.064,  # 43 AWG
        spindle_rpm=300.0
    )
    
    # Start synchronization
    sync_controller.start_sync()
    
    # Simulate winding process
    for i in range(10):
        # Update spindle RPM (simulate)
        rpm = 300.0 + (i * 10)  # Increasing RPM
        sync_controller.update_spindle_rpm(rpm)
        
        # Get sync commands
        commands = sync_controller.get_sync_commands()
        print(f"Step {i+1}: {commands['gcode']}")
        
        # Update traverse position (simulate)
        new_position = sync_controller.traverse_position + 0.1
        sync_controller.update_traverse_position(new_position)
        
        # Get status
        status = sync_controller.get_sync_status()
        print(f"   Status: RPM={status['spindle_rpm']:.1f}, Pos={status['traverse_position']:.3f}, Error={status['sync_error']:.3f}")
        
        time.sleep(0.1)
    
    # Stop synchronization
    sync_controller.stop_sync()
    
    print("✅ Synchronization test completed")
