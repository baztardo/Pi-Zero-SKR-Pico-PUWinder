// =============================================================================
// gcode_g1_complete.cpp - Complete G1 Command Implementation
// Purpose: Connect G1 commands to StepCompressor and MoveQueue for actual movement
// =============================================================================

#include "gcode_interface.h"
#include "stepcompress.h"
#include "config.h"
#include <cmath>
#include <cstdio>

// =============================================================================
// Complete G0/G1 Implementation (Linear Move)
// =============================================================================
bool GCodeInterface::execute_g0_g1(const char* command) {
    printf("\n📍 Executing G1 Command: %s\n", command);
    
    // Parse parameters (default to current position/feedrate)
    double target_y = current_y;
    double feedrate = current_feedrate;
    
    // Parse Y and F parameters
    parse_parameters(command, "YF", &target_y, &feedrate);
    
    // Update feedrate if specified
    if (feedrate > 0.0) {
        current_feedrate = feedrate;
    }
    
    // Calculate move parameters
    double distance_mm = fabs(target_y - current_y);
    
    // Check if move is needed
    if (distance_mm < 0.001) {
        printf("⚠️  No movement needed (distance < 0.001mm)\n");
        return true;
    }
    
    // Validate move is within bounds
    if (target_y < Y_MIN_POSITION_MM || target_y > Y_MAX_POSITION_MM) {
        printf("❌ ERROR: Target Y=%.3f out of bounds [%.1f, %.1f]\n",
               target_y, Y_MIN_POSITION_MM, Y_MAX_POSITION_MM);
        set_error("Position out of bounds");
        return false;
    }
    
    // Convert to steps
    uint32_t total_steps = (uint32_t)(distance_mm * Y_STEPS_PER_MM);
    
    printf("  Distance: %.3f mm (%u steps)\n", distance_mm, total_steps);
    printf("  Feedrate: %.1f mm/min (%.2f mm/s)\n", 
           current_feedrate, current_feedrate / 60.0);
    
    // Calculate velocities
    double velocity_mms = current_feedrate / 60.0;  // mm/min → mm/s
    double cruise_velocity_sps = velocity_mms * Y_STEPS_PER_MM;  // steps/sec
    
    // Start from current velocity (0 if stationary)
    double start_velocity_sps = 0.0;
    
    // Use configured acceleration
    double accel_mms2 = Y_MAX_ACCEL;  // mm/s²
    double accel_sps2 = accel_mms2 * Y_STEPS_PER_MM;  // steps/s²
    
    printf("  Cruise velocity: %.1f steps/sec\n", cruise_velocity_sps);
    printf("  Acceleration: %.1f mm/s² (%.1f steps/s²)\n", 
           accel_mms2, accel_sps2);
    
    // Check if move queue is available
    if (!winding_controller || !move_queue) {
        printf("❌ ERROR: Move queue or winding controller not initialized\n");
        set_error("System not initialized");
        return false;
    }
    
    // Set traverse direction
    bool dir_positive = (target_y > current_y);
    winding_controller->set_traverse_direction(dir_positive);
    
    printf("  Direction: %s\n", dir_positive ? "POSITIVE" : "NEGATIVE");
    
    // Generate step chunks using enhanced compression
    std::vector<StepChunk> chunks;
    
    printf("\n🔧 Generating step chunks...\n");
    
    // Use enhanced compression with velocity spike detection
    StepCompressor::compress_trapezoid_with_optimization(
        chunks,
        total_steps,
        start_velocity_sps,
        cruise_velocity_sps,
        accel_sps2,
        20.0,   // max_err_us: 20μs timing tolerance
        100.0   // max_spike_sps: 100 steps/sec velocity spike tolerance
    );
    
    if (chunks.empty()) {
        printf("❌ ERROR: Failed to generate step chunks\n");
        set_error("Step compression failed");
        return false;
    }
    
    printf("✓ Generated %zu chunks\n", chunks.size());
    
    // Check if queue has space
    size_t queue_depth = move_queue->get_queue_depth(AXIS_TRAVERSE);
    size_t queue_space = 64 - queue_depth;  // Assuming 64-entry queue
    
    if (chunks.size() > queue_space) {
        printf("⚠️  WARNING: Queue nearly full (%zu/%zu), move may block\n",
               queue_depth, (size_t)64);
    }
    
    // Push chunks to move queue
    printf("\n📤 Pushing chunks to move queue...\n");
    
    size_t chunks_pushed = 0;
    for (const auto& chunk : chunks) {
        if (!move_queue->push_chunk(AXIS_TRAVERSE, chunk)) {
            printf("❌ ERROR: Failed to push chunk %zu/%zu (queue full)\n",
                   chunks_pushed + 1, chunks.size());
            set_error("Move queue full");
            
            // Partial move completed
            if (chunks_pushed > 0) {
                printf("⚠️  Partial move: %zu/%zu chunks queued\n",
                       chunks_pushed, chunks.size());
                
                // Estimate partial distance
                uint32_t steps_queued = 0;
                for (size_t i = 0; i < chunks_pushed; i++) {
                    steps_queued += chunks[i].count;
                }
                double distance_queued_mm = (double)steps_queued / Y_STEPS_PER_MM;
                current_y += (dir_positive ? distance_queued_mm : -distance_queued_mm);
                
                printf("  Estimated position: Y=%.3f\n", current_y);
            }
            
            return false;
        }
        chunks_pushed++;
    }
    
    printf("✓ All %zu chunks pushed successfully\n", chunks_pushed);
    
    // Update current position (assuming move will complete)
    current_y = target_y;
    
    // Calculate estimated move time
    double time_sec = distance_mm / velocity_mms;
    printf("\n✓ G1 Move Queued Successfully\n");
    printf("  Current → Target: %.3f → %.3f mm\n", 
           target_y - distance_mm * (dir_positive ? 1.0 : -1.0), target_y);
    printf("  Estimated time: %.2f seconds\n", time_sec);
    printf("  Queue depth: %zu/%zu\n", 
           move_queue->get_queue_depth(AXIS_TRAVERSE), (size_t)64);
    
    return true;
}

// =============================================================================
// Wait for Move Completion
// =============================================================================
bool GCodeInterface::wait_for_move_completion(uint32_t timeout_ms) {
    printf("\n⏳ Waiting for move completion (timeout: %u ms)...\n", timeout_ms);
    
    uint64_t start_time = time_us_64();
    uint64_t timeout_us = (uint64_t)timeout_ms * 1000;
    
    while (true) {
        // Check if traverse is still active
        bool traverse_active = move_queue->is_active(AXIS_TRAVERSE);
        size_t queue_depth = move_queue->get_queue_depth(AXIS_TRAVERSE);
        
        if (!traverse_active && queue_depth == 0) {
            printf("✓ Move completed\n");
            return true;
        }
        
        // Check timeout
        uint64_t elapsed = time_us_64() - start_time;
        if (elapsed > timeout_us) {
            printf("⚠️  Timeout waiting for move completion\n");
            printf("  Queue depth: %zu, Active: %s\n", 
                   queue_depth, traverse_active ? "YES" : "NO");
            return false;
        }
        
        // Small delay to avoid busy-waiting
        sleep_ms(10);
        
        // Print progress every 500ms
        if ((elapsed % 500000) < 10000) {
            printf("  Progress: queue=%zu, active=%s, elapsed=%.1fs\n",
                   queue_depth, traverse_active ? "Y" : "N", elapsed / 1e6);
        }
    }
}

// =============================================================================
// Get Current Position
// =============================================================================
void GCodeInterface::get_current_position(double& y_pos) {
    y_pos = current_y;
}

// =============================================================================
// Set Current Position (for homing)
// =============================================================================
void GCodeInterface::set_current_position(double y_pos) {
    current_y = y_pos;
    printf("✓ Position set to Y=%.3f\n", y_pos);
}

// =============================================================================
// Emergency Stop
// =============================================================================
void GCodeInterface::emergency_stop() {
    printf("\n🛑 EMERGENCY STOP TRIGGERED\n");
    
    // Clear all queued moves
    if (move_queue) {
        move_queue->clear_queue(AXIS_TRAVERSE);
        move_queue->clear_queue(AXIS_SPINDLE);
        move_queue->set_enable(AXIS_TRAVERSE, false);
        move_queue->set_enable(AXIS_SPINDLE, false);
    }
    
    // Disable spindle
    if (winding_controller) {
        winding_controller->disable_all();
    }
    
    printf("✓ All motion stopped\n");
}

// =============================================================================
// Test G1 Implementation
// =============================================================================
void GCodeInterface::test_g1_implementation() {
    printf("\n========================================\n");
    printf("Testing G1 Implementation\n");
    printf("========================================\n\n");
    
    // Test 1: Small move
    printf("Test 1: Small move (10mm)\n");
    printf("----------------------------------\n");
    execute_g0_g1("G1 Y10.0 F50");
    sleep_ms(100);
    
    // Test 2: Medium move
    printf("\nTest 2: Medium move (50mm)\n");
    printf("----------------------------------\n");
    execute_g0_g1("G1 Y60.0 F100");
    sleep_ms(100);
    
    // Test 3: Return to start
    printf("\nTest 3: Return to start\n");
    printf("----------------------------------\n");
    execute_g0_g1("G1 Y0.0 F100");
    
    printf("\n========================================\n");
    printf("G1 Test Complete\n");
    printf("========================================\n\n");
}

// =============================================================================
// G1 Diagnostics
// =============================================================================
void GCodeInterface::print_g1_diagnostics() {
    printf("\n=== G1 Command Diagnostics ===\n");
    printf("Current Position: Y=%.3f mm\n", current_y);
    printf("Current Feedrate: %.1f mm/min\n", current_feedrate);
    
    if (move_queue) {
        printf("Traverse Queue:   %zu/64 chunks\n", 
               move_queue->get_queue_depth(AXIS_TRAVERSE));
        printf("Traverse Active:  %s\n",
               move_queue->is_active(AXIS_TRAVERSE) ? "YES" : "NO");
        printf("Traverse Steps:   %u\n",
               move_queue->get_step_count(AXIS_TRAVERSE));
    } else {
        printf("Move Queue:       NOT INITIALIZED\n");
    }
    
    if (winding_controller) {
        printf("Winding Ctrl:     INITIALIZED\n");
    } else {
        printf("Winding Ctrl:     NOT INITIALIZED\n");
    }
    
    printf("============================\n\n");
}

// =============================================================================
// Helper: Parse Parameters (improved)
// =============================================================================
void GCodeInterface::parse_parameters(const char* command, 
                                     const char* param_names,
                                     ...) {
    va_list args;
    va_start(args, param_names);
    
    size_t num_params = strlen(param_names);
    
    for (size_t i = 0; i < num_params; i++) {
        char param = param_names[i];
        double* value_ptr = va_arg(args, double*);
        
        // Find parameter in command string
        const char* param_pos = strchr(command, param);
        if (param_pos) {
            // Parse numeric value after parameter letter
            double value = 0.0;
            if (sscanf(param_pos + 1, "%lf", &value) == 1) {
                *value_ptr = value;
            }
        }
    }
    
    va_end(args);
}
