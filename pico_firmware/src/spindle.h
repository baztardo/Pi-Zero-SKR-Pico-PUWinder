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
    
    static void isr_wrapper(uint gpio, uint32_t events);
    void handle_pulse();
};