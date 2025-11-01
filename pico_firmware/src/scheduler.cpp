// =============================================================================
// scheduler.cpp - CLEAN WORKING SCHEDULER
// Using consistent Pico SDK API - no mixing with raw hardware registers
// =============================================================================

#include "scheduler.h"
#include "pico/stdlib.h"
#include <cstdio>
#include "config.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

// Global scheduler instance for callback
static Scheduler* g_scheduler_instance = nullptr;

// Alarm callback function - called every 50µs (20kHz)
void scheduler_alarm_callback(uint alarm_num) {
    // Call ISR handler
    if (g_scheduler_instance) {
        g_scheduler_instance->handle_isr();
    }
    
    // CRITICAL: Re-arm the alarm for the next interrupt using SDK
    // This ensures the timer keeps firing at 20kHz
    hardware_alarm_set_target(alarm_num, delayed_by_us(get_absolute_time(), 50));
}

Scheduler::Scheduler(MoveQueue* mq)
    : move_queue(mq)
    , tick_count(0)
    , interval_us(HEARTBEAT_US)
    , running(false)
    , user_callback(nullptr)
    , user_callback_data(nullptr) {
    
    g_scheduler_instance = this;
}

bool Scheduler::start(uint32_t interval) {
    printf("[Scheduler] Starting scheduler...\n");
    
    // Initialize heartbeat pin
    gpio_init(SCHED_HEARTBEAT_PIN);
    gpio_set_dir(SCHED_HEARTBEAT_PIN, GPIO_OUT);
    gpio_put(SCHED_HEARTBEAT_PIN, 0);
    
    // Initialize FAN1 LED for ISR heartbeat (toggled in traverse_isr_handler)
    gpio_init(ISR_HEARTBEAT_PIN);
    gpio_set_dir(ISR_HEARTBEAT_PIN, GPIO_OUT);
    gpio_put(ISR_HEARTBEAT_PIN, 0); 
    printf("[Scheduler] FAN1 LED initialized on pin ISR_HEARTBEAT_PIN\n");
    
    interval_us = 50; // 20kHz = 50µs period
    
    // Claim hardware alarm 0
    printf("[Scheduler] Claiming hardware alarm 0...\n");
    hardware_alarm_claim(0);
    printf("[Scheduler] Hardware alarm 0 claimed\n");
    
    // Set up callback function
    printf("[Scheduler] Setting up alarm callback...\n");
    hardware_alarm_set_callback(0, scheduler_alarm_callback);
    printf("[Scheduler] Callback registered\n");
    
    // Start first interrupt using SDK - this will trigger the callback
    printf("[Scheduler] Starting first interrupt...\n");
    hardware_alarm_set_target(0, delayed_by_us(get_absolute_time(), 50));
    printf("[Scheduler] First alarm set\n");
    
    running = true;
    printf("[Scheduler] ✓ Hardware timer started at 20kHz\n");
    printf("[Scheduler] ISR will call MoveQueue every 50µs\n");
    
    return true;
}

void Scheduler::stop() {
    if (!running) return;
    
    // Cancel hardware alarm
    hardware_alarm_cancel(0);
    hardware_alarm_unclaim(0);
    
    running = false;
    printf("[Scheduler] Hardware timer stopped\n");
}

bool Scheduler::is_running() const {
    return running;
}

uint32_t Scheduler::get_tick_count() const {
    return tick_count;
}

uint32_t Scheduler::get_frequency_hz() const {
    if (interval_us == 0) return 0;
    return 1000000 / interval_us;
}

void Scheduler::register_callback(void (*callback)(void*), void* user_data) {
    user_callback = callback;
    user_callback_data = user_data;
}

// =============================================================================
// ISR Handler - MUST BE FAST, NO PRINTF!
// =============================================================================
void Scheduler::handle_isr() {
    tick_count++;
    
    // Toggle debug LED every 10000 calls (0.5 sec @ 20kHz) 
    static uint32_t led_count = 0;
    if ((++led_count % 10000) == 0) {
        static bool led_state = false;
        led_state = !led_state;
        gpio_put(SCHED_HEARTBEAT_PIN, led_state);
    }
    
    // Process traverse move queue
    if (move_queue) {
        move_queue->traverse_isr_handler();
    }
    
    // Call user callback if registered
    if (user_callback) {
        user_callback(user_callback_data);
    }
}
