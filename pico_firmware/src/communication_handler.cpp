// =============================================================================
// communication_handler.cpp - UART Communication Handler Implementation
// Purpose: Handle all UART communication with Pi Zero
// =============================================================================

#include "communication_handler.h"
#include "gcode_interface.h"
#include "config.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include <cstdio>
#include <cstring>

#ifdef ENABLE_USB_COMM
// Include TinyUSB config BEFORE tusb.h
#include "tusb_config.h"

#include "bsp/board.h"
#include "tusb.h"

// Static instance for callbacks
static CommunicationHandler* usb_comm_handler = nullptr;

// USB descriptors (Klipper-style) - consolidated here
const tusb_desc_device_t usb_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,  // USB 2.0

    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,  // Version 1.0

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
};

// Configuration descriptor
enum {
    USB_ITF_NUM_CDC = 0,
    USB_ITF_NUM_CDC_DATA,
    USB_ITF_NUM_TOTAL
};

// USB configuration using TinyUSB's built-in macros
#define USB_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

const uint8_t usb_desc_configuration[] = {
    // Config descriptor
    TUD_CONFIG_DESCRIPTOR(1, USB_ITF_NUM_TOTAL, 0, USB_CONFIG_TOTAL_LEN,
                         TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // CDC descriptors
    TUD_CDC_DESCRIPTOR(USB_ITF_NUM_CDC, 4, 0x81, 8, 0x02, 0x83, 64)
};

// String descriptors
const char* usb_string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // 0: Supported language is English (0x0409)
    USB_MANUFACTURER,               // 1: Manufacturer
    USB_PRODUCT,                   // 2: Product
    USB_SERIAL,                    // 3: Serial number
    "CDC",                         // 4: CDC Interface
};

// Buffer for string descriptors
static uint16_t usb_desc_str[32];

#endif

CommunicationHandler::CommunicationHandler(GCodeInterface* gcode_interface)
    : gcode_interface(gcode_interface)
    , buffer_pos(0)
    , initialized(false)
    , usb_enabled(false)
{
    command_buffer[0] = '\0';

#ifdef ENABLE_USB_COMM
    usb_comm_handler = this;
#endif

    printf("[CommunicationHandler] Created\n");
}

bool CommunicationHandler::init() {
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);

#ifdef ENABLE_USB_COMM
    // Initialize USB
    init_usb();
#endif

    initialized = true;
    printf("[CommunicationHandler] UART initialized on pins %d,%d at %d baud\n",
           PI_UART_TX, PI_UART_RX, PI_UART_BAUD);

#ifdef ENABLE_USB_COMM
    printf("[CommunicationHandler] USB communication enabled\n");
#endif

    return true;
}

void CommunicationHandler::update() {
    if (!initialized) return;

    // Process incoming UART data
    while (uart_is_readable(PI_UART_ID)) {
        char c = uart_getc(PI_UART_ID);
        process_incoming_char(c);
    }

#ifdef ENABLE_USB_COMM
    // Process USB tasks
    tud_task();

    // Process USB data if available
    if (usb_enabled) {
        process_usb_data();
    }
#endif
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
    if (!initialized) return;

    printf("ERROR: %s\n", error);

    // Send via UART
    while (!uart_is_writable(PI_UART_ID)) {
        tight_loop_contents();
    }
    uart_puts(PI_UART_ID, error);
    uart_puts(PI_UART_ID, "\n");
    uart_tx_wait_blocking(PI_UART_ID);

#ifdef ENABLE_USB_COMM
    // Also send via USB if enabled
    if (usb_enabled) {
        send_usb_error(error);
    }
#endif
}

#ifdef ENABLE_USB_COMM

// USB initialization (Klipper-style)
void CommunicationHandler::init_usb() {
    // Initialize TinyUSB
    tusb_init();
    usb_enabled = true;
    printf("[USB] Initialized\n");
}

// Process incoming USB data
void CommunicationHandler::process_usb_data() {
    if (!tud_cdc_available()) return;

    // Read available data
    char usb_buffer[256];
    uint32_t count = tud_cdc_read(usb_buffer, sizeof(usb_buffer) - 1);

    if (count > 0) {
        usb_buffer[count] = '\0';
        printf("[USB] Received: %s\n", usb_buffer);

        // Process the command (same as UART)
        if (gcode_interface) {
            gcode_interface->process_command(usb_buffer);
        }
    }
}

// Send response via USB
void CommunicationHandler::send_usb_response(const char* response) {
    if (!usb_enabled || !tud_cdc_connected()) return;

    tud_cdc_write_str(response);
    tud_cdc_write_str("\n");
    tud_cdc_write_flush();
}

// Send error via USB
void CommunicationHandler::send_usb_error(const char* error) {
    if (!usb_enabled || !tud_cdc_connected()) return;

    tud_cdc_write_str("!! ");
    tud_cdc_write_str(error);
    tud_cdc_write_str("\n");
    tud_cdc_write_flush();
}

// USB CDC callbacks (public methods)
void CommunicationHandler::usb_cdc_rx_callback(void) {
    // Data received callback - handled in process_usb_data()
}

void CommunicationHandler::usb_cdc_tx_complete_callback(void) {
    // Transmission complete callback
}

void CommunicationHandler::usb_cdc_line_state_callback(uint32_t itf, bool dtr, bool rts) {
    (void) itf;
    (void) rts;

    if (dtr) {
        usb_enabled = true;
        printf("[USB] Host connected\n");
    } else {
        usb_enabled = false;
        printf("[USB] Host disconnected\n");
    }
}

// TinyUSB callbacks (must be extern "C")
extern "C" {

// USB descriptor callbacks
uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &usb_desc_device;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return usb_desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;

    uint8_t chr_count;

    if (index == 0) {
        memcpy(&usb_desc_str[1], usb_string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        // Convert ASCII string into UTF-16
        if (index >= sizeof(usb_string_desc_arr) / sizeof(usb_string_desc_arr[0])) return NULL;

        const char* str = usb_string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            usb_desc_str[1 + i] = str[i];
        }
    }

    // First byte is length (including header), second byte is string type
    usb_desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return usb_desc_str;
}

// CDC callbacks
void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;
    if (usb_comm_handler) {
        usb_comm_handler->usb_cdc_rx_callback();
    }
}

void tud_cdc_tx_complete_cb(uint8_t itf) {
    (void) itf;
    if (usb_comm_handler) {
        usb_comm_handler->usb_cdc_tx_complete_callback();
    }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    if (usb_comm_handler) {
        usb_comm_handler->usb_cdc_line_state_callback(itf, dtr, rts);
    }
}

} // extern "C"

#endif // ENABLE_USB_COMM

