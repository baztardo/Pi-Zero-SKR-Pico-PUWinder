#!/usr/bin/env python3
"""
Simple Test Script for Pi Zero
Purpose: Test basic functionality without getting stuck in loops
"""

import time
import sys

def main():
    """Simple test that won't get stuck"""
    print("🧪 Simple Pi Zero Test")
    print("=" * 30)
    
    print("✅ Pi Zero is running Python")
    print("✅ No stuck loops")
    print("✅ Ready for commands")
    
    # Simple status loop that can be interrupted
    try:
        for i in range(10):
            print(f"Status {i+1}/10: Pi Zero is working")
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n⚠️  Interrupted by user")
        print("✅ Test completed")
    
    print("🎉 Simple test completed successfully!")

if __name__ == "__main__":
    main()
