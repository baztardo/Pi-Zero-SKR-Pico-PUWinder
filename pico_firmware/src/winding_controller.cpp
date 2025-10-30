// =============================================================================
// winding_controller.cpp - Winding Controller Implementation (FIXED)
// Purpose: Full winding controller - ALL BUGS FIXED
// =============================================================================

#include "winding_controller.h"
#include "config.h"
#include "spindle.h"
#include "stepcompress.h"
#include "traverse_controller.h"
#include "pico/stdlib.h"
#include <cmath>
#include <cstdio>

// External reference to traverse controller for handoff
extern TraverseController* traverse_controller;

// Default constants
#ifndef WIRE_TENSION_FACTOR
#define WIRE_TENSION_FACTOR 0.95f  // 5% compression for tight winding
#endif

#ifndef TRAVERSE_MIN_WINDING_SPEED
#define TRAVERSE_MIN_WINDING_SPEED 1000.0f  // steps/sec
#endif

WindingController::WindingController(MoveQueue* mq, BLDC_MOTOR* spindle_motor)
    : move_queue(mq)
    , spindle_motor(spindle_motor)
    , state(WindingState::IDLE)
    , current_layer(0)
    , turns_completed(0)
    , turns_this_layer(0)
    , current_rpm(0.0f)
    , last_rpm_update_time(0)
    , traverse_direction(true)
    , current_traverse_position_mm(0.0f)
    , ramp_started(false)
    , ramp_start_time(0)
    , turn_accum(0.0)
    , encoder_sign(1)  // Assume forward initially
    , traverse_steps_emitted(0.0)
    , enc_last_sync(0)
    , enc_last_rpm(0)

{
    printf("[WindingController] Created with spindle motor\n");
}

void WindingController::init() {
    printf("WindingController::init() called\n");
    
    if (spindle_motor) {
        spindle_motor->set_pulses_per_revolution(BLDC_DEFAULT_PPR);
        printf("WindingController initialized with BLDC motor\n");
    } else {
        printf("ERROR: Spindle motor not provided to constructor\n");
    }
}

void WindingController::set_parameters(const WindingParams& p) {
    params = p;
    
    // Calculate derived parameters
    params.wire_pitch_mm = params.wire_diameter_mm;
    params.turns_per_layer = (uint32_t)(params.layer_width_mm / params.wire_pitch_mm);
    if (params.turns_per_layer == 0) params.turns_per_layer = 1;
    params.total_layers = (params.target_turns + params.turns_per_layer - 1) / params.turns_per_layer;
    
    printf("Parameters set: %u turns, %.1f RPM, %.3fmm wire\n", 
           params.target_turns, params.spindle_rpm, params.wire_diameter_mm);
    printf("  Turns per layer: %u, Total layers: %u\n", 
           params.turns_per_layer, params.total_layers);
}

bool WindingController::start() {
    printf("[WindingController] start() called - current state: %d\n", (int)state);
    if (state != WindingState::IDLE) {
        printf("❌ Cannot start: not idle (state=%d)\n", (int)state);
        return false;
    }
    
    printf("Starting winding process\n");
    state = WindingState::RAMPING_UP;
    current_layer = 0;
    turns_completed = 0;
    turns_this_layer = 0;
    current_rpm = 0.0f;
     
    // Enable traverse controller for winding
    extern TraverseController* traverse_controller;
    if (traverse_controller) {
        traverse_controller->enable();
        printf("✓ Traverse controller enabled for winding\n");
    }
    
    return true;
}

void WindingController::stop() {
    printf("Stopping winding process\n");
    state = WindingState::IDLE;
    if (spindle_motor) {
        spindle_motor->set_brake(true);
    }

    // ⭐ DEACTIVATE PIO MODE - release GPIO back for homing
    if (move_queue) {
        move_queue->deactivate_pio_mode();
    }

    // ⭐ CRITICAL FIX: Reset traverse controller state and DISABLE for power saving
    // Stop any ongoing movement, reset state, and disable motor to prevent heat/wear
    if (traverse_controller) {
        traverse_controller->stop_steps();  // Stop any ongoing steps
        traverse_controller->emergency_stop();  // Reset emergency_stopped flag
        traverse_controller->clear_emergency_stop();  // Clear emergency stop
        traverse_controller->disable();  // ⭐ DISABLE motor to prevent heat/wear
        printf("✓ Traverse controller reset and disabled for power saving\n");
    }
}

void WindingController::update() {
    switch (state) {
        case WindingState::IDLE:
            break;
            
        case WindingState::RAMPING_UP:
            ramp_up_spindle();
            break;
            
        case WindingState::WINDING:
            execute_winding();
            break;
            
        case WindingState::RAMPING_DOWN:
            ramp_down_spindle();
            break;
            
        case WindingState::COMPLETE:
            // Stay in complete state until stopped
            break;
            
        case WindingState::ERROR:
            // Stay in error state until stopped
            break;
    }
}

void WindingController::emergency_stop() {
    printf("EMERGENCY STOP!\n");
    state = WindingState::IDLE;
    if (spindle_motor) {
        spindle_motor->set_brake(true);
    }
    // maybe add a check to see if the traverse is homed and if not, home it
    if (!traverse_controller->is_homed()) {
        traverse_controller->home();
    }
}

// =============================================================================
// NEW: reset() - Reset controller to IDLE state for new windings
// =============================================================================
void WindingController::reset() {
    printf("[WindingController] Reset to IDLE state for new winding (was state: %d)\n", (int)state);

    // Reset all state variables
    state = WindingState::IDLE;
    current_layer = 0;
    turns_completed = 0;
    turns_this_layer = 0;
    current_rpm = 0.0f;
    last_rpm_update_time = 0;
    traverse_direction = true;  // Start moving RIGHT
    current_traverse_position_mm = 0.0f;  // Will be set from traverse controller during ramp-up
    ramp_started = false;
    ramp_start_time = 0;
    turn_accum = 0.0f;
    encoder_sign = 1;

    // Reset move queue if available
    if (move_queue) {
        move_queue->clear_queue();
        move_queue->set_enable(false);
        move_queue->deactivate_pio_mode();  // ⭐ EXTRA: Ensure PIO mode is deactivated
    }

    // ⭐ CRITICAL FIX: Also reset traverse controller state for re-homing
    if (traverse_controller) {
        traverse_controller->stop_steps();  // Stop any ongoing steps
        traverse_controller->emergency_stop();  // Reset state
        traverse_controller->clear_emergency_stop();  // Clear emergency flag
        traverse_controller->disable();  // ⭐ DISABLE motor to prevent heat/wear
        // DON'T reset homed state - keep it homed for new windings
        printf("✓ Traverse controller reset and disabled for power saving\n");
    }

    // Reset spindle motor state (but don't stop if running)
    if (spindle_motor) {
        // Don't reset RPM here - let new WIND command set it
        // spindle_motor->set_brake(false);  // Don't touch brake state
    }

    printf("[WindingController] ✓ Reset complete - now in IDLE state\n");
}

// =============================================================================
// FIXED: execute_winding() - Now complete with proper logic
// =============================================================================
void WindingController::execute_winding() {
    // Check if target reached
    if (turns_completed >= params.target_turns) {
        printf("Target turns reached! Stopping spindle.\n");
        state = WindingState::RAMPING_DOWN;
        return;
    }
    
    // Update RPM (FIXED - now properly updates cursor)
    update_rpm();
    
    // Sync traverse to spindle
    sync_traverse_to_spindle();
    
    // Update winding progress
    if (spindle_motor) {
        float total_revolutions = spindle_motor->get_revolutions();
        uint32_t new_turns_completed = (uint32_t)total_revolutions;
        
        if (new_turns_completed > turns_completed) {
            uint32_t delta = new_turns_completed - turns_completed;
            turns_completed = new_turns_completed;
            turns_this_layer += delta;
            
            if (turns_completed % 100 == 0) {
                printf("Progress: %u/%u turns (%.1f%%)\n", 
                       turns_completed, params.target_turns,
                       (float)turns_completed * 100.0f / params.target_turns);
            }
            
            update_display();
        }
    }
}

void WindingController::ramp_up_spindle() {
    if (!ramp_started) {
        printf("Starting spindle ramp up to %.1f RPM over %.1f seconds\n", 
               params.spindle_rpm, params.ramp_time_sec);
        ramp_started = true;
        ramp_start_time = time_us_32();
        
        // ⭐ ACTIVATE PIO MODE for high-speed stepping during winding
        if (move_queue) {
            move_queue->activate_pio_mode();
            printf("[RAMP] ✓ PIO mode activated for winding\n");
        }
        
        // ⭐ Initialize traverse position from homing
        if (traverse_controller) {
            current_traverse_position_mm = traverse_controller->get_position();
            printf("[RAMP] Traverse starting position: %.2f mm\n", current_traverse_position_mm);
        }
    }
    
    uint32_t elapsed = time_us_32() - ramp_start_time;
    float ramp_progress = (float)elapsed / (params.ramp_time_sec * 1000000.0f);
    
    if (ramp_progress > 1.0f) ramp_progress = 1.0f;
    
    float current_target_rpm = params.spindle_rpm * ramp_progress;
    current_rpm = current_target_rpm;
    
    // ⭐ CRITICAL FIX: Actually apply the RPM to the motor (was missing!)
    if (spindle_motor) {
        spindle_motor->set_rpm_pwm(current_target_rpm);
    }
    
    // ⭐ CRITICAL FIX: Start traverse movement during ramp-up so it syncs from the start
    // This ensures traverse velocity matches spindle RPM as it ramps up
    sync_traverse_to_spindle();
    
    if (ramp_progress >= 1.0f) {
        printf("Spindle ramp up complete\n");
        state = WindingState::WINDING;
        ramp_started = false;
    }
}

void WindingController::ramp_down_spindle() {
    if (!ramp_started) {
        ramp_started = true;
        ramp_start_time = time_us_32();
    }
    
    uint32_t elapsed = time_us_32() - ramp_start_time;
    float ramp_progress = (float)elapsed / (params.ramp_time_sec * 1000000.0f);
    
    if (ramp_progress > 1.0f) ramp_progress = 1.0f;
    
    float target_rpm = params.spindle_rpm * (1.0f - ramp_progress);
    current_rpm = target_rpm;
    
    // Actually set the spindle RPM
    if (spindle_motor) {
        spindle_motor->set_rpm_pwm(target_rpm);
    }
    
    if (target_rpm <= 0.0f) {
        if (spindle_motor) {
            spindle_motor->set_pwm_duty(0.0f);  // Stop PWM
            spindle_motor->set_brake(true);     // Engage brake
        }
        printf("Spindle stopped - Winding complete!\n");
        state = WindingState::COMPLETE;
        ramp_started = false;
    }
}

void WindingController::sync_traverse_to_spindle() {
    if (!spindle_motor) {
        printf("[SYNC] No spindle motor!\n");
        return;
    }
    
    // Calibrated steps per mm
    const float steps_per_mm = 6135.0f;
    
    // ⭐ CRITICAL: Check for layer edge and reverse direction if needed
    // ⚠️ SAFETY: Always check edges (even if queue is full) to prevent missing reversals
    float edge_margin = 0.5f;  // 0.5mm safety margin from edges
    float left_limit = params.start_position_mm + edge_margin;
    float right_limit = params.start_position_mm + params.layer_width_mm - edge_margin;
    
    // ⚠️ SAFETY: Validate position is in reasonable range before edge check
    // Physical limits: Start ~38mm, max travel from start is 40-45mm
    // So absolute maximum position: ~38mm + 45mm = ~83mm
    // Using 90mm as max to give safety margin above physical limit
    const float MAX_PHYSICAL_POSITION_MM = 90.0f;  // Safety margin above ~83mm absolute max
    
    if (current_traverse_position_mm >= 0.0f && 
        current_traverse_position_mm <= MAX_PHYSICAL_POSITION_MM) {
        
        // Check if we need to reverse direction based on current position
        if (traverse_direction && current_traverse_position_mm >= right_limit) {
            // Hit right edge - reverse to LEFT
            printf("[SYNC] ⚠️  RIGHT EDGE DETECTED at %.3f mm (limit: %.3f) - REVERSING to LEFT\n", 
                   current_traverse_position_mm, right_limit);
            traverse_direction = false;
            if (move_queue) {
                move_queue->set_direction(traverse_direction);
            }
            current_layer++;
            turns_this_layer = 0;
        } else if (!traverse_direction && current_traverse_position_mm <= left_limit) {
            // Hit left edge - reverse to RIGHT
            printf("[SYNC] ⚠️  LEFT EDGE DETECTED at %.3f mm (limit: %.3f) - REVERSING to RIGHT\n", 
                   current_traverse_position_mm, left_limit);
            traverse_direction = true;
            if (move_queue) {
                move_queue->set_direction(traverse_direction);
            }
            current_layer++;
            turns_this_layer = 0;
        }
    } else {
        // ⚠️ SAFETY: Invalid position detected - emergency stop
        printf("[SYNC] ❌ INVALID POSITION: %.3f mm - EMERGENCY STOP!\n", 
               current_traverse_position_mm);
        emergency_stop();
        return;
    }
    // Timing check
    static uint32_t last_sync_time = 0;
    uint32_t current_time = time_us_32();
    
    if (last_sync_time == 0) {
        last_sync_time = current_time;
        return;
    }
    
    uint32_t delta_time_us = current_time - last_sync_time;
    
    // Return early if not time yet - THIS PREVENTS THE FAST LOOP!
    if (delta_time_us < 250000) {
        return;  // Not time yet - skip everything below
    }

    // ⭐ Check queue depth to prevent overflow (check before generating new chunks)
    uint32_t queue_depth = move_queue->get_queue_depth();
    const uint32_t MAX_QUEUE_DEPTH = 100;  // Leave 28 slots free
    
    if (queue_depth >= MAX_QUEUE_DEPTH) {
        // Queue too full - skip chunk generation (but edge detection already happened above)
        static uint32_t skip_count = 0;
        static uint32_t consecutive_skips = 0;
        
        consecutive_skips++;
        
        // ⚠️ SAFETY: If queue stays full for too long (5000 skips = ~5 seconds), EMERGENCY STOP
        if (consecutive_skips >= 5000) {
            printf("\n");
            printf("╔═══════════════════════════════════════════════════════════════╗\n");
            printf("║  ❌ EMERGENCY STOP - QUEUE OVERFLOW DETECTED                 ║\n");
            printf("╚═══════════════════════════════════════════════════════════════╝\n");
            printf("[SAFETY] Queue stayed full for 5+ seconds - ISR cannot keep up!\n");
            printf("[SAFETY] Queue depth: %u, Consecutive skips: %u\n", queue_depth, consecutive_skips);
            printf("[SAFETY] Stopping all motion for safety...\n");
            
            // Stop winding
            stop();
            
            // Stop spindle
            if (spindle_motor) {
                spindle_motor->set_pwm_duty(0.0f);
                spindle_motor->set_brake(true);
            }
            
            // Clear and disable move queue
            if (move_queue) {
                move_queue->clear_queue();
                move_queue->set_enable(false);
            }
            
            printf("[SAFETY] ✓ All motion stopped. Please investigate and reset.\n\n");
            consecutive_skips = 0;  // Reset counter
            return;
        }
        
        // Print warning every 500 skips
        if ((skip_count++ % 500) == 0) {
            printf("[SYNC] ⚠️  Queue depth %u - skipping chunk generation (skip #%u)\n", 
                   queue_depth, consecutive_skips);
        }
        return;
    }
    
    // ✅ Queue is healthy - reset consecutive skip counter
    static uint32_t consecutive_skips = 0;
    consecutive_skips = 0;
    
    // REAL-TIME VELOCITY MATCHING (like Klipper/FluidNC)
    // Calculate current traverse velocity based on current RPM
    float current_rpm = spindle_motor->get_rpm();
    
    // Calculate required traverse velocity: RPM × wire_diameter
    float required_traverse_velocity_mm_per_min = current_rpm * params.wire_diameter_mm;
    float required_traverse_velocity_mm_per_sec = required_traverse_velocity_mm_per_min / 60.0f;
    
    // Convert to steps per second
    float required_steps_per_sec = required_traverse_velocity_mm_per_sec * steps_per_mm;
    
    // Only move if we have meaningful velocity
    if (required_steps_per_sec < 1.0f) {
        return;  // Too slow to matter
    }
    
    float delta_time_sec = delta_time_us / 1000000.0f;
    int32_t steps_to_generate = (int32_t)(required_steps_per_sec * delta_time_sec);
    
    if (steps_to_generate > 0) {
        // Apply direction
        if (!traverse_direction) {
            steps_to_generate = -steps_to_generate;
        }
        
        // ⭐ PERFORMANCE: Disable ALL sync printf during winding (too slow!)
        // Uncomment for debugging only:
        // static uint32_t sync_count = 0;
        // if ((sync_count++ % 100) == 0) {
        //     printf("[SYNC] RPM: %.1f, Velocity: %.3f mm/s, Steps: %d\n", 
        //            current_rpm, required_traverse_velocity_mm_per_sec, abs(steps_to_generate));
        // }
        
        // CRITICAL: Enable MoveQueue and set direction for sync moves
        move_queue->set_enable(true);
        move_queue->set_direction(traverse_direction);
        
        // Generate smooth step commands
        auto chunks = StepCompressor::compress_constant_velocity(
            abs(steps_to_generate), (uint32_t)required_steps_per_sec);
        
        // Push all chunks (no printf for performance)
        for (const auto& chunk : chunks) {
            move_queue->push_chunk(chunk);
        }
        
        // ⭐ CRITICAL: Update position based on steps we QUEUED
        // Note: This tracks where the traverse WILL BE after the queue executes
        float distance_moved_mm = (float)abs(steps_to_generate) / steps_per_mm;
        if (traverse_direction) {
            current_traverse_position_mm += distance_moved_mm;  // Moving RIGHT
        } else {
            current_traverse_position_mm -= distance_moved_mm;  // Moving LEFT
        }
        
        // Position tracking (no printf for performance)
        
        // ⭐ CRITICAL: Update last_sync_time ONLY after successful sync
        last_sync_time = current_time;
    }
}

// =============================================================================
// FIXED: update_rpm() - Now properly updates the cursor variable
// =============================================================================
void WindingController::update_rpm() {
    if (spindle_motor) {
        current_rpm = spindle_motor->get_rpm();
        last_rpm_update_time = time_us_32();
        enc_last_rpm = spindle_motor->get_pulse_count();  // ✅ FIXED: Was missing!
    } else {
        current_rpm = 0.0f;
    }
}

void WindingController::update_display() {
    // Send status to UART for Pi Zero
    char status_buffer[128];
    snprintf(status_buffer, sizeof(status_buffer), 
             "Status: Layer %u/%u, Turns %u/%u, RPM %.1f\n", 
             current_layer, params.total_layers, turns_completed, 
             params.target_turns, current_rpm);
    
    // Send via UART (for Pi Zero)
    for (int i = 0; status_buffer[i] != '\0'; i++) {
        uart_putc(PI_UART_ID, status_buffer[i]);
    }
    
    // ⭐ PERFORMANCE: USB printf is SLOW (~1-5ms). Only print every 50 turns to reduce CPU load
    static uint32_t last_print_turn = 0;
    if (turns_completed - last_print_turn >= 50) {
        printf("Status: Layer %u/%u, Turns %u/%u, RPM %.1f\n", 
               current_layer, params.total_layers, turns_completed, 
               params.target_turns, current_rpm);
        last_print_turn = turns_completed;
    }
}

uint32_t WindingController::mm_to_steps(float mm) {
    // Use original calibrated steps per mm value
    return (uint32_t)(mm * 6135.0f);
}

float WindingController::steps_to_mm(uint32_t steps) {
    // Use original calibrated steps per mm value
    return (float)steps / 6135.0f;
}

void WindingController::adjust_traverse_speed() {
    if (!spindle_motor) return;
    float actual_rpm = spindle_motor->get_rpm();
    if (actual_rpm < 10.0f) return;
    
    // Dynamic speed adjustment based on actual RPM
    float speed_factor = actual_rpm / params.spindle_rpm;
    // Implementation can be added here
}

void WindingController::print_winding_metrics() {
    printf("\n===== Winding Metrics =====\n");
    printf("State: %d\n", (int)state);
    printf("Turns: %u / %u (%.1f%%)\n", turns_completed, params.target_turns,
           (float)turns_completed * 100.0f / params.target_turns);
    printf("Layer: %u / %u\n", current_layer, params.total_layers);
    printf("RPM: %.1f\n", current_rpm);
    printf("Traverse: %.2fmm\n", current_traverse_position_mm);
    printf("===========================\n\n");
}
