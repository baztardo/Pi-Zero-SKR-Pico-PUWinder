// =============================================================================
// move_queue.cpp - COMPLETE WORKING VERSION
// Fixed ISR-driven step execution with full debugging
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
    feeding_paused = false;
    emergency_stop_active = false;
}

void MoveQueue::init() {
    printf("[MoveQueue] Initializing GPIO pins...\n");
    
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
    
    // Initialize debug LEDs
    gpio_init(17);  // FAN1
    gpio_set_dir(17, GPIO_OUT);
    gpio_put(17, 0);
    
    gpio_init(18);  // FAN2
    gpio_set_dir(18, GPIO_OUT);
    gpio_put(18, 0);
    
    printf("[MoveQueue] Initialization complete\n");
    printf("[MoveQueue] - STEP pin (GPIO %d): Output, LOW\n", TRAVERSE_STEP_PIN);
    printf("[MoveQueue] - DIR pin (GPIO %d): Output, LOW\n", TRAVERSE_DIR_PIN);
    printf("[MoveQueue] - ENA pin (GPIO %d): Output, LOW (ENABLED)\n", TRAVERSE_ENA_PIN);
    printf("[MoveQueue] - Debug LED FAN1 (GPIO 17): Ready\n");
    printf("[MoveQueue] - Debug LED FAN2 (GPIO 18): Ready\n");
}

bool MoveQueue::push_chunk(const StepChunk& chunk) {
    uint16_t h = head;
    uint16_t t = tail;
    
    // Check if queue is full
    if (((h + 1) % MOVE_CHUNKS_CAPACITY) == t) {
        printf("[MoveQueue] ❌ ERROR: Queue FULL! Cannot push chunk\n");
        return false;
    }
    
    queue[h] = chunk;
    head = (h + 1) % MOVE_CHUNKS_CAPACITY;
    
    // ✅ Debug: Confirm push (reduced frequency)
    static uint32_t push_count = 0;
    if ((push_count++ % 10) == 0) {
        printf("[MoveQueue] Chunk pushed: %u steps @ %u us/step (depth: %u)\n", 
               chunk.count, chunk.interval_us, get_queue_depth());
    }
    
    return true;
}

bool MoveQueue::pop_chunk(StepChunk& out) {
    uint16_t h = head;
    uint16_t t = tail;
    
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
    
    static bool last_state = false;
    if (enable != last_state) {
        printf("[MoveQueue] Motor %s (ENA=%d)\n", 
               enable ? "ENABLED" : "DISABLED",
               enable ? 0 : 1);
        last_state = enable;
    }
}

bool MoveQueue::is_active() {
    return active_running;
}

int32_t MoveQueue::get_step_count() {
    return step_count;
}

void MoveQueue::execute_step_pulse() {
    // Generate step pulse
    gpio_put(TRAVERSE_STEP_PIN, 1);
    busy_wait_us(STEP_PULSE_US);
    gpio_put(TRAVERSE_STEP_PIN, 0);
    
    // ✅ Visual feedback - toggle FAN2 LED
    static volatile uint32_t pulse_count = 0;
    pulse_count++;
    
    if ((pulse_count % 10) == 0) {
        static bool led_state = false;
        led_state = !led_state;
        gpio_put(18, led_state);  // FAN2 flashes with steps
    }
    
    // ✅ Debug output (reduced frequency)
    if ((pulse_count % 100) == 0) {
        printf("[MoveQueue] ✓ Step #%lu executed\n", pulse_count);
    }
}

void MoveQueue::traverse_isr_handler() {
    // ✅ Counter for debug output
    static volatile uint32_t isr_call_count = 0;
    isr_call_count++;
    
    // ✅ Periodic ISR heartbeat (every 2000 calls = 0.1 sec @ 20kHz)
    if ((isr_call_count % 2000) == 0) {
        // Toggle FAN1 LED to show ISR is running
        static bool fan1_state = false;
        fan1_state = !fan1_state;
        gpio_put(17, fan1_state);
        
        printf("[MoveQueue-ISR] #%lu: depth=%u, active=%s, h=%u, t=%u\n",
               isr_call_count, get_queue_depth(), 
               active_running ? "YES" : "NO", head, tail);
    }
    
    // ✅ CRITICAL: Check safety flags FIRST
    if (feeding_paused) {
        static uint32_t pause_warn_count = 0;
        if ((pause_warn_count++ % 10000) == 0) {
            printf("[MoveQueue-ISR] ⚠️  Feed hold active - not processing moves\n");
        }
        return;
    }
    
    if (emergency_stop_active) {
        return;  // Don't process any moves during emergency stop
    }
    
    // If not running an active chunk, try to load one
    if (!active_running) {
        if (head == tail) {
            // Queue empty - this is normal when not winding
            return;
        }
        
        // ✅ Load next chunk
        active = queue[tail];
        tail = (tail + 1) % MOVE_CHUNKS_CAPACITY;
        active_running = true;
        last_step_time = time_us_32();
        
        // ✅ CRITICAL DEBUG: Show chunk loading
        printf("[MoveQueue-ISR] *** LOADING CHUNK ***: %u steps @ %u us/step\n", 
               active.count, active.interval_us);
        printf("[MoveQueue-ISR] Queue depth now: %u, Active: YES\n", 
               get_queue_depth());
        
        return;  // Process on next ISR tick
    }
    
    // ✅ Process active chunk - single step per ISR tick
    uint32_t now = time_us_32();
    int32_t time_diff = (int32_t)(now - last_step_time);
    
    // Check if it's time for next step
    if (time_diff < (int32_t)active.interval_us) {
        return;  // Not time yet
    }

    // ✅ EXECUTE STEP
    execute_step_pulse();

    // Update timing
    last_step_time += active.interval_us;
    step_count++;

    // Decrement remaining steps
    if (active.count > 0) active.count--;

    // Adjust interval for acceleration
    int64_t next_interval = (int64_t)active.interval_us + (int64_t)active.add_us;
    active.interval_us = (uint32_t)std::max((int64_t)1, next_interval);

    // Check if chunk complete
    if (active.count == 0) {
        active_running = false;
        printf("[MoveQueue-ISR] Chunk complete, total steps: %ld\n", step_count);
    }
}

// =============================================================================
// Safety and Feed Control
// =============================================================================

void MoveQueue::pause_feeding() {
    feeding_paused = true;
    printf("[MoveQueue] ⚠️  Feed hold ACTIVATED\n");
}

void MoveQueue::resume_feeding() {
    feeding_paused = false;
    printf("[MoveQueue] ✓ Feed hold RELEASED\n");
}

void MoveQueue::emergency_stop() {
    emergency_stop_active = true;
    feeding_paused = true;
    
    // Stop traverse movement
    active_running = false;
    clear_queue();
    set_enable(false);  // Disable traverse motor
    
    printf("[MoveQueue] 🛑 EMERGENCY STOP ACTIVATED\n");
}
