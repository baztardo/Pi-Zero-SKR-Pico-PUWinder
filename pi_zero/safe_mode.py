#!/usr/bin/env python3
"""
Safe Mode Script for Pi Zero
Purpose: Safe mode that won't get stuck in loops
"""

import time
import sys

def main():
    """Safe mode that can be interrupted"""
    print("🛡️  Pi Zero Safe Mode")
    print("=" * 30)
    print("✅ Safe mode activated")
    print("✅ No auto-start winding process")
    print("✅ Ready for manual control")
    
    # Simple status that can be interrupted
    try:
        counter = 0
        while True:
            print(f"🔄 Safe mode running... {counter}")
            counter += 1
            time.sleep(5)  # Check every 5 seconds
            
    except KeyboardInterrupt:
        print("\n⚠️  Interrupted by user")
        print("✅ Safe mode stopped")
        sys.exit(0)

if __name__ == "__main__":
    main()
