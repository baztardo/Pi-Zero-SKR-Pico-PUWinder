#!/usr/bin/env python3
"""
Pickup Winder Control - Spindle + Traverse
Communicates with Pico via USB serial
"""

import serial
import time

class PickupWinder:
    def __init__(self, port='/dev/ttyACM0', baud=115200):
        """Initialize connection to Pico"""
        self.ser = serial.Serial(port, baud, timeout=1)
        time.sleep(2)
        
        line = self.ser.readline().decode().strip()
        if line != "READY":
            raise Exception(f"Pico not ready. Got: {line}")
        print("Connected to pickup winder")
    
    def _send_cmd(self, cmd):
        """Send command and get response"""
        self.ser.write(f"{cmd}\n".encode())
        resp = self.ser.readline().decode().strip()
        return resp
    
    # ========== Spindle Control ==========
    
    def wind_cw(self, rpm):
        """Start winding clockwise at RPM"""
        resp = self._send_cmd(f"CW {rpm}")
        print(resp)
        return resp.startswith("OK")
    
    def wind_ccw(self, rpm):
        """Start winding counter-clockwise at RPM"""
        resp = self._send_cmd(f"CCW {rpm}")
        print(resp)
        return resp.startswith("OK")
    
    def stop(self):
        """Stop spindle with ramp-down"""
        resp = self._send_cmd("STOP")
        print(resp)
        return resp.startswith("OK")
    
    def brake(self):
        """Emergency brake - immediate stop"""
        resp = self._send_cmd("BRAKE")
        print(resp)
        return resp.startswith("OK")
    
    def get_turns(self):
        """Get turn count from Hall sensor"""
        resp = self._send_cmd("TURNS?")
        try:
            return int(resp.split()[1])
        except:
            print(f"Error getting turns: {resp}")
            return 0
    
    def reset_turns(self):
        """Reset turn counter to zero"""
        resp = self._send_cmd("RESET")
        print(resp)
        return resp.startswith("OK")
    
    # ========== Traverse Control ==========
    
    def move_traverse(self, position_mm):
        """Move wire guide to absolute position (mm)"""
        resp = self._send_cmd(f"TRAV {position_mm}")
        print(resp)
        return resp.startswith("OK")
    
    def move_traverse_rel(self, distance_mm):
        """Move wire guide relative distance (mm)"""
        resp = self._send_cmd(f"TREL {distance_mm}")
        print(resp)
        return resp.startswith("OK")
    
    def stop_traverse(self):
        """Stop traverse motion"""
        resp = self._send_cmd("TSTOP")
        print(resp)
        return resp.startswith("OK")
    
    def get_traverse_pos(self):
        """Get traverse position (mm)"""
        resp = self._send_cmd("TPOS?")
        try:
            return float(resp.split()[1])
        except:
            print(f"Error getting position: {resp}")
            return 0.0
    
    def zero_traverse(self):
        """Zero traverse position"""
        resp = self._send_cmd("TZERO")
        print(resp)
        return resp.startswith("OK")
    
    def enable_traverse(self, enable=True):
        """Enable/disable traverse motor"""
        resp = self._send_cmd(f"TENABLE {1 if enable else 0}")
        print(resp)
        return resp.startswith("OK")
    
    # ========== High-Level Winding ==========
    
    def wind_to_count(self, rpm, target_turns, update_interval=0.1):
        """
        Wind until target turn count reached
        
        Args:
            rpm: Winding speed in RPM
            target_turns: Target number of turns
            update_interval: Status update frequency (seconds)
        """
        print(f"Winding {target_turns} turns at {rpm} RPM...")
        
        self.reset_turns()
        self.wind_cw(rpm)
        
        last_turns = 0
        start_time = time.time()
        
        try:
            while True:
                turns = self.get_turns()
                
                if turns != last_turns and (turns % 100 == 0 or turns >= target_turns):
                    elapsed = time.time() - start_time
                    print(f"Progress: {turns}/{target_turns} turns ({elapsed:.1f}s)")
                    last_turns = turns
                
                if turns >= target_turns:
                    break
                    
                time.sleep(update_interval)
        
        except KeyboardInterrupt:
            print("\nInterrupted by user")
        
        finally:
            self.stop()
            final_turns = self.get_turns()
            elapsed = time.time() - start_time
            print(f"\nComplete: {final_turns} turns in {elapsed:.1f}s")
    
    def wind_with_traverse(self, rpm, target_turns, traverse_width_mm, 
                          passes_per_layer=10):
        """
        Wind with automatic traverse (scatter wind)
        
        Args:
            rpm: Winding speed
            target_turns: Total turns to wind
            traverse_width_mm: Width to traverse
            passes_per_layer: Turns per traverse pass
        """
        print(f"Winding {target_turns} turns with {traverse_width_mm}mm traverse")
        
        self.enable_traverse(True)
        self.zero_traverse()
        self.reset_turns()
        self.wind_cw(rpm)
        
        turns_per_pass = target_turns // passes_per_layer
        direction = 1
        
        try:
            for layer in range(passes_per_layer):
                target = (layer + 1) * turns_per_pass
                
                # Move traverse
                if direction > 0:
                    self.move_traverse(traverse_width_mm)
                else:
                    self.move_traverse(0)
                direction *= -1
                
                # Wait for turns
                while self.get_turns() < target:
                    time.sleep(0.1)
                
                print(f"Layer {layer+1}/{passes_per_layer} complete")
            
            # Finish remaining turns
            while self.get_turns() < target_turns:
                time.sleep(0.1)
        
        except KeyboardInterrupt:
            print("\nInterrupted")
        
        finally:
            self.stop()
            self.stop_traverse()
            print(f"Final count: {self.get_turns()} turns")
    
    def close(self):
        """Close serial connection"""
        self.ser.close()


# Example usage
if __name__ == "__main__":
    print("Pickup Winder Test - Spindle + Traverse")
    print("=" * 40)
    
    try:
        winder = PickupWinder()
        
        # Test 1: Enable and test traverse
        print("\nTest 1: Traverse motion")
        winder.enable_traverse(True)
        winder.zero_traverse()
        winder.move_traverse(25.0)
        time.sleep(2)
        print(f"Position: {winder.get_traverse_pos():.2f}mm")
        winder.move_traverse(0)
        time.sleep(2)
        
        # Test 2: Simple wind
        print("\nTest 2: Simple wind 500 turns @ 400 RPM")
        winder.wind_to_count(400, 500)
        
        time.sleep(2)
        
        # Test 3: Wind with traverse
        print("\nTest 3: Wind with traverse")
        winder.wind_with_traverse(rpm=500, target_turns=1000, 
                                 traverse_width_mm=30, passes_per_layer=5)
        
        print("\nAll tests complete!")
        
    except KeyboardInterrupt:
        print("\nTest interrupted")
        winder.stop()
        winder.stop_traverse()
    
    except Exception as e:
        print(f"Error: {e}")
    
    finally:
        winder.close()
