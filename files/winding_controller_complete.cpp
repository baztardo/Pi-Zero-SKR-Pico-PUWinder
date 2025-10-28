// =============================================================================
// winding_controller.cpp - COMPLETE WORKING VERSION
// Fixed sync and proper handoff between TraverseController and MoveQueue
// =============================================================================

// ... (keep all existing includes and constructor - no changes needed)

// Add this helper function at the top after includes:
extern TraverseController* traverse_controller;  // Declare external

// =============================================================================
// Move to Start Position - WITH PROPER HANDOFF
// =============================================================================
void WindingController::move_to_start() {
    printf("\n[WindingController] ========================================\n");
    printf("[WindingController] MOVING TO START POSITION\n");
    printf("[WindingController] ========================================\n");
    
    // Safety checks
    if (params.start_position_mm > 120.0f) {
        printf("❌ ERROR: Start position %.1f mm exceeds limit (120mm)\n", 
               params.start_position_mm);
        params.start_position_mm = 50.0f;
    }
    if (params.start_position_mm < 0.0f) {
        printf("❌ ERROR: Start position %.1f mm is negative\n", 
               params.start_position_mm);
        params.start_position_mm = 0.0f;
    }
    
    printf("[WindingController] Target: %.2f mm\n", params.start_position_mm);
    
    // ✅ CRITICAL: Handoff from TraverseController to MoveQueue
    if (traverse_controller) {
        // Stop TraverseController from trying to generate steps
        // (it will only be used for position queries now)
        printf("[WindingController] Stopping TraverseController step generation\n");
        traverse_controller->stop_steps();  // Sets moving=false
    }
    
    // ✅ CRITICAL: MoveQueue takes control
    printf("[WindingController] MoveQueue taking control of traverse motor\n");
    
    // Ensure motor is enabled
    move_queue->set_enable(true);  // Sets ENA=0 (active low)
    printf("[WindingController] Motor ENABLED via MoveQueue\n");
    
    // Ensure feed hold is released
    if (move_queue->is_feeding_paused()) {
        printf("[WindingController] Releasing feed hold\n");
        move_queue->resume_feeding();
    }
    
    // Set direction
    move_queue->set_direction(true);  // Forward to start position
    printf("[WindingController] Direction: FORWARD\n");
    
    // Calculate steps
    uint32_t steps = mm_to_steps(params.start_position_mm);
    printf("[WindingController] Generating %u steps (%.2f mm)\n", 
           steps, params.start_position_mm);
    
    // Generate and push chunks
    auto chunks = StepCompressor::compress_constant_velocity(
        steps, TRAVERSE_RAPID_SPEED);
    
    printf("[WindingController] Generated %zu chunks\n", chunks.size());
    
    int pushed_count = 0;
    for (const auto& chunk : chunks) {
        if (move_queue->push_chunk(chunk)) {
            pushed_count++;
        } else {
            printf("❌ ERROR: Failed to push chunk!\n");
        }
    }
    
    printf("[WindingController] Pushed %d/%zu chunks successfully\n", 
           pushed_count, chunks.size());
    printf("[WindingController] Queue depth: %u\n", 
           move_queue->get_queue_depth());
    
    // Update position tracking
    current_traverse_position_mm = params.start_position_mm;
    
    // Move to next state
    state = WindingState::RAMPING_UP;
    printf("[WindingController] State: RAMPING_UP\n");
    printf("[WindingController] ========================================\n\n");
}

// =============================================================================
// Sync Traverse to Spindle - WITH FULL DEBUG
// =============================================================================
void WindingController::sync_traverse_to_spindle() {
    if (!spindle_motor) {
        return;
    }
    
    // Get current RPM
    float current_rpm = spindle_motor->get_rpm();
    
    if (current_rpm < 10.0f) {
        return;  // Spindle not spinning fast enough
    }
    
    // Calculate required traverse velocity
    float required_traverse_velocity_mm_per_min = current_rpm * params.wire_diameter_mm;
    float required_traverse_velocity_mm_per_sec = required_traverse_velocity_mm_per_min / 60.0f;
    
    // Convert to steps per second
    float steps_per_mm = 6135.0f;  // Calibrated value
    float required_steps_per_sec = required_traverse_velocity_mm_per_sec * steps_per_mm;
    
    if (required_steps_per_sec < 1.0f) {
        return;  // Too slow
    }
    
    // Calculate steps to generate this cycle
    static uint32_t last_sync_time = 0;
    uint32_t current_time = time_us_32();
    uint32_t delta_time_us = current_time - last_sync_time;
    
    if (delta_time_us < 10000) {  // 10ms minimum between syncs
        return;
    }
    
    float delta_time_sec = delta_time_us / 1000000.0f;
    int32_t steps_to_generate = (int32_t)(required_steps_per_sec * delta_time_sec);
    
    if (steps_to_generate > 0) {
        // Apply direction
        if (!traverse_direction) {
            steps_to_generate = -steps_to_generate;
        }
        
        // ✅ FULL DEBUG OUTPUT
        static uint32_t sync_count = 0;
        if ((sync_count++ % 10) == 0) {  // Every 10th sync
            printf("\n[SYNC] ========================================\n");
            printf("[SYNC] Sync #%u\n", sync_count);
            printf("[SYNC] Spindle RPM: %.1f\n", current_rpm);
            printf("[SYNC] Traverse velocity: %.3f mm/s\n", 
                   required_traverse_velocity_mm_per_sec);
            printf("[SYNC] Steps to generate: %d\n", abs(steps_to_generate));
            printf("[SYNC] Direction: %s\n", traverse_direction ? "FORWARD" : "REVERSE");
        }
        
        // Enable and set direction
        move_queue->set_enable(true);
        move_queue->set_direction(traverse_direction);
        
        // Generate chunks
        auto chunks = StepCompressor::compress_constant_velocity(
            abs(steps_to_generate), (uint32_t)required_steps_per_sec);
        
        if ((sync_count % 10) == 0) {
            printf("[SYNC] Generated %zu chunks\n", chunks.size());
        }
        
        // Push chunks
        int pushed = 0;
        for (const auto& chunk : chunks) {
            if (move_queue->push_chunk(chunk)) {
                pushed++;
            } else {
                printf("[SYNC] ❌ ERROR: push_chunk FAILED!\n");
            }
        }
        
        // ✅ CRITICAL DEBUG: Verify push was successful
        if ((sync_count % 10) == 0) {
            printf("[SYNC] Pushed %d/%zu chunks\n", pushed, chunks.size());
            printf("[SYNC] Queue depth: %u\n", move_queue->get_queue_depth());
            printf("[SYNC] Queue active: %s\n", 
                   move_queue->is_active() ? "YES" : "NO");
            
            // Check for problems
            if (pushed < (int)chunks.size()) {
                printf("[SYNC] ⚠️  WARNING: Not all chunks pushed!\n");
            }
            if (move_queue->get_queue_depth() == 0 && pushed > 0) {
                printf("[SYNC] ⚠️  WARNING: Queue empty after push!\n");
            }
            printf("[SYNC] ========================================\n\n");
        }
        
        // Update position
        float distance_moved_mm = (float)steps_to_generate / steps_per_mm;
        current_traverse_position_mm += distance_moved_mm;
        
        if ((sync_count % 20) == 0) {
            printf("[SYNC] New traverse position: %.3f mm\n", 
                   current_traverse_position_mm);
        }
    }
    
    last_sync_time = current_time;
    
    // Check for layer edges
    float edge_margin = 0.5f;
    float left_limit = params.start_position_mm + edge_margin;
    float right_limit = params.start_position_mm + params.layer_width_mm - edge_margin;
    
    if (traverse_direction && current_traverse_position_mm >= right_limit) {
        printf("\n[SYNC] ⚠️  RIGHT EDGE - Reversing to LEFT\n");
        traverse_direction = false;
        current_layer++;
        turns_this_layer = 0;
    } else if (!traverse_direction && current_traverse_position_mm <= left_limit) {
        printf("\n[SYNC] ⚠️  LEFT EDGE - Reversing to RIGHT\n");
        traverse_direction = true;
        current_layer++;
        turns_this_layer = 0;
    }
}

// =============================================================================
// Start Winding - WITH PROPER INITIALIZATION
// =============================================================================
bool WindingController::start() {
    if (state != WindingState::IDLE) {
        printf("[WindingController] Cannot start: not idle (state=%d)\n", (int)state);
        return false;
    }
    
    printf("\n[WindingController] ========================================\n");
    printf("[WindingController] STARTING WINDING PROCESS\n");
    printf("[WindingController] ========================================\n");
    printf("[WindingController] Parameters:\n");
    printf("  - Target turns: %u\n", params.target_turns);
    printf("  - Spindle RPM: %.1f\n", params.spindle_rpm);
    printf("  - Wire diameter: %.3f mm\n", params.wire_diameter_mm);
    printf("  - Layer width: %.1f mm\n", params.layer_width_mm);
    printf("  - Start position: %.1f mm\n", params.start_position_mm);
    
    // Reset state
    state = WindingState::HOMING_SPINDLE;
    current_layer = 0;
    turns_completed = 0;
    turns_this_layer = 0;
    current_rpm = 0.0f;
    homing_started = false;
    traverse_homing_started = false;
    
    // ✅ CRITICAL: Ensure MoveQueue is ready
    if (move_queue->is_feeding_paused()) {
        printf("[WindingController] Resuming feeding\n");
        move_queue->resume_feeding();
    }
    
    if (move_queue->is_emergency_stopped()) {
        printf("[WindingController] ❌ ERROR: Emergency stop active!\n");
        return false;
    }
    
    printf("[WindingController] State: HOMING_SPINDLE\n");
    printf("[WindingController] ========================================\n\n");
    
    return true;
}

// ... (keep all other existing functions unchanged)
