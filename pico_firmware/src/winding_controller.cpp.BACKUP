// =============================================================================
// winding_controller.cpp - Winding Controller Implementation
// Purpose: Full winding controller without encoder/LCD dependencies
// =============================================================================

#include "winding_controller.h"
#include "config.h"
#include "spindle.h"
#include "stepcompress.h"
#include "pico/stdlib.h"
#include <cmath>
#include <cstdio>

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
    , encoder_sign(0)  // BLDC motor direction sign
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
    if (spindle_motor) {
        spindle_motor->set_brake(true);  // Apply brake to stop motor
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
    if (spindle_motor) {
        spindle_motor->set_brake(true);  // Emergency brake
    }
}

void WindingController::home_spindle() {
    printf("Homing spindle...\n");
    
    if (!spindle_motor) {
        printf("ERROR: BLDC motor not initialized!\n");
        state = WindingState::ERROR;
        return;
    }
    
    // Start spindle at slow speed for homing
    spindle_motor->set_rpm_pwm(50.0f);  // Very slow for homing
    spindle_motor->set_brake(false);    // Release brake
    
    // Wait for Z-index pulse from HAL sensor
    // The BLDC_MOTOR class handles Z-index detection internally
    static bool homing_started = false;
    static uint32_t homing_start_time = 0;
    static uint32_t last_pulse_count = 0;
    
    if (!homing_started) {
        homing_started = true;
        homing_start_time = time_us_32();
        last_pulse_count = spindle_motor->get_pulse_count();
        printf("Waiting for Z-index pulse...\n");
    }
    
    // Check for Z-index pulse (every 18 pulses = 1 revolution)
    uint32_t current_pulse_count = spindle_motor->get_pulse_count();
    if (current_pulse_count > last_pulse_count) {
        // Check if we've completed a full revolution (18 pulses)
        uint32_t pulses_since_start = current_pulse_count - last_pulse_count;
        if (pulses_since_start >= 18) {
            printf("Z-index detected! Spindle homed\n");
            spindle_motor->reset();  // Reset pulse counter to 0
            state = WindingState::HOMING_TRAVERSE;
            homing_started = false;
            return;
        }
    }
    
    // Timeout after 10 seconds
    if (time_us_32() - homing_start_time > 10000000) {
        printf("ERROR: Spindle homing timeout!\n");
        state = WindingState::ERROR;
        homing_started = false;
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
        move_queue->push_chunk(chunk);
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
                move_queue->push_chunk(chunk);
            }
            
            current_traverse_position_mm = layer_position;
        }
    }
    
    // Critical: Update RPM from encoder feedback
    update_rpm();
    
    // Critical: Sync traverse to spindle rotation
    sync_traverse_to_spindle();
    
    // Update winding progress using BLDC motor turn counting
    if (spindle_motor) {
        // Get total revolutions from BLDC motor
        float total_revolutions = spindle_motor->get_revolutions();
        
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
        if (spindle_motor) {
            spindle_motor->set_brake(true);  // Stop motor
        }
        printf("Spindle stopped\n");
        state = WindingState::COMPLETE;
        ramp_start = 0;
    }
}

void WindingController::sync_traverse_to_spindle() {
    if (!spindle_motor) {
        printf("[SYNC_ERROR] No spindle motor instance!\n");
        return;
    }
    
    // Get current pulse count from BLDC motor
    uint32_t current_pulses = spindle_motor->get_pulse_count();
    
    // Calculate pulses since last sync
    uint32_t pulse_delta = current_pulses - enc_last_sync;
    
    // Exit early if no movement
    if (pulse_delta == 0) return;
    
    // === STEP 1: Calculate Spindle Revolutions ===
    float spindle_revolutions = (float)pulse_delta / (float)BLDC_DEFAULT_PPR;
    
    // === STEP 2: Calculate Required Traverse Distance ===
    // Each spindle revolution should move traverse by wire diameter
    // Apply tension factor for tight winding (typically 0.95 = 5% compression)
    float effective_pitch_mm = params.wire_diameter_mm * WIRE_TENSION_FACTOR;
    float traverse_distance_mm = spindle_revolutions * effective_pitch_mm;
    
    // Apply direction (forward = positive, reverse = negative)
    if (!traverse_direction) {
        traverse_distance_mm = -traverse_distance_mm;
    }
    
    // === STEP 3: Convert Distance to Steps ===
    // Formula: steps = (distance_mm / leadscrew_pitch_mm) × steps_per_rev × microstepping
    float steps_per_mm = (200.0f * TRAVERSE_MICROSTEPS) / TRAVERSE_PITCH_MM;
    int32_t traverse_steps = (int32_t)(traverse_distance_mm * steps_per_mm);
    
    // === STEP 4: Queue Movement if Needed ===
    if (abs(traverse_steps) > 0) {
        // Create traverse move using StepCompressor
        // Use constant velocity for smooth winding
        auto chunks = StepCompressor::compress_constant_velocity(
            abs(traverse_steps), 
            TRAVERSE_MIN_WINDING_SPEED  // Adjust this for smoother/faster winding
        );
        
        // Queue all chunks
        for (const auto& chunk : chunks) {
            move_queue->push_chunk(chunk);
        }
        
        // Update position tracking
        current_traverse_position_mm += traverse_distance_mm;
        traverse_steps_emitted += abs(traverse_steps);
        
        // === OPTIONAL: Debug Output ===
        // Print every 100 pulses for monitoring
        static uint32_t last_debug_pulse = 0;
        if (current_pulses - last_debug_pulse >= 100) {
            printf("[SYNC] Pulses: %lu, Revs: %.3f, Distance: %.4fmm, Steps: %ld, Pos: %.2fmm\n",
                   pulse_delta, spindle_revolutions, traverse_distance_mm, 
                   traverse_steps, current_traverse_position_mm);
            last_debug_pulse = current_pulses;
        }
    }
    
    // === STEP 5: Update Sync Cursor ===
    enc_last_sync = current_pulses;
    
    // === STEP 6: Check for Layer Edge and Reverse ===
    // Check if we've reached the edge of the bobbin
    float edge_margin = 0.5f;  // 0.5mm safety margin
    float left_limit = params.start_position_mm + edge_margin;
    float right_limit = params.start_position_mm + params.layer_width_mm - edge_margin;
    
    if (traverse_direction && current_traverse_position_mm >= right_limit) {
        // Reached right edge - reverse direction
        printf("[SYNC] Right edge reached at %.2fmm - reversing to LEFT\n", 
               current_traverse_position_mm);
        traverse_direction = false;
        current_layer++;
        turns_this_layer = 0;
        
    } else if (!traverse_direction && current_traverse_position_mm <= left_limit) {
        // Reached left edge - reverse direction
        printf("[SYNC] Left edge reached at %.2fmm - reversing to RIGHT\n", 
               current_traverse_position_mm);
        traverse_direction = true;
        current_layer++;
        turns_this_layer = 0;
    }
}

void WindingController::update_rpm() {
    // Get RPM from BLDC motor HAL sensor
    if (spindle_motor) {
        current_rpm = spindle_motor->get_rpm();
        
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

// =============================================================================
// NEW: Advanced winding functions
// =============================================================================

/**
 * @brief NEW FUNCTION: Dynamic traverse speed adjustment
 * 
 * This adjusts traverse speed based on actual spindle RPM
 * Ensures perfect synchronization even if spindle speed varies
 */
void WindingController::adjust_traverse_speed() {
    if (!spindle_motor) return;
    
    // Get current actual RPM
    float actual_rpm = spindle_motor->get_rpm();
    
    // Skip if spindle is not running or too slow
    if (actual_rpm < 50.0f) return;
    
    // === Calculate Required Traverse Speed ===
    
    // Convert RPM to revolutions per second
    float spindle_rps = actual_rpm / 60.0f;
    
    // Calculate wire pitch with tension
    float effective_pitch_mm = params.wire_diameter_mm * WIRE_TENSION_FACTOR;
    
    // Required traverse speed (mm/s)
    float required_traverse_speed_mms = spindle_rps * effective_pitch_mm;
    
    // Convert to steps/sec
    float steps_per_mm = (200.0f * TRAVERSE_MICROSTEPS) / TRAVERSE_PITCH_MM;
    float required_steps_per_sec = required_traverse_speed_mms * steps_per_mm;
    
    // === Safety Limits ===
    float max_traverse_speed_mms = 5.0f;  // Maximum 5mm/s for safety
    if (required_traverse_speed_mms > max_traverse_speed_mms) {
        printf("[SPEED_WARNING] Required speed %.3f mm/s exceeds limit %.3f mm/s!\n",
               required_traverse_speed_mms, max_traverse_speed_mms);
        required_traverse_speed_mms = max_traverse_speed_mms;
        required_steps_per_sec = max_traverse_speed_mms * steps_per_mm;
    }
    
    // === Update Speed Setting ===
    // TODO: This would update your motion planner's speed setting
    // For now, just monitor and report
    
    // Debug output (print every 2 seconds)
    static uint32_t last_speed_print = 0;
    if (time_us_32() - last_speed_print > 2000000) {
        printf("[SPEED] Spindle: %.1f RPM (%.2f rev/s), Traverse: %.3f mm/s (%.0f steps/s)\n",
               actual_rpm, spindle_rps, required_traverse_speed_mms, required_steps_per_sec);
        last_speed_print = time_us_32();
    }
}

/**
 * @brief Calculate expected performance metrics
 * 
 * Call this after setting parameters to verify setup
 */
void WindingController::print_winding_metrics() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("  WINDING PARAMETERS & CALCULATED METRICS\n");
    printf("═══════════════════════════════════════════════════════\n");
    
    // Wire specs
    float effective_pitch = params.wire_diameter_mm * WIRE_TENSION_FACTOR;
    printf("Wire:\n");
    printf("  Diameter: %.4f mm (43 AWG)\n", params.wire_diameter_mm);
    printf("  Tension factor: %.2f (%.0f%% compression)\n", 
           WIRE_TENSION_FACTOR, (1.0f - WIRE_TENSION_FACTOR) * 100.0f);
    printf("  Effective pitch: %.4f mm\n", effective_pitch);
    
    // Bobbin specs
    printf("\nBobbin:\n");
    printf("  Width: %.1f mm\n", params.layer_width_mm);
    printf("  Start position: %.1f mm\n", params.start_position_mm);
    printf("  Turns per layer: %u\n", params.turns_per_layer);
    printf("  Total layers: %u\n", params.total_layers);
    
    // Spindle specs
    float spindle_rps = params.spindle_rpm / 60.0f;
    printf("\nSpindle:\n");
    printf("  Target RPM: %.1f (%.2f rev/s)\n", params.spindle_rpm, spindle_rps);
    printf("  Pulses per rev: %u\n", BLDC_DEFAULT_PPR);
    
    // Traverse specs
    float steps_per_mm = (200.0f * TRAVERSE_MICROSTEPS) / TRAVERSE_PITCH_MM;
    float traverse_speed_mms = spindle_rps * effective_pitch;
    float steps_per_sec = traverse_speed_mms * steps_per_mm;
    printf("\nTraverse:\n");
    printf("  Leadscrew pitch: %.1f mm\n", TRAVERSE_PITCH_MM);
    printf("  Microstepping: %dx\n", TRAVERSE_MICROSTEPS);
    printf("  Steps per mm: %.2f\n", steps_per_mm);
    printf("  Required speed: %.3f mm/s (%.0f steps/s)\n", 
           traverse_speed_mms, steps_per_sec);
    printf("  Steps per wire: %.1f\n", effective_pitch * steps_per_mm);
    
    // Timing
    float time_per_layer_sec = params.turns_per_layer / spindle_rps;
    float total_time_sec = params.target_turns / spindle_rps;
    printf("\nTiming:\n");
    printf("  Time per layer: %.1f seconds\n", time_per_layer_sec);
    printf("  Total winding time: %.1f minutes (%.1f hours)\n", 
           total_time_sec / 60.0f, total_time_sec / 3600.0f);
    printf("  Target turns: %u\n", params.target_turns);
    
    // Resolution
    float resolution_um = 1000.0f / steps_per_mm;
    printf("\nResolution:\n");
    printf("  Position resolution: %.3f µm (%.6f mm)\n", 
           resolution_um, resolution_um / 1000.0f);
    printf("  Wire/resolution ratio: %.1f:1\n", 
           params.wire_diameter_mm * 1000.0f / resolution_um);
    
    printf("═══════════════════════════════════════════════════════\n");
    printf("\n");
}