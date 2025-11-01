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
TraverseController::TraverseController(MoveQueue* mq)
    : step_pin(TRAVERSE_STEP_PIN)
    , dir_pin(TRAVERSE_DIR_PIN)
    , enable_pin(TRAVERSE_ENA_PIN)
    , home_pin(TRAVERSE_HOME_PIN)
    , tmc_driver(nullptr)
    , move_queue(mq)
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
    if (tmc_driver) {
        delete tmc_driver;
        tmc_driver = nullptr;
    }
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
    set_direction(false);  // Default direction
    
    // Configure enable pin
    gpio_init(enable_pin);
    gpio_set_dir(enable_pin, GPIO_OUT);
    gpio_put(enable_pin, 1);  // Start disabled (active low)
    
    // Configure home switch
    gpio_init(home_pin);
    gpio_set_dir(home_pin, GPIO_IN);
    gpio_pull_up(home_pin);

    // Initialize TMC2209 driver (Hardware UART - worked great!)
    printf("[TraverseController] Initializing TMC2209 driver (Hardware UART)...\n");
    tmc_driver = new TMC2209_UART(TMC_UART_ID, TMC_UART_TX_PIN, TMC_UART_RX_PIN, 0);
    printf("[TraverseController] TMC2209 object created (UART%d, TX=%d, RX=%d)\n", TMC_UART_ID, TMC_UART_TX_PIN, TMC_UART_RX_PIN);

    if (tmc_driver->begin(TMC_UART_BAUD)) {
        printf("[TraverseController] UART begin() succeeded\n");
        if (tmc_driver->init_driver(TRAVERSE_CURRENT_MA, TRAVERSE_MICROSTEPS)) {
            printf("[TraverseController] ✓ TMC2209 initialized successfully\n");
            printf("[TraverseController]   Current: %d mA, Microsteps: %d\n", TRAVERSE_CURRENT_MA, TRAVERSE_MICROSTEPS);

    // Test read a register to verify communication
    uint32_t status = 0;
    if (tmc_driver->get_driver_status(&status)) {
        printf("[TraverseController] ✓ TMC2209 status read: 0x%08X\n", status);
    } else {
        printf("[TraverseController] ✗ Failed to read TMC2209 status register\n");
    }

    // Test CHOPCONF register (microstepping configuration)
    uint32_t chopconf = 0;
    if (tmc_driver->readRegister(0x6C, chopconf)) {  // CHOPCONF register
        uint8_t mres = (chopconf >> 24) & 0x0F;
        uint8_t microsteps = 0;
        switch(mres) {
            case 1: microsteps = 128; break;
            case 2: microsteps = 64; break;
            case 3: microsteps = 32; break;
            case 4: microsteps = 16; break;
            case 5: microsteps = 8; break;
            case 6: microsteps = 4; break;
            case 7: microsteps = 2; break;
            case 8: microsteps = 1; break;
            default: microsteps = 1; break;
        }
        printf("[TraverseController] ✓ CHOPCONF: 0x%08X (MRES=%d, microsteps=%d)\n", chopconf, mres, microsteps);
    } else {
        printf("[TraverseController] ✗ Cannot read CHOPCONF register\n");
    }
        } else {
            printf("[TraverseController] ✗ TMC2209 driver initialization failed\n");
        }
    } else {
        printf("[TraverseController] ✗ TMC2209 UART initialization failed\n");
    }

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
    printf("[TraverseController] Enabled (ENA pin %d = LOW)\n", enable_pin);
}

// =============================================================================
// Set direction with inversion
// =============================================================================
void TraverseController::set_direction(bool direction) {
    // Apply direction inversion if configured
    bool actual_direction = direction;
    if (TRAVERSE_DIR_INVERT) {
        actual_direction = !direction;
    }
    gpio_put(dir_pin, actual_direction ? 1 : 0);
}

// =============================================================================
// Test TMC2209 communication
// =============================================================================
bool TraverseController::test_tmc2209_status(uint32_t* status) {
    if (!tmc_driver || !status) {
        return false;
    }

    // Try to read the TMC2209 CHOPCONF register (0x6C) to check microstepping
    return tmc_driver->readRegister(0x6C, *status);
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

    // Use the MoveQueue-based stepper_move_to method
    stepper_move_to(position_mm, current_speed_mm_per_sec * 60.0f);  // Convert mm/s to mm/min
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
        // Move -200mm at 20mm/s (should be enough to hit switch)
        stepper_move_to(-200.0f, 1200.0f);  // 1200 mm/min = 20 mm/s
        homing = true;
        homing_phase = 1;
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
    // Move +8mm at 10mm/s (600 mm/min)
    stepper_move_to(8.0f, 600.0f);
    homing = true;
    homing_phase = 2;
}

void TraverseController::move_to_start_position() {
    printf("[TraverseController] Phase 3: Moving to start position (%.2fmm)...\n", TC_start_offset);
    // Move to start offset position at 10mm/s (600 mm/min)
    stepper_move_to(TC_start_offset, 600.0f);
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
    printf("[TraverseController] is_homed() called, homed=%d\n", homed);
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
    homing = false;
    homing_phase = 0;
    steps_remaining = 0;
    printf("[TraverseController] EMERGENCY STOP!\n");
}

// =============================================================================
// Clear emergency stop (reset state)
// =============================================================================
void TraverseController::clear_emergency_stop() {
    emergency_stopped = false;
    printf("[TraverseController] Emergency stop cleared\n");
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
// Generate steps (call from main loop) - Now monitors MoveQueue
// =============================================================================
void TraverseController::generate_steps() {
    if (!move_queue) {
        return;
    }

    // Check if MoveQueue has completed all chunks
    if (moving && !move_queue->has_chunk()) {
        // Move is complete
        moving = false;
        current_position_mm = target_position_mm;
        printf("[TraverseController] Move complete at %.2f mm\n", current_position_mm);
    }

    // Handle homing phases (still need direct control for switch detection)
    if (homing) {
        if (homing_phase == 1) {
            // Check home switch during homing
            if (!gpio_get(home_pin)) {
                printf("[TraverseController] Home switch triggered! Stopping homing move...\n");
                // Stop the MoveQueue chunks
                move_queue->clear_queue();
                current_position_mm = 0.0f;
                homing_phase = 2;
                back_off_from_switch();
                return;
            }
        } else if (homing_phase == 2) {
            // Phase 2: Backing off - check if MoveQueue is done
            if (!move_queue->has_chunk()) {
                printf("[TraverseController] Back-off complete. Moving to phase 3...\n");
                current_position_mm = 8.0f;  // 8mm from home
                homing_phase = 3;
                move_to_start_position();
            }
        } else if (homing_phase == 3) {
            // Phase 3: Moving to start position
            if (!move_queue->has_chunk()) {
                printf("[TraverseController] Start position reached. Homing complete!\n");
                moving = false;
                homing = false;
                homed = true;
                current_position_mm = target_position_mm;
                printf("[TraverseController] Final position: %.2f mm, homed=%d\n", current_position_mm, homed);
            }
        }
    }
}

// =============================================================================
// Stop steps
// =============================================================================
void TraverseController::stop_steps() {
    // Stop generating steps but DON'T disable the motor
    // This allows MoveQueue ISR to take over control
    moving = false;
    steps_remaining = 0;
    homing = false;
    
    // DON'T touch GPIO pins - MoveQueue will control them
    
    printf("[TraverseController] Step generation stopped (MoveQueue takeover)\n");
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
    if (!move_queue) {
        printf("[TraverseController] ERROR: No MoveQueue available for move!\n");
        return;
    }

    target_position_mm = position;
    float distance = position - current_position_mm;

    if (fabsf(distance) < 0.001f) {
        printf("[TraverseController] Already at position %.3fmm\n", position);
        return;
    }

    uint32_t total_steps = (uint32_t)fabsf(distance * steps_per_mm);
    double velocity_steps_per_sec = (feed_rate / 60.0f) * steps_per_mm;  // Convert mm/min to steps/sec

    printf("[TraverseController] MoveQueue: %.2fmm (%.0f steps, %.0f steps/sec)\n",
           distance, total_steps, velocity_steps_per_sec);

    // Generate step chunks using constant velocity
    auto chunks = StepCompressor::compress_constant_velocity(
        total_steps,
        velocity_steps_per_sec,
        20.0  // max timing error in microseconds
    );

    // Set direction
    bool direction = distance > 0;
    set_direction(direction);

    // Push chunks to MoveQueue
    size_t chunks_queued = 0;
    for (const auto& chunk : chunks) {
        if (move_queue->push_chunk(chunk)) {
            chunks_queued++;
        } else {
            printf("[TraverseController] ERROR: MoveQueue full after %lu chunks!\n", chunks_queued);
            break;
        }
    }

    moving = true;
    printf("[TraverseController] Queued %lu chunks for %.2fmm move\n", chunks_queued, distance);
}

bool TraverseController::stepper_home() {
    // Home stepper motor (from Code-snippets)
    printf("Homing stepper...\n");
    
    // Move towards home switch
    set_direction(false);  // Move towards home
    
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
