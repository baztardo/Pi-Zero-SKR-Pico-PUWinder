// =============================================================================
// communication_handler.cpp - UART Communication Handler Implementation
// Purpose: Handle UART communication, buffering, and command routing
// =============================================================================

#include "communication_handler.h"
#include "gcode_interface.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <cstdio>

// =============================================================================
// Constructor
// =============================================================================
CommunicationHandler::CommunicationHandler(GCodeInterface* gcode_interface)
    : gcode_interface(gcode_interface)
    , initialized(false)
    , command_count(0)
    , buffer_pos(0)
{
    command_buffer[0] = '\0';
    printf("[CommunicationHandler] Created\n");
}

// =============================================================================
// Destructor
// =============================================================================
CommunicationHandler::~CommunicationHandler() {
    printf("[CommunicationHandler] Destroyed\n");
}

// =============================================================================
// Initialize UART communication
// =============================================================================
bool CommunicationHandler::init() {
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    initialized = true;
    printf("[CommunicationHandler] UART initialized (ID: %d, Baud: %d)\n", PI_UART_ID, PI_UART_BAUD);
    printf("[CommunicationHandler] TX pin: %d, RX pin: %d\n", PI_UART_TX, PI_UART_RX);
    
    return true;
}

// =============================================================================
// Update communication handler (call from main loop)
// =============================================================================
void CommunicationHandler::update() {
    if (!initialized || !gcode_interface) {
        return;
    }
    
    // Check for UART commands
    if (uart_is_readable(PI_UART_ID)) {
        char c = uart_getc(PI_UART_ID);
        
        if (c == '\n' || c == '\r') {
            if (buffer_pos > 0) {
                command_buffer[buffer_pos] = '\0';
                printf("[CommunicationHandler] Received: %s\n", command_buffer);
                process_command(command_buffer);
                buffer_pos = 0;
            }
        } else if (buffer_pos < sizeof(command_buffer) - 1) {
            command_buffer[buffer_pos++] = c;
        }
    }
}

// =============================================================================
// Process a complete command
// =============================================================================
void CommunicationHandler::process_command(const char* command) {
    if (!gcode_interface) {
        send_error("ERROR_INTERFACE_NOT_INIT");
        return;
    }
    
    // Process command through G-code interface
    gcode_interface->process_command(command);
    command_count++;
}

// =============================================================================
// Send response over UART
// =============================================================================
void CommunicationHandler::send_response(const char* response) {
    if (!initialized) return;
    
    uart_puts(PI_UART_ID, response);
    uart_puts(PI_UART_ID, "\n");
}

// =============================================================================
// Send error over UART
// =============================================================================
void CommunicationHandler::send_error(const char* error) {
    if (!initialized) return;
    
    uart_puts(PI_UART_ID, error);
    uart_puts(PI_UART_ID, "\n");
}
