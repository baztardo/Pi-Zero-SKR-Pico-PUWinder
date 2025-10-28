// =============================================================================
// traverse_controller.cpp - Traverse Stepper Motor Controller Implementation
// Purpose: Control traverse stepper motor for winding machine
// =============================================================================

#include "traverse_controller.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include <cstdio>
#include <cmath>

// Global instance for ISR
TraverseController* TraverseController::instance = nullptr;
TraverseController* g_traverse_controller = nullptr;

// =============================================================================
// Constructor
// =============================================================================
TraverseController::TraverseController()
    : step_pin(TRAVERSE_STEP_PIN)
    , dir_pin(TRAVERSE_DIR_PIN)
    , enable_pin(TRAVERSE_ENA_PIN)
    , home_pin(TRAVERSE_HOME_PIN)
    , current_position_mm(0.0f)
    , target_position_mm(0.0f)
    , current_speed_mm_per_sec(0.0f)
    , max_speed_mm_per_sec(50.0f)  // 50 mm/s max
    , acceleration_mm_per_sec2(100.0f)  // 100 mm/s²
    , moving(false)
    , homing(false)
    , homed(false)
    , enabled(false)
    , emergency_stopped(false)
    , homing_phase(0)
    , steps_remaining(0)
    , step_interval_us(0)
    , last_step_time(0)
    , step_direction(true)
    , steps_per_mm(6135.0f)  // 6135 steps/mm (calibrated value)
    , microsteps(TRAVERSE_MICROSTEPS)
{
    instance = this;
    g_traverse_controller = this;
    printf("[TraverseController] Created\n");
}

// =============================================================================
// Destructor
// =============================================================================
TraverseController::~TraverseController() {
    disable();
    instance = nullptr;
    g_traverse_controller = nullptr;
}

// =============================================================================
// Initialize hardware
// =============================================================================
void TraverseController::init() {
    printf("[TraverseController] Initializing...\n");
    
    // Configure step pin
    gpio_init(step_pin);
    gpio_set_dir(step_pin, GPIO_OUT);
    gpio_put(step_pin, 0);
    
    // Configure direction pin
    gpio_init(dir_pin);
    gpio_set_dir(dir_pin, GPIO_OUT);
    gpio_put(dir_pin, 0);
    
    // Configure enable pin
    gpio_init(enable_pin);
    gpio_set_dir(enable_pin, GPIO_OUT);
    gpio_put(enable_pin, 1);  // Start disabled (active low)
    
    // Configure home switch
    gpio_init(home_pin);
    gpio_set_dir(home_pin, GPIO_IN);
    gpio_pull_up(home_pin);
    
    // Calculate steps per mm - based on original calibration
    // Original calibration: 10000 steps = 1.63mm
    // So: steps_per_mm = 10000 / 1.63 = 6135 steps/mm
    steps_per_mm = 10000.0f / 1.63f;  // Based on original calibration
    printf("[TraverseController] Steps per mm calculation: 10000 / 1.63 = %.1f (original calibration)\n", steps_per_mm);
    
    printf("[TraverseController] Initialized - Steps/mm: %.1f\n", steps_per_mm);
}

// =============================================================================
// Enable/disable stepper
// =============================================================================
void TraverseController::enable() {
    gpio_put(enable_pin, 0);  // Active low
    enabled = true;
    printf("[TraverseController] Enabled\n");
}

void TraverseController::disable() {
    gpio_put(enable_pin, 1);  // Active low
    enabled = false;
    moving = false;
    printf("[TraverseController] Disabled\n");
}

// =============================================================================
// Move to absolute position
// =============================================================================
void TraverseController::move_to_position(float position_mm) {
    if (!enabled || emergency_stopped) {
        printf("[TraverseController] Cannot move - disabled or emergency stopped\n");
        return;
    }
    
    target_position_mm = position_mm;
    float distance = target_position_mm - current_position_mm;
    
    if (fabs(distance) < 0.01f) {  // Already at position
        printf("[TraverseController] Already at position %.2f mm\n", position_mm);
        return;
    }
    
    // Calculate steps needed
    steps_remaining = (int32_t)(fabs(distance) * steps_per_mm);
    step_direction = (distance > 0);
    
    // Set direction pin
    gpio_put(dir_pin, step_direction ? 1 : 0);
    
    // Calculate step interval based on speed
    calculate_step_interval();
    
    moving = true;
    printf("[TraverseController] Moving to %.2f mm (%d steps)\n", position_mm, steps_remaining);
}

// =============================================================================
// Move relative distance
// =============================================================================
void TraverseController::move_relative(float distance_mm) {
    move_to_position(current_position_mm + distance_mm);
}

// =============================================================================
// Home the traverse
// =============================================================================
void TraverseController::home() {
    if (!enabled || emergency_stopped) {
        printf("[TraverseController] Cannot home - disabled or emergency stopped\n");
        return;
    }
    
    printf("[TraverseController] Starting homing sequence...\n");
    
    // Check initial home switch state
    bool home_state = gpio_get(home_pin);
    printf("[TraverseController] Home switch initial state: %s\n", home_state ? "HIGH (not triggered)" : "LOW (triggered)");
    
    // Enable stepper
    gpio_put(enable_pin, 0);  // Enable (active low)
    
    // Phase 1: Move towards home switch (if not already triggered)
    if (home_state) {  // Switch not triggered, move towards it
        printf("[TraverseController] Phase 1: Moving towards home switch (no limit)...\n");
        gpio_put(dir_pin, 0);  // Move towards home
        step_direction = false;
        steps_remaining = 800000;  // 800k steps = ~130mm - enough for full 120mm travel
        current_speed_mm_per_sec = 20.0f;  // Faster homing speed
        calculate_step_interval();
        moving = true;
        homing = true;
        homing_phase = 1;  // Phase 1: moving towards switch
    } else {
        // Switch already triggered, go to phase 2 (back off)
        printf("[TraverseController] Switch already triggered, going to phase 2...\n");
        homing_phase = 2;
        back_off_from_switch();
    }
    
    printf("[TraverseController] Homing phase %d started\n", homing_phase);
}

void TraverseController::back_off_from_switch() {
    printf("[TraverseController] Phase 2: Backing off from home switch...\n");
    gpio_put(dir_pin, 1);  // Move away from home (opposite of homing direction)
    step_direction = true;
    steps_remaining = 49080;  // Back off exactly 8mm (8 * 6135 = 49080 steps)
    current_speed_mm_per_sec = 10.0f;  // Faster back-off speed
    calculate_step_interval();
    moving = true;
    homing = true;
    homing_phase = 2;
}

void TraverseController::move_to_start_position() {
    printf("[TraverseController] Phase 3: Moving to start position (22mm)...\n");
    target_position_mm = 22.0f;  // 20mm + 2mm offset
    float distance = target_position_mm - current_position_mm;
    steps_remaining = (int32_t)(distance * steps_per_mm);
    
    if (distance > 0) {
        gpio_put(dir_pin, 1);  // Move away from home
        step_direction = true;
    } else {
        gpio_put(dir_pin, 0);  // Move towards home
        step_direction = false;
    }
    
    current_speed_mm_per_sec = 20.0f;  // Fast speed to start position
    calculate_step_interval();
    moving = true;
    homing = true;
    homing_phase = 3;
}

// =============================================================================
// Set speed
// =============================================================================
void TraverseController::set_speed(float speed_mm_per_sec) {
    if (speed_mm_per_sec > 0 && speed_mm_per_sec <= max_speed_mm_per_sec) {
        current_speed_mm_per_sec = speed_mm_per_sec;
        calculate_step_interval();
        printf("[TraverseController] Speed set to %.1f mm/s\n", speed_mm_per_sec);
    }
}

// =============================================================================
// Set acceleration
// =============================================================================
void TraverseController::set_acceleration(float accel_mm_per_sec2) {
    if (accel_mm_per_sec2 > 0) {
        acceleration_mm_per_sec2 = accel_mm_per_sec2;
        printf("[TraverseController] Acceleration set to %.1f mm/s²\n", accel_mm_per_sec2);
    }
}

// =============================================================================
// Get current position
// =============================================================================
float TraverseController::get_position() const {
    return current_position_mm;
}

// =============================================================================
// Check if homed
// =============================================================================
bool TraverseController::is_homed() const {
    return homed;
}

// =============================================================================
// Check if moving
// =============================================================================
bool TraverseController::is_moving() const {
    return moving;
}

// =============================================================================
// Get current speed
// =============================================================================
float TraverseController::get_speed() const {
    return current_speed_mm_per_sec;
}

// =============================================================================
// Check if moving

float TraverseController::get_steps_per_mm() const {
    return steps_per_mm;
}

// =============================================================================
// Emergency stop
// =============================================================================
void TraverseController::emergency_stop() {
    emergency_stopped = true;
    moving = false;
    steps_remaining = 0;
    printf("[TraverseController] EMERGENCY STOP!\n");
}

// =============================================================================
// Set brake
// =============================================================================
void TraverseController::set_brake(bool brake_enable) {
    // For stepper motors, "brake" is just disabling the motor
    if (brake_enable) {
        disable();
    } else {
        enable();
    }
}

// =============================================================================
// Generate steps (call from main loop)
// =============================================================================
void TraverseController::generate_steps() {
    if (!moving || !enabled || emergency_stopped) {
        return;
    }
    
    // Handle different homing phases
    if (homing) {
        if (homing_phase == 1) {
            // Phase 1: Moving towards home switch - handled in main loop
            // (Home switch detection moved to main generate_steps loop)
        } else if (homing_phase == 2) {
            // Phase 2: Backing off from switch
            if (steps_remaining <= 0) {
                printf("[TraverseController] Back-off complete. Moving to phase 3...\n");
                // Reset position to 0.0mm after back-off
                current_position_mm = 0.0f;
                homing_phase = 3;
                move_to_start_position();
                // Don't return - continue to Phase 3 processing
            }
        } else if (homing_phase == 3) {
            // Phase 3: Moving to start position
            if (steps_remaining <= 0) {
                printf("[TraverseController] Start position reached. Homing complete!\n");
                moving = false;
                homing = false;
                homed = true;
                current_position_mm = target_position_mm;
                printf("[TraverseController] Final position: %.2f mm\n", current_position_mm);
                return;
            }
        }
    }
    
    // Debug: Show step generation progress
    static uint32_t debug_counter = 0;
    if (homing && (debug_counter++ % 1000 == 0)) {
        printf("[TraverseController] Homing debug: steps_remaining=%d, home_switch=%s, moving=%d, enabled=%d, phase=%d\n", 
               steps_remaining, gpio_get(home_pin) ? "HIGH" : "LOW", moving, enabled, homing_phase);
    }
    
    uint32_t now = time_us_32();
    
    // CRITICAL: Check home switch BEFORE any step processing during homing
    if (homing && homing_phase == 1 && !gpio_get(home_pin)) {
        printf("[TraverseController] Home switch triggered at position %.2fmm! Moving to phase 2...\n", current_position_mm);
        printf("[TraverseController] Travel distance to home: %.2fmm\n", fabs(current_position_mm));
        printf("[TraverseController] BEFORE ZEROING: current_position_mm = %.2f\n", current_position_mm);
        // ZERO the position IMMEDIATELY when home switch is hit
        current_position_mm = 0.0f;
        printf("[TraverseController] AFTER ZEROING: current_position_mm = %.2f\n", current_position_mm);
        printf("[TraverseController] Position zeroed to 0.0mm\n");
        homing_phase = 2;
        back_off_from_switch();
        return;
    }
    
    if (now - last_step_time >= step_interval_us) {
        
        // Generate step pulse
        gpio_put(step_pin, 1);
        sleep_us(2);  // Short pulse
        gpio_put(step_pin, 0);
        
        // Update position
        if (step_direction) {
            current_position_mm += 1.0f / steps_per_mm;
        } else {
            current_position_mm -= 1.0f / steps_per_mm;
        }
        
        steps_remaining--;
        last_step_time = now;
        
        // Debug: Show step generation
        static uint32_t step_counter = 0;
        if (homing && (step_counter++ % 100 == 0)) {
            printf("[TraverseController] Step %d: pos=%.2fmm, remaining=%d\n", 
                   step_counter, current_position_mm, steps_remaining);
        }
        
        // Check if movement complete
        if (steps_remaining <= 0) {
            if (homing) {
                // Handle homing phase transitions
                if (homing_phase == 1) {
                    printf("[TraverseController] Homing timeout - no home switch detected after %.2fmm travel\n", fabs(current_position_mm));
                    moving = false;
                    homing = false;
                }
                // For phases 2 and 3, let the homing logic handle the transition
                // Don't set moving=false or homing=false here
            } else {
                moving = false;
                printf("[TraverseController] Movement complete at %.2f mm\n", current_position_mm);
            }
        }
    }
}

// =============================================================================
// Stop steps
// =============================================================================
void TraverseController::stop_steps() {
    moving = false;
    steps_remaining = 0;
    printf("[TraverseController] Steps stopped\n");
}

// =============================================================================
// Calculate step interval based on speed
// =============================================================================
void TraverseController::calculate_step_interval() {
    if (current_speed_mm_per_sec > 0) {
        float steps_per_sec = current_speed_mm_per_sec * steps_per_mm;
        step_interval_us = (uint32_t)(1000000.0f / steps_per_sec);
        printf("[TraverseController] Speed: %.1f mm/s, Steps/mm: %.1f, Steps/sec: %.1f, Interval: %dus\n", 
               current_speed_mm_per_sec, steps_per_mm, steps_per_sec, step_interval_us);
    } else {
        step_interval_us = 1000;  // 1ms default
    }
}

// =============================================================================
// Update position (for homing)
// =============================================================================
void TraverseController::update_position() {
    // Check home switch during homing
    if (moving && !step_direction) {  // Moving towards home
        if (check_home_switch()) {
            // Home switch triggered
            current_position_mm = 0.0f;
            moving = false;
            homed = true;
            printf("[TraverseController] Homed at 0.0 mm\n");
        }
    }
}

// =============================================================================
// Check home switch
// =============================================================================
bool TraverseController::check_home_switch() {
    return !gpio_get(home_pin);  // Active low
}

// =============================================================================
// Static ISR wrapper (for future interrupt-based stepping)
// =============================================================================
void TraverseController::step_timer_isr() {
    if (instance) {
        instance->generate_steps();
    }
}

// =============================================================================
// Code-snippets Functions (from cpp_snippets.cpp)
// =============================================================================
void TraverseController::stepper_step() {
    // Generate step pulse (from Code-snippets)
    gpio_put(step_pin, 1);
    sleep_us(10);  // Step pulse width
    gpio_put(step_pin, 0);
    steps_remaining--;
}

void TraverseController::stepper_move_to(float position, float feed_rate) {
    // Move stepper to position (from Code-snippets)
    target_position_mm = position;
    current_speed_mm_per_sec = feed_rate / 60.0f;  // Convert mm/min to mm/s
    
    float distance = position - current_position_mm;
    bool direction = distance > 0;
    
    gpio_put(dir_pin, direction);
    
    // Calculate number of steps (assuming 200 steps/mm)
    uint32_t steps = (uint32_t)fabsf(distance * steps_per_mm);
    
    // Calculate step delay based on feed rate
    uint32_t step_delay_us = (uint32_t)(60000000.0f / (feed_rate * steps_per_mm));
    
    printf("Moving %.2f mm at %.1f mm/min (%d steps)\n", 
           distance, feed_rate, steps);
    
    for (uint32_t i = 0; i < steps; i++) {
        stepper_step();
        sleep_us(step_delay_us);
    }
    
    current_position_mm = position;
    moving = false;
}

bool TraverseController::stepper_home() {
    // Home stepper motor (from Code-snippets)
    printf("Homing stepper...\n");
    
    // Move towards home switch
    gpio_put(dir_pin, 0);  // Move towards home
    
    while (!gpio_get(home_pin)) {
        stepper_step();
        sleep_us(1000);  // Slow homing speed
    }
    
    current_position_mm = 0.0f;
    target_position_mm = 0.0f;
    steps_remaining = 0;
    homed = true;
    
    printf("Stepper homed\n");
    return true;
}
