// traverse.c - FluidNC-inspired stepper control with trapezoid motion
#include "traverse.h"
#include <math.h>
#include <stdlib.h>

static traverse_t* g_traverse = NULL; // For callback

static void do_step(traverse_t* t) {
    gpio_put(t->step_pin, 1);
    busy_wait_us(t->pulse_us);
    gpio_put(t->step_pin, 0);
    
    t->current_pos += t->direction ? 1 : -1;
}

// Step timer callback
static int64_t step_callback(alarm_id_t id, void* user_data) {
    traverse_t* t = (traverse_t*)user_data;
    
    if (t->current_pos == t->target_pos) {
        t->step_alarm = 0;
        t->current_speed = 0;
        return 0; // Don't reschedule
    }
    
    // Trapezoid profile
    int32_t remaining = abs(t->target_pos - t->current_pos);
    int32_t elapsed = abs(t->current_pos - (t->target_pos > t->current_pos ? 
                                            t->target_pos - remaining : 
                                            t->target_pos + remaining));
    
    // Calculate speed based on profile
    float max_speed = t->target_speed;
    float accel = t->acceleration_mm_per_sec2 * t->steps_per_mm;
    
    // Deceleration phase
    float decel_distance = (t->current_speed * t->current_speed) / (2.0 * accel);
    if (remaining <= (int32_t)decel_distance) {
        t->current_speed -= accel * 0.001; // Per ms
        if (t->current_speed < 50) t->current_speed = 50;
    }
    // Acceleration phase
    else if (t->current_speed < max_speed) {
        t->current_speed += accel * 0.001;
        if (t->current_speed > max_speed) {
            t->current_speed = max_speed;
        }
    }
    
    do_step(t);
    
    // Calculate next interval (microseconds)
    uint32_t interval_us = (uint32_t)(1000000.0 / t->current_speed);
    if (interval_us < 100) interval_us = 100; // Limit max speed
    
    return interval_us; // Reschedule
}

void traverse_init(traverse_t* t, uint8_t step, uint8_t dir, uint8_t enable) {
    t->step_pin = step;
    t->dir_pin = dir;
    t->enable_pin = enable;
    
    // Defaults (FluidNC style for pickup winder traverse)
    t->steps_per_mm = 100.0;
    t->max_rate_mm_per_min = 2000.0;
    t->acceleration_mm_per_sec2 = 200.0;
    t->max_travel_mm = 50.0;
    t->pulse_us = 4;
    t->dir_delay_us = 1;
    
    t->current_pos = 0;
    t->target_pos = 0;
    t->direction = true;
    t->enabled = false;
    t->current_speed = 0;
    t->step_alarm = 0;
    
    // Setup GPIO
    gpio_init(step);
    gpio_set_dir(step, GPIO_OUT);
    gpio_put(step, 0);
    
    gpio_init(dir);
    gpio_set_dir(dir, GPIO_OUT);
    gpio_put(dir, 0);
    
    gpio_init(enable);
    gpio_set_dir(enable, GPIO_OUT);
    gpio_put(enable, 1); // Disabled
    
    g_traverse = t;
}

void traverse_enable(traverse_t* t, bool enable) {
    t->enabled = enable;
    gpio_put(t->enable_pin, !enable); // Active low
}

void traverse_move_abs(traverse_t* t, float position_mm) {
    if (!t->enabled) return;
    
    // Stop any existing motion
    if (t->step_alarm) {
        cancel_alarm(t->step_alarm);
        t->step_alarm = 0;
    }
    
    // Clamp to travel limits
    if (position_mm < 0) position_mm = 0;
    if (position_mm > t->max_travel_mm) position_mm = t->max_travel_mm;
    
    int32_t target_steps = (int32_t)(position_mm * t->steps_per_mm);
    t->target_pos = target_steps;
    
    // Check if already at position
    if (t->target_pos == t->current_pos) return;
    
    // Set direction
    bool new_dir = (t->target_pos > t->current_pos);
    if (new_dir != t->direction) {
        t->direction = new_dir;
        gpio_put(t->dir_pin, new_dir);
        busy_wait_us(t->dir_delay_us);
    }
    
    // Calculate target speed (steps/sec)
    t->target_speed = (t->max_rate_mm_per_min / 60.0) * t->steps_per_mm;
    
    // Calculate trapezoid points
    int32_t total_steps = abs(t->target_pos - t->current_pos);
    float accel = t->acceleration_mm_per_sec2 * t->steps_per_mm;
    float accel_steps = (t->target_speed * t->target_speed) / (2.0 * accel);
    
    if (accel_steps * 2 > total_steps) {
        // Triangle profile - peak speed lower than max
        float peak_speed = sqrt(accel * total_steps);
        t->target_speed = peak_speed;
        t->accel_until = total_steps / 2;
        t->decel_from = total_steps / 2;
    } else {
        // Trapezoid profile
        t->accel_until = (uint32_t)accel_steps;
        t->decel_from = total_steps - (uint32_t)accel_steps;
    }
    
    // Start from low speed for smooth start
    t->current_speed = 50; // steps/sec
    
    // Start stepping
    t->step_alarm = add_alarm_in_us(100, step_callback, t, false);
}

void traverse_move_rel(traverse_t* t, float distance_mm) {
    float current_mm = t->current_pos / t->steps_per_mm;
    float new_pos_mm = current_mm + distance_mm;
    traverse_move_abs(t, new_pos_mm);
}

void traverse_stop(traverse_t* t) {
    if (t->step_alarm) {
        cancel_alarm(t->step_alarm);
        t->step_alarm = 0;
    }
    t->target_pos = t->current_pos;
    t->current_speed = 0;
}

bool traverse_is_moving(traverse_t* t) {
    return (t->current_pos != t->target_pos) || (t->step_alarm != 0);
}

float traverse_get_position_mm(traverse_t* t) {
    return t->current_pos / t->steps_per_mm;
}

void traverse_set_zero(traverse_t* t) {
    traverse_stop(t);
    t->current_pos = 0;
    t->target_pos = 0;
}
