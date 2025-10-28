// =============================================================================
// gcode_interface.h - G-code Interface for Pico
// Purpose: Parse and execute G-code commands from Pi Zero
// =============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>

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
    TOKEN_M112 = 6,    // Emergency stop
    TOKEN_S = 7,       // Set spindle speed
    TOKEN_M6 = 8,      // Tool change
    TOKEN_M7 = 9,      // Coolant on
    TOKEN_M8 = 10,     // Coolant off
    TOKEN_M9 = 11,     // Coolant off
    TOKEN_M10 = 12,    // Traverse brake on
    TOKEN_M11 = 13,    // Traverse brake off
    TOKEN_M12 = 14,    // Spindle brake on
    TOKEN_M13 = 15,    // Spindle brake off
    TOKEN_M14 = 16,    // Wire tension on
    TOKEN_M15 = 17,    // Wire tension off
    TOKEN_M16 = 18,    // Home all axes
    TOKEN_M17 = 19,    // Enable steppers
    TOKEN_M18 = 20,    // Disable steppers
    TOKEN_M19 = 21,    // Spindle orientation
    TOKEN_M42 = 22,    // Set pin state
    TOKEN_M47 = 23,    // Set pin value
    TOKEN_PING = 24,   // Ping command
    TOKEN_VERSION = 25, // Version command
    TOKEN_STATUS = 26,  // Status command
    // ⭐ NEW: FluidNC-style Safety Commands
    TOKEN_M0 = 27,     // Feed hold
    TOKEN_M1 = 28,     // Resume from hold
    TOKEN_M410 = 29,   // Quick stop
    TOKEN_M999 = 30,   // Reset from emergency stop
    TOKEN_G4 = 31,     // Dwell with planner sync
    TOKEN_GET_HALL_RPM = 32,  // Get hall sensor RPM
    TOKEN_CHECK_HALL = 33,    // Check hall sensor pin state
    TOKEN_WIND = 34,          // Start winding sequence
    TOKEN_PAUSE_WIND = 35,    // Pause winding
    TOKEN_RESUME_WIND = 36,   // Resume winding
    TOKEN_STOP_WIND = 37,     // Stop winding
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
    // Winding parameters
    float T;            // Target turns
    float W;            // Wire diameter (mm)
    float B;            // Bobbin width (mm)
    float O;            // Offset from edge (mm)
    bool has_X, has_Y, has_Z, has_F, has_S, has_P;
    bool has_T, has_W, has_B, has_O;
    
    GCodeParams() : X(0), Y(0), Z(0), F(0), S(0), P(0), T(1000), W(0.064), B(12.0), O(22.0),
                    has_X(false), has_Y(false), has_Z(false),
                    has_F(false), has_S(false), has_P(false),
                    has_T(false), has_W(false), has_B(false), has_O(false) {}
};

// =============================================================================
// G-code Interface Class
// =============================================================================
// Forward declarations
class BLDC_MOTOR;
class TraverseController;
class MoveQueue;
class WindingController;
class CommunicationHandler;

class GCodeInterface {
public:
    GCodeInterface(BLDC_MOTOR* spindle, TraverseController* traverse, MoveQueue* queue, WindingController* winding);
    ~GCodeInterface();
    
    // Command processing
    bool parse_command(const char* command);
    bool execute_command();
    void process_command(const char* command);  // Combined parse + execute
    
    // Communication setup
    void set_communication_handler(CommunicationHandler* comm_handler);
    
    // Response methods (now use CommunicationHandler)
    void send_response(const char* response);
    void send_error(const char* error);
    
    // Status
    bool is_busy() const;
    GCodeTokenType get_current_command() const;
    const GCodeParams& get_params() const;
    
    // Error handling
    void set_error(const char* error);
    const char* get_last_error() const;
    void clear_error();
    
private:
    // Controller references
    BLDC_MOTOR* spindle_controller;
    TraverseController* traverse_controller;
    MoveQueue* move_queue;
    WindingController* winding_controller;
    CommunicationHandler* communication_handler;
    
    // Current command state
    GCodeTokenType current_command;
    GCodeParams params;
    char command_buffer[256];
    char last_error[128];
    
    // Status
    bool busy;
    bool error_state;
    
    // Token-based parsing (from Code-snippets improvement)
    GCodeTokenType parse_token(const char* command);
    bool parse_parameters_tokenized(const char* cmd);
    bool validate_parameters();
    
    // ⭐ NEW: FluidNC-style enhanced validation
    bool validate_coordinate_ranges();
    bool validate_feed_rate();
    bool validate_spindle_speed();
    bool validate_pin_number();
    float parse_float(const char* str);
    
    // Command execution
    bool execute_g0_g1();
    bool execute_g28();
    bool execute_m3_m4();
    bool execute_m5();
    bool execute_m112();
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
    bool execute_get_hall_rpm();
    bool execute_check_hall();
    
    // ⭐ NEW: FluidNC-style Safety Commands
    bool execute_m0();     // Feed hold
    bool execute_m1();      // Resume from hold
    bool execute_m410();    // Quick stop
    bool execute_m999();    // Reset from emergency stop
    bool execute_g4();      // Dwell with planner sync
    
    // Winding sequence commands
    bool execute_wind();    // Start winding sequence
    bool execute_pause_wind();  // Pause winding
    bool execute_resume_wind(); // Resume winding
    bool execute_stop_wind();   // Stop winding
    
    // Helper functions
    void log_command(const char* cmd);
    void log_error(const char* error);
};
