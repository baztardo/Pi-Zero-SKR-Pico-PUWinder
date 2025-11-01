#!/usr/bin/env python3
"""
Pi CM4 Motion Planner for Pico Coil Winder
Generates winding patterns and controls the Pico via USB
"""

import serial
import time
import sys
import glob
import math
from dataclasses import dataclass
from typing import List, Tuple

@dataclass
class CoilSpec:
    """Coil specifications"""
    inner_diameter: float  # mm
    outer_diameter: float  # mm
    wire_diameter: float   # mm
    turns_per_layer: int
    total_turns: int
    winding_speed: float   # mm/min

@dataclass
class MachineConfig:
    """Machine configuration"""
    traverse_pitch: float     # mm per revolution
    max_traverse_speed: float # mm/min
    spindle_speed: float      # RPM

class PicoMotionPlanner:
    """Motion planner for Pico coil winder"""

    def __init__(self):
        self.serial_port = None
        self.serial = None
        self.machine_config = MachineConfig(
            traverse_pitch=2.0,      # 2mm per revolution (typical)
            max_traverse_speed=500,  # 500 mm/min
            spindle_speed=300        # 300 RPM
        )

    def find_pico_device(self):
        """Find the Pico device"""
        devices = glob.glob('/dev/ttyACM*')
        if not devices:
            devices = glob.glob('/dev/ttyUSB*')

        for device in devices:
            try:
                with serial.Serial(device, 115200, timeout=1) as ser:
                    ser.write(b'VERSION\n')
                    time.sleep(0.1)
                    response = ser.read(200).decode('utf-8', errors='ignore')
                    if 'Pico_Spindle' in response:
                        print(f"Found Pico device: {device}")
                        self.serial_port = device
                        return
            except:
                continue

        raise Exception("Pico device not found. Make sure it's connected via USB.")

    def connect(self):
        """Connect to Pico"""
        if not self.serial_port:
            self.find_pico_device()

        self.serial = serial.Serial(
            self.serial_port,
            baudrate=115200,
            timeout=2,
            write_timeout=1
        )
        print(f"Connected to Pico motion controller")

        # Test connection
        response = self.send_command("PING")
        if "PONG" not in response:
            raise Exception("Failed to communicate with Pico")
        print("✓ Connection verified")

    def send_command(self, command: str, timeout: float = 2.0) -> str:
        """Send command and get response"""
        if not self.serial:
            raise Exception("Not connected")

        # Send command
        self.serial.write(f"{command}\n".encode())
        self.serial.flush()

        # Read response
        response = ""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.serial.in_waiting:
                data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                response += data
                if '\n' in response:
                    break
            time.sleep(0.01)

        return response.strip()

    def home_machine(self):
        """Home the traverse axis"""
        print("🏠 Homing traverse axis...")
        response = self.send_command("G28", timeout=30)  # Longer timeout for homing
        if "HOMING" in response or "OK" in response:
            print("✓ Homing completed")
            return True
        else:
            print(f"✗ Homing failed: {response}")
            return False

    def set_spindle_speed(self, rpm: float):
        """Set spindle speed"""
        print(f"🌀 Setting spindle to {rpm} RPM")
        response = self.send_command(f"M3 S{rpm}")
        if "OK" in response:
            print("✓ Spindle started")
            return True
        else:
            print(f"✗ Spindle control failed: {response}")
            return False

    def stop_spindle(self):
        """Stop spindle"""
        print("🛑 Stopping spindle")
        response = self.send_command("M5")
        if "OK" in response:
            print("✓ Spindle stopped")
            return True
        else:
            print(f"✗ Spindle stop failed: {response}")
            return False

    def move_traverse(self, position: float, feedrate: float = None):
        """Move traverse to position"""
        if feedrate is None:
            feedrate = self.machine_config.max_traverse_speed

        print(f"📏 Moving traverse to {position}mm at {feedrate}mm/min")
        response = self.send_command(f"G1 X{position} F{feedrate}", timeout=10)
        if "OK" in response:
            print("✓ Move completed")
            return True
        else:
            print(f"✗ Move failed: {response}")
            return False

    def calculate_layer_winding(self, coil_spec: CoilSpec) -> List[Tuple[float, int]]:
        """
        Calculate simple layer winding pattern
        Returns list of (traverse_position, turns_in_layer) tuples
        """
        pattern = []

        current_radius = coil_spec.inner_diameter / 2.0
        layer = 0
        total_turns_planned = 0

        while total_turns_planned < coil_spec.total_turns:
            # Calculate wire diameter including insulation
            wire_pitch = coil_spec.wire_diameter * 1.1  # 10% extra for insulation

            # Circumference at current radius
            circumference = 2 * math.pi * current_radius

            # Maximum turns possible at this radius
            max_turns = int(circumference / wire_pitch)

            # Use specified turns per layer or max available
            turns_this_layer = min(coil_spec.turns_per_layer, max_turns)

            # Don't exceed total turns needed
            turns_this_layer = min(turns_this_layer, coil_spec.total_turns - total_turns_planned)

            if turns_this_layer <= 0:
                break

            # Calculate traverse position for this layer
            traverse_pos = layer * coil_spec.wire_diameter

            pattern.append((traverse_pos, turns_this_layer))

            # Update for next layer
            current_radius += coil_spec.wire_diameter
            total_turns_planned += turns_this_layer
            layer += 1

        return pattern

    def execute_winding_pattern(self, pattern: List[Tuple[float, int]], coil_spec: CoilSpec):
        """Execute the calculated winding pattern"""
        print(f"🔄 Starting winding pattern for {coil_spec.total_turns} total turns")
        print(f"   Pattern: {len(pattern)} layers")

        total_turns_completed = 0

        for layer_idx, (traverse_pos, turns_in_layer) in enumerate(pattern):
            print(f"\n📋 Layer {layer_idx + 1}: {turns_in_layer} turns at position {traverse_pos:.2f}mm")

            # Move to layer position
            if not self.move_traverse(traverse_pos):
                print("❌ Failed to position traverse")
                return False

            # Execute winding for this layer
            print(f"🧵 Winding {turns_in_layer} turns...")
            response = self.send_command(f"WIND T{turns_in_layer} S{coil_spec.winding_speed}", timeout=60)

            if "OK" in response:
                total_turns_completed += turns_in_layer
                print(f"✅ Layer {layer_idx + 1} completed. Total turns: {total_turns_completed}")
            else:
                print(f"❌ Winding failed on layer {layer_idx + 1}: {response}")
                return False

        print(f"\n🎉 Winding completed! Total turns: {total_turns_completed}")
        return True

    def wind_simple_coil(self, coil_spec: CoilSpec):
        """Wind a simple layer-wound coil"""
        print("🎯 Starting Simple Coil Winding Process")
        print(f"   Inner diameter: {coil_spec.inner_diameter}mm")
        print(f"   Wire diameter: {coil_spec.wire_diameter}mm")
        print(f"   Total turns: {coil_spec.total_turns}")
        print(f"   Turns per layer: {coil_spec.turns_per_layer}")

        try:
            # Step 1: Home machine
            if not self.home_machine():
                return False

            # Step 2: Start spindle
            if not self.set_spindle_speed(coil_spec.winding_speed):
                return False

            # Step 3: Calculate winding pattern
            pattern = self.calculate_layer_winding(coil_spec)
            if not pattern:
                print("❌ Failed to calculate winding pattern")
                return False

            print(f"📊 Winding pattern calculated: {len(pattern)} layers")

            # Step 4: Execute winding
            success = self.execute_winding_pattern(pattern, coil_spec)

            # Step 5: Stop spindle
            self.stop_spindle()

            # Step 6: Return home
            self.move_traverse(0)

            if success:
                print("\n🎊 Coil winding completed successfully!")
                return True
            else:
                print("\n❌ Coil winding failed!")
                return False

        except Exception as e:
            print(f"❌ Error during winding: {e}")
            self.stop_spindle()
            return False

    def close(self):
        """Close connection"""
        if self.serial:
            self.stop_spindle()  # Safety first
            self.serial.close()
            print("Disconnected from Pico")

def main():
    print("Pi CM4 Motion Planner for Pico Coil Winder")
    print("=" * 50)

    # Example coil specification
    coil = CoilSpec(
        inner_diameter=10.0,   # 10mm inner diameter
        outer_diameter=50.0,   # 50mm outer diameter (max)
        wire_diameter=0.5,     # 0.5mm wire
        turns_per_layer=50,    # 50 turns per layer
        total_turns=500,       # 500 total turns
        winding_speed=200.0    # 200 RPM
    )

    planner = PicoMotionPlanner()

    try:
        # Connect to Pico
        planner.connect()

        # Wind the coil
        success = planner.wind_simple_coil(coil)

        if success:
            print("\n🎉 Motion planning and execution completed successfully!")
        else:
            print("\n❌ Motion planning failed!")
            sys.exit(1)

    except KeyboardInterrupt:
        print("\n⏹️  Operation interrupted by user")
        planner.stop_spindle()
    except Exception as e:
        print(f"\n❌ Error: {e}")
        sys.exit(1)
    finally:
        planner.close()

if __name__ == "__main__":
    main()
