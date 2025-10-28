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

CommunicationHandler::CommunicationHandler(GCodeInterface* gcode_interface)
    : gcode_interface(gcode_interface)
    , buffer_pos(0)
    , initialized(false)
{
    command_buffer[0] = '\0';
    printf("[CommunicationHandler] Created\n");
}

bool CommunicationHandler::init() {
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    initialized = true;
    printf("[CommunicationHandler] UART initialized on pins %d,%d at %d baud\n", 
           PI_UART_TX, PI_UART_RX, PI_UART_BAUD);
    return true;
}

void CommunicationHandler::update() {
    if (!initialized) return;
    
    // Process incoming UART data
    while (uart_is_readable(PI_UART_ID)) {
        char c = uart_getc(PI_UART_ID);
        process_incoming_char(c);
    }
}

void CommunicationHandler::process_incoming_char(char c) {
    if (c == '\n' || c == '\r') {
        // End of command
        if (buffer_pos > 0) {
            command_buffer[buffer_pos] = '\0';
            printf("[CommunicationHandler] Received: %s\n", command_buffer);
            
            // Process command
            if (gcode_interface) {
                gcode_interface->process_command(command_buffer);
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
    // Send response over UART
    for (int i = 0; response[i] != '\0'; i++) {
        uart_putc(PI_UART_ID, response[i]);
    }
    uart_putc(PI_UART_ID, '\n');
}

void CommunicationHandler::send_error(const char* error) {
    if (!initialized) return;
    
    printf("ERROR: %s\n", error);
    // Send error over UART
    for (int i = 0; error[i] != '\0'; i++) {
        uart_putc(PI_UART_ID, error[i]);
    }
    uart_putc(PI_UART_ID, '\n');
}
