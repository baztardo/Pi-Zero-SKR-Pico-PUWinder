// =============================================================================
// gcode_parser_improved.h - Klipper-Style Improved G-code Parser
// Purpose: Token-based parsing with lookup tables for efficiency
// =============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <unordered_map>

// =============================================================================
// G-code Token Types
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
// G-code Parameter Structure
// =============================================================================
struct GCodeParamsImproved {
    float X, Y, Z;      // Position coordinates
    float F;            // Feed rate
    float S;            // Spindle speed
    float P;            // Parameter P
    bool has_X, has_Y, has_Z, has_F, has_S, has_P;
    
    GCodeParamsImproved() : X(0), Y(0), Z(0), F(0), S(0), P(0),
                           has_X(false), has_Y(false), has_Z(false),
                           has_F(false), has_S(false), has_P(false) {}
};

// =============================================================================
// G-code Token Structure
// =============================================================================
struct GCodeToken {
    GCodeTokenType type;
    GCodeParamsImproved params;
    const char* original_command;
    size_t command_length;
};

// =============================================================================
// Improved G-code Parser (Klipper-style)
// =============================================================================
class GCodeParserImproved {
public:
    /**
     * @brief Constructor
     */
    GCodeParserImproved();
    
    /**
     * @brief Destructor
     */
    ~GCodeParserImproved();
    
    /**
     * @brief Parse G-code command into tokens
     * @param command G-code command string
     * @param token Output token structure
     * @return true if successful
     */
    bool parse_command(const char* command, GCodeToken& token);
    
    /**
     * @brief Get command type from string
     * @param command Command string
     * @return Command type
     */
    GCodeTokenType get_command_type(const char* command);
    
    /**
     * @brief Parse parameters from command
     * @param command Command string
     * @param params Output parameters
     * @return true if successful
     */
    bool parse_parameters(const char* command, GCodeParamsImproved& params);
    
    /**
     * @brief Validate command syntax
     * @param command Command string
     * @return true if valid
     */
    bool validate_command(const char* command);
    
    /**
     * @brief Get error message
     * @return Error message string
     */
    const char* get_error_message() const;
    
    /**
     * @brief Clear error state
     */
    void clear_error();

private:
    // Command lookup table for fast parsing
    std::unordered_map<std::string, GCodeTokenType> command_lookup;
    
    // Error handling
    char error_message[128];
    bool has_error;
    
    /**
     * @brief Initialize command lookup table
     */
    void initialize_command_lookup();
    
    /**
     * @brief Parse coordinate parameter
     * @param command Command string
     * @param coord Coordinate character (X, Y, Z, F, S, P)
     * @param value Output value
     * @return true if found
     */
    bool parse_coordinate(const char* command, char coord, float& value);
    
    /**
     * @brief Skip whitespace
     * @param str String pointer
     * @return Pointer to first non-whitespace character
     */
    const char* skip_whitespace(const char* str);
    
    /**
     * @brief Parse float value
     * @param str String to parse
     * @param value Output value
     * @return true if successful
     */
    bool parse_float(const char* str, float& value);
    
    /**
     * @brief Set error message
     * @param message Error message
     */
    void set_error(const char* message);
};
