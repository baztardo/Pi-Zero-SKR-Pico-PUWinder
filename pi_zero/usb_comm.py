#!/usr/bin/env python3
"""
USB Communication Library for Pi CM4 Host
Handles USB CDC ACM communication with Pico device
"""

import serial
import threading
import time
import struct
from enum import Enum
from typing import Optional, Callable, Any
import logging

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# =============================================================================
# CONSTANTS (Mirrored from usb_protocol.h)
# =============================================================================

USB_COMM_VENDOR_ID = 0xCAFE
USB_COMM_PRODUCT_ID = 0x4001
USB_COMM_SERIAL_STR = "PU-Winder"

USB_COMM_CONNECT_TIMEOUT = 5000
USB_COMM_SEND_TIMEOUT = 1.0
USB_COMM_RECV_TIMEOUT = 1.0

USB_COMM_TX_BUFFER_SIZE = 512
USB_COMM_RX_BUFFER_SIZE = 512

# Message types
class USBMsgType(Enum):
    HEARTBEAT = 0x01
    STATUS = 0x02
    COMMAND = 0x03
    RESPONSE = 0x04
    ERROR = 0x05
    DEBUG = 0x06
    WINDING_DATA = 0x07
    MOTION_DATA = 0x08

# Command codes
class USBCommand(Enum):
    PING = 0x01
    GET_STATUS = 0x02
    START_WINDING = 0x03
    STOP_WINDING = 0x04
    EMERGENCY_STOP = 0x05
    RESET = 0x06
    GET_CONFIG = 0x07
    SET_CONFIG = 0x08
    MOVE_TRAVERSE = 0x09
    SET_SPINDLE_RPM = 0x0A
    GET_DIAGNOSTICS = 0x0B

# Error codes
class USBError(Enum):
    NONE = 0x00
    INVALID_COMMAND = 0x01
    INVALID_DATA = 0x02
    TIMEOUT = 0x03
    BUSY = 0x04
    NOT_READY = 0x05
    HARDWARE_FAULT = 0x06
    SAFETY_VIOLATION = 0x07
    OUT_OF_RANGE = 0x08
    UNKNOWN = 0xFF

# =============================================================================
# DATA STRUCTURES
# =============================================================================

class USBMessageHeader:
    """USB Message Header Structure"""
    SYNC_BYTE = 0xAA
    HEADER_SIZE = 8

    def __init__(self):
        self.sync_byte = self.SYNC_BYTE
        self.msg_type = 0
        self.msg_id = 0
        self.payload_length = 0
        self.checksum = 0

    def pack(self) -> bytes:
        """Pack header into bytes"""
        return struct.pack('<BBHHH',
                          self.sync_byte,
                          self.msg_type,
                          self.msg_id,
                          self.payload_length,
                          self.checksum)

    @classmethod
    def unpack(cls, data: bytes) -> 'USBMessageHeader':
        """Unpack bytes into header"""
        header = cls()
        header.sync_byte, header.msg_type, header.msg_id, header.payload_length, header.checksum = \
            struct.unpack('<BBHHH', data[:cls.HEADER_SIZE])
        return header

class USBHeartbeatPayload:
    """Heartbeat message payload"""
    def __init__(self):
        self.timestamp_ms = 0
        self.status_flags = 0

    def pack(self) -> bytes:
        return struct.pack('<LB', self.timestamp_ms, self.status_flags)

    @classmethod
    def unpack(cls, data: bytes) -> 'USBHeartbeatPayload':
        payload = cls()
        payload.timestamp_ms, payload.status_flags = struct.unpack('<LB', data)
        return payload

class USBStatusPayload:
    """Status message payload"""
    def __init__(self):
        self.system_state = 0
        self.spindle_state = 0
        self.traverse_state = 0
        self.safety_state = 0
        self.spindle_rpm = 0.0
        self.traverse_pos_mm = 0.0
        self.turns_completed = 0

    def pack(self) -> bytes:
        return struct.pack('<BBBBffL',
                          self.system_state, self.spindle_state, self.traverse_state, self.safety_state,
                          self.spindle_rpm, self.traverse_pos_mm, self.turns_completed)

    @classmethod
    def unpack(cls, data: bytes) -> 'USBStatusPayload':
        payload = cls()
        payload.system_state, payload.spindle_state, payload.traverse_state, payload.safety_state, \
        payload.spindle_rpm, payload.traverse_pos_mm, payload.turns_completed = \
            struct.unpack('<BBBBffL', data)
        return payload

# =============================================================================
# USB HOST CLASS
# =============================================================================

class USBHostPi:
    """USB Host class for Pi CM4 communication with Pico device"""

    def __init__(self, device_path: str = "/dev/ttyACM0", baudrate: int = 115200):
        self.device_path = device_path
        self.baudrate = baudrate
        self.serial: Optional[serial.Serial] = None
        self.connected = False
        self.next_msg_id = 0
        self.receive_thread: Optional[threading.Thread] = None
        self.running = False

        # Callbacks
        self.on_message_received: Optional[Callable[[USBMessageHeader, bytes], None]] = None
        self.on_connection_changed: Optional[Callable[[bool], None]] = None

        # Threading
        self.lock = threading.Lock()

    def connect(self) -> bool:
        """Connect to Pico USB device"""
        try:
            logger.info(f"Connecting to {self.device_path}...")
            self.serial = serial.Serial(
                port=self.device_path,
                baudrate=self.baudrate,
                timeout=USB_COMM_RECV_TIMEOUT,
                write_timeout=USB_COMM_SEND_TIMEOUT
            )

            # Wait for connection
            start_time = time.time()
            while not self.serial.is_open and (time.time() - start_time) < (USB_COMM_CONNECT_TIMEOUT / 1000):
                time.sleep(0.1)

            if self.serial.is_open:
                self.connected = True
                self.running = True
                self.receive_thread = threading.Thread(target=self._receive_worker, daemon=True)
                self.receive_thread.start()

                logger.info("✅ Connected to Pico USB device")
                if self.on_connection_changed:
                    self.on_connection_changed(True)
                return True
            else:
                logger.error("❌ Failed to open serial port")
                return False

        except Exception as e:
            logger.error(f"❌ Connection failed: {e}")
            return False

    def disconnect(self):
        """Disconnect from device"""
        self.running = False
        self.connected = False

        if self.receive_thread:
            self.receive_thread.join(timeout=1.0)

        if self.serial and self.serial.is_open:
            self.serial.close()

        logger.info("🔌 Disconnected from Pico USB device")
        if self.on_connection_changed:
            self.on_connection_changed(False)

    def send_message(self, msg_type: USBMsgType, payload: bytes = b'') -> bool:
        """Send message to device"""
        if not self.connected or not self.serial:
            return False

        try:
            # Create header
            header = USBMessageHeader()
            header.msg_type = msg_type.value
            header.msg_id = self._get_next_msg_id()
            header.payload_length = len(payload)

            # Calculate checksum (simple sum for now)
            header_data = header.pack()
            checksum_data = header_data + payload
            header.checksum = sum(checksum_data) & 0xFFFF

            # Send header + payload
            message = header.pack() + payload

            with self.lock:
                self.serial.write(message)
                self.serial.flush()

            logger.debug(f"→ Sent {msg_type.name} (ID:{header.msg_id}, Len:{len(payload)})")
            return True

        except Exception as e:
            logger.error(f"❌ Send failed: {e}")
            return False

    def send_command(self, command: USBCommand, params: bytes = b'') -> bool:
        """Send command to device"""
        command_payload = struct.pack('<BH', command.value, len(params)) + params
        return self.send_message(USBMsgType.COMMAND, command_payload)

    def _receive_worker(self):
        """Background thread for receiving messages"""
        while self.running:
            try:
                if not self.serial or not self.serial.is_open:
                    time.sleep(0.1)
                    continue

                # Try to read header
                header_data = self.serial.read(USBMessageHeader.HEADER_SIZE)
                if len(header_data) < USBMessageHeader.HEADER_SIZE:
                    continue

                # Parse header
                header = USBMessageHeader.unpack(header_data)

                # Validate header
                if header.sync_byte != USBMessageHeader.SYNC_BYTE:
                    logger.warning("❌ Invalid sync byte")
                    continue

                # Read payload
                payload = b''
                if header.payload_length > 0:
                    payload = self.serial.read(header.payload_length)
                    if len(payload) != header.payload_length:
                        logger.warning(f"❌ Incomplete payload: got {len(payload)}, expected {header.payload_length}")
                        continue

                # Notify callback
                msg_type = USBMsgType(header.msg_type)
                logger.debug(f"← Received {msg_type.name} (ID:{header.msg_id}, Len:{len(payload)})")

                if self.on_message_received:
                    self.on_message_received(header, payload)

            except Exception as e:
                logger.error(f"❌ Receive error: {e}")
                time.sleep(0.1)

    def _get_next_msg_id(self) -> int:
        """Get next message ID (thread-safe)"""
        with self.lock:
            msg_id = self.next_msg_id
            self.next_msg_id = (self.next_msg_id + 1) & 0xFFFF
            return msg_id

    def ping(self) -> bool:
        """Send ping command"""
        return self.send_command(USBCommand.PING)

    def get_status(self) -> bool:
        """Request status update"""
        return self.send_command(USBCommand.GET_STATUS)

    def emergency_stop(self) -> bool:
        """Send emergency stop command"""
        return self.send_command(USBCommand.EMERGENCY_STOP)

    def start_winding(self) -> bool:
        """Start winding operation"""
        return self.send_command(USBCommand.START_WINDING)

    def stop_winding(self) -> bool:
        """Stop winding operation"""
        return self.send_command(USBCommand.STOP_WINDING)

    def set_spindle_rpm(self, rpm: float) -> bool:
        """Set spindle RPM"""
        params = struct.pack('<f', rpm)
        return self.send_command(USBCommand.SET_SPINDLE_RPM, params)

    def move_traverse(self, position_mm: float, speed_mm_s: float) -> bool:
        """Move traverse to position"""
        params = struct.pack('<ff', position_mm, speed_mm_s)
        return self.send_command(USBCommand.MOVE_TRAVERSE, params)

# =============================================================================
# UTILITY FUNCTIONS
# =============================================================================

def find_pico_device() -> Optional[str]:
    """Find Pico USB device by VID/PID"""
    import glob

    # Look for ACM devices (CDC serial)
    acm_devices = glob.glob('/dev/ttyACM*')

    for device in acm_devices:
        try:
            # Could check VID/PID here using udevadm or pyusb
            # For now, just return first ACM device found
            return device
        except:
            continue

    return None

def crc16(data: bytes) -> int:
    """Calculate CRC-16 checksum"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return crc

# =============================================================================
# MAIN FUNCTION (for testing)
# =============================================================================

if __name__ == "__main__":
    # Example usage
    device_path = find_pico_device()
    if not device_path:
        print("❌ No Pico device found")
        exit(1)

    print(f"Found Pico at: {device_path}")

    host = USBHostPi(device_path)

    def on_message(header, payload):
        msg_type = USBMsgType(header.msg_type)
        print(f"Received: {msg_type.name} (ID:{header.msg_id})")

        if msg_type == USBMsgType.STATUS and len(payload) >= 16:
            status = USBStatusPayload.unpack(payload)
            print(f"  Spindle RPM: {status.spindle_rpm:.1f}")
            print(f"  Traverse Pos: {status.traverse_pos_mm:.2f}mm")
            print(f"  Turns: {status.turns_completed}")

    host.on_message_received = on_message

    if host.connect():
        try:
            # Test communication
            host.ping()
            time.sleep(0.5)
            host.get_status()
            time.sleep(2)

        except KeyboardInterrupt:
            print("\nShutting down...")
        finally:
            host.disconnect()
    else:
        print("❌ Failed to connect")
