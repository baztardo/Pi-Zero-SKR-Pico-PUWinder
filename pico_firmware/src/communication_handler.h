// =============================================================================
// communication_handler.h - UART/USB Communication Handler
// Purpose: Handle UART communication with Pi Zero AND USB with Pi CM4
// Based on Klipper's USB CDC ACM approach
// =============================================================================

#pragma once

#include <cstdint>
#include "config.h"
#include "hardware/uart.h"

#ifdef ENABLE_USB_COMM
#include "tusb.h"
#include "class/cdc/cdc_device.h"

// USB descriptors (Klipper-style) - all in one place
#define USB_VID 0x1d50  // OpenMoko VID (like Klipper)
#define USB_PID 0x614e  // Klipper-like PID

// USB descriptor strings
#define USB_MANUFACTURER "Pi-Zero-SKR-Pico-PUWinder"
#define USB_PRODUCT "Coil Winder Controller"
#define USB_SERIAL "0001"

// USB endpoint sizes
#define USB_CDC_EP_SIZE 64

// USB descriptor externs
extern const tusb_desc_device_t usb_desc_device;
extern const uint8_t usb_desc_configuration[];
extern const char* usb_string_desc_arr[];

#endif

class GCodeInterface; // Forward declaration

class CommunicationHandler {
public:
    CommunicationHandler(GCodeInterface* gcode_interface);
    bool init();
    void update(); // Call this in main loop to process incoming data
    void send_response(const char* response);
    void send_error(const char* error);

    // USB functionality (Klipper-style) - public for extern "C" access
    #ifdef ENABLE_USB_COMM
    void init_usb();
    void process_usb_data();
    void send_usb_response(const char* response);
    void send_usb_error(const char* error);

    // USB CDC callbacks (public for extern "C" access)
    void usb_cdc_rx_callback(void);
    void usb_cdc_tx_complete_callback(void);
    void usb_cdc_line_state_callback(uint32_t itf, bool dtr, bool rts);
    #endif

private:
    GCodeInterface* gcode_interface;
    char command_buffer[256];
    int buffer_pos;
    bool initialized;
    bool usb_enabled;

    void process_incoming_char(char c);
};