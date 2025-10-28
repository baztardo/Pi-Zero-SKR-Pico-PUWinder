// =============================================================================
// communication_handler.h - UART Communication Handler
// Purpose: Handle all UART communication with Pi Zero
// =============================================================================

#pragma once

#include <cstdint>
#include "config.h"
#include "hardware/uart.h"

class GCodeInterface; // Forward declaration

class CommunicationHandler {
public:
    CommunicationHandler(GCodeInterface* gcode_interface);
    bool init();
    void update(); // Call this in main loop to process incoming data
    void send_response(const char* response);
    void send_error(const char* error);

private:
    GCodeInterface* gcode_interface;
    char command_buffer[256];
    int buffer_pos;
    bool initialized;

    void process_incoming_char(char c);
};