// =============================================================================
// bldc_speed_pulse.cpp - BLDC Speed Pulse ISR Handler
// Purpose: Count speed pulses from SC output and calculate RPM
// =============================================================================

#include "spindle.h"
#include "pico/stdlib.h"
#include <cstdio>

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
    , pulses_per_revolution(6)  // Default: 6 edges per full rotation
    , direction(DIRECTION_CW)
    , brake(false)

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
    
    gpio_init(BLDC_MOTOR_DIR_PIN);
    gpio_set_dir(BLDC_MOTOR_DIR_PIN, GPIO_OUT);
    gpio_put(BLDC_MOTOR_DIR_PIN, 1);  // Start forward

    gpio_init(BLDC_MOTOR_BRAKE_PIN);
    gpio_set_dir(BLDC_MOTOR_BRAKE_PIN, GPIO_OUT);
    gpio_put(BLDC_MOTOR_BRAKE_PIN, 0);  // Start brake OFF

    // Enable interrupt on rising edge
    gpio_set_irq_enabled_with_callback(
        pulse_pin,
        GPIO_IRQ_EDGE_RISE,  // Trigger on rising edge
        true,
        &BLDC_MOTOR::isr_wrapper
    );
    
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
    
    // Debounce: ignore edges faster than 100µs (noise)
    if (dt_us < 100) {
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
            
            // Also calculate pulse frequency (Hz)
            pulse_frequency = 1.0f / (dt_us / 1e6f);
        }
    }
}

void BLDC_MOTOR::set_direction(MotorDirection direction) {
    this->direction = direction;
    gpio_put(BLDC_MOTOR_DIR_PIN, direction);  // 1=HIGH(CW), 0=LOW(CCW)f
}

void BLDC_MOTOR::set_brake(bool brake) {
    this->brake = brake;
    gpio_put(BLDC_MOTOR_BRAKE_PIN, brake);  // 1=HIGH(Brake), 0=LOW(Release)
}   
void BLDC_MOTOR::get_brake() const {
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
    
    if (alpha < 0.0f) alpha = 0.0f;
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