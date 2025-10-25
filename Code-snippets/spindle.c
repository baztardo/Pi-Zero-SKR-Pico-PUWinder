// spindle.c
#include "spindle.h"
#include "pico/time.h"
#include <stdlib.h>

// FluidNC-style speed mapping
static uint8_t map_speed(spindle_t* s, uint32_t rpm) {
    if (!s->speed_map || s->speed_map_size == 0) {
        // Default linear 0-1000 RPM = 0-100%
        if (rpm > 1000) rpm = 1000;
        return (rpm * 100) / 1000;
    }
    
    if (rpm == 0) return s->speed_map[0].duty_percent;
    
    // Piecewise linear interpolation
    for (uint8_t i = 1; i < s->speed_map_size; i++) {
        if (rpm <= s->speed_map[i].rpm) {
            uint32_t rpm0 = s->speed_map[i-1].rpm;
            uint32_t rpm1 = s->speed_map[i].rpm;
            uint8_t duty0 = s->speed_map[i-1].duty_percent;
            uint8_t duty1 = s->speed_map[i].duty_percent;
            
            // Linear interpolation
            int32_t duty_range = duty1 - duty0;
            int32_t rpm_range = rpm1 - rpm0;
            int32_t rpm_offset = rpm - rpm0;
            
            return duty0 + (duty_range * rpm_offset) / rpm_range;
        }
    }
    
    return s->speed_map[s->speed_map_size - 1].duty_percent;
}

static void set_pwm_duty(spindle_t* s, uint8_t duty_percent) {
    uint16_t level = (duty_percent * 65535) / 100;
    pwm_set_gpio_level(s->pwm_pin, level);
    s->current_duty = duty_percent;
}

static void ramp_to_duty(spindle_t* s, uint8_t target_duty, uint32_t ramp_ms) {
    if (ramp_ms == 0 || s->current_duty == target_duty) {
        set_pwm_duty(s, target_duty);
        return;
    }
    
    int steps = abs((int)target_duty - (int)s->current_duty);
    uint32_t delay_per_step = ramp_ms / steps;
    
    if (s->current_duty < target_duty) {
        for (uint8_t d = s->current_duty; d <= target_duty; d++) {
            set_pwm_duty(s, d);
            sleep_ms(delay_per_step);
        }
    } else {
        for (int d = s->current_duty; d >= (int)target_duty; d--) {
            set_pwm_duty(s, d);
            sleep_ms(delay_per_step);
        }
    }
}

void spindle_init(spindle_t* s, uint8_t pwm, uint8_t dir, uint8_t brake, uint8_t hall) {
    s->pwm_pin = pwm;
    s->dir_pin = dir;
    s->brake_pin = brake;
    s->hall_pin = hall;
    s->pwm_freq = 5000;
    s->spinup_ms = 1000;
    s->spindown_ms = 2000;
    s->state = SPINDLE_DISABLE;
    s->target_rpm = 0;
    s->current_duty = 0;
    s->turn_count = 0;
    s->speed_map = NULL;
    s->speed_map_size = 0;
    
    // Setup GPIO
    gpio_init(dir);
    gpio_set_dir(dir, GPIO_OUT);
    gpio_put(dir, 0);
    
    gpio_init(brake);
    gpio_set_dir(brake, GPIO_OUT);
    gpio_put(brake, 1); // Brake on
    
    gpio_init(hall);
    gpio_set_dir(hall, GPIO_IN);
    gpio_pull_up(hall);
    
    // Setup PWM
    gpio_set_function(pwm, GPIO_FUNC_PWM);
    s->pwm_slice = pwm_gpio_to_slice_num(pwm);
    s->pwm_channel = pwm_gpio_to_channel(pwm);
    
    pwm_set_wrap(s->pwm_slice, 65535);
    pwm_set_clkdiv(s->pwm_slice, 125000000.0f / (65536 * s->pwm_freq));
    pwm_set_chan_level(s->pwm_slice, s->pwm_channel, 0);
    pwm_set_enabled(s->pwm_slice, true);
}

void spindle_set_speed_map(spindle_t* s, speed_point_t* map, uint8_t size) {
    s->speed_map = map;
    s->speed_map_size = size;
}

void spindle_set_state(spindle_t* s, spindle_state_t state, uint32_t rpm) {
    s->state = state;
    s->target_rpm = rpm;
    
    switch (state) {
        case SPINDLE_CW:
            gpio_put(s->brake_pin, 0); // Release brake
            gpio_put(s->dir_pin, 1);
            ramp_to_duty(s, map_speed(s, rpm), s->spinup_ms);
            break;
            
        case SPINDLE_CCW:
            gpio_put(s->brake_pin, 0);
            gpio_put(s->dir_pin, 0);
            ramp_to_duty(s, map_speed(s, rpm), s->spinup_ms);
            break;
            
        case SPINDLE_DISABLE:
            ramp_to_duty(s, 0, s->spindown_ms);
            gpio_put(s->brake_pin, 1); // Brake on
            break;
    }
}

void spindle_stop(spindle_t* s) {
    spindle_set_state(s, SPINDLE_DISABLE, 0);
}

void spindle_brake(spindle_t* s) {
    set_pwm_duty(s, 0);
    gpio_put(s->brake_pin, 1);
}

uint32_t spindle_get_turns(spindle_t* s) {
    return s->turn_count;
}

void spindle_reset_turns(spindle_t* s) {
    s->turn_count = 0;
}

void spindle_hall_callback(spindle_t* s) {
    s->turn_count++;
}
