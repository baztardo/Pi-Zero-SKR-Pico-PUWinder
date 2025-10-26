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
    , pulse_index(0)
    , last_edge_time(0)
    , edge_count(0)
    , measured_rpm(0.0f)
    , filtered_rpm(0.0f)
    , last_rpm_update(0)
    , instantaneous_rpm(0.0f)
    , last_pulse_period(0)
    , pulses_per_revolution(BLDC_DEFAULT_PPR)  // From config.h
    , direction(DIRECTION_CW)
    , brake(false)
    , target_rpm(0.0f)
    , current_rpm(0.0f)
    , ramp_rate_percent_per_second(10.0f)  // Default 10% per second
    , max_rpm(3000.0f)
    , min_rpm(0.0f)
    , is_ramping_to_target(false)
    , ramp_start_time(0)
{
    g_speed_pulse_instance = this;
    
    // Initialize pulse history arrays
    for (int i = 0; i < HISTORY_SIZE; i++) {
        pulse_times[i] = 0;
        pulse_timestamps[i] = 0;
    }
}

// =============================================================================
// Initialize GPIO and ISR
// =============================================================================
void BLDC_MOTOR::init() {
    // Initialize variables
    pulse_index = 0;
    edge_count = 0;
    last_edge_time = 0;
    measured_rpm = 0.0f;
    filtered_rpm = 0.0f;
    last_rpm_update = 0;
    
    // Configure GPIO
    gpio_init(pulse_pin);
    gpio_set_dir(pulse_pin, GPIO_IN);
    gpio_pull_up(pulse_pin);
    
    // Enable interrupt on rising edge
    gpio_set_irq_enabled_with_callback(
        pulse_pin,
        GPIO_IRQ_EDGE_RISE,
        true,
        &BLDC_MOTOR::isr_wrapper
    );
    
    // Initialize PWM for spindle control
    gpio_set_function(SPINDLE_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
    uint channel = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
    
    // Set PWM frequency to 10kHz
    pwm_set_clkdiv(slice_num, 1.0f);
    pwm_set_wrap(slice_num, 12500);
    pwm_set_chan_level(slice_num, channel, 0);
    pwm_set_enabled(slice_num, true);
    
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
    
    // Filter: Ignore pulses faster than 2500 μs
    if (dt_us < 2500) {
        return;
    }
    
    // Store this pulse
    pulse_times[pulse_index] = dt_us;
    pulse_timestamps[pulse_index] = now;
    pulse_index = (pulse_index + 1) % HISTORY_SIZE;
    edge_count++;
    last_edge_time = now;
    last_pulse_period = dt_us;
    
    // Calculate instantaneous RPM (for quick response)
    float pulses_per_second = 1000000.0f / dt_us;
    instantaneous_rpm = (pulses_per_second * 60.0f) / 18.0f;
    
    // Calculate averaged RPM (for stability)
    if (edge_count >= 10) {
        calculate_rpm();
    }
    
    // Debug output
    if (edge_count <= 10) {
        printf("[PULSE] #%lu, dt=%lu us\n", edge_count, dt_us);
    }
    
    if (edge_count % 100 == 0) {
        printf("[PULSE] #%lu, dt=%lu us, RPM: %.1f\n", edge_count, dt_us, filtered_rpm);
    }
}

void BLDC_MOTOR::calculate_rpm() {
    // METHOD 1: Simple Moving Average (good for steady speed)
    uint32_t sum = 0;
    int count = (edge_count < HISTORY_SIZE) ? edge_count : HISTORY_SIZE;
    
    for (int i = 0; i < count; i++) {
        sum += pulse_times[i];
    }
    
    uint32_t avg_period = sum / count;
    float pulses_per_second = 1000000.0f / avg_period;
    float rpm_avg = (pulses_per_second * 60.0f) / 18.0f;
    
    // METHOD 2: Linear Regression (best for accuracy during acceleration)
    // Calculate RPM from first to last pulse in history
    int oldest_index = (pulse_index + HISTORY_SIZE - count) % HISTORY_SIZE;
    uint32_t time_span = pulse_timestamps[pulse_index] - pulse_timestamps[oldest_index];
    
    if (time_span > 0) {
        // pulses / time = frequency
        float freq = (count * 1000000.0f) / time_span;
        float rpm_regression = (freq * 60.0f) / 18.0f;
        
        // Blend both methods: 70% regression, 30% average
        // Regression is better during speed changes
        measured_rpm = (rpm_regression * 0.7f) + (rpm_avg * 0.3f);
    } else {
        measured_rpm = rpm_avg;
    }
    
    // Apply exponential smoothing filter
    // Alpha = 0.3 (adjust: lower = smoother but slower response)
    const float ALPHA = 0.3f;
    if (filtered_rpm == 0.0f) {
        filtered_rpm = measured_rpm;
    } else {
        filtered_rpm = (ALPHA * measured_rpm) + ((1.0f - ALPHA) * filtered_rpm);
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
    return filtered_rpm;  // Return stable, filtered RPM
}

// ⭐ NEW: Advanced RPM methods
float BLDC_MOTOR::get_instantaneous_rpm() const {
    return instantaneous_rpm;
}

uint32_t BLDC_MOTOR::get_time_since_pulse() const {
    return time_us_32() - last_edge_time;
}

bool BLDC_MOTOR::is_running() const {
    return (get_time_since_pulse() < 100000);  // 100ms timeout
}

float BLDC_MOTOR::get_angular_velocity() const {
    // RPM to rad/s: RPM * 2π / 60
    return filtered_rpm * 0.10472f;  // 2π/60 = 0.10472
}

uint32_t BLDC_MOTOR::get_predicted_next_pulse() const {
    if (last_pulse_period == 0) return 0;
    uint32_t elapsed = time_us_32() - last_edge_time;
    if (elapsed >= last_pulse_period) return 0;
    return last_pulse_period - elapsed;
}

int BLDC_MOTOR::get_pulse_position() const {
    return edge_count % 18;
}

// =============================================================================
// Get pulse frequency (Hz)
// =============================================================================
float BLDC_MOTOR::get_frequency() const {
    if (last_pulse_period == 0) return 0.0f;
    return 1000000.0f / last_pulse_period;  // Convert period to frequency
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
    
    // Convert to PWM level (using 12500 wrap value for 10kHz frequency)
    uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
    uint channel = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
    uint16_t pwm_level = (uint16_t)((duty_percent / 100.0f) * 12500);
    
    pwm_set_chan_level(slice_num, channel, pwm_level);
    printf("Set spindle PWM to %.1f%% (level: %d, slice: %d, channel: %d)\n", 
           duty_percent, pwm_level, slice_num, channel);
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
        // For BLDC motors, we need a minimum PWM to start (usually 20-30%)
        float min_duty = 20.0f;  // Minimum 20% to start motor
        float max_duty = 100.0f; // Maximum 100%
        
        // Calibrated mapping: 500 RPM -> 24.8% duty -> 1050 RPM actual
        // So we need to scale down the duty cycle to get correct RPM
        float scale_factor = 500.0f / 1050.0f;  // ~0.48
        float calibrated_rpm = rpm * scale_factor;
        
        // Linear mapping from 0-3000 RPM to 20-100% duty
        float duty_percent = min_duty + ((calibrated_rpm / 3000.0f) * (max_duty - min_duty));
        
        printf("RPM: %.1f -> Duty: %.1f%%\n", rpm, duty_percent);
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
    filtered_rpm = 0.0f;
    instantaneous_rpm = 0.0f;
    last_pulse_period = 0;
    pulse_index = 0;
    last_edge_time = time_us_32();
    
    // Clear pulse history
    for (int i = 0; i < HISTORY_SIZE; i++) {
        pulse_times[i] = 0;
        pulse_timestamps[i] = 0;
    }
    
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
    printf("║ Frequency:          %.1f Hz             ║\n", get_frequency());
    printf("║ Pulses/Rev:         %u                ║\n", pulses_per_revolution);
    printf("╚════════════════════════════════════════╝\n");
}