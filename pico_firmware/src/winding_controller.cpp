// =============================================================================
// winding_controller.cpp - Winding Controller Implementation
// Purpose: Full winding controller without encoder/LCD dependencies
// =============================================================================

#include "winding_controller.h"
#include "config.h"
#include "spindle.h"
#include "stepcompress.h"
#include "pico/stdlib.h"
#include <cstdio>

// Global BLDC motor instance for spindle control
static BLDC_MOTOR* g_spindle_motor = nullptr;

WindingController::WindingController(MoveQueue* mq)
    : move_queue(mq)
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
    , encoder_sign(0)  // BLDC motor direction sign
    , traverse_steps_emitted(0.0)
    , enc_last_sync(0)
    , enc_last_rpm(0)
{
    printf("[WindingController] Created\n");
}

void WindingController::init() {
    printf("WindingController::init() called\n");
    
    // Use the global spindle controller from main.cpp
    extern BLDC_MOTOR* spindle_controller;
    g_spindle_motor = spindle_controller;
    
    if (g_spindle_motor) {
        g_spindle_motor->set_pulses_per_revolution(BLDC_DEFAULT_PPR);
        printf("WindingController initialized with existing BLDC motor\n");
    } else {
        printf("ERROR: Spindle controller not initialized in main.cpp\n");
    }
}

void WindingController::set_parameters(const WindingParams& p) {
    params = p;
    
    // Calculate derived parameters
    params.wire_pitch_mm = params.wire_diameter_mm;  // Tight winding
    params.turns_per_layer = (uint32_t)(params.layer_width_mm / params.wire_pitch_mm);
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
    
    return true;
}

void WindingController::stop() {
    printf("Stopping winding process\n");
    state = WindingState::IDLE;
    if (g_spindle_motor) {
        g_spindle_motor->set_brake(true);  // Apply brake to stop motor
    }
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
            printf("Winding complete!\n");
            break;
    }
}

void WindingController::emergency_stop() {
    printf("EMERGENCY STOP!\n");
    state = WindingState::IDLE;
    if (g_spindle_motor) {
        g_spindle_motor->set_brake(true);  // Emergency brake
    }
}

void WindingController::home_spindle() {
    printf("Homing spindle...\n");
    
    if (!g_spindle_motor) {
        printf("ERROR: BLDC motor not initialized!\n");
        state = WindingState::ERROR;
        return;
    }
    
    // Start spindle at slow speed for homing
    // The BLDC motor will use PWM control, not step pulses
    // This is handled by the main.cpp UART commands
    
    // For now, simulate homing completion after delay
    // In real implementation, this would wait for Z-index pulse from HAL sensor
    static uint32_t homing_start = 0;
    if (homing_start == 0) {
        homing_start = time_us_32();
    }
    
    if (time_us_32() - homing_start > 2000000) {  // 2 seconds
        printf("Spindle homed\n");
        state = WindingState::HOMING_TRAVERSE;
        homing_start = 0;
    }
}

void WindingController::home_traverse() {
    printf("Starting traverse homing\n");
    
    // Create homing move using StepCompressor
    uint32_t homing_steps = 10000;  // Large number of steps to ensure we hit home
    auto chunks = StepCompressor::compress_trapezoid(
        homing_steps, 0.0, TRAVERSE_HOMING_SPEED, TRAVERSE_RAPID_ACCEL);
    
    // Queue the chunks
    for (const auto& chunk : chunks) {
        move_queue->push_chunk(AXIS_TRAVERSE, chunk);
    }
    
    // Wait for homing to complete
    static uint32_t homing_start = 0;
    if (homing_start == 0) {
        homing_start = time_us_32();
    }
    
    if (time_us_32() - homing_start > 3000000) {  // 3 seconds
        printf("Traverse homed\n");
        current_traverse_position_mm = 0.0f;
        state = WindingState::MOVING_TO_START;
        homing_start = 0;
    }
}

void WindingController::move_to_start() {
    printf("Moving to start position: %.1f mm\n", params.start_position_mm);
    
    uint32_t steps = mm_to_steps(params.start_position_mm);
    
    // Create move using StepCompressor
    auto chunks = StepCompressor::compress_trapezoid(
        steps, 0.0, TRAVERSE_RAPID_SPEED, TRAVERSE_RAPID_ACCEL);
    
    // Queue the chunks
    for (const auto& chunk : chunks) {
        move_queue->push_chunk(AXIS_TRAVERSE, chunk);
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
    
    // Calculate current RPM based on ramp time
    uint32_t elapsed = time_us_32() - ramp_start_time;
    float ramp_progress = (float)elapsed / (params.ramp_time_sec * 1000000.0f);
    
    if (ramp_progress > 1.0f) ramp_progress = 1.0f;
    
    float current_target_rpm = params.spindle_rpm * ramp_progress;
    
    // BLDC motor speed control is handled by main.cpp UART commands
    // The winding controller just tracks the target RPM
    current_rpm = current_target_rpm;
    
    if (ramp_progress >= 1.0f) {
        printf("Spindle ramp up complete\n");
        state = WindingState::WINDING;
        ramp_started = false;
    }
}

void WindingController::execute_winding() {
    // Check if we've reached target turns
    if (turns_completed >= params.target_turns) {
        printf("Target turns reached! Stopping spindle.\n");
        state = WindingState::RAMPING_DOWN;
        return;
    }
    
    // Calculate current layer progress
    uint32_t turns_in_current_layer = turns_completed % params.turns_per_layer;
    
    // Check if we need to move to next layer
    if (turns_in_current_layer == 0 && turns_completed > 0) {
        current_layer++;
        printf("Starting layer %u/%u\n", current_layer, params.total_layers);
        
        // Move traverse to next layer position
        float layer_position = params.start_position_mm + (current_layer * params.wire_diameter_mm);
        uint32_t steps = mm_to_steps(layer_position - current_traverse_position_mm);
        
        if (steps > 0) {
            // Create move using StepCompressor
            auto chunks = StepCompressor::compress_trapezoid(
                steps, 0.0, TRAVERSE_MIN_WINDING_SPEED, TRAVERSE_RAPID_ACCEL);
            
            // Queue the chunks
            for (const auto& chunk : chunks) {
                move_queue->push_chunk(AXIS_TRAVERSE, chunk);
            }
            
            current_traverse_position_mm = layer_position;
        }
    }
    
    // Critical: Update RPM from encoder feedback
    update_rpm();
    
    // Critical: Sync traverse to spindle rotation
    sync_traverse_to_spindle();
    
    // Update winding progress using BLDC motor turn counting
    if (g_spindle_motor) {
        // Get total revolutions from BLDC motor
        float total_revolutions = g_spindle_motor->get_revolutions();
        
        // Update turns completed based on actual motor revolutions
        uint32_t new_turns_completed = (uint32_t)total_revolutions;
        
        if (new_turns_completed > turns_completed) {
            uint32_t delta_turns = new_turns_completed - turns_completed;
            turns_completed = new_turns_completed;
            turns_this_layer += delta_turns;
            
            if (turns_completed % 100 == 0) {
                printf("Progress: %u/%u turns (%.1f%%)\n", 
                       turns_completed, params.target_turns,
                       (float)turns_completed * 100.0f / params.target_turns);
            }
            
            // Critical: Update display with current status
            update_display();
        }
    }
}

void WindingController::ramp_down_spindle() {
    printf("Ramping down spindle...\n");
    
    // Calculate ramp down progress
    static uint32_t ramp_start = 0;
    if (ramp_start == 0) {
        ramp_start = time_us_32();
    }
    
    uint32_t elapsed = time_us_32() - ramp_start;
    float ramp_progress = (float)elapsed / (params.ramp_time_sec * 1000000.0f);
    
    if (ramp_progress > 1.0f) ramp_progress = 1.0f;
    
    float target_rpm = params.spindle_rpm * (1.0f - ramp_progress);
    current_rpm = target_rpm;
    
    if (target_rpm > 0.0f) {
        // BLDC motor speed control is handled by main.cpp UART commands
        printf("Ramping down to %.1f RPM\n", target_rpm);
    } else {
        if (g_spindle_motor) {
            g_spindle_motor->set_brake(true);  // Stop motor
        }
        printf("Spindle stopped\n");
        state = WindingState::COMPLETE;
        ramp_start = 0;
    }
}

void WindingController::sync_traverse_to_spindle() {
    // Sync traverse movement to spindle rotation using BLDC motor pulse counting
    if (!g_spindle_motor) return;
    
    // Get current pulse count from BLDC motor HAL sensor
    uint32_t current_pulses = g_spindle_motor->get_pulse_count();
    uint32_t pulse_delta = current_pulses - enc_last_sync;
    
    if (pulse_delta > 0) {
        // Calculate how many traverse steps needed based on spindle rotation
        float turns = (float)pulse_delta / (float)BLDC_DEFAULT_PPR;
        
        // Calculate traverse steps needed (this is the core winding logic)
        float traverse_steps_needed = turns * params.wire_diameter_mm * (200.0f * TRAVERSE_MICROSTEPS) / TRAVERSE_PITCH_MM;
        
        if (traverse_steps_needed > 0) {
            // Create traverse move using StepCompressor
            auto chunks = StepCompressor::compress_constant_velocity(
                (uint32_t)traverse_steps_needed, TRAVERSE_MIN_WINDING_SPEED);
            
            // Queue the chunks
            for (const auto& chunk : chunks) {
                move_queue->push_chunk(AXIS_TRAVERSE, chunk);
            }
            
            // Update position tracking
            current_traverse_position_mm += (traverse_steps_needed * TRAVERSE_PITCH_MM) / (200.0f * TRAVERSE_MICROSTEPS);
            traverse_steps_emitted += traverse_steps_needed;
        }
        
        enc_last_sync = current_pulses;
    }
}

void WindingController::update_rpm() {
    // Get RPM from BLDC motor HAL sensor
    if (g_spindle_motor) {
        current_rpm = g_spindle_motor->get_rpm();
        
        // Update last RPM calculation time
        last_rpm_update_time = time_us_32();
    } else {
        current_rpm = 0.0f;
    }
}

void WindingController::update_display() {
    // Simplified display update without LCD
    // In real implementation, this would update LCD display
    printf("Status: Layer %u/%u, Turns %u/%u, RPM %.1f\n", 
           current_layer, params.total_layers, turns_completed, params.target_turns, current_rpm);
}

uint32_t WindingController::mm_to_steps(float mm) {
    float revs = mm / TRAVERSE_PITCH_MM;
    return (uint32_t)(revs * 200.0f * TRAVERSE_MICROSTEPS);
}

float WindingController::steps_to_mm(uint32_t steps) {
    float revs = (float)steps / (200.0f * TRAVERSE_MICROSTEPS);
    return revs * TRAVERSE_PITCH_MM;
}

// =============================================================================
// Public homing methods
// =============================================================================
void WindingController::home_all_axes() {
    printf("[WindingController] Homing all axes\n");
    home_traverse();
    home_spindle();
}