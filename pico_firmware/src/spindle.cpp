// =============================================================================
// bldc_speed_pulse.cpp - BLDC Speed Pulse ISR Handler
// Purpose: Count speed pulses from SC output and calculate RPM
// =============================================================================

#include "spindle.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <cstdio>
#include <cmath>

// Global instance pointer for ISR
static BLDC_MOTOR* g_speed_pulse_instance = nullptr;
// =============================================================================
// Constructor
// =============================================================================
BLDC_MOTOR::BLDC_MOTOR(uint pulse_pin)
    : pulse_pin(pulse_pin)
    , edge_count(0)
    , last_edge_time(0)
    , measured_rpm(0.0f)
    , pulse_frequency(0.0f)
    , last_rpm_calculation_time(0)
    , pulses_per_revolution(BLDC_DEFAULT_PPR)  // From config.h
    , direction(DIRECTION_CW)
    , brake(false)
    // ⭐ NEW: Initialize FluidNC-style enhanced spindle control
    , target_rpm(0.0f)
    , current_rpm(0.0f)
    , ramp_rate_percent_per_second(10.0f)  // Default 10% per second
    , max_rpm(3000.0f)
    , min_rpm(0.0f)
    , is_ramping_to_target(false)
    , ramp_start_time(0)
{
    g_speed_pulse_instance = this;
}

// =============================================================================
// Initialize GPIO and ISR
// =============================================================================
void BLDC_MOTOR::init() {
    // Configure GPIO
    gpio_init(pulse_pin);
    gpio_set_dir(pulse_pin, GPIO_IN);
    gpio_pull_up(pulse_pin);
    
    gpio_init(SPINDLE_DIR_PIN);
    gpio_set_dir(SPINDLE_DIR_PIN, GPIO_OUT);
    gpio_put(SPINDLE_DIR_PIN, 1);  // Start forward

    gpio_init(SPINDLE_BRAKE_PIN);
    gpio_set_dir(SPINDLE_BRAKE_PIN, GPIO_OUT);
    gpio_put(SPINDLE_BRAKE_PIN, 0);  // Start brake OFF

    // Enable interrupt on rising edge
    gpio_set_irq_enabled_with_callback(
        pulse_pin,
        GPIO_IRQ_EDGE_RISE,  // Trigger on rising edge
        true,
        &BLDC_MOTOR::isr_wrapper
    );
    
    // Initialize PWM for spindle control (from Code-snippets improvement)
    gpio_set_function(SPINDLE_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
    uint channel = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
    
    // Set PWM frequency to 1kHz
    pwm_set_clkdiv(slice_num, 125.0f / 1000.0f);
    pwm_set_wrap(slice_num, 65535);
    pwm_set_chan_level(slice_num, channel, 0);
    pwm_set_enabled(slice_num, true);
    
    printf("PWM initialized on pin %d\n", SPINDLE_PWM_PIN);
    
    printf("[BLDC_MOTOR] Initialized on GPIO %u\n", pulse_pin);
    printf("[BLDC_MOTOR] Pulses per revolution: %u\n", pulses_per_revolution);
}

// =============================================================================
// Static ISR wrapper (required for Pico SDK)
// =============================================================================
void BLDC_MOTOR::isr_wrapper(uint gpio, uint32_t events) {
    if (g_speed_pulse_instance) {
        g_speed_pulse_instance->handle_pulse();
    }
}

// =============================================================================
// Instance ISR handler
// =============================================================================
void BLDC_MOTOR::handle_pulse() {
    uint32_t now = time_us_32();
    uint32_t dt_us = now - last_edge_time;
    
    // Debounce: ignore edges faster than configured debounce time
    if (dt_us < BLDC_MIN_PULSE_DT_US) {
        return;
    }
    
    last_edge_time = now;
    edge_count++;
    
    // Calculate RPM every N pulses (e.g., every full revolution)
    // This reduces jitter in RPM reading
    if (edge_count % pulses_per_revolution == 0) {
        // Time for one full revolution (pulses_per_revolution edges)
        float time_per_rev_sec = (dt_us * pulses_per_revolution) / 1e6f;
        
        if (time_per_rev_sec > 0.0f) {
            // RPM = (60 seconds/minute) / (time_per_rev_seconds)
            measured_rpm = 60.0f / time_per_rev_sec;
            
            // Clamp RPM to reasonable range (from Code-snippets improvement)
            if (measured_rpm > 10000.0f) {
                measured_rpm = 0.0f;  // Likely noise
            }
            
            // Also calculate pulse frequency (Hz)
            pulse_frequency = 1.0f / (dt_us / 1e6f);
        }
    }
}

void BLDC_MOTOR::set_direction(MotorDirection direction) {
    this->direction = direction;
    gpio_put(SPINDLE_DIR_PIN, direction);  // 1=HIGH(CW), 0=LOW(CCW)
}

void BLDC_MOTOR::set_brake(bool brake) {
    this->brake = brake;
    gpio_put(SPINDLE_BRAKE_PIN, brake);  // 1=HIGH(Brake), 0=LOW(Release)
}   
bool BLDC_MOTOR::get_brake() const {
    return brake;
}

MotorDirection BLDC_MOTOR::get_direction() const {
    return direction;
}
// =============================================================================
// Get current RPM
// =============================================================================
float BLDC_MOTOR::get_rpm() const {
    return measured_rpm;
}

// =============================================================================
// Get pulse frequency (Hz)
// =============================================================================
float BLDC_MOTOR::get_frequency() const {
    return pulse_frequency;
}

// =============================================================================
// Get total pulse count
// =============================================================================
uint32_t BLDC_MOTOR::get_pulse_count() const {
    return edge_count;
}

// =============================================================================
// Get revolutions (calculated from pulse count)
// =============================================================================
float BLDC_MOTOR::get_revolutions() const {
    if (pulses_per_revolution == 0) return 0.0f;
    return (float)edge_count / (float)pulses_per_revolution;
}

// =============================================================================
// PWM Control Methods (from Code-snippets improvement)
// =============================================================================
void BLDC_MOTOR::set_pwm_duty(float duty_percent) {
    // Clamp duty cycle to 0-100%
    duty_percent = fmaxf(0.0f, fminf(100.0f, duty_percent));
    
    // Convert to PWM level
    uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
    uint channel = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
    uint16_t pwm_level = (uint16_t)((duty_percent / 100.0f) * 65535);
    
    pwm_set_chan_level(slice_num, channel, pwm_level);
    printf("Set spindle PWM to %.1f%% (level: %d)\n", duty_percent, pwm_level);
}

// =============================================================================
// ⭐ NEW: FluidNC-style Enhanced Spindle Control Methods
// =============================================================================

void BLDC_MOTOR::set_ramp_rate(float ramp_rate_percent_per_second) {
    this->ramp_rate_percent_per_second = ramp_rate_percent_per_second;
    printf("Spindle ramp rate set to %.1f%%/sec\n", ramp_rate_percent_per_second);
}

void BLDC_MOTOR::set_max_rpm(float max_rpm) {
    this->max_rpm = max_rpm;
    printf("Spindle max RPM set to %.1f\n", max_rpm);
}

void BLDC_MOTOR::set_min_rpm(float min_rpm) {
    this->min_rpm = min_rpm;
    printf("Spindle min RPM set to %.1f\n", min_rpm);
}

bool BLDC_MOTOR::is_ramping() const {
    return is_ramping_to_target;
}

float BLDC_MOTOR::get_ramp_progress() const {
    if (!is_ramping_to_target) return 1.0f;
    
    uint32_t current_time = time_us_32();
    uint32_t ramp_duration = current_time - ramp_start_time;
    
    // Calculate progress based on ramp rate
    float progress = (ramp_duration / 1000000.0f) * ramp_rate_percent_per_second / 100.0f;
    return fminf(1.0f, fmaxf(0.0f, progress));
}

void BLDC_MOTOR::set_rpm_pwm(float rpm) {
    // Clamp RPM to reasonable range
    rpm = fmaxf(0.0f, fminf(3000.0f, rpm));
    
    if (rpm > 0) {
        // Calculate PWM duty cycle based on RPM
        float duty_percent = (rpm / 3000.0f) * 100.0f;
        set_pwm_duty(duty_percent);
    } else {
        // Stop PWM
        set_pwm_duty(0.0f);
    }
}

// =============================================================================
// Set pulses per revolution (for different motor configurations)
// =============================================================================
void BLDC_MOTOR::set_pulses_per_revolution(uint ppr) {
    if (ppr > 0) {
        pulses_per_revolution = ppr;
        printf("[BLDC-PULSE] Pulses per revolution set to %u\n", ppr);
    }
}

// =============================================================================
// Reset counters
// =============================================================================
void BLDC_MOTOR::reset() {
    edge_count = 0;
    measured_rpm = 0.0f;
    pulse_frequency = 0.0f;
    last_edge_time = time_us_32();
    printf("[BLDC-PULSE] Counters reset\n");
}

// =============================================================================
// Get smoothed RPM (with low-pass filter to reduce jitter)
// =============================================================================
float BLDC_MOTOR::get_smoothed_rpm(float alpha) {
    // alpha: 0.0 (no filtering) to 1.0 (full filtering)
    // Typical: 0.1 = 10% new value, 90% old value
    static float smoothed_rpm = 0.0f;
    
    // Use default alpha from config if not specified
    if (alpha < 0.0f) alpha = BLDC_SMOOTH_ALPHA;
    if (alpha > 1.0f) alpha = 1.0f;
    
    smoothed_rpm = (alpha * measured_rpm) + ((1.0f - alpha) * smoothed_rpm);
    return smoothed_rpm;
}


// =============================================================================
// Print debug info
// =============================================================================
void BLDC_MOTOR::debug_status() const {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║  BLDC Speed Pulse Debug Status         ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║ GPIO Pin:           %2u                ║\n", pulse_pin);
    printf("║ Total Pulses:       %lu                ║\n", (unsigned long)edge_count);
    printf("║ Revolutions:        %.2f               ║\n", get_revolutions());
    printf("║ RPM:                %.1f               ║\n", measured_rpm);
    printf("║ Frequency:          %.1f Hz             ║\n", pulse_frequency);
    printf("║ Pulses/Rev:         %u                ║\n", pulses_per_revolution);
    printf("╚════════════════════════════════════════╝\n");
}