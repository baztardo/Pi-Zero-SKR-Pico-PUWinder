// =============================================================================
// move_queue.cpp - FIXED VERSION (ISR Printf Removed)
// Key fixes:
// 1. Remove ALL printf from ISR
// 2. Add LED indicators for ISR state
// 3. Add diagnostic counters readable from main loop
// 4. Keep safety checks but make them visible
// =============================================================================

#include "move_queue.h"
#include "config.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

// ISR diagnostic counters (volatile for thread safety)
static volatile uint32_t g_isr_call_count = 0;
static volatile uint32_t g_chunks_loaded = 0;
static volatile uint32_t g_steps_executed = 0;
static volatile uint32_t g_feeding_paused_hits = 0;
static volatile uint32_t g_emergency_stop_hits = 0;
static volatile bool g_last_active_state = false;

MoveQueue::MoveQueue() {
    head = 0;
    tail = 0;
    active_running = false;
    step_count = 0;
    feeding_paused = false;
    emergency_stop_active = false;
    pio_stepper = nullptr;
}

void MoveQueue::init() {
    printf("[MoveQueue] Initializing with PIO hybrid mode...\n");
    
    // ⭐ Create PIO stepper (but keep it INACTIVE for now)
    // This allows TraverseController to use GPIO for homing
    pio_stepper = new PIOStepper(TRAVERSE_STEP_PIN, TRAVERSE_DIR_PIN);
    
    // Initialize STEP pin as regular GPIO (for homing)
    // PIO will take over when activate_pio_mode() is called
    gpio_init(TRAVERSE_STEP_PIN);
    gpio_set_dir(TRAVERSE_STEP_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_STEP_PIN, 0);
    
    // DIR pin is always regular GPIO (not PIO-controlled)
    gpio_init(TRAVERSE_DIR_PIN);
    gpio_set_dir(TRAVERSE_DIR_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_DIR_PIN, 0);
    
    // Enable pin
    gpio_init(TRAVERSE_ENA_PIN);
    gpio_set_dir(TRAVERSE_ENA_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_ENA_PIN, 0);  // Active low - motor enabled
    
    printf("[MoveQueue] ✓ Initialization complete (GPIO mode for homing)\n");
    printf("[MoveQueue] - STEP pin (GPIO %d): GPIO mode (PIO available)\n", TRAVERSE_STEP_PIN);
    printf("[MoveQueue] - DIR pin (GPIO %d): GPIO mode\n", TRAVERSE_DIR_PIN);
    printf("[MoveQueue] - ENA pin (GPIO %d): Active LOW (ENABLED)\n", TRAVERSE_ENA_PIN);
}

bool MoveQueue::push_chunk(const StepChunk& chunk) {
    uint16_t h = head;
    uint16_t t = tail;
    
    // Check if queue is full
    if (((h + 1) % MOVE_CHUNKS_CAPACITY) == t) {
        // Don't printf here - called from sync loop at high frequency
        return false;
    }
    
    queue[h] = chunk;
    head = (h + 1) % MOVE_CHUNKS_CAPACITY;
    
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
    g_last_active_state = false;
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
    // Generate step pulse
    gpio_put(TRAVERSE_STEP_PIN, 1);
    busy_wait_us(STEP_PULSE_US);
    gpio_put(TRAVERSE_STEP_PIN, 0);
    
    // Increment counter (volatile, ISR-safe)
    g_steps_executed++;
}

// =============================================================================
// ISR Handler - NO PRINTF ALLOWED
// =============================================================================
void MoveQueue::traverse_isr_handler() {
    // Increment call counter
    g_isr_call_count++;
    
    // Heartbeat LED (FAN1) - toggle every 2000 calls (0.1 sec @ 20kHz)
    if ((g_isr_call_count % 2000) == 0) {
        static bool heartbeat = false;
        heartbeat = !heartbeat;
        gpio_put(17, heartbeat);  // FAN1
    }
    
    // CRITICAL: Check safety flags FIRST
    if (feeding_paused) {
        g_feeding_paused_hits++;
        return;
    }
    
    if (emergency_stop_active) {
        g_emergency_stop_hits++;
        return;
    }
    
    // If not running an active chunk, try to load one
    if (!active_running) {
        if (head == tail) {
            // Queue empty - normal when not winding
            return;
        }
        
        // Load next chunk
        active = queue[tail];
        tail = (tail + 1) % MOVE_CHUNKS_CAPACITY;
        active_running = true;
        last_step_time = time_us_32();
        
        // Update counters
        g_chunks_loaded++;
        g_last_active_state = true;
        
        // Turn on FAN2 LED to show active
        gpio_put(18, 1);
        
        return;  // Process on next ISR tick
    }
    
    if (pio_stepper && pio_stepper->is_active()) {
        // DEBUG: Print once to confirm PIO path
        static bool pio_debug_printed = false;
        if (!pio_debug_printed) {
            printf("[ISR] ✓✓✓ PIO MODE ACTIVE - feeding hardware FIFO!\n");
            pio_debug_printed = true;
        }
        
        // PIO mode - feed steps to hardware FIFO
        while (active.count > 0 && pio_stepper->can_queue_step()) {
            // Queue step with current interval
            if (pio_stepper->queue_step(active.interval_us)) {
                step_count++;
                g_steps_executed++;
                active.count--;
                
                // Adjust interval for acceleration
                if (active.add_us != 0) {
                    int64_t next_interval = (int64_t)active.interval_us + (int64_t)active.add_us;
                    active.interval_us = (uint32_t)std::max((int64_t)1, next_interval);
                }
            } else {
                break;  // FIFO full, try again next ISR call
            }
        }
        
        // Check if chunk complete
        if (active.count == 0) {
            active_running = false;
            g_last_active_state = false;
        }
        return;
    }
    
    // ⭐ GPIO MODE: Direct GPIO stepping (for homing/manual moves)
    // This is the fallback when PIO is not active
    uint32_t now = time_us_32();
    int32_t time_diff = (int32_t)(now - last_step_time);
    
    // Check if it's time for next step
    if (time_diff < (int32_t)active.interval_us) {
        return;  // Not time yet
    }

    // EXECUTE STEP via GPIO
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
        g_last_active_state = false;
        
        // Turn off FAN2 LED
        gpio_put(18, 0);
    }
}

// =============================================================================
// Diagnostics - Call from main loop, not ISR
// =============================================================================
void MoveQueue::print_diagnostics() {
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║              MOVEQUEUE DIAGNOSTICS                        ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("ISR Stats:\n");
    printf("  - ISR calls:           %lu\n", g_isr_call_count);
    printf("  - Chunks loaded:       %lu\n", g_chunks_loaded);
    printf("  - Steps executed:      %lu\n", g_steps_executed);
    printf("  - Feed paused hits:    %lu\n", g_feeding_paused_hits);
    printf("  - E-stop hits:         %lu\n", g_emergency_stop_hits);
    printf("\n");
    printf("Queue State:\n");
    printf("  - Queue depth:         %u / %d\n", get_queue_depth(), MOVE_CHUNKS_CAPACITY);
    printf("  - Head:                %u\n", head);
    printf("  - Tail:                %u\n", tail);
    printf("  - Active running:      %s\n", active_running ? "YES" : "NO");
    printf("  - Last active state:   %s\n", g_last_active_state ? "YES" : "NO");
    printf("\n");
    printf("Safety Flags:\n");
    printf("  - Feeding paused:      %s\n", feeding_paused ? "⚠️ YES" : "✓ NO");
    printf("  - Emergency stop:      %s\n", emergency_stop_active ? "🛑 YES" : "✓ NO");
    printf("\n");
    printf("Active Chunk:\n");
    if (active_running) {
        printf("  - Steps remaining:     %u\n", active.count);
        printf("  - Interval:            %u µs\n", active.interval_us);
        printf("  - Acceleration:        %d µs/step\n", active.add_us);
    } else {
        printf("  - No active chunk\n");
    }
    printf("═══════════════════════════════════════════════════════════\n\n");
}

// Reset diagnostic counters
void MoveQueue::reset_diagnostics() {
    g_isr_call_count = 0;
    g_chunks_loaded = 0;
    g_steps_executed = 0;
    g_feeding_paused_hits = 0;
    g_emergency_stop_hits = 0;
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

bool MoveQueue::is_feeding_paused() const {
    return feeding_paused;
}

bool MoveQueue::is_emergency_stopped() const {
    return emergency_stop_active;
}

// =============================================================================
// PIO Mode Control - Handoff between GPIO (homing) and PIO (winding)
// =============================================================================
void MoveQueue::activate_pio_mode() {
    if (pio_stepper) {
        printf("[MoveQueue] 🚀 Activating PIO mode for winding...\n");
        pio_stepper->activate();
    }
}

void MoveQueue::deactivate_pio_mode() {
    if (pio_stepper) {
        printf("[MoveQueue] 🏠 Deactivating PIO mode (GPIO ready for homing)...\n");
        pio_stepper->deactivate();
    }
}

bool MoveQueue::is_pio_active() const {
    return pio_stepper && pio_stepper->is_active();
}