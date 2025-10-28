// =============================================================================
// gcode_interface.cpp - G-code Interface Implementation (FIXED)
// Purpose: Parse and execute G-code commands from Pi Zero
// =============================================================================

#include "gcode_interface.h"
#include "communication_handler.h"
#include "config.h"
#include "spindle.h"
#include "traverse_controller.h"
#include "move_queue.h"
#include "winding_controller.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <cstdio>
#include <cstdlib>

GCodeInterface::GCodeInterface(BLDC_MOTOR* spindle, TraverseController* traverse, 
                               MoveQueue* queue, WindingController* winding)
    : spindle_controller(spindle)
    , traverse_controller(traverse)
    , move_queue(queue)
    , winding_controller(winding)
    , communication_handler(nullptr)
    , current_command(TOKEN_UNKNOWN)
    , busy(false)
    , error_state(false)
{
    command_buffer[0] = '\0';
    last_error[0] = '\0';
    printf("[GCodeInterface] Created with controller references\n");
}

GCodeInterface::~GCodeInterface() {
    printf("[GCodeInterface] Destroyed\n");
}

// =============================================================================
// FIXED: set_error() - Now actually sends the error response
// =============================================================================
void GCodeInterface::set_error(const char* error) {
    strncpy(last_error, error, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
    error_state = true;
    printf("[GCodeInterface] Error: %s\n", error);
    
    // ✅ FIXED: Actually send the error response
    send_error(error);
}

// =============================================================================
// FIXED: execute_g0_g1() - Now sends proper error responses
// =============================================================================
bool GCodeInterface::execute_g0_g1() {
    if (!traverse_controller || !move_queue) {
        set_error("ERROR_TRAVERSE_NOT_INIT");  // Sends error automatically now
        return false;
    }
    
    if (params.has_Y) {
        traverse_controller->move_to_position(params.Y);
        send_response("OK");
        return true;
    }
    
    set_error("ERROR_G0_REQUIRES_Y");  // Sends error automatically now
    return false;
}

bool GCodeInterface::execute_g28() {
    if (!traverse_controller) {
        set_error("ERROR_TRAVERSE_NOT_INIT");
        return false;
    }
    
    printf("G28: Homing traverse axis only\n");
    traverse_controller->home();
    send_response("OK");
    return true;
}

bool GCodeInterface::execute_m3_m4() {
    if (!spindle_controller) {
        set_error("ERROR_SPINDLE_NOT_INIT");
        return false;
    }
    
    if (params.has_S) {
        if (current_command == TOKEN_M3) {
            spindle_controller->set_direction(DIRECTION_CW);
            gpio_put(SPINDLE_DIR_PIN, 1);
            printf("M3: Direction CW\n");
        } else {
            spindle_controller->set_direction(DIRECTION_CCW);
            gpio_put(SPINDLE_DIR_PIN, 0);
            printf("M4: Direction CCW\n");
        }
        
        spindle_controller->set_rpm_pwm(params.S);
        spindle_controller->set_brake(false);
        
        char response[64];
        snprintf(response, sizeof(response), "OK S%.1f", params.S);
        send_response(response);
        return true;
    }
    
    set_error("ERROR_M3_REQUIRES_S");
    return false;
}

bool GCodeInterface::execute_m5() {
    if (!spindle_controller) {
        set_error("ERROR_SPINDLE_NOT_INIT");
        return false;
    }
    
    spindle_controller->set_brake(true);
    send_response("OK");
    return true;
}

bool GCodeInterface::execute_ping() {
    send_response("PONG");
    return true;
}

bool GCodeInterface::execute_version() {
    send_response("Pico_Spindle_v1.0");
    return true;
}

bool GCodeInterface::execute_status() {
    char status_buffer[256];
    float spindle_rpm = 0.0f;
    float traverse_pos = 0.0f;
    bool spindle_running = false;
    
    if (spindle_controller) {
        spindle_rpm = spindle_controller->get_rpm();
        spindle_running = !spindle_controller->get_brake();
    }
    
    if (traverse_controller) {
        traverse_pos = traverse_controller->get_current_position();
    }
    
    snprintf(status_buffer, sizeof(status_buffer), 
             "STATUS: Spindle=%.1fRPM(%s) Traverse=%.2fmm", 
             spindle_rpm, 
             spindle_running ? "RUN" : "STOP", 
             traverse_pos);
    
    send_response(status_buffer);
    return true;
}

// Minimal implementation for demonstration
bool GCodeInterface::parse_command(const char* command) {
    if (!command || strlen(command) == 0) {
        set_error("ERROR_EMPTY_COMMAND");
        return false;
    }
    
    strncpy(command_buffer, command, sizeof(command_buffer) - 1);
    command_buffer[sizeof(command_buffer) - 1] = '\0';
    
    params = GCodeParams();
    
    // Simple token detection
    if (strncmp(command, "PING", 4) == 0) {
        current_command = TOKEN_PING;
    } else if (strncmp(command, "VERSION", 7) == 0) {
        current_command = TOKEN_VERSION;
    } else if (strncmp(command, "STATUS", 6) == 0) {
        current_command = TOKEN_STATUS;
    } else if (strncmp(command, "G0", 2) == 0) {
        current_command = TOKEN_G0;
    } else if (strncmp(command, "G1", 2) == 0) {
        current_command = TOKEN_G1;
    } else if (strncmp(command, "G28", 3) == 0) {
        current_command = TOKEN_G28;
    } else if (strncmp(command, "M112", 4) == 0) {
        current_command = TOKEN_M112;
    } else if (strncmp(command, "M3", 2) == 0) {
        current_command = TOKEN_M3;
    } else if (strncmp(command, "M4", 2) == 0) {
        current_command = TOKEN_M4;
    } else if (strncmp(command, "M5", 2) == 0) {
        current_command = TOKEN_M5;
    } else {
        current_command = TOKEN_UNKNOWN;
    }
    
    // Parse parameters (simplified)
    const char* ptr = command;
    while (*ptr) {
        if (*ptr == 'Y') {
            params.Y = atof(ptr + 1);
            params.has_Y = true;
        } else if (*ptr == 'S') {
            params.S = atof(ptr + 1);
            params.has_S = true;
        } else if (*ptr == 'F') {
            params.F = atof(ptr + 1);
            params.has_F = true;
        }
        ptr++;
    }
    
    return true;
}

bool GCodeInterface::execute_command() {
    switch (current_command) {
        case TOKEN_PING:
            return execute_ping();
        case TOKEN_VERSION:
            return execute_version();
        case TOKEN_STATUS:
            return execute_status();
        case TOKEN_G0:
        case TOKEN_G1:
            return execute_g0_g1();
        case TOKEN_G28:
            return execute_g28();
        case TOKEN_M3:
        case TOKEN_M4:
            return execute_m3_m4();
        case TOKEN_M5:
            return execute_m5();
        case TOKEN_M112:
            return execute_m112();
        default:
            set_error("ERROR_UNKNOWN_COMMAND");
            return false;
    }
}

void GCodeInterface::process_command(const char* command) {
    if (parse_command(command)) {
        execute_command();
    }
}

void GCodeInterface::set_communication_handler(CommunicationHandler* comm_handler) {
    communication_handler = comm_handler;
    printf("[GCodeInterface] Communication handler set\n");
}

void GCodeInterface::send_response(const char* response) {
    if (communication_handler) {
        communication_handler->send_response(response);
    } else {
        uart_puts(PI_UART_ID, response);
        uart_puts(PI_UART_ID, "\n");
    }
}

bool GCodeInterface::execute_m112() {
    printf("🚨 EMERGENCY STOP M112!\n");
    
    // Stop spindle immediately
    if (spindle_controller) {
        spindle_controller->set_brake(true);
        printf("✓ Spindle brake engaged\n");
    }
    
    // Emergency stop move queue
    if (move_queue) {
        move_queue->emergency_stop();
        printf("✓ Move queue emergency stopped\n");
    }
    
    // Stop winding controller
    if (winding_controller) {
        winding_controller->emergency_stop();
        printf("✓ Winding controller stopped\n");
    }
    
    send_response("OK EMERGENCY_STOPPED");
    return true;
}

void GCodeInterface::send_error(const char* error) {
    if (communication_handler) {
        communication_handler->send_error(error);
    } else {
        uart_puts(PI_UART_ID, error);
        uart_puts(PI_UART_ID, "\n");
    }
}
