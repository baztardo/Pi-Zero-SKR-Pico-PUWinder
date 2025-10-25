// =============================================================================
// gcode_interface.cpp - Safety Command Additions
// =============================================================================

// ⭐ ADD THESE NEW COMMAND HANDLERS TO YOUR gcode_interface.cpp:

// =============================================================================
// Execute M0 (Pause / Feed Hold)
// =============================================================================
bool GCodeInterface::execute_m0() {
    extern MoveQueue* move_queue;
    
    if (!move_queue) {
        set_error("Move queue not initialized");
        return false;
    }
    
    printf("[M0] Feed hold requested\n");
    move_queue->pause_feeding();
    
    send_response("PAUSED");
    return true;
}

// =============================================================================
// Execute M1 (Resume from hold)
// =============================================================================
bool GCodeInterface::execute_m1() {
    extern MoveQueue* move_queue;
    extern volatile bool g_emergency_stop;
    
    if (!move_queue) {
        set_error("Move queue not initialized");
        return false;
    }
    
    // Don't allow resume if emergency stop is active
    if (g_emergency_stop) {
        set_error("Cannot resume - emergency stop active. Use M999 to reset");
        return false;
    }
    
    printf("[M1] Resume requested\n");
    move_queue->resume_feeding();
    
    send_response("RESUMED");
    return true;
}

// =============================================================================
// Execute M112 (Emergency Stop via G-code)
// =============================================================================
bool GCodeInterface::execute_m112() {
    extern volatile bool g_emergency_stop;
    extern volatile SystemState g_system_state;
    
    printf("[M112] Emergency stop triggered via G-code\n");
    
    g_emergency_stop = true;
    g_system_state = STATE_ALARM;
    
    send_response("EMERGENCY_STOP");
    return true;
}

// =============================================================================
// Execute M999 (Reset from Emergency Stop)
// =============================================================================
bool GCodeInterface::execute_m999() {
    extern volatile bool g_emergency_stop;
    extern volatile SystemState g_system_state;
    extern MoveQueue* move_queue;
    
    printf("[M999] Reset from emergency stop\n");
    
    // Clear emergency stop
    g_emergency_stop = false;
    g_system_state = STATE_IDLE;
    
    // Re-enable motors
    if (move_queue) {
        move_queue->set_enable(AXIS_TRAVERSE, true);
        move_queue->set_enable(AXIS_SPINDLE, true);
        move_queue->resume_feeding();
    }
    
    printf("[M999] System reset complete\n");
    send_response("RESET_OK");
    return true;
}

// =============================================================================
// Execute M410 (Quick Stop - decelerate and stop)
// =============================================================================
bool GCodeInterface::execute_m410() {
    extern MoveQueue* move_queue;
    
    if (!move_queue) {
        set_error("Move queue not initialized");
        return false;
    }
    
    printf("[M410] Quick stop requested\n");
    
    // Pause feeding new moves
    move_queue->pause_feeding();
    
    // Wait for current moves to finish
    uint32_t start_time = time_us_32();
    while ((move_queue->is_active(AXIS_SPINDLE) || 
            move_queue->is_active(AXIS_TRAVERSE)) &&
           (time_us_32() - start_time) < 5000000) {  // 5 second timeout
        sleep_ms(10);
    }
    
    // Clear any remaining queued moves
    move_queue->clear_queue(AXIS_SPINDLE);
    move_queue->clear_queue(AXIS_TRAVERSE);
    
    send_response("STOPPED");
    return true;
}

// =============================================================================
// ⭐ UPDATED: Execute G4 (Dwell) with Planner Sync
// =============================================================================
bool GCodeInterface::execute_g4() {
    if (!params.has_P) {
        set_error("No delay specified");
        return false;
    }
    
    // G4 P0 = Special case: Sync planner (wait for all moves to complete)
    if (params.P == 0.0f) {
        printf("[G4 P0] Planner sync - waiting for all moves to complete\n");
        
        extern MoveQueue* move_queue;
        if (move_queue) {
            uint32_t start_time = time_us_32();
            
            // Wait for all queued and active moves to complete
            while (move_queue->has_chunk(AXIS_SPINDLE) || 
                   move_queue->has_chunk(AXIS_TRAVERSE) ||
                   move_queue->is_active(AXIS_SPINDLE) ||
                   move_queue->is_active(AXIS_TRAVERSE)) {
                
                sleep_ms(10);
                
                // Timeout after 10 seconds
                if ((time_us_32() - start_time) > 10000000) {
                    printf("[G4] WARNING: Timeout waiting for moves to complete\n");
                    break;
                }
            }
            
            printf("[G4 P0] Planner synced - all moves complete\n");
        }
        
        send_response("OK");
        return true;
    }
    
    // Normal dwell with delay
    printf("[G4] Dwelling for %.1f ms\n", params.P);
    sleep_ms((uint32_t)params.P);
    
    send_response("OK");
    return true;
}

// =============================================================================
// ⭐ ADD TO execute_command() SWITCH STATEMENT:
// =============================================================================

// In your execute_command() function, add these cases to the M-code switch:

bool GCodeInterface::execute_command() {
    // ... existing code ...
    
    // M-codes
    if (last_command_type == COMMAND_M) {
        switch (last_m_command) {
            case 0:   return execute_m0();    // ⭐ NEW: Feed hold
            case 1:   return execute_m1();    // ⭐ NEW: Resume
            case 3:   return execute_m3_m4();
            case 4:   return execute_m3_m4();
            case 5:   return execute_m5();
            case 112: return execute_m112();  // ⭐ NEW: Emergency stop
            case 410: return execute_m410();  // ⭐ NEW: Quick stop
            case 999: return execute_m999();  // ⭐ NEW: Reset from e-stop
            
            // ... existing cases ...
            
            default:
                set_error("Unknown M-code");
                return false;
        }
    }
    
    // ... rest of function ...
}

// =============================================================================
// ⭐ ADD THESE DECLARATIONS TO gcode_interface.h:
// =============================================================================

/*
In your gcode_interface.h file, add these method declarations:

    bool execute_m0();     // Feed hold
    bool execute_m1();     // Resume
    bool execute_m112();   // Emergency stop
    bool execute_m410();   // Quick stop
    bool execute_m999();   // Reset from emergency stop
    // execute_g4() should already exist, just update its implementation
*/
