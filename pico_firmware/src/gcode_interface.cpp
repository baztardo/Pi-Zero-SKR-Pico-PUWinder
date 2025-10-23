// =============================================================================
// gcode_interface.cpp - G-code Interface Implementation
// Purpose: Parse and execute G-code commands from Pi Zero
// =============================================================================

#include "gcode_interface.h"
#include "config.h"
#include "spindle.h"
#include "traverse_controller.h"
#include "move_queue.h"
#include "winding_controller.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include <cstdio>
#include <cstdlib>

// =============================================================================
// Constructor
// =============================================================================
GCodeInterface::GCodeInterface()
    : current_command(GCODE_UNKNOWN)
    , busy(false)
    , error_state(false)
{
    command_buffer[0] = '\0';
    last_error[0] = '\0';
    printf("[GCodeInterface] Created\n");
}

// =============================================================================
// Destructor
// =============================================================================
GCodeInterface::~GCodeInterface() {
    printf("[GCodeInterface] Destroyed\n");
}

// =============================================================================
// Parse command
// =============================================================================
bool GCodeInterface::parse_command(const char* command) {
    if (!command || strlen(command) == 0) {
        set_error("Empty command");
        return false;
    }
    
    // Copy command to buffer
    strncpy(command_buffer, command, sizeof(command_buffer) - 1);
    command_buffer[sizeof(command_buffer) - 1] = '\0';
    
    // Reset parameters
    params = GCodeParams();
    
    // Parse command type
    if (strncmp(command, "G0", 2) == 0 || strncmp(command, "G1", 2) == 0) {
        current_command = (command[1] == '0') ? GCODE_G0 : GCODE_G1;
        return parse_parameters(command + 2);
    }
    else if (strncmp(command, "G28", 3) == 0) {
        current_command = GCODE_G28;
        return parse_parameters(command + 3);
    }
    else if (strncmp(command, "M3", 2) == 0) {
        current_command = GCODE_M3;
        return parse_parameters(command + 2);
    }
    else if (strncmp(command, "M4", 2) == 0) {
        current_command = GCODE_M4;
        return parse_parameters(command + 2);
    }
    else if (strncmp(command, "M5", 2) == 0) {
        current_command = GCODE_M5;
        return true;
    }
    else if (command[0] == 'S') {
        current_command = GCODE_S;
        return parse_parameters(command + 1);
    }
    else if (strncmp(command, "M6", 2) == 0) {
        current_command = GCODE_M6;
        return true;
    }
    else if (strncmp(command, "M7", 2) == 0) {
        current_command = GCODE_M7;
        return true;
    }
    else if (strncmp(command, "M8", 2) == 0) {
        current_command = GCODE_M8;
        return true;
    }
    else if (strncmp(command, "M9", 2) == 0) {
        current_command = GCODE_M9;
        return true;
    }
    else if (strncmp(command, "M10", 3) == 0) {
        current_command = GCODE_M10;
        return true;
    }
    else if (strncmp(command, "M11", 3) == 0) {
        current_command = GCODE_M11;
        return true;
    }
    else if (strncmp(command, "M12", 3) == 0) {
        current_command = GCODE_M12;
        return true;
    }
    else if (strncmp(command, "M13", 3) == 0) {
        current_command = GCODE_M13;
        return true;
    }
    else if (strncmp(command, "M14", 3) == 0) {
        current_command = GCODE_M14;
        return true;
    }
    else if (strncmp(command, "M15", 3) == 0) {
        current_command = GCODE_M15;
        return true;
    }
    else if (strncmp(command, "M16", 3) == 0) {
        current_command = GCODE_M16;
        return true;
    }
    else if (strncmp(command, "M17", 3) == 0) {
        current_command = GCODE_M17;
        return true;
    }
    else if (strncmp(command, "M18", 3) == 0) {
        current_command = GCODE_M18;
        return true;
    }
    else if (strncmp(command, "M19", 3) == 0) {
        current_command = GCODE_M19;
        return true;
    }
    else if (strncmp(command, "M42", 3) == 0) {
        current_command = GCODE_M42;
        return parse_parameters(command + 3);
    }
    else if (strncmp(command, "M47", 3) == 0) {
        current_command = GCODE_M47;
        return parse_parameters(command + 3);
    }
    else if (strcmp(command, "PING") == 0) {
        current_command = GCODE_PING;
        return true;
    }
    else if (strcmp(command, "VERSION") == 0) {
        current_command = GCODE_VERSION;
        return true;
    }
    else {
        current_command = GCODE_UNKNOWN;
        set_error("Unknown command");
        return false;
    }
}

// =============================================================================
// Execute command
// =============================================================================
bool GCodeInterface::execute_command() {
    if (current_command == GCODE_UNKNOWN) {
        set_error("No command to execute");
        return false;
    }
    
    busy = true;
    clear_error();
    
    bool result = false;
    
    switch (current_command) {
        case GCODE_G0:
        case GCODE_G1:
            result = execute_g0_g1();
            break;
        case GCODE_G28:
            result = execute_g28();
            break;
        case GCODE_M3:
        case GCODE_M4:
            result = execute_m3_m4();
            break;
        case GCODE_M5:
            result = execute_m5();
            break;
        case GCODE_S:
            result = execute_s();
            break;
        case GCODE_M6:
            result = execute_m6();
            break;
        case GCODE_M7:
        case GCODE_M8:
        case GCODE_M9:
            result = execute_m7_m8_m9();
            break;
        case GCODE_M10:
        case GCODE_M11:
            result = execute_m10_m11();
            break;
        case GCODE_M12:
        case GCODE_M13:
            result = execute_m12_m13();
            break;
        case GCODE_M14:
        case GCODE_M15:
            result = execute_m14_m15();
            break;
        case GCODE_M16:
            result = execute_m16();
            break;
        case GCODE_M17:
        case GCODE_M18:
            result = execute_m17_m18();
            break;
        case GCODE_M19:
            result = execute_m19();
            break;
        case GCODE_M42:
            result = execute_m42();
            break;
        case GCODE_M47:
            result = execute_m47();
            break;
        case GCODE_PING:
            result = execute_ping();
            break;
        case GCODE_VERSION:
            result = execute_version();
            break;
        default:
            set_error("Unsupported command");
            result = false;
            break;
    }
    
    busy = false;
    return result;
}

// =============================================================================
// Send response
// =============================================================================
void GCodeInterface::send_response(const char* response) {
    printf("RESPONSE: %s\n", response);
    // Send via UART
    uart_puts(PI_UART_ID, response);
    uart_puts(PI_UART_ID, "\n");
}

// =============================================================================
// Send error
// =============================================================================
void GCodeInterface::send_error(const char* error) {
    printf("ERROR: %s\n", error);
    // Send via UART
    uart_puts(PI_UART_ID, "ERROR_");
    uart_puts(PI_UART_ID, error);
    uart_puts(PI_UART_ID, "\n");
}

// =============================================================================
// Check if busy
// =============================================================================
bool GCodeInterface::is_busy() const {
    return busy;
}

// =============================================================================
// Get current command
// =============================================================================
GCodeType GCodeInterface::get_current_command() const {
    return current_command;
}

// =============================================================================
// Get parameters
// =============================================================================
const GCodeParams& GCodeInterface::get_params() const {
    return params;
}

// =============================================================================
// Set error
// =============================================================================
void GCodeInterface::set_error(const char* error) {
    strncpy(last_error, error, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
    error_state = true;
    log_error(error);
}

// =============================================================================
// Get last error
// =============================================================================
const char* GCodeInterface::get_last_error() const {
    return last_error;
}

// =============================================================================
// Clear error
// =============================================================================
void GCodeInterface::clear_error() {
    last_error[0] = '\0';
    error_state = false;
}

// =============================================================================
// Parse G command
// =============================================================================
bool GCodeInterface::parse_g_command(const char* cmd) {
    // Implementation for G commands
    return true;
}

// =============================================================================
// Parse M command
// =============================================================================
bool GCodeInterface::parse_m_command(const char* cmd) {
    // Implementation for M commands
    return true;
}

// =============================================================================
// Parse parameters
// =============================================================================
bool GCodeInterface::parse_parameters(const char* cmd) {
    if (!cmd) return true;
    
    // Skip whitespace
    while (*cmd == ' ') cmd++;
    
    // Parse parameters
    while (*cmd) {
        if (*cmd == 'X') {
            params.X = parse_float(cmd + 1);
            params.has_X = true;
        }
        else if (*cmd == 'Y') {
            params.Y = parse_float(cmd + 1);
            params.has_Y = true;
        }
        else if (*cmd == 'Z') {
            params.Z = parse_float(cmd + 1);
            params.has_Z = true;
        }
        else if (*cmd == 'F') {
            params.F = parse_float(cmd + 1);
            params.has_F = true;
        }
        else if (*cmd == 'S') {
            params.S = parse_float(cmd + 1);
            params.has_S = true;
        }
        else if (*cmd == 'P') {
            params.P = parse_float(cmd + 1);
            params.has_P = true;
        }
        
        // Move to next parameter
        while (*cmd && *cmd != ' ') cmd++;
        while (*cmd == ' ') cmd++;
    }
    
    return true;
}

// =============================================================================
// Parse float value
// =============================================================================
float GCodeInterface::parse_float(const char* str) {
    return (float)atof(str);
}

// =============================================================================
// Execute G0/G1 (rapid/linear move) - Klipper-style
// =============================================================================
bool GCodeInterface::execute_g0_g1(const char* command) {
    double target_y = current_y;
    double feedrate = current_feedrate;
    
    parse_parameters(command, "YF", &target_y, &feedrate);
    
    if (feedrate > 0.0) current_feedrate = feedrate;
    
    double distance_mm = fabs(target_y - current_y);
    if (distance_mm < 0.001) return true;
    
    // Convert to steps
    uint32_t total_steps = (uint32_t)(distance_mm * Y_STEPS_PER_MM);
    double velocity_mms = current_feedrate / 60.0;
    double cruise_velocity_sps = velocity_mms * Y_STEPS_PER_MM;
    double accel_sps2 = Y_MAX_ACCEL * Y_STEPS_PER_MM;
    
    // Generate step chunks
    std::vector<StepChunk> chunks;
    StepCompressor::compress_trapezoid_into(
        chunks, total_steps, 0.0, cruise_velocity_sps, accel_sps2);
    
    // Set direction
    bool dir = (target_y > current_y);
    winding_controller->set_traverse_direction(dir);
    
    // Push to queue
    for (const auto& chunk : chunks) {
        if (!move_queue->push_chunk(AXIS_TRAVERSE, chunk)) {
            printf("ERROR: Move queue full\n");
            return false;
        }
    }
    
    current_y = target_y;
    printf("✓ G1 Y%.3f queued (%zu chunks)\n", target_y, chunks.size());
    return true;
}

// =============================================================================
// Execute G28 (home) - Klipper-style
// =============================================================================
bool GCodeInterface::execute_g28() {
    extern WindingController* winding_controller;
    
    if (!winding_controller) {
        set_error("Winding controller not initialized");
        return false;
    }
    
    // Use Klipper-style homing via WindingController
    winding_controller->home_all_axes();
    send_response("OK");
    return true;
}

// =============================================================================
// Execute M3/M4 (spindle CW/CCW)
// =============================================================================
bool GCodeInterface::execute_m3_m4() {
    extern BLDC_MOTOR* spindle_controller;
    
    if (!spindle_controller) {
        set_error("Spindle controller not initialized");
        return false;
    }
    
    if (params.has_S) {
        // Set spindle speed and direction
        if (current_command == GCODE_M3) {
            spindle_controller->set_direction(DIRECTION_CW);
        } else {
            spindle_controller->set_direction(DIRECTION_CCW);
        }
        
        // Convert RPM to PWM duty cycle
        float rpm = params.S;
        if (rpm < 0 || rpm > 3000) {
            set_error("RPM out of range (0-3000)");
            return false;
        }
        
        float duty_cycle = (rpm / 3000.0f) * 100.0f;
        if (duty_cycle > 100.0f) duty_cycle = 100.0f;
        
        // Set PWM duty cycle
        uint32_t pwm_value = (uint32_t)(duty_cycle * 65535.0f / 100.0f);
        pwm_set_gpio_level(SPINDLE_PWM_PIN, pwm_value);
        
        send_response("OK");
        return true;
    } else {
        set_error("No speed specified");
        return false;
    }
}

// =============================================================================
// Execute M5 (spindle stop)
// =============================================================================
bool GCodeInterface::execute_m5() {
    extern BLDC_MOTOR* spindle_controller;
    
    if (!spindle_controller) {
        set_error("Spindle controller not initialized");
        return false;
    }
    
    // Stop spindle PWM
    pwm_set_gpio_level(SPINDLE_PWM_PIN, 0);
    
    // Stop spindle
    spindle_controller->set_brake(true);
    send_response("STOPPED");
    return true;
}

// =============================================================================
// Execute S (set spindle speed)
// =============================================================================
bool GCodeInterface::execute_s() {
    if (params.has_S) {
        // Set spindle speed via PWM
        float rpm = params.S;
        if (rpm < 0 || rpm > 3000) {
            set_error("RPM out of range (0-3000)");
            return false;
        }
        
        // Convert RPM to PWM duty cycle
        float duty_cycle = (rpm / 3000.0f) * 100.0f;
        if (duty_cycle > 100.0f) duty_cycle = 100.0f;
        
        // Set PWM duty cycle
        uint32_t pwm_value = (uint32_t)(duty_cycle * 65535.0f / 100.0f);
        pwm_set_gpio_level(SPINDLE_PWM_PIN, pwm_value);
        
        send_response("OK");
        return true;
    }
    
    set_error("No speed specified");
    return false;
}

// =============================================================================
// Execute M6 (tool change)
// =============================================================================
bool GCodeInterface::execute_m6() {
    send_response("Tool change required");
    return true;
}

// =============================================================================
// Execute M7/M8/M9 (coolant)
// =============================================================================
bool GCodeInterface::execute_m7_m8_m9() {
    send_response("OK");
    return true;
}

// =============================================================================
// Execute M10/M11 (traverse brake) - Klipper-style
// =============================================================================
bool GCodeInterface::execute_m10_m11() {
    extern MoveQueue* move_queue;
    
    if (!move_queue) {
        set_error("Move queue not initialized");
        return false;
    }
    
    bool enable = (current_command == GCODE_M10);
    // Use Klipper-style move queue for brake control
    move_queue->set_enable(AXIS_TRAVERSE, !enable); // Brake = disable motor
    send_response(enable ? "Traverse brake engaged" : "Traverse brake released");
    return true;
}

// =============================================================================
// Execute M12/M13 (spindle brake)
// =============================================================================
bool GCodeInterface::execute_m12_m13() {
    send_response(current_command == GCODE_M12 ? "Spindle brake engaged" : "Spindle brake released");
    return true;
}

// =============================================================================
// Execute M14/M15 (wire tension)
// =============================================================================
bool GCodeInterface::execute_m14_m15() {
    send_response(current_command == GCODE_M14 ? "Wire tension enabled" : "Wire tension disabled");
    return true;
}

// =============================================================================
// Execute M16 (home all axes) - Klipper-style
// =============================================================================
bool GCodeInterface::execute_m16() {
    extern WindingController* winding_controller;
    
    if (!winding_controller) {
        set_error("Winding controller not initialized");
        return false;
    }
    
    // Use Klipper-style homing via WindingController
    winding_controller->home_all_axes();
    send_response("All axes homed");
    return true;
}

// =============================================================================
// Execute M17/M18 (enable/disable steppers) - Klipper-style
// =============================================================================
bool GCodeInterface::execute_m17_m18() {
    extern MoveQueue* move_queue;
    
    if (!move_queue) {
        set_error("Move queue not initialized");
        return false;
    }
    
    if (current_command == GCODE_M17) {
        // Enable both axes
        move_queue->set_enable(AXIS_SPINDLE, true);
        move_queue->set_enable(AXIS_TRAVERSE, true);
        send_response("Steppers enabled");
    } else {
        // Disable both axes
        move_queue->set_enable(AXIS_SPINDLE, false);
        move_queue->set_enable(AXIS_TRAVERSE, false);
        send_response("Steppers disabled");
    }
    
    return true;
}

// =============================================================================
// Execute M19 (spindle orientation)
// =============================================================================
bool GCodeInterface::execute_m19() {
    send_response("Spindle orientation enabled");
    return true;
}

// =============================================================================
// Execute M42 (set pin state)
// =============================================================================
bool GCodeInterface::execute_m42() {
    if (params.has_P && params.has_S) {
        // Set pin state
        send_response("OK");
        return true;
    }
    
    set_error("Missing P or S parameter");
    return false;
}

// =============================================================================
// Execute M47 (set pin value)
// =============================================================================
bool GCodeInterface::execute_m47() {
    if (params.has_P && params.has_S) {
        // Set pin value
        send_response("OK");
        return true;
    }
    
    set_error("Missing P or S parameter");
    return false;
}

// =============================================================================
// Execute PING
// =============================================================================
bool GCodeInterface::execute_ping() {
    send_response("PONG");
    return true;
}

// =============================================================================
// Execute VERSION
// =============================================================================
bool GCodeInterface::execute_version() {
    send_response("Pico_Spindle_v1.0");
    return true;
}

// =============================================================================
// Log command
// =============================================================================
void GCodeInterface::log_command(const char* cmd) {
    printf("[GCode] %s\n", cmd);
}

// =============================================================================
// Log error
// =============================================================================
void GCodeInterface::log_error(const char* error) {
    printf("[GCode ERROR] %s\n", error);
}
