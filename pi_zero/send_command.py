#!/usr/bin/env python3
import serial
import time
import sys

def send_command(cmd):
    try:
        ser = serial.Serial('/dev/serial0', 115200, timeout=2)
        print(f"📤 Sending: {cmd}")
        ser.write(f"{cmd}\n".encode())
        
        time.sleep(0.1)
        response = ser.readline().decode().strip()
        print(f"📥 Response: {response}")
        
        ser.close()
        return response
    except Exception as e:
        print(f"❌ Error: {e}")
        return None

if __name__ == "__main__":
    if len(sys.argv) > 1:
        cmd = " ".join(sys.argv[1:])
        send_command(cmd)
    else:
        print("Usage: python3 send_command.py <command>")
        print("Examples:")
        print("  python3 send_command.py PING")
        print("  python3 send_command.py VERSION")
        print("  python3 send_command.py SET_BLDC_RPM 100")
