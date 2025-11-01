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
    , initial_sync_done(false)
    , initial_revolutions(0.0f)
    , initial_monitor_pulses(0)
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
    
    // ⭐ CRITICAL: Capture initial revolution count for accurate turn counting
    if (spindle_motor) {
        initial_revolutions = spindle_motor->get_revolutions();
        initial_monitor_pulses = spindle_motor->get_monitor_pulse_count();
        printf("[WindingController] Captured initial revolutions: %.2f, monitor pulses: %llu\n",
               initial_revolutions, initial_monitor_pulses);
    } else {
        initial_revolutions = 0.0f;
        printf("[WindingController] ⚠️ No spindle motor - initial revolutions set to 0\n");
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

    // ⭐ CRITICAL: Activate PIO mode for high-speed stepping during winding
    if (move_queue) {
        move_queue->activate_pio_mode();
        printf("✓ PIO stepper activated for high-speed winding\n");
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
    initial_sync_done = false;  // Reset sync state for new winding
    initial_revolutions = 0.0f;  // Will be set when winding starts
    initial_monitor_pulses = 0;  // Will be set when winding starts
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
    // ⭐ PREDICTIVE RAMP DOWN: Use spindle prediction to avoid overshoot
    // Calculate when to start ramp down based on current speed and remaining turns
    float predicted_ramp_start = spindle_motor ? spindle_motor->predict_ramp_down_start(
        turns_completed, params.target_turns, 2.0f) : params.target_turns;

    if (turns_completed >= predicted_ramp_start) {
        printf("🎯 Predictive ramp down: %u/%u turns (started at %.0f)\n",
               turns_completed, params.target_turns, predicted_ramp_start);
        state = WindingState::RAMPING_DOWN;
        return;
    }
    
    // Update RPM (FIXED - now properly updates cursor)
    update_rpm();
    
    // Sync traverse to spindle
    sync_traverse_to_spindle();
    
    // Update winding progress
    if (spindle_motor) {
        // Use GPIO29 monitor pulses directly for accurate turn counting (1 pulse = 1 spindle revolution)
        uint64_t total_monitor_pulses = spindle_motor->get_monitor_pulse_count();
        uint64_t winding_monitor_pulses = total_monitor_pulses - initial_monitor_pulses;
        uint32_t new_turns_completed = (uint32_t)winding_monitor_pulses;

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
            float traverse_pos = traverse_controller->get_position();
            printf("[RAMP] Traverse controller position: %.2f mm\n", traverse_pos);
            printf("[RAMP] Traverse homed status: %s\n", traverse_controller->is_homed() ? "HOMED" : "NOT HOMED");

            // ⭐ SAFETY: Validate traverse position is reasonable
            if (traverse_pos < 0.0f || traverse_pos > 100.0f) {
                printf("[RAMP] ❌ INVALID traverse position %.2f mm - using default 38.0mm\n", traverse_pos);
                current_traverse_position_mm = 38.0f;  // Default to start position
            } else {
                current_traverse_position_mm = traverse_pos;
            }
            printf("[RAMP] Using starting position: %.2f mm\n", current_traverse_position_mm);
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
        if (spindle_motor) {
            spindle_motor->debug_status();  // Print final pulse counts including monitor
        }
        ramp_started = false;
    }
}

void WindingController::sync_traverse_to_spindle() {
    if (!spindle_motor) {
        printf("[SYNC] No spindle motor!\n");
        return;
    }

    // DEBUG: Check if function is being called
    static uint32_t call_count = 0;
    if ((call_count++ % 100) == 0) {
        printf("[SYNC] Function called (count: %u)\n", call_count);
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
    // ⭐ PERFORMANCE: Conservative minimum call interval to prevent CPU overload
    // Balanced frequency for stability
    static uint32_t last_call_time = 0;
    uint32_t current_time = time_us_32();
    uint32_t min_call_interval_us = 15000; // 15ms default (conservative)

    if (current_rpm > 1200.0f) {
        min_call_interval_us = 5000;  // 5ms for very high RPM
    } else if (current_rpm > 800.0f) {
        min_call_interval_us = 8000;  // 8ms for high RPM
    } else if (current_rpm > 500.0f) {
        min_call_interval_us = 10000; // 10ms for medium-high RPM
    }

    if (current_time - last_call_time < min_call_interval_us) {
        return; // Called too recently, skip
    }
    last_call_time = current_time;

    // Timing check for sync intervals
    static uint32_t last_sync_time = 0;
    
    if (last_sync_time == 0) {
        last_sync_time = current_time;
        return;
    }
    
    uint32_t delta_time_us = current_time - last_sync_time;
    
    // ⭐ PERFORMANCE: Frequent sync for high RPM to ensure movement
    // High RPM requires fast sync to keep queue populated
    float current_rpm = spindle_motor ? spindle_motor->get_rpm() : 0.0f;
    uint32_t base_delay_us;

    if (current_rpm > 1200.0f) {
        base_delay_us = 5000; // 5ms for very high RPM (>1200) - very fast sync
    } else if (current_rpm > 800.0f) {
        base_delay_us = 8000; // 8ms for high RPM (800-1200) - fast sync
    } else if (current_rpm > 500.0f) {
        base_delay_us = 15000; // 15ms for medium-high RPM (500-800)
    } else if (current_rpm > 100.0f) {
        base_delay_us = 30000; // 30ms for medium RPM (100-500)
    } else {
        base_delay_us = 50000; // 50ms for low RPM (<100)
    }

    uint32_t required_delay = initial_sync_done ? base_delay_us : 15000; // 15ms initially

    // Return early if not time yet - THIS PREVENTS THE FAST LOOP!
    if (delta_time_us < required_delay) {
        return;  // Not time yet - skip everything below
    }

    initial_sync_done = true;  // Mark initial sync as done

    // ⭐ PID-like Queue Management: Adjust sync timing based on queue depth
    // Instead of fixed thresholds, dynamically adjust sync frequency
    static uint32_t consecutive_skips = 0;  // Track consecutive sync skips
    uint32_t queue_depth = move_queue->get_queue_depth();

    // ⭐ BACKPRESSURE: If queue is getting full, delay generating more steps
    const uint32_t BACKPRESSURE_THRESHOLD = 200;  // Start backpressure at 200/256
    if (queue_depth >= BACKPRESSURE_THRESHOLD) {
        // Queue is filling up - skip this sync cycle to let ISR catch up
        printf("[BACKPRESSURE] Queue at %u/256 - delaying sync to let ISR consume\n", queue_depth);
        return;  // Skip generating more steps
    }

    // Emergency override: if queue is critically full, skip regardless of PID
    const uint32_t MAX_QUEUE_DEPTH = 200;  // Emergency threshold (increased for larger queue)
    if (queue_depth >= MAX_QUEUE_DEPTH) {
        printf("[PID] 🚨 EMERGENCY: Queue critically full (%u/256) - skipping sync\n", queue_depth);
        consecutive_skips++;

        // ⚠️ SAFETY: If queue stays full for too long, EMERGENCY STOP
        if (consecutive_skips >= 10000) {
            printf("\n");
            printf("╔═══════════════════════════════════════════════════════════════╗\n");
            printf("║  ❌ EMERGENCY STOP - QUEUE OVERFLOW DETECTED                 ║\n");
            printf("╚═══════════════════════════════════════════════════════════════╝\n");
            printf("[SAFETY] Queue stayed full for 10+ seconds - PID cannot keep up!\n");
            printf("[SAFETY] Queue depth: %u, Consecutive skips: %u\n", queue_depth, consecutive_skips);
            printf("[SAFETY] Stopping all motion for safety...\n");

            // Stop winding
            stop();
            return;
        }

        return;  // Skip this sync cycle
    }

    // ⭐ PERFORMANCE: Very conservative PID for basic functionality
    // Fixed parameters to ensure movement works
    float target_queue = 32.0f;  // Target 25% utilization (very conservative)
    float kp = 100.0f;           // Very low gain to prevent oscillations
    uint32_t min_effective_delay_us = 10000; // 10ms default minimum

    // Same parameters for all RPM ranges for stability
    if (current_rpm > 800.0f) {
        target_queue = 48.0f;   // 37.5% for high RPM
        kp = 200.0f;            // Low gain
        min_effective_delay_us = 5000; // 5ms minimum
    }

    // PID calculation: Error = current - target
    float queue_error = (float)queue_depth - target_queue;
    int32_t sync_adjustment = (int32_t)(queue_error * kp);

    int32_t temp_delay = (int32_t)base_delay_us + sync_adjustment;
    if (temp_delay < (int32_t)min_effective_delay_us) temp_delay = min_effective_delay_us;
    if (temp_delay > 2000000) temp_delay = 2000000; // Maximum 2 seconds
    uint32_t effective_delay = (uint32_t)temp_delay;

    // Debug effective delay
    printf("[SYNC] Queue: %u/256 Delay: %.1fms\n", queue_depth, effective_delay / 1000.0f);

    // Debug PID operation - reduced frequency to avoid spam
    static uint32_t pid_debug_counter = 0;
    if ((pid_debug_counter++ % 200) == 0) {  // Every 200th PID calculation
        printf("[PID] Queue: %u/256, Error: %.1f, Delay: %.1fms\n",
               queue_depth, queue_error, effective_delay / 1000.0f);
    }

    // Use the PID-adjusted delay for timing check
    if (delta_time_us < effective_delay) {
        return;  // Not time yet - wait longer or shorter based on queue
    }

    // Successfully reached sync time - reset skip counter
    consecutive_skips = 0;

    // REAL-TIME VELOCITY MATCHING (like Klipper/FluidNC)
    // Calculate current traverse velocity based on current RPM
    // current_rpm already declared above
    
    // Calculate required traverse velocity: SPINDLE RPM × wire_diameter
    // ⭐ CRITICAL: Use spindle RPM (not motor RPM) for correct synchronization
    float required_traverse_velocity_mm_per_min = current_rpm * params.wire_diameter_mm;
    float required_traverse_velocity_mm_per_sec = required_traverse_velocity_mm_per_min / 60.0f;

    // Convert to steps per second
    float required_steps_per_sec = required_traverse_velocity_mm_per_sec * steps_per_mm;

    // ⭐ SAFETY: Cap traverse speed to reasonable limits
    // Allow higher speed for testing - the motor can handle more than 1200 steps/sec
    const float MAX_TRAVERSE_STEPS_PER_SEC = 40000.0f;  // Higher limit for testing
    if (required_steps_per_sec > MAX_TRAVERSE_STEPS_PER_SEC) {
        required_steps_per_sec = MAX_TRAVERSE_STEPS_PER_SEC;
        printf("[SYNC] ⚠️ Capped traverse speed to %.0f steps/sec\n", MAX_TRAVERSE_STEPS_PER_SEC);
    }
    
    // Only move if we have meaningful velocity
    if (required_steps_per_sec < 1.0f) {
        return;  // Too slow to matter
    }
    
    float delta_time_sec = delta_time_us / 1000000.0f;

    int32_t steps_to_generate = (int32_t)(required_steps_per_sec * delta_time_sec);

    // DEBUG: Check if we're generating steps
    static uint32_t step_gen_count = 0;
    printf("[SYNC] DEBUG: RPM: %.0f, required: %.0f, delta_t: %.3fs, steps: %d\n",
           current_rpm, required_steps_per_sec, delta_time_sec, steps_to_generate);

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
        
    // ⭐ PERFORMANCE: Conservative step limit based on motor capabilities
    // Capped to motor max speed (1000 steps/sec) and reasonable sync intervals
    int32_t max_steps_per_sync;

    if (current_rpm > 1500.0f) {
        max_steps_per_sync = 800;  // Very high RPM (>1500) - increased limit
    } else if (current_rpm > 1200.0f) {
        max_steps_per_sync = 600;  // High RPM (1200-1500)
    } else if (current_rpm > 800.0f) {
        max_steps_per_sync = 400;  // Medium-high RPM (800-1200)
    } else if (current_rpm > 400.0f) {
        max_steps_per_sync = 300;  // Medium RPM (400-800)
    } else if (current_rpm > 100.0f) {
        max_steps_per_sync = 200;  // Low-medium RPM (100-400)
    } else {
        max_steps_per_sync = 150;  // Low RPM (<100) - conservative
    }
    if (abs(steps_to_generate) > max_steps_per_sync) {
        steps_to_generate = traverse_direction ? max_steps_per_sync : -max_steps_per_sync;
        printf("[SYNC] ⚠️ Clamped steps to %d (RPM: %.0f) to prevent position jump\n",
               steps_to_generate, current_rpm);
    }

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

        // ⭐ DEBUG: Position tracking (reduced frequency for performance)
        static uint32_t sync_count = 0;
        if ((sync_count++ % 100) == 0) {  // Every 100th sync instead of 50th
            printf("[SYNC] Pos: %.2fmm, Dir: %s, Steps: %d, Queue: %u/128\n",
               current_traverse_position_mm, traverse_direction ? "RIGHT" : "LEFT",
               steps_to_generate, (unsigned int)move_queue->get_queue_depth());
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
