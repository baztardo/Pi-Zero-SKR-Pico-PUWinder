// =============================================================================
// winding_controller_FIXED.cpp - Queue Overflow Prevention
// Key fixes:
// 1. Limit sync frequency to prevent queue overflow
// 2. Check queue depth before pushing chunks
// 3. Reduce chunk generation rate
// 4. Add backpressure handling
// =============================================================================

// Add this to your existing winding_controller.cpp

// =============================================================================
// Improved sync_traverse_to_spindle with queue depth check
// =============================================================================
void WindingController::sync_traverse_to_spindle() {
    if (!spindle_motor) {
        return;
    }
    
    // ⭐ NEW: Check queue depth BEFORE generating chunks
    uint32_t queue_depth = move_queue->get_queue_depth();
    const uint32_t MAX_QUEUE_DEPTH = 100;  // Leave headroom
    
    if (queue_depth >= MAX_QUEUE_DEPTH) {
        // Queue is getting full - skip this sync cycle
        static uint32_t backpressure_count = 0;
        if ((backpressure_count++ % 100) == 0) {
            printf("[SYNC] ⚠️ Queue depth %u - skipping sync (backpressure)\n", queue_depth);
        }
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
    
    // ⭐ CRITICAL: Limit sync frequency to prevent overwhelming the queue
    static uint32_t last_sync_time = 0;
    uint32_t current_time = time_us_32();
    uint32_t delta_time_us = current_time - last_sync_time;
    
    // ⭐ NEW: Increase minimum time between syncs from 10ms to 50ms
    const uint32_t MIN_SYNC_INTERVAL_US = 50000;  // 50ms = 20Hz sync rate
    
    if (delta_time_us < MIN_SYNC_INTERVAL_US) {
        return;  // Too soon since last sync
    }
    
    last_sync_time = current_time;
    
    // Calculate steps to generate for this time period
    float delta_time_sec = delta_time_us / 1000000.0f;
    int32_t steps_to_generate = (int32_t)(required_steps_per_sec * delta_time_sec);
    
    if (steps_to_generate <= 0) {
        return;
    }
    
    // Apply direction
    uint32_t abs_steps = abs(steps_to_generate);
    
    // Set direction
    move_queue->set_direction(traverse_direction);
    
    // ⭐ NEW: Generate fewer, larger chunks instead of many small chunks
    // This reduces queue overhead
    const uint32_t MIN_CHUNK_SIZE = 50;  // Minimum steps per chunk
    
    if (abs_steps < MIN_CHUNK_SIZE) {
        // Accumulate small movements
        static float accumulated_steps = 0.0f;
        accumulated_steps += abs_steps;
        
        if (accumulated_steps >= MIN_CHUNK_SIZE) {
            abs_steps = (uint32_t)accumulated_steps;
            accumulated_steps = 0.0f;
        } else {
            return;  // Not enough steps yet
        }
    }
    
    // Generate chunks with constant velocity
    auto chunks = StepCompressor::compress_constant_velocity(
        abs_steps, required_steps_per_sec);
    
    // ⭐ DEBUG: Only print occasionally to avoid serial flood
    static uint32_t sync_count = 0;
    bool should_print = (sync_count % 20) == 0;  // Every 20th sync = 1 second
    
    if (should_print) {
        printf("[SYNC] RPM: %.1f, Velocity: %.3f mm/s, Steps: %u, Chunks: %zu, Depth: %u\n", 
               current_rpm, 
               required_traverse_velocity_mm_per_sec,
               abs_steps,
               chunks.size(),
               queue_depth);
    }
    
    // Push chunks to queue
    int pushed_count = 0;
    for (const auto& chunk : chunks) {
        if (move_queue->push_chunk(chunk)) {
            pushed_count++;
        } else {
            // Queue full during push
            if (should_print) {
                printf("[SYNC] ⚠️ Queue full mid-push (%d/%zu chunks pushed)\n", 
                       pushed_count, chunks.size());
            }
            break;
        }
    }
    
    if (should_print) {
        printf("[SYNC] Pushed %d/%zu chunks, Queue depth: %u, Active: %s\n",
               pushed_count, chunks.size(), 
               move_queue->get_queue_depth(),
               move_queue->is_active() ? "YES" : "NO");
    }
    
    sync_count++;
    
    // Update position tracking
    if (traverse_direction) {
        current_traverse_position_mm += (abs_steps / steps_per_mm);
    } else {
        current_traverse_position_mm -= (abs_steps / steps_per_mm);
    }
    
    // Check for layer boundaries
    check_layer_bounds();
}

// =============================================================================
// Add this to your main.cpp update loop
// =============================================================================
/*
void main_loop() {
    // Create diagnostic monitor
    DiagnosticMonitor* monitor = new DiagnosticMonitor(move_queue);
    
    while (true) {
        // Update winding controller
        if (winding_controller) {
            winding_controller->update();
        }
        
        // Update traverse controller (only when moving for homing)
        if (traverse_controller && traverse_controller->is_moving()) {
            traverse_controller->generate_steps();
        }
        
        // Handle commands
        if (communication_handler) {
            communication_handler->update();
        }
        
        // ⭐ NEW: Print diagnostics every second
        monitor->update(1000);
        
        // ⭐ NEW: Print full diagnostics every 10 seconds
        static uint32_t last_full_diag = 0;
        if ((time_us_32() - last_full_diag) > 10000000) {
            monitor->print_full_diagnostics();
            last_full_diag = time_us_32();
        }
        
        // Small delay
        sleep_us(100);
    }
}
*/
