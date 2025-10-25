// =============================================================================
// move_queue.cpp - MCU Move Queue Implementation
// =============================================================================

#include "move_queue.h"
#include "config.h"
#include "pico/stdlib.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

MoveQueue::MoveQueue() {
    memset(head, 0, sizeof(head));
    memset(tail, 0, sizeof(tail));
    memset(active_running, 0, sizeof(active_running));
    memset(last_step_time, 0, sizeof(last_step_time));
    memset(step_count, 0, sizeof(step_count));
    feeding_paused = false;  // ⭐ NEW: Initialize feed control
}

void MoveQueue::init() {
    // Initialize spindle step/dir/enable pins
    gpio_init(SPINDLE_STEP_PIN);
    gpio_set_dir(SPINDLE_STEP_PIN, GPIO_OUT);
    gpio_put(SPINDLE_STEP_PIN, 0);
    
    gpio_init(SPINDLE_DIR_PIN);
    gpio_set_dir(SPINDLE_DIR_PIN, GPIO_OUT);
    gpio_put(SPINDLE_DIR_PIN, 0);
    
    gpio_init(SPINDLE_ENA_PIN);
    gpio_set_dir(SPINDLE_ENA_PIN, GPIO_OUT);
    gpio_put(SPINDLE_ENA_PIN, 1);  // Disabled initially (active low)
    
    // Initialize traverse step/dir/enable pins
    gpio_init(TRAVERSE_STEP_PIN);
    gpio_set_dir(TRAVERSE_STEP_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_STEP_PIN, 0);
    
    gpio_init(TRAVERSE_DIR_PIN);
    gpio_set_dir(TRAVERSE_DIR_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_DIR_PIN, 0);
    
    gpio_init(TRAVERSE_ENA_PIN);
    gpio_set_dir(TRAVERSE_ENA_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_ENA_PIN, 1);  // Disabled initially (active low)
    
    printf("[MoveQueue] Initialized\n");
}

// =============================================================================
// ⭐ UPDATED: Push chunk with safety checks
// =============================================================================
bool MoveQueue::push_chunk(uint8_t axis, const StepChunk& chunk) {
    if (axis >= NUM_AXES) return false;
    
    // ⭐ NEW: Check if feeding is paused
    if (feeding_paused) {
        printf("[HOLD] Not accepting new moves\n");
        return false;
    }
    
    // ⭐ NEW: Check soft limits for traverse
    if (axis == AXIS_TRAVERSE) {
        int32_t target_pos = step_count[axis] + (int32_t)chunk.count;
        if (target_pos < TRAVERSE_MIN_STEPS || target_pos > TRAVERSE_MAX_STEPS) {
            printf("[ERROR] Soft limit! Target: %ld, Limits: [%ld, %ld]\n", 
                   target_pos, (long)TRAVERSE_MIN_STEPS, (long)TRAVERSE_MAX_STEPS);
            return false;
        }
    }
    
    // Check queue full
    if ((head[axis] + 1) % MOVE_CHUNKS_CAPACITY == tail[axis]) {
        printf("[ERROR] Queue full for axis %d\n", axis);
        return false;
    }
    
    // Add chunk
    queues[axis][head[axis]] = chunk;
    head[axis] = (head[axis] + 1) % MOVE_CHUNKS_CAPACITY;
    
    return true;
}

bool MoveQueue::pop_chunk(uint8_t axis, StepChunk& out) {
    if (axis >= NUM_AXES) return false;
    
    if (head[axis] == tail[axis]) {
        return false;  // Queue empty
    }
    
    out = queues[axis][tail[axis]];
    tail[axis] = (tail[axis] + 1) % MOVE_CHUNKS_CAPACITY;
    return true;
}

bool MoveQueue::has_chunk(uint8_t axis) {
    if (axis >= NUM_AXES) return false;
    return head[axis] != tail[axis];
}

void MoveQueue::clear_queue(uint8_t axis) {
    if (axis >= NUM_AXES) return;
    head[axis] = 0;
    tail[axis] = 0;
    active_running[axis] = false;
}

void MoveQueue::set_direction(uint8_t axis, bool forward) {
    uint pin = (axis == AXIS_SPINDLE) ? SPINDLE_DIR_PIN : TRAVERSE_DIR_PIN;
    gpio_put(pin, forward ? 1 : 0);
}

void MoveQueue::set_enable(uint8_t axis, bool enable) {
    uint pin = (axis == AXIS_SPINDLE) ? SPINDLE_ENA_PIN : TRAVERSE_ENA_PIN;
    gpio_put(pin, enable ? 0 : 1);  // Active low
}

bool MoveQueue::is_active(uint8_t axis) {
    if (axis >= NUM_AXES) return false;
    return active_running[axis];
}

int32_t MoveQueue::get_step_count(uint8_t axis) {
    if (axis >= NUM_AXES) return 0;
    return step_count[axis];
}

void MoveQueue::execute_step_pulse(uint32_t step_pin) {
    gpio_put(step_pin, 1);
    busy_wait_us(STEP_PULSE_US);
    gpio_put(step_pin, 0);
}

void MoveQueue::axis_isr_handler(uint8_t axis) {
    if (axis >= NUM_AXES) return;
    
    // If not running an active chunk, try to load one
    if (!active_running[axis]) {
        if (head[axis] == tail[axis]) {
            return;  // Queue empty
        }
        
        // Load next chunk
        active[axis] = queues[axis][tail[axis]];
        tail[axis] = (tail[axis] + 1) % MOVE_CHUNKS_CAPACITY;
        active_running[axis] = true;
        last_step_time[axis] = time_us_32();
        return;
    }
    
    // Single-step per tick (stable)
    uint32_t now = time_us_32();
    int32_t time_diff = (int32_t)(now - last_step_time[axis]);
    if (time_diff < (int32_t)active[axis].interval_us) return;

    uint step_pin = (axis == AXIS_SPINDLE) ? SPINDLE_STEP_PIN : TRAVERSE_STEP_PIN;
    execute_step_pulse(step_pin);

    last_step_time[axis] += active[axis].interval_us;
    step_count[axis]++;

    if (active[axis].count > 0) active[axis].count--;

    int64_t next_interval = (int64_t)active[axis].interval_us + (int64_t)active[axis].add_us;
    active[axis].interval_us = (uint32_t)std::max((int64_t)1, next_interval);

    if (active[axis].count == 0) active_running[axis] = false;
}

// =============================================================================
// ⭐ UPDATED: ISR tick handler with emergency stop check
// =============================================================================
void MoveQueue::handle_isr_tick() {
    // ⭐ CRITICAL: Check emergency stop FIRST
    extern volatile bool g_emergency_stop;
    if (g_emergency_stop) {
        // Disable ALL motors immediately
        gpio_put(TRAVERSE_ENA_PIN, 1);  // Active low = disabled
        gpio_put(SPINDLE_ENA_PIN, 1);
        
        // Clear all moves
        for (uint8_t axis = 0; axis < NUM_AXES; axis++) {
            clear_queue(axis);
            active_running[axis] = false;
        }
        
        return;  // Exit immediately - do not process any moves
    }
    
    // Normal alternating axis processing
    static uint8_t last_axis = AXIS_TRAVERSE;
    uint8_t first  = (last_axis == AXIS_TRAVERSE) ? AXIS_SPINDLE : AXIS_TRAVERSE;
    uint8_t second = (first == AXIS_SPINDLE) ? AXIS_TRAVERSE : AXIS_SPINDLE;

    axis_isr_handler(first);
    axis_isr_handler(second);

    last_axis = second;
}

// =============================================================================
// ⭐ NEW: Feed control methods
// =============================================================================
void MoveQueue::pause_feeding() {
    feeding_paused = true;
    printf("[HOLD] Feed paused - finishing current moves\n");
}

void MoveQueue::resume_feeding() {
    feeding_paused = false;
    printf("[RESUME] Feed resumed - accepting new moves\n");
}
