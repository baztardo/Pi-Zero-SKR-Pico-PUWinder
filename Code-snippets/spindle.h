// spindle.h
#pragma once
#include "pico/stdlib.h"
#include "hardware/pwm.h"

typedef enum {
    SPINDLE_DISABLE = 0,
    SPINDLE_CW = 1,
    SPINDLE_CCW = 2
} spindle_state_t;

typedef struct {
    uint32_t rpm;
    uint8_t duty_percent;
} speed_point_t;

typedef struct {
    // Pins
    uint8_t pwm_pin;
    uint8_t dir_pin;
    uint8_t brake_pin;
    uint8_t hall_pin;
    
    // PWM config
    uint pwm_slice;
    uint pwm_channel;
    uint32_t pwm_freq;
    
    // Speed map
    speed_point_t* speed_map;
    uint8_t speed_map_size;
    
    // Timing
    uint32_t spinup_ms;
    uint32_t spindown_ms;
    
    // State
    spindle_state_t state;
    uint32_t target_rpm;
    uint8_t current_duty;
    volatile uint32_t turn_count;
    
} spindle_t;

void spindle_init(spindle_t* s, uint8_t pwm, uint8_t dir, uint8_t brake, uint8_t hall);
void spindle_set_speed_map(spindle_t* s, speed_point_t* map, uint8_t size);
void spindle_set_state(spindle_t* s, spindle_state_t state, uint32_t rpm);
void spindle_stop(spindle_t* s);
void spindle_brake(spindle_t* s);
uint32_t spindle_get_turns(spindle_t* s);
void spindle_reset_turns(spindle_t* s);
void spindle_hall_callback(spindle_t* s); // Call from GPIO ISR
