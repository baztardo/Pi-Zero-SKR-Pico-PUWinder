// traverse.h - Stepper control for wire guide (from FluidNC concepts)
#pragma once
#include "pico/stdlib.h"
#include "pico/time.h"

typedef struct {
    // Pins
    uint8_t step_pin;
    uint8_t dir_pin;
    uint8_t enable_pin;
    
    // Motion params (FluidNC-style)
    float steps_per_mm;
    float max_rate_mm_per_min;
    float acceleration_mm_per_sec2;
    float max_travel_mm;
    
    // Timing (microseconds)
    uint32_t pulse_us;
    uint32_t dir_delay_us;
    
    // State
    volatile int32_t current_pos;      // Steps
    volatile int32_t target_pos;       // Steps
    bool direction;           // 0=neg, 1=pos
    bool enabled;
    
    // Trapezoid motion
    volatile float current_speed;      // steps/sec
    float target_speed;       // steps/sec
    uint32_t accel_until;     // Step to stop accel
    uint32_t decel_from;      // Step to start decel
    
    // Timer
    alarm_id_t step_alarm;
    
} traverse_t;

void traverse_init(traverse_t* t, uint8_t step, uint8_t dir, uint8_t enable);
void traverse_enable(traverse_t* t, bool enable);
void traverse_move_abs(traverse_t* t, float position_mm);
void traverse_move_rel(traverse_t* t, float distance_mm);
void traverse_stop(traverse_t* t);
bool traverse_is_moving(traverse_t* t);
float traverse_get_position_mm(traverse_t* t);
void traverse_set_zero(traverse_t* t);
