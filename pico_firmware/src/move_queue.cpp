// =============================================================================
// move_queue.cpp - MCU Move Queue Implementation
// =============================================================================

#include "move_queue.h"
#include "config.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

MoveQueue::MoveQueue() {
    head = 0;
    tail = 0;
    active_running = false;
    step_count = 0;
    
    // ⭐ NEW: Initialize FluidNC-style safety and feed control
    feeding_paused = false;
    emergency_stop_active = false;
}

void MoveQueue::init() {
    
    // Initialize traverse stepper pins
    gpio_init(TRAVERSE_STEP_PIN);
    gpio_set_dir(TRAVERSE_STEP_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_STEP_PIN, 0);
    
    gpio_init(TRAVERSE_DIR_PIN);
    gpio_set_dir(TRAVERSE_DIR_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_DIR_PIN, 0);
    
    gpio_init(TRAVERSE_ENA_PIN);
    gpio_set_dir(TRAVERSE_ENA_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_ENA_PIN, 0);  // Active low - motor enabled
}

bool MoveQueue::push_chunk(const StepChunk& chunk) {
    uint16_t h = head;
    uint16_t t = tail;
    
    // Check if queue is full
    if (((h + 1) % MOVE_CHUNKS_CAPACITY) == t) {
        return false;
    }
    
    queue[h] = chunk;
    head = (h + 1) % MOVE_CHUNKS_CAPACITY;
    
    return true;
}

bool MoveQueue::pop_chunk(StepChunk& out) {
    uint16_t h = head;
    uint16_t t = tail;
    
    // Check if queue is empty
    if (h == t) return false;
    
    out = queue[t];
    tail = (t + 1) % MOVE_CHUNKS_CAPACITY;
    
    return true;
}

bool MoveQueue::has_chunk() {
    return head != tail;
}

uint32_t MoveQueue::get_queue_depth() const {
    uint16_t h = head;
    uint16_t t = tail;
    
    if (h >= t) {
        return h - t;
    } else {
        return MOVE_CHUNKS_CAPACITY - t + h;
    }
}

void MoveQueue::clear_queue() {
    tail = head;
    active_running = false;
}

void MoveQueue::set_direction(bool forward) {
    gpio_put(TRAVERSE_DIR_PIN, forward ? 1 : 0);
}

void MoveQueue::set_enable(bool enable) {
    gpio_put(TRAVERSE_ENA_PIN, enable ? 0 : 1);  // Active low
}

bool MoveQueue::is_active() {
    return active_running;
}

int32_t MoveQueue::get_step_count() {
    return step_count;
}

void MoveQueue::execute_step_pulse() {
    gpio_put(TRAVERSE_STEP_PIN, 1);
    busy_wait_us(STEP_PULSE_US);
    gpio_put(TRAVERSE_STEP_PIN, 0);
    
    // Debug: Show step execution
    static uint32_t step_debug_count = 0;
    if ((step_debug_count++ % 50) == 0) {  // Every 50 steps
        printf("[MoveQueue] Step #%u executed\n", step_debug_count);
    }
}

void MoveQueue::traverse_isr_handler() {
    // Debug: Show ISR calls occasionally
    static uint32_t isr_debug_count = 0;
    if ((isr_debug_count++ % 1000) == 0) {  // Every 1000 ISR calls
        printf("[MoveQueue] ISR called #%u, Queue depth: %u, Active: %s\n", 
               isr_debug_count, get_queue_depth(), active_running ? "YES" : "NO");
    }
    
    // If not running an active chunk, try to load one
    if (!active_running) {
        if (head == tail) {
            return;  // Queue empty
        }
        
        // Load next chunk
        active = queue[tail];
        tail = (tail + 1) % MOVE_CHUNKS_CAPACITY;
        active_running = true;
        last_step_time = time_us_32();
        
        // Debug: Show chunk loading
        printf("[MoveQueue] Loaded chunk: %u steps, %u us interval\n", 
               active.count, active.interval_us);
        printf("[MoveQueue] Queue depth: %u, Active: %s\n", 
               get_queue_depth(), active_running ? "YES" : "NO");
        return;
    }
    
    // Single-step per tick (stable)
    uint32_t now = time_us_32();
    int32_t time_diff = (int32_t)(now - last_step_time);
    if (time_diff < (int32_t)active.interval_us) return;

    execute_step_pulse();

    last_step_time += active.interval_us;
    step_count++;

    if (active.count > 0) active.count--;

    int64_t next_interval = (int64_t)active.interval_us + (int64_t)active.add_us;
    active.interval_us = (uint32_t)std::max((int64_t)1, next_interval);

    if (active.count == 0) active_running = false;
}


// =============================================================================
// ⭐ NEW: FluidNC-style Safety and Feed Control Methods
// =============================================================================

void MoveQueue::pause_feeding() {
    feeding_paused = true;
    printf("[MoveQueue] Feed hold activated\n");
}

void MoveQueue::resume_feeding() {
    feeding_paused = false;
    printf("[MoveQueue] Feed hold released\n");
}

void MoveQueue::emergency_stop() {
    emergency_stop_active = true;
    feeding_paused = true;
    
    // Stop traverse movement
    active_running = false;
    clear_queue();
    set_enable(false);  // Disable traverse motor
    
    printf("[MoveQueue] EMERGENCY STOP ACTIVATED\n");
}