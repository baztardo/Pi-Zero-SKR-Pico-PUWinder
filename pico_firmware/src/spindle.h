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
    
    
private:
    uint pulse_pin;
    volatile uint32_t edge_count;
    volatile uint32_t last_edge_time;
    volatile float measured_rpm;
    volatile float pulse_frequency;
    uint32_t last_rpm_calculation_time;
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
    
    static void isr_wrapper(uint gpio, uint32_t events);
    void handle_pulse();
};