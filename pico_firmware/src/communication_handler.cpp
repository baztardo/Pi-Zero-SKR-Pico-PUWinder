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
