// =============================================================================
// traverse_controller.h - Traverse Stepper Motor Controller
// Purpose: Control traverse stepper motor for winding machine
// =============================================================================

#pragma once

#include <cstdint>
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "config.h"
#include "tmc2209.h"
#include "move_queue.h"
#include "stepcompress.h"

// =============================================================================
// Traverse Controller Class
// =============================================================================
class TraverseController {
public:
    TraverseController(MoveQueue* move_queue);
    ~TraverseController();
    
    // Initialization
    void init();
    void enable();
    void disable();
    
    // Movement control
    void move_to_position(float position_mm);
    void move_relative(float distance_mm);
    void home();
    
    // Speed control
    void set_speed(float speed_mm_per_sec);
    void set_acceleration(float accel_mm_per_sec2);
    
    // Status
    float get_position() const;
    float get_speed() const;
    bool is_moving() const;
    bool is_homed() const;
    bool is_enabled() const { return enabled; }
    float get_steps_per_mm() const;

    // Force reset homed state (for G28 re-homing)
    void force_unhome() { homed = false; }
    
    // Emergency functions
    void emergency_stop();
    void clear_emergency_stop();  // Reset emergency stop state
    void set_brake(bool enable);
    
    // MoveQueue handoff
    void stop_steps();
    
    // Step generation (for real-time control)
    void generate_steps();
    
    // Homing phases
    void back_off_from_switch();
    void move_to_start_position();
    
private:
    // Hardware pins
    uint step_pin;
    uint dir_pin;
    uint enable_pin;
    uint home_pin;

    // TMC2209 driver
    TMC2209_UART* tmc_driver;

    // MoveQueue for real-time stepping
    MoveQueue* move_queue;
    
    // Position tracking
    volatile float current_position_mm;
    volatile float target_position_mm;
    volatile float current_speed_mm_per_sec;
    volatile float max_speed_mm_per_sec;
    volatile float acceleration_mm_per_sec2;
    
    // Movement state
    volatile bool moving;
    volatile bool homing;
    volatile bool homed;
    volatile bool enabled;
    volatile bool emergency_stopped;
    volatile uint8_t homing_phase;  // 1=towards switch, 2=back off, 3=move to start
    
    // Step generation
    volatile int32_t steps_remaining;
    volatile int32_t step_interval_us;
    volatile uint32_t last_step_time;
    volatile bool step_direction;
    
    // Hardware configuration
    float steps_per_mm;
    uint32_t microsteps;
    
public:
    // Test methods (public for debugging)
    bool test_tmc2209_status(uint32_t* status);  // Test TMC2209 communication

private:
    // Internal methods
    void step_motor();
    void calculate_step_interval();
    void update_position();
    bool check_home_switch();
    void set_direction(bool direction);  // Set direction with inversion
    
    // Static ISR wrapper
    static void step_timer_isr();
    static TraverseController* instance;
    
    // Code-snippets Functions (from cpp_snippets.cpp)
    void stepper_step();
    void stepper_move_to(float position, float feed_rate);
    bool stepper_home();
    
public:
    // Additional methods
    float get_current_position() const { return current_position_mm; }
};

// =============================================================================
// Global traverse controller instance
// =============================================================================
extern TraverseController* g_traverse_controller;
