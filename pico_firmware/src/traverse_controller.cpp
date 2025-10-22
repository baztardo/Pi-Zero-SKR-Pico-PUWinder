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
    , homed(false)
    , enabled(false)
    , emergency_stopped(false)
    , steps_remaining(0)
    , step_interval_us(0)
    , last_step_time(0)
    , step_direction(true)
    , steps_per_mm(200.0f)  // 200 steps/mm (1.8° stepper, 8mm lead screw)
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
    
    // Calculate steps per mm with microstepping
    steps_per_mm = (200.0f * microsteps) / 8.0f;  // 8mm lead screw
    
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
    
    if (abs(distance) < 0.01f) {  // Already at position
        printf("[TraverseController] Already at position %.2f mm\n", position_mm);
        return;
    }
    
    // Calculate steps needed
    steps_remaining = (int32_t)(abs(distance) * steps_per_mm);
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
    
    printf("[TraverseController] Homing...\n");
    
    // Move towards home switch
    gpio_put(dir_pin, 0);  // Move towards home
    step_direction = false;
    
    // Move until home switch is triggered
    steps_remaining = 10000;  // Large number of steps
    calculate_step_interval();
    moving = true;
    
    // Check for home switch in main loop
    // (This is a simplified version - real implementation would use interrupts)
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
// Get current speed
// =============================================================================
float TraverseController::get_speed() const {
    return current_speed_mm_per_sec;
}

// =============================================================================
// Check if moving
// =============================================================================
bool TraverseController::is_moving() const {
    return moving;
}

// =============================================================================
// Check if homed
// =============================================================================
bool TraverseController::is_homed() const {
    return homed;
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
void TraverseController::set_brake(bool enable) {
    // For stepper motors, "brake" is just disabling the motor
    if (enable) {
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
    
    uint32_t now = time_us_32();
    
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
        
        // Check if movement complete
        if (steps_remaining <= 0) {
            moving = false;
            printf("[TraverseController] Movement complete at %.2f mm\n", current_position_mm);
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
