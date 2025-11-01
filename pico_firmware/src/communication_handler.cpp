// =============================================================================
// communication_handler.cpp - Communication Handler (UART + USB) Implementation
// Purpose: Handle all communication with Pi CM4 (UART and USB)
// =============================================================================

#include "communication_handler.h"
#include "gcode_interface.h"
#include "config.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"
#include "class/cdc/cdc_device.h"
#include <cstdio>
#include <cstring>

// USB Device Descriptors (required for TinyUSB)
#define TUD_RPI_PICO_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL
};

static tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_COMM_VENDOR_ID,
    .idProduct = USB_COMM_PRODUCT_ID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_RPI_PICO_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x81, 8, 0x02, 0x82, 64),
};

static char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    "PU-Winder",                   // 1: Manufacturer
    "Pico Controller",             // 2: Product
    USB_COMM_SERIAL_STR,           // 3: Serials
    "CDC",                         // 4: CDC Interface
};

static uint16_t _desc_str[32];

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}

// USB CDC Callbacks
void tud_cdc_rx_cb(uint8_t itf) {
    // This will be handled in the update loop
}

void tud_cdc_tx_complete_cb(uint8_t itf) {
    // TX complete callback
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    // Connection state change - handled in update loop
}

CommunicationHandler::CommunicationHandler(GCodeInterface* gcode_interface)
    : gcode_interface(gcode_interface)
    , buffer_pos(0)
    , uart_initialized(false)
    , usb_initialized(false)
    , usb_rx_pos(0)
    , usb_connected(false)
{
    command_buffer[0] = '\0';
    printf("[CommunicationHandler] Created (UART + USB support)\n");
}

bool CommunicationHandler::init() {
    bool success = true;

    // Initialize UART (legacy Pi Zero support)
    printf("[CommunicationHandler] Initializing UART...\n");
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    uart_initialized = true;
    printf("[CommunicationHandler] ✓ UART initialized on pins %d,%d at %d baud\n",
           PI_UART_TX, PI_UART_RX, PI_UART_BAUD);

    // Initialize USB (Pi CM4 communication)
    printf("[CommunicationHandler] Initializing USB...\n");
    tusb_init();
    usb_initialized = true;
    printf("[CommunicationHandler] ✓ USB device initialized (VID:0x%04X, PID:0x%04X)\n",
           USB_COMM_VENDOR_ID, USB_COMM_PRODUCT_ID);

    printf("[CommunicationHandler] ✓ Communication handler initialized\n");
    return success;
}

void CommunicationHandler::update() {
    if (!uart_initialized && !usb_initialized) return;

    // Process incoming UART data (Pi Zero legacy support)
    if (uart_initialized) {
        while (uart_is_readable(PI_UART_ID)) {
            char c = uart_getc(PI_UART_ID);
            process_incoming_char(c);
        }
    }

    // Process USB tasks and data (Pi CM4 communication)
    if (usb_initialized) {
        tud_task(); // Keep TinyUSB alive

        // Check connection status
        bool was_connected = usb_connected;
        usb_connected = tud_cdc_connected();

        if (usb_connected != was_connected) {
            printf("[CommunicationHandler] USB %s\n",
                   usb_connected ? "connected" : "disconnected");
        }

        // Process incoming USB data
        if (usb_connected) {
            process_usb_data();
        }
    }
}

void CommunicationHandler::process_incoming_char(char c) {
    if (c == '\n' || c == '\r') {
        // End of command
        if (buffer_pos > 0) {
            command_buffer[buffer_pos] = '\0';

            // ⭐ FIX: Strip leading garbage/whitespace from command
            char* clean_cmd = command_buffer;
            while (*clean_cmd && (*clean_cmd < ' ' || *clean_cmd > '~')) {
                clean_cmd++;  // Skip non-printable chars
            }

            // Skip leading whitespace
            while (*clean_cmd == ' ' || *clean_cmd == '\t') {
                clean_cmd++;
            }

            if (*clean_cmd != '\0') {
                printf("[CommunicationHandler] Received: %s\n", clean_cmd);

                // Process command
                if (gcode_interface) {
                    gcode_interface->process_command(clean_cmd);
                }
            }

            buffer_pos = 0;  // Reset buffer
        }
    }
    else if (buffer_pos < (sizeof(command_buffer) - 1)) {
        command_buffer[buffer_pos++] = c;
    }
    else {
        // Buffer overflow - reset
        printf("[CommunicationHandler] Buffer overflow, resetting\n");
        buffer_pos = 0;
    }
}

void CommunicationHandler::send_response(const char* response) {
    if (!initialized) return;

    printf("RESPONSE: %s\n", response);

    // Wait for UART to be ready (like FluidNC does)
    while (!uart_is_writable(PI_UART_ID)) {
        tight_loop_contents();
    }

    // Send response atomically (like FluidNC)
    uart_puts(PI_UART_ID, response);
    uart_puts(PI_UART_ID, "\n");

    // Wait for transmission to complete
    uart_tx_wait_blocking(PI_UART_ID);
}

void CommunicationHandler::send_error(const char* error) {
    if (!uart_initialized) return;

    printf("UART ERROR: %s\n", error);

    // Wait for UART to be ready (like FluidNC does)
    while (!uart_is_writable(PI_UART_ID)) {
        tight_loop_contents();
    }

    // Send error atomically (like FluidNC)
    uart_puts(PI_UART_ID, error);
    uart_puts(PI_UART_ID, "\n");

    // Wait for transmission to complete
    uart_tx_wait_blocking(PI_UART_ID);
}

// =============================================================================
// USB COMMUNICATION METHODS
// =============================================================================

bool CommunicationHandler::send_usb_status(const status_t* status) {
    if (!usb_connected || !usb_initialized) return false;

    // Send message type + status data
    uint8_t buffer[1 + sizeof(status_t)];
    buffer[0] = MSG_TYPE_STATUS;
    memcpy(buffer + 1, status, sizeof(status_t));

    size_t sent = tud_cdc_write(buffer, sizeof(buffer));
    tud_cdc_write_flush();

    return sent == sizeof(buffer);
}

bool CommunicationHandler::send_usb_response(uint8_t command, uint8_t result) {
    if (!usb_connected || !usb_initialized) return false;

    response_t response = {command, result, 0};
    uint8_t buffer[1 + sizeof(response_t)];
    buffer[0] = MSG_TYPE_RESPONSE;
    memcpy(buffer + 1, &response, sizeof(response_t));

    size_t sent = tud_cdc_write(buffer, sizeof(buffer));
    tud_cdc_write_flush();

    return sent == sizeof(buffer);
}

bool CommunicationHandler::send_usb_error(const char* error_msg) {
    if (!usb_connected || !usb_initialized) return false;

    size_t msg_len = strlen(error_msg);
    uint8_t buffer[1 + 1 + msg_len]; // type + length + message
    buffer[0] = MSG_TYPE_ERROR;
    buffer[1] = msg_len;
    memcpy(buffer + 2, error_msg, msg_len);

    size_t sent = tud_cdc_write(buffer, sizeof(buffer));
    tud_cdc_write_flush();

    return sent == sizeof(buffer);
}

void CommunicationHandler::process_motion_command(const motion_command_t* cmd) {
    printf("[CommunicationHandler] Processing motion command: %u steps @ %u μs intervals\n",
           cmd->stepper_steps, cmd->stepper_interval);

    // TODO: Forward to move_queue for execution
    // This will be implemented when we integrate with the motion system
}

void CommunicationHandler::process_usb_data() {
    // Read available USB data
    while (tud_cdc_available()) {
        uint8_t byte;
        uint32_t count = tud_cdc_read(&byte, 1);

        if (count == 1) {
            if (usb_rx_pos < sizeof(usb_rx_buffer)) {
                usb_rx_buffer[usb_rx_pos++] = byte;

                // Check for complete message (simple protocol: type + data)
                if (usb_rx_pos >= 1) { // At least message type
                    uint8_t msg_type = usb_rx_buffer[0];

                    // For now, assume fixed-size messages based on type
                    size_t expected_size = 1; // Message type

                    switch (msg_type) {
                        case MSG_TYPE_COMMAND:
                            expected_size += 1; // Command byte
                            break;
                        case MSG_TYPE_MOTION_COMMAND:
                            expected_size += sizeof(motion_command_t);
                            break;
                        default:
                            // Unknown message type, reset buffer
                            usb_rx_pos = 0;
                            continue;
                    }

                    if (usb_rx_pos >= expected_size) {
                        // Process complete message
                        handle_usb_command(usb_rx_buffer, usb_rx_pos);
                        usb_rx_pos = 0; // Reset for next message
                    }
                }
            } else {
                // Buffer overflow, reset
                usb_rx_pos = 0;
            }
        }
    }
}

void CommunicationHandler::handle_usb_command(const uint8_t* data, size_t length) {
    if (length < 1) return;

    uint8_t msg_type = data[0];

    switch (msg_type) {
        case MSG_TYPE_COMMAND: {
            if (length >= 2) {
                usb_command_t cmd = (usb_command_t)data[1];
                printf("[CommunicationHandler] Received USB command: %d\n", cmd);

                // Handle command
                switch (cmd) {
                    case CMD_PING:
                        send_usb_response(CMD_PING, 0); // Success
                        break;

                    case CMD_GET_STATUS: {
                        // TODO: Get actual status from system
                        status_t status = {0, 1, 0, 0, 1, 0.0f, 0.0f, 0};
                        send_usb_status(&status);
                        break;
                    }

                    case CMD_EMERGENCY_STOP:
                        printf("[CommunicationHandler] Emergency stop requested via USB\n");
                        // TODO: Trigger emergency stop
                        send_usb_response(CMD_EMERGENCY_STOP, 0);
                        break;

                    default:
                        printf("[CommunicationHandler] Unknown USB command: %d\n", cmd);
                        send_usb_response(cmd, 1); // Error
                        break;
                }
            }
            break;
        }

        case MSG_TYPE_MOTION_COMMAND: {
            if (length >= 1 + sizeof(motion_command_t)) {
                const motion_command_t* cmd = (const motion_command_t*)(data + 1);
                process_motion_command(cmd);
                // Send acknowledgment
                send_usb_response(CMD_MOVE_TRAVERSE, 0);
            }
            break;
        }

        default:
            printf("[CommunicationHandler] Unknown USB message type: %d\n", msg_type);
            send_usb_error("Unknown message type");
            break;
    }
}
