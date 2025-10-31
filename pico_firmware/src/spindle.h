// =============================================================================
// bldc_speed_pulse.h - BLDC Speed Pulse ISR Handler
// Purpose: Count speed pulses from SC output and calculate RPM
// =============================================================================

#pragma once

#include <cstdint>
#include "hardware/gpio.h"
#include "config.h"

// =============================================================================
// BLDC_MOTOR Class
// =============================================================================

enum MotorDirection {
    DIRECTION_CCW = BLDC_DIRECTION_CCW,
    DIRECTION_CW = BLDC_DIRECTION_CW
};

class BLDC_MOTOR {
public:
    /**
     * @brief Constructor
     * @param pulse_pin GPIO pin connected to SC speed pulse output
     */
    BLDC_MOTOR(uint pulse_pin);
    

    void init();
    float get_rpm() const;
    float get_frequency() const;
    uint32_t get_pulse_count() const;
    void set_pulses_per_revolution(uint ppr);
    void reset();
    float get_smoothed_rpm(float alpha = 0.1f);
    void debug_status() const;
    MotorDirection get_direction() const;
    void set_direction(MotorDirection direction);
    void set_brake(bool brake);
    bool get_brake() const;
    float get_revolutions() const;
    
    // PWM control methods (from Code-snippets improvement)
    void set_pwm_duty(float duty_percent);
    void set_rpm_pwm(float rpm);
    
    // ⭐ NEW: FluidNC-style enhanced spindle control
    void set_ramp_rate(float ramp_rate_percent_per_second);
    void set_max_rpm(float max_rpm);
    void set_min_rpm(float min_rpm);
    bool is_ramping() const;
    float get_ramp_progress() const;
    
    // ⭐ NEW: Advanced RPM methods
    float get_instantaneous_rpm() const;
    uint32_t get_time_since_pulse() const;
    bool is_running() const;
    float get_angular_velocity() const;
    uint32_t get_predicted_next_pulse() const;
    int get_pulse_position() const;

    // Motor Hall sensor RPM (separate from spindle monitor)
    float get_motor_rpm() const;

    // Predictive ramp down calculations
    float predict_ramp_down_start(float current_turns, float target_turns, float ramp_time_sec = 2.0f) const;

    // Hall sensor monitoring
    uint64_t get_monitor_pulse_count() const { return monitor_pulse_count; }
    void reset_monitor_pulse_count() { monitor_pulse_count = 0; }


private:
    // Pulse tracking
    static const int HISTORY_SIZE = 20;
    uint32_t pulse_times[HISTORY_SIZE];
    uint32_t pulse_timestamps[HISTORY_SIZE];
    int pulse_index;
    uint32_t last_edge_time;
    uint32_t edge_count;
    
    // RPM calculation
    float measured_rpm;
    float filtered_rpm;
    uint32_t last_rpm_update;
    
    // For synchronization
    float instantaneous_rpm;
    uint32_t last_pulse_period;
    
    // Motor control
    uint pulse_pin;
    uint pulses_per_revolution;
    MotorDirection direction = DIRECTION_CW;  // Default
    bool brake = false;
    
    // ⭐ NEW: FluidNC-style enhanced spindle control
    float target_rpm;
    float current_rpm;
    float ramp_rate_percent_per_second;
    float max_rpm;
    float min_rpm;
    bool is_ramping_to_target;
    uint32_t ramp_start_time;

    // Hall sensor monitoring for debugging
    uint64_t monitor_pulse_count;

    // Monitor pulse timing (for accurate 1-pulse-per-revolution RPM)
    uint32_t monitor_pulse_times[HISTORY_SIZE];
    uint32_t monitor_pulse_timestamps[HISTORY_SIZE];
    int monitor_pulse_index;
    uint32_t last_monitor_edge_time;
    uint32_t monitor_edge_count;

    static void isr_wrapper(uint gpio, uint32_t events);
    void handle_pulse();
    void handle_monitor_pulse();
    void calculate_rpm();
    void calculate_rpm_from_monitor();
};