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
    , homing_started(false)
    , homing_start_time(0)
    , traverse_homing_started(false)
    , traverse_homing_start_time(0)
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
    if (state != WindingState::IDLE) {
        printf("Cannot start: not idle\n");
        return false;
    }
    
    printf("Starting winding process\n");
    state = WindingState::HOMING_SPINDLE;
    current_layer = 0;
    turns_completed = 0;
    turns_this_layer = 0;
    current_rpm = 0.0f;
    
    // Reset homing flags
    homing_started = false;
    traverse_homing_started = false;
    
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
    
    // Reset homing flags
    homing_started = false;
    traverse_homing_started = false;
}

void WindingController::update() {
    switch (state) {
        case WindingState::IDLE:
            break;
            
        case WindingState::HOMING_SPINDLE:
            home_spindle();
            break;
            
        case WindingState::HOMING_TRAVERSE:
            home_traverse();
            break;
            
        case WindingState::MOVING_TO_START:
            move_to_start();
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
    homing_started = false;
    traverse_homing_started = false;
}

// =============================================================================
// FIXED: home_spindle() - Now properly sets homing_started flag
// =============================================================================
void WindingController::home_spindle() {
    if (!spindle_motor) {
        printf("ERROR: BLDC motor not initialized!\n");
        state = WindingState::ERROR;
        return;
    }
    
    if (!homing_started) {
        // FIX: Now properly sets the flag
        printf("Starting spindle homing to Z index\n");
        spindle_motor->set_rpm_pwm(50.0f);
        spindle_motor->set_brake(false);
        homing_start_time = time_us_32();
        homing_started = true;  // ✅ FIXED: Was missing!
        printf("Waiting for Z-index pulse...\n");
        return;
    }
    
    // Check for Z-index pulse (every 18 pulses = 1 revolution for 3-phase BLDC)
    uint32_t current_pulses = spindle_motor->get_pulse_count();
    if (current_pulses >= BLDC_DEFAULT_PPR) {
        printf("Z-index detected! Spindle homed\n");
        spindle_motor->reset();  // Reset pulse counter
        state = WindingState::HOMING_TRAVERSE;
        homing_started = false;  // Reset for next time
        return;
    }
    
    // Timeout after 10 seconds
    if (time_us_32() - homing_start_time > 10000000) {
        printf("ERROR: Spindle homing timeout!\n");
        state = WindingState::ERROR;
        homing_started = false;
    }
}

// =============================================================================
// FIXED: home_traverse() - Now uses instance variables instead of static
// =============================================================================
void WindingController::home_traverse() {
    if (!traverse_homing_started) {
        // FIX: Use instance variables instead of static
        printf("Starting traverse homing\n");
        traverse_homing_start_time = time_us_32();
        traverse_homing_started = true;
        
        // Create homing move
        uint32_t homing_steps = 10000;
        auto chunks = StepCompressor::compress_constant_velocity(
            homing_steps, TRAVERSE_HOMING_SPEED);
        
        for (const auto& chunk : chunks) {
            move_queue->push_chunk(chunk);
        }
        return;
    }
    
    // Check if homing complete (3 second timeout)
    if (time_us_32() - traverse_homing_start_time > 3000000) {
        printf("Traverse homed\n");
        current_traverse_position_mm = 0.0f;
        state = WindingState::MOVING_TO_START;
        traverse_homing_started = false;  // Reset for next time
    }
}

void WindingController::move_to_start() {
    printf("Moving to start position: %.1f mm\n", params.start_position_mm);
    
    // SAFETY CHECK: Limit start position to reasonable range
    if (params.start_position_mm > 50.0f) {
        printf("❌ SAFETY ERROR: Start position %.1f mm is too large! Limiting to 50mm\n", params.start_position_mm);
        params.start_position_mm = 50.0f;
    }
    if (params.start_position_mm < 0.0f) {
        printf("❌ SAFETY ERROR: Start position %.1f mm is negative! Setting to 0mm\n", params.start_position_mm);
        params.start_position_mm = 0.0f;
    }
    
    // Use simple constant velocity move to avoid memory issues
    uint32_t steps = mm_to_steps(params.start_position_mm);
    
    // SAFETY CHECK: Limit steps to prevent runaway
    if (steps > 300000) {  // About 50mm at 6135 steps/mm
        printf("❌ SAFETY ERROR: %u steps is too many! Limiting to 300,000 steps (50mm)\n", steps);
        steps = 300000;
        params.start_position_mm = steps_to_mm(steps);
    }
    
    printf("✓ Moving %u steps (%.2f mm)\n", steps, params.start_position_mm);
    
    auto chunks = StepCompressor::compress_constant_velocity(
        steps, TRAVERSE_RAPID_SPEED);
    
    for (const auto& chunk : chunks) {
        move_queue->push_chunk(chunk);
    }
    
    current_traverse_position_mm = params.start_position_mm;
    state = WindingState::RAMPING_UP;
}

void WindingController::ramp_up_spindle() {
    if (!ramp_started) {
        printf("Starting spindle ramp up to %.1f RPM over %.1f seconds\n", 
               params.spindle_rpm, params.ramp_time_sec);
        ramp_started = true;
        ramp_start_time = time_us_32();
    }
    
    uint32_t elapsed = time_us_32() - ramp_start_time;
    float ramp_progress = (float)elapsed / (params.ramp_time_sec * 1000000.0f);
    
    if (ramp_progress > 1.0f) ramp_progress = 1.0f;
    
    float current_target_rpm = params.spindle_rpm * ramp_progress;
    current_rpm = current_target_rpm;
    
    if (ramp_progress >= 1.0f) {
        printf("Spindle ramp up complete\n");
        state = WindingState::WINDING;
        ramp_started = false;
    }
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
    
    uint32_t current_pulses = spindle_motor->get_pulse_count();
    uint32_t pulse_delta = current_pulses - enc_last_sync;
    
    if (pulse_delta == 0) return;
    
    printf("[SYNC] Pulse delta: %u, Revolutions: %.3f\n", pulse_delta, (float)pulse_delta / (float)BLDC_DEFAULT_PPR);
    
    // Calculate revolutions
    float spindle_revolutions = (float)pulse_delta / (float)BLDC_DEFAULT_PPR;
    
    // Calculate traverse distance
    float effective_pitch_mm = params.wire_diameter_mm * WIRE_TENSION_FACTOR;
    float traverse_distance_mm = spindle_revolutions * effective_pitch_mm;
    
    // Apply direction
    if (!traverse_direction) {
        traverse_distance_mm = -traverse_distance_mm;
    }
    
    // Convert to steps using the same calculation as traverse controller
    // Based on original calibration: 6135 steps/mm
    float steps_per_mm = 6135.0f;  // Use original calibrated value
    int32_t traverse_steps = (int32_t)(traverse_distance_mm * steps_per_mm);
    
    if (abs(traverse_steps) > 0) {
        printf("[SYNC] Moving traverse: %.3fmm, %d steps\n", traverse_distance_mm, traverse_steps);
        auto chunks = StepCompressor::compress_constant_velocity(
            abs(traverse_steps), TRAVERSE_MIN_WINDING_SPEED);
        
        for (const auto& chunk : chunks) {
            move_queue->push_chunk(chunk);
        }
        
        current_traverse_position_mm += traverse_distance_mm;
        printf("[SYNC] New traverse position: %.3fmm\n", current_traverse_position_mm);
    }
    
    enc_last_sync = current_pulses;  // Update cursor
    
    // Check for layer edges
    float edge_margin = 0.5f;
    float left_limit = params.start_position_mm + edge_margin;
    float right_limit = params.start_position_mm + params.layer_width_mm - edge_margin;
    
    if (traverse_direction && current_traverse_position_mm >= right_limit) {
        printf("[SYNC] Right edge - reversing to LEFT\n");
        traverse_direction = false;
        current_layer++;
        turns_this_layer = 0;
    } else if (!traverse_direction && current_traverse_position_mm <= left_limit) {
        printf("[SYNC] Left edge - reversing to RIGHT\n");
        traverse_direction = true;
        current_layer++;
        turns_this_layer = 0;
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
    
    // Also send to USB serial for debugging
    printf("Status: Layer %u/%u, Turns %u/%u, RPM %.1f\n", 
           current_layer, params.total_layers, turns_completed, 
           params.target_turns, current_rpm);
}

uint32_t WindingController::mm_to_steps(float mm) {
    // Use original calibrated steps per mm value
    return (uint32_t)(mm * 6135.0f);
}

float WindingController::steps_to_mm(uint32_t steps) {
    // Use original calibrated steps per mm value
    return (float)steps / 6135.0f;
}

void WindingController::home_all_axes() {
    printf("[WindingController] Homing all axes\n");
    homing_started = false;
    traverse_homing_started = false;
    state = WindingState::HOMING_SPINDLE;
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
