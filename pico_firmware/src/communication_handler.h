// =============================================================================
// communication_handler.h - Communication Handler (UART + USB)
// Purpose: Handle all communication with Pi CM4 (UART and USB)
// =============================================================================

#pragma once

#include <cstdint>
#include "config.h"
#include "hardware/uart.h"

// TinyUSB includes
#include "tusb.h"
#include "class/cdc/cdc_device.h"

class GCodeInterface; // Forward declaration

// USB Protocol Definitions
#define USB_COMM_VENDOR_ID       0xCAFE
#define USB_COMM_PRODUCT_ID      0x4001
#define USB_COMM_SERIAL_STR      "PU-Winder"

// Message types
typedef enum {
    MSG_TYPE_COMMAND        = 0x01,
    MSG_TYPE_MOTION_COMMAND = 0x02,
    MSG_TYPE_STATUS         = 0x03,
    MSG_TYPE_RESPONSE       = 0x04,
    MSG_TYPE_ERROR          = 0x05,
} usb_msg_type_t;

// Command codes
typedef enum {
    CMD_PING                = 0x01,
    CMD_GET_STATUS          = 0x02,
    CMD_START_WINDING       = 0x03,
    CMD_STOP_WINDING        = 0x04,
    CMD_EMERGENCY_STOP      = 0x05,
    CMD_RESET               = 0x06,
    CMD_SET_SPINDLE_RPM     = 0x07,
    CMD_MOVE_TRAVERSE       = 0x08,
} usb_command_t;

// Motion command structure
typedef struct __attribute__((packed)) {
    uint32_t sequence_id;
    uint32_t stepper_steps;     // Steps to move
    uint32_t stepper_interval;  // Interval between steps (μs)
    uint8_t direction;          // 0=CCW, 1=CW
    uint8_t end_of_move;        // 1 if this is the last segment
} motion_command_t;

// Status structure
typedef struct __attribute__((packed)) {
    uint32_t sequence_id;
    uint8_t system_state;
    uint8_t spindle_state;
    uint8_t traverse_state;
    uint8_t safety_state;
    float spindle_rpm;
    float traverse_pos_mm;
    uint32_t turns_completed;
} status_t;

// Response structure
typedef struct __attribute__((packed)) {
    uint8_t command;
    uint8_t result;
    uint16_t data_length;
} response_t;

class CommunicationHandler {
public:
    CommunicationHandler(GCodeInterface* gcode_interface);
    bool init();
    void update(); // Call this in main loop to process incoming data

    // UART communication (legacy Pi Zero support)
    void send_response(const char* response);
    void send_error(const char* error);

    // USB communication
    bool send_usb_status(const status_t* status);
    bool send_usb_response(uint8_t command, uint8_t result);
    bool send_usb_error(const char* error_msg);

    // Motion command interface
    void process_motion_command(const motion_command_t* cmd);

private:
    GCodeInterface* gcode_interface;
    char command_buffer[256];
    int buffer_pos;
    bool uart_initialized;
    bool usb_initialized;

    // USB state
    uint8_t usb_rx_buffer[512];
    size_t usb_rx_pos;
    bool usb_connected;

    // UART processing
    void process_incoming_char(char c);

    // USB processing
    void process_usb_data();
    void handle_usb_command(const uint8_t* data, size_t length);
};