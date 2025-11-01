#!/usr/bin/env python3
"""
Test if Pico GPIO pins are damaged
"""

import serial
import time

def test_gpio_output(pin):
    """Test GPIO output by toggling LED"""
    print(f"\n=== Testing GPIO {pin} Output ===")

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    # Send command to toggle GPIO
    ser.write(f'TEST_GPIO_OUT P{pin}\n'.encode())
    time.sleep(0.5)
    response = ser.read(500).decode('utf-8', errors='ignore')
    print(f"GPIO {pin} test: {response.strip()}")

    ser.close()

def test_gpio_input(pin):
    """Test GPIO input"""
    print(f"\n=== Testing GPIO {pin} Input ===")

    ser = serial.Serial('/dev/ttyACM0', 115200, timeout=2)

    ser.write(f'TEST_GPIO_IN P{pin}\n'.encode())
    time.sleep(0.5)
    response = ser.read(500).decode('utf-8', errors='ignore')
    print(f"GPIO {pin} test: {response.strip()}")

    ser.close()

def test_stepper_pins():
    """Test the stepper-related pins"""
    print("Stepper Pin Damage Test")
    print("=" * 25)

    # Test output pins
    output_pins = [4, 5, 6]  # DIR, STEP, EN
    for pin in output_pins:
        test_gpio_output(pin)

    # Test UART pins (can be tested as GPIO)
    uart_pins = [8, 9]  # UART TX, RX
    for pin in uart_pins:
        test_gpio_output(pin)

    print("\nIf any pin test fails, that GPIO is likely damaged!")

if __name__ == "__main__":
    test_stepper_pins()
