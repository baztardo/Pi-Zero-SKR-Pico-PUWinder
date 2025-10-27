// =============================================================================
// communication_handler.h - UART Communication Handler
// Purpose: Handle UART communication, buffering, and command routing
// =============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
#include "config.h"

// Forward declarations
class GCodeInterface;

class CommunicationHandler {
public:
    /**
     * @brief Constructor
     * @param gcode_interface Reference to G-code interface for command processing
     */
    CommunicationHandler(GCodeInterface* gcode_interface);
    
    /**
     * @brief Destructor
     */
    ~CommunicationHandler();
    
    /**
     * @brief Initialize UART communication
     * @return true if successful, false otherwise
     */
    bool init();
    
    /**
     * @brief Update communication handler (call from main loop)
     * Handles UART reading, buffering, and command processing
     */
    void update();
    
    /**
     * @brief Send response over UART
     * @param response Response string to send
     */
    void send_response(const char* response);
    
    /**
     * @brief Send error over UART
     * @param error Error string to send
     */
    void send_error(const char* error);
    
    /**
     * @brief Check if UART is initialized
     * @return true if initialized
     */
    bool is_initialized() const { return initialized; }
    
    /**
     * @brief Get number of commands processed
     * @return Command count
     */
    uint32_t get_command_count() const { return command_count; }

private:
    GCodeInterface* gcode_interface;
    bool initialized;
    uint32_t command_count;
    
    // UART buffer management
    char command_buffer[256];
    int buffer_pos;
    
    /**
     * @brief Process a complete command
     * @param command Command string to process
     */
    void process_command(const char* command);
};
