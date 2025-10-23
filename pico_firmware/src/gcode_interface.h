// =============================================================================
// gcode_interface.h - G-code Interface for Pico
// Purpose: Parse and execute G-code commands from Pi Zero
// =============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>

// =============================================================================
// G-code Command Types
// =============================================================================
enum GCodeType {
    GCODE_UNKNOWN = 0,
    GCODE_G0,      // Rapid positioning
    GCODE_G1,      // Linear interpolation
    GCODE_G28,     // Home
    GCODE_M3,      // Spindle CW
    GCODE_M4,      // Spindle CCW
    GCODE_M5,      // Spindle stop
    GCODE_S,       // Set spindle speed
    GCODE_M6,      // Tool change
    GCODE_M7,      // Coolant on
    GCODE_M8,      // Coolant off
    GCODE_M9,      // Coolant off
    GCODE_M10,     // Traverse brake on
    GCODE_M11,     // Traverse brake off
    GCODE_M12,     // Spindle brake on
    GCODE_M13,     // Spindle brake off
    GCODE_M14,     // Wire tension on
    GCODE_M15,     // Wire tension off
    GCODE_M16,     // Home all axes
    GCODE_M17,     // Enable steppers
    GCODE_M18,     // Disable steppers
    GCODE_M19,     // Spindle orientation
    GCODE_M42,     // Set pin state
    GCODE_M47,     // Set pin value
    GCODE_PING,    // Ping command
    GCODE_VERSION, // Version command
    GCODE_STATUS,  // Status command
    GCODE_ERROR    // Error state
};

// =============================================================================
// Token-based G-code Parsing (from Code-snippets improvement)
// =============================================================================
enum GCodeTokenType {
    TOKEN_G0 = 0,      // Rapid positioning
    TOKEN_G1 = 1,      // Linear interpolation
    TOKEN_G28 = 2,     // Home
    TOKEN_M3 = 3,      // Spindle CW
    TOKEN_M4 = 4,      // Spindle CCW
    TOKEN_M5 = 5,      // Spindle stop
    TOKEN_S = 6,       // Set spindle speed
    TOKEN_M6 = 7,      // Tool change
    TOKEN_M7 = 8,      // Coolant on
    TOKEN_M8 = 9,      // Coolant off
    TOKEN_M9 = 10,     // Coolant off
    TOKEN_M10 = 11,    // Traverse brake on
    TOKEN_M11 = 12,    // Traverse brake off
    TOKEN_M12 = 13,    // Spindle brake on
    TOKEN_M13 = 14,    // Spindle brake off
    TOKEN_M14 = 15,    // Wire tension on
    TOKEN_M15 = 16,    // Wire tension off
    TOKEN_M16 = 17,    // Home all axes
    TOKEN_M17 = 18,    // Enable steppers
    TOKEN_M18 = 19,    // Disable steppers
    TOKEN_M19 = 20,    // Spindle orientation
    TOKEN_M42 = 21,    // Set pin state
    TOKEN_M47 = 22,    // Set pin value
    TOKEN_PING = 23,   // Ping command
    TOKEN_VERSION = 24, // Version command
    TOKEN_UNKNOWN = 255
};

// =============================================================================
// G-code Parameters
// =============================================================================
struct GCodeParams {
    float X, Y, Z;      // Position coordinates
    float F;            // Feed rate
    float S;            // Spindle speed
    float P;            // Parameter P
    bool has_X, has_Y, has_Z, has_F, has_S, has_P;
    
    GCodeParams() : X(0), Y(0), Z(0), F(0), S(0), P(0),
                    has_X(false), has_Y(false), has_Z(false),
                    has_F(false), has_S(false), has_P(false) {}
};

// =============================================================================
// G-code Interface Class
// =============================================================================
class GCodeInterface {
public:
    GCodeInterface();
    ~GCodeInterface();
    
    // Command processing
    bool parse_command(const char* command);
    bool execute_command();
    void send_response(const char* response);
    void send_error(const char* error);
    
    // Status
    bool is_busy() const;
    GCodeType get_current_command() const;
    const GCodeParams& get_params() const;
    
    // Error handling
    void set_error(const char* error);
    const char* get_last_error() const;
    void clear_error();
    
private:
    // Current command state
    GCodeType current_command;
    GCodeParams params;
    char command_buffer[256];
    char last_error[128];
    
    // Status
    bool busy;
    bool error_state;
    
    // Command parsing
    bool parse_g_command(const char* cmd);
    bool parse_m_command(const char* cmd);
    bool parse_parameters(const char* cmd);
    float parse_float(const char* str);
    
    // Token-based parsing (from Code-snippets improvement)
    GCodeTokenType parse_token(const char* command);
    bool parse_parameters_tokenized(const char* cmd);
    bool validate_parameters();
    
    // Command execution
    bool execute_g0_g1();
    bool execute_g28();
    bool execute_m3_m4();
    bool execute_m5();
    bool execute_s();
    bool execute_m6();
    bool execute_m7_m8_m9();
    bool execute_m10_m11();
    bool execute_m12_m13();
    bool execute_m14_m15();
    bool execute_m16();
    bool execute_m17_m18();
    bool execute_m19();
    bool execute_m42();
    bool execute_m47();
    bool execute_ping();
    bool execute_version();
    bool execute_status();
    
    // Helper functions
    void log_command(const char* cmd);
    void log_error(const char* error);
};
