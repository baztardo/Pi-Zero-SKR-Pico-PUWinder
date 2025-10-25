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
    : current_command(TOKEN_UNKNOWN)
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
        current_command = (command[1] == '0') ? TOKEN_G0 : TOKEN_G1;
        return parse_parameters_tokenized(command + 2);
    }
    else if (strncmp(command, "G28", 3) == 0) {
        current_command = TOKEN_G28;
        return parse_parameters_tokenized(command + 3);
    }
    else if (strncmp(command, "M3", 2) == 0) {
        current_command = TOKEN_M3;
        return parse_parameters_tokenized(command + 2);
    }
    else if (strncmp(command, "M4", 2) == 0) {
        current_command = TOKEN_M4;
        return parse_parameters_tokenized(command + 2);
    }
    else if (strncmp(command, "M5", 2) == 0) {
        current_command = TOKEN_M5;
        return true;
    }
    // Handle special commands (from Code-snippets improvement)
    else if (strcmp(command, "PING") == 0) {
        current_command = TOKEN_PING;
        return true;
    }
    else if (strcmp(command, "VERSION") == 0) {
        current_command = TOKEN_VERSION;
        return true;
    }
    else if (strcmp(command, "STATUS") == 0) {
        current_command = TOKEN_STATUS;
        return true;
    }
    else if (command[0] == 'S') {
        current_command = TOKEN_S;
        return parse_parameters_tokenized(command + 1);
    }
    else if (strncmp(command, "M6", 2) == 0) {
        current_command = TOKEN_M6;
        return true;
    }
    else if (strncmp(command, "M7", 2) == 0) {
        current_command = TOKEN_M7;
        return true;
    }
    else if (strncmp(command, "M8", 2) == 0) {
        current_command = TOKEN_M8;
        return true;
    }
    else if (strncmp(command, "M9", 2) == 0) {
        current_command = TOKEN_M9;
        return true;
    }
    else if (strncmp(command, "M10", 3) == 0) {
        current_command = TOKEN_M10;
        return true;
    }
    else if (strncmp(command, "M11", 3) == 0) {
        current_command = TOKEN_M11;
        return true;
    }
    else if (strncmp(command, "M12", 3) == 0) {
        current_command = TOKEN_M12;
        return true;
    }
    else if (strncmp(command, "M13", 3) == 0) {
        current_command = TOKEN_M13;
        return true;
    }
    else if (strncmp(command, "M14", 3) == 0) {
        current_command = TOKEN_M14;
        return true;
    }
    else if (strncmp(command, "M15", 3) == 0) {
        current_command = TOKEN_M15;
        return true;
    }
    else if (strncmp(command, "M16", 3) == 0) {
        current_command = TOKEN_M16;
        return true;
    }
    else if (strncmp(command, "M17", 3) == 0) {
        current_command = TOKEN_M17;
        return true;
    }
    else if (strncmp(command, "M18", 3) == 0) {
        current_command = TOKEN_M18;
        return true;
    }
    else if (strncmp(command, "M19", 3) == 0) {
        current_command = TOKEN_M19;
        return true;
    }
    else if (strncmp(command, "M42", 3) == 0) {
        current_command = TOKEN_M42;
        return parse_parameters_tokenized(command + 3);
    }
    else if (strncmp(command, "M47", 3) == 0) {
        current_command = TOKEN_M47;
        return parse_parameters_tokenized(command + 3);
    }
    else if (strcmp(command, "PING") == 0) {
        current_command = TOKEN_PING;
        return true;
    }
    else if (strcmp(command, "VERSION") == 0) {
        current_command = TOKEN_VERSION;
        return true;
    }
    else {
        current_command = TOKEN_UNKNOWN;
        set_error("Unknown command");
        return false;
    }
}

// =============================================================================
// Execute command
// =============================================================================
bool GCodeInterface::execute_command() {
    if (current_command == TOKEN_UNKNOWN) {
        set_error("No command to execute");
        return false;
    }
    
    busy = true;
    clear_error();
    
    bool result = false;
    
    switch (current_command) {
        case TOKEN_G0:
        case TOKEN_G1:
            result = execute_g0_g1();
            break;
        case TOKEN_G28:
            result = execute_g28();
            break;
        case TOKEN_M3:
        case TOKEN_M4:
            result = execute_m3_m4();
            break;
        case TOKEN_M5:
            result = execute_m5();
            break;
        case TOKEN_S:
            result = execute_s();
            break;
        case TOKEN_M6:
            result = execute_m6();
            break;
        case TOKEN_M7:
        case TOKEN_M8:
        case TOKEN_M9:
            result = execute_m7_m8_m9();
            break;
        case TOKEN_M10:
        case TOKEN_M11:
            result = execute_m10_m11();
            break;
        case TOKEN_M12:
        case TOKEN_M13:
            result = execute_m12_m13();
            break;
        case TOKEN_M14:
        case TOKEN_M15:
            result = execute_m14_m15();
            break;
        case TOKEN_M16:
            result = execute_m16();
            break;
        case TOKEN_M17:
        case TOKEN_M18:
            result = execute_m17_m18();
            break;
        case TOKEN_M19:
            result = execute_m19();
            break;
        case TOKEN_M42:
            result = execute_m42();
            break;
        case TOKEN_M47:
            result = execute_m47();
            break;
        case TOKEN_PING:
            result = execute_ping();
            break;
        case TOKEN_VERSION:
            result = execute_version();
            break;
        case TOKEN_STATUS:
            result = execute_status();
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
GCodeTokenType GCodeInterface::get_current_command() const {
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
// Old parse_g_command removed - using token-based parsing instead
// =============================================================================

// =============================================================================
// Old parse_m_command removed - using token-based parsing instead
// =============================================================================

// =============================================================================
// Old parse_parameters removed - using token-based parsing instead
// =============================================================================

// =============================================================================
// Parse float value
// =============================================================================
float GCodeInterface::parse_float(const char* str) {
    return (float)atof(str);
}

// =============================================================================
// Execute G0/G1 (rapid/linear move) - Token-based
// =============================================================================
bool GCodeInterface::execute_g0_g1() {
    extern TraverseController* traverse_controller;
    extern MoveQueue* move_queue;
    
    if (!traverse_controller || !move_queue) {
        set_error("Traverse controller or move queue not initialized");
        return false;
    }
    
    // TODO: Implement proper traverse movement using StepCompressor
    // For now, just move to the target position
    if (params.has_Y) {
        traverse_controller->move_to_position(params.Y);
        send_response("OK");
        return true;
    }
    
    set_error("G0/G1 requires Y parameter");
    return false;
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
        if (current_command == TOKEN_M3) {
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
    
    bool enable = (current_command == TOKEN_M10);
    // Use Klipper-style move queue for brake control
    move_queue->set_enable(AXIS_TRAVERSE, !enable); // Brake = disable motor
    send_response(enable ? "Traverse brake engaged" : "Traverse brake released");
    return true;
}

// =============================================================================
// Execute M12/M13 (spindle brake)
// =============================================================================
bool GCodeInterface::execute_m12_m13() {
    send_response(current_command == TOKEN_M12 ? "Spindle brake engaged" : "Spindle brake released");
    return true;
}

// =============================================================================
// Execute M14/M15 (wire tension)
// =============================================================================
bool GCodeInterface::execute_m14_m15() {
    send_response(current_command == TOKEN_M14 ? "Wire tension enabled" : "Wire tension disabled");
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
    
    if (current_command == TOKEN_M17) {
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
// Execute STATUS (from Code-snippets improvement)
// =============================================================================
bool GCodeInterface::execute_status() {
    extern BLDC_MOTOR* spindle_controller;
    extern TraverseController* traverse_controller;
    
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

// =============================================================================
// Token-based Parsing (from Code-snippets improvement)
// =============================================================================
GCodeTokenType GCodeInterface::parse_token(const char* command) {
    if (!command) return TOKEN_UNKNOWN;
    
    // Skip whitespace
    while (*command == ' ' || *command == '\t') command++;
    
    // Parse G commands
    if (command[0] == 'G') {
        if (strncmp(command, "G0", 2) == 0) return TOKEN_G0;
        if (strncmp(command, "G1", 2) == 0) return TOKEN_G1;
        if (strncmp(command, "G28", 3) == 0) return TOKEN_G28;
    }
    
    // Parse M commands
    if (command[0] == 'M') {
        if (strncmp(command, "M3", 2) == 0) return TOKEN_M3;
        if (strncmp(command, "M4", 2) == 0) return TOKEN_M4;
        if (strncmp(command, "M5", 2) == 0) return TOKEN_M5;
        if (strncmp(command, "M6", 2) == 0) return TOKEN_M6;
        if (strncmp(command, "M7", 2) == 0) return TOKEN_M7;
        if (strncmp(command, "M8", 2) == 0) return TOKEN_M8;
        if (strncmp(command, "M9", 2) == 0) return TOKEN_M9;
        if (strncmp(command, "M10", 3) == 0) return TOKEN_M10;
        if (strncmp(command, "M11", 3) == 0) return TOKEN_M11;
        if (strncmp(command, "M12", 3) == 0) return TOKEN_M12;
        if (strncmp(command, "M13", 3) == 0) return TOKEN_M13;
        if (strncmp(command, "M14", 3) == 0) return TOKEN_M14;
        if (strncmp(command, "M15", 3) == 0) return TOKEN_M15;
        if (strncmp(command, "M16", 3) == 0) return TOKEN_M16;
        if (strncmp(command, "M17", 3) == 0) return TOKEN_M17;
        if (strncmp(command, "M18", 3) == 0) return TOKEN_M18;
        if (strncmp(command, "M19", 3) == 0) return TOKEN_M19;
        if (strncmp(command, "M42", 3) == 0) return TOKEN_M42;
        if (strncmp(command, "M47", 3) == 0) return TOKEN_M47;
    }
    
    // Parse S command
    if (command[0] == 'S') return TOKEN_S;
    
    // Parse special commands
    if (strcmp(command, "PING") == 0) return TOKEN_PING;
    if (strcmp(command, "VERSION") == 0) return TOKEN_VERSION;
    if (strcmp(command, "STATUS") == 0) return TOKEN_VERSION; // Note: STATUS not in token enum
    
    return TOKEN_UNKNOWN;
}

bool GCodeInterface::parse_parameters_tokenized(const char* cmd) {
    if (!cmd) return true;
    
    // Skip whitespace
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    
    // Enhanced token-based parsing with validation
    while (*cmd) {
        // Skip whitespace
        while (*cmd == ' ' || *cmd == '\t') cmd++;
        if (!*cmd) break;
        
        char param = *cmd++;
        float value = parse_float(cmd);
        
        // Enhanced parameter handling with validation
        switch (param) {
            case 'X': 
                params.X = value; 
                params.has_X = true;
                // Clamp to reasonable range
                if (params.X < -1000.0f || params.X > 1000.0f) {
                    set_error("X parameter out of range");
                    return false;
                }
                break;
            case 'Y': 
                params.Y = value; 
                params.has_Y = true;
                // Clamp to reasonable range
                if (params.Y < -1000.0f || params.Y > 1000.0f) {
                    set_error("Y parameter out of range");
                    return false;
                }
                break;
            case 'Z': 
                params.Z = value; 
                params.has_Z = true;
                // Clamp to reasonable range
                if (params.Z < -1000.0f || params.Z > 1000.0f) {
                    set_error("Z parameter out of range");
                    return false;
                }
                break;
            case 'F': 
                params.F = value; 
                params.has_F = true;
                // Clamp feed rate to reasonable range
                if (params.F < 0.1f || params.F > 10000.0f) {
                    set_error("F parameter out of range");
                    return false;
                }
                break;
            case 'S': 
                params.S = value; 
                params.has_S = true;
                // Clamp spindle speed to reasonable range
                if (params.S < 0.0f || params.S > 10000.0f) {
                    set_error("S parameter out of range");
                    return false;
                }
                break;
            case 'P': 
                params.P = value; 
                params.has_P = true;
                // Clamp pin number to reasonable range
                if (params.P < 0.0f || params.P > 40.0f) {
                    set_error("P parameter out of range");
                    return false;
                }
                break;
        }
        
        // Skip to next parameter
        while (*cmd && *cmd != ' ' && *cmd != '\t') cmd++;
    }
    
    return validate_parameters();
}

bool GCodeInterface::validate_parameters() {
    // Validate parameter combinations based on command type
    switch (current_command) {
        case TOKEN_G0:
        case TOKEN_G1:
            // G0/G1 should have at least one coordinate
            if (!params.has_X && !params.has_Y && !params.has_Z) {
                set_error("G0/G1 requires at least one coordinate");
                return false;
            }
            break;
            
        case TOKEN_M3:
        case TOKEN_M4:
            // M3/M4 should have S parameter for speed
            if (!params.has_S) {
                set_error("M3/M4 requires S parameter for speed");
                return false;
            }
            break;
            
        case TOKEN_M42:
            // M42 requires P parameter for pin number
            if (!params.has_P) {
                set_error("M42 requires P parameter for pin number");
                return false;
            }
            break;
            
        default:
            // No specific validation required
            break;
    }
    
    return true;
}

// =============================================================================
// ⭐ NEW: FluidNC-style Enhanced Validation Methods
// =============================================================================

bool GCodeInterface::validate_coordinate_ranges() {
    // Validate coordinate ranges based on machine limits
    if (params.has_X && (params.X < -1000.0f || params.X > 1000.0f)) {
        set_error("X coordinate out of range (-1000 to 1000)");
        return false;
    }
    if (params.has_Y && (params.Y < -1000.0f || params.Y > 1000.0f)) {
        set_error("Y coordinate out of range (-1000 to 1000)");
        return false;
    }
    if (params.has_Z && (params.Z < -1000.0f || params.Z > 1000.0f)) {
        set_error("Z coordinate out of range (-1000 to 1000)");
        return false;
    }
    return true;
}

bool GCodeInterface::validate_feed_rate() {
    if (params.has_F && (params.F < 0.1f || params.F > 10000.0f)) {
        set_error("Feed rate out of range (0.1 to 10000 mm/min)");
        return false;
    }
    return true;
}

bool GCodeInterface::validate_spindle_speed() {
    if (params.has_S && (params.S < 0.0f || params.S > 10000.0f)) {
        set_error("Spindle speed out of range (0 to 10000 RPM)");
        return false;
    }
    return true;
}

bool GCodeInterface::validate_pin_number() {
    if (params.has_P && (params.P < 0.0f || params.P > 40.0f)) {
        set_error("Pin number out of range (0 to 40)");
        return false;
    }
    return true;
}
