// =============================================================================
// scheduler.cpp - Hardware Timer ISR Scheduler Implementation
// =============================================================================

#include "scheduler.h"
#include "pico/stdlib.h"
#include <cstdio>
#include "config.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

// Forward declaration of the stepper pulse handler
void scheduler_tick();

// Global scheduler instance for callback
static Scheduler* g_scheduler_instance = nullptr;

// Alarm callback function
void scheduler_alarm_callback(uint alarm_num) {
    // Clear interrupt
    hw_clear_bits(&timer_hw->intr, 1u << alarm_num);
    // Schedule next (absolute time prevents drift)
    timer_hw->alarm[alarm_num] = timer_hw->timerawl + 50;
    // Call ISR
    if (g_scheduler_instance) {
        g_scheduler_instance->handle_isr();
    }
}

struct StepperState {
    uint step_pin;
    uint32_t interval_us;
    int32_t add_us;
    uint32_t remaining;
    absolute_time_t next_pulse_time;
    bool active;
};

static StepperState steppers[4];  // X, Y, Z, E or spindle/traverse

// uint32_t Encoder::get_isr_hits() const { return g_isr_hits; }
// uint32_t Encoder::get_isr_hits() const { return isr_hits; }

void scheduler_queue_step(uint axis, uint32_t interval_us, int32_t add_us, uint32_t count) {
    if (axis >= 4) return;  // safety check
    auto &s = steppers[axis];

    s.interval_us = interval_us;
    s.add_us = add_us;
    s.remaining = count;
    s.next_pulse_time = make_timeout_time_us(interval_us);
    s.active = true;
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
    
    gpio_init(SCHED_HEARTBEAT_PIN);
    gpio_set_dir(SCHED_HEARTBEAT_PIN, GPIO_OUT);
    gpio_put(SCHED_HEARTBEAT_PIN, 0);
    
    // Initialize FAN1 LED for debug
    gpio_init(17);
    gpio_set_dir(17, GPIO_OUT);
    gpio_put(17, 0);
    printf("[Scheduler] FAN1 LED initialized on pin 17\n");
    
    interval_us = 50; // 20kHz = 50us period
    
    // Claim hardware alarm 0
    printf("[Scheduler] Claiming hardware alarm 0...\n");
    hardware_alarm_claim(0);
    
    // Set up callback
    printf("[Scheduler] Setting up alarm callback...\n");
    hardware_alarm_set_callback(0, scheduler_alarm_callback);
    
    // Start first interrupt using Pico SDK
    printf("[Scheduler] Starting first interrupt...\n");
    hardware_alarm_set_target(0, delayed_by_us(get_absolute_time(), 50));
    
    // Test if timer is working
    printf("[Scheduler] Timer setup complete. Testing...\n");
    sleep_ms(100);
    printf("[Scheduler] Timer test complete.\n");
    
    running = true;
    printf("[Scheduler] Hardware timer started at 20kHz\n");
    return true;
}

void Scheduler::stop() {
    if (!running) return;
    
    cancel_repeating_timer(&timer);
    running = false;
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

bool Scheduler::timer_callback(repeating_timer_t* rt) {
    if (g_scheduler_instance) {
        g_scheduler_instance->handle_isr();
    }
    return true;  // Keep repeating
}

void Scheduler::handle_isr() {
    tick_count++;
    
    // Debug: Toggle FAN1 LED to show ISR is running
    static uint32_t led_toggle_count = 0;
    if ((led_toggle_count++ % 10000) == 0) {  // Every 10k ISR calls (0.5 second at 20kHz)
        static bool led_state = false;
        led_state = !led_state;
        gpio_put(17, led_state);  // FAN1 pin 17
        
        // Also try the heartbeat pin
        gpio_put(SCHED_HEARTBEAT_PIN, led_state);
        
        // Force a printf to see if ISR is running
        printf("[ISR] Running! Count: %u\n", led_toggle_count);
    }
    
    // Debug: Show scheduler ISR running occasionally
    static uint32_t sched_debug_count = 0;
    if ((sched_debug_count++ % 20000) == 0) {  // Every 20k ISR calls (1 second at 20kHz)
        printf("[Scheduler] ISR #%u running, MoveQueue: %s\n", 
               sched_debug_count, move_queue ? "YES" : "NO");
    }
    
    // check_endstops(move_queue);  // TODO: Implement endstop checking
    // Encoder now updated by Core 1 (see main.cpp core1_entry)
    // Removed: spindle_encoder->update() to reduce Core 0 ISR load
    
    // Process traverse move queue
    if (move_queue) {
        move_queue->traverse_isr_handler();
    } else {
        static uint32_t no_movequeue_count = 0;
        if ((no_movequeue_count++ % 20000) == 0) {
            printf("[Scheduler] ERROR: MoveQueue is NULL!\n");
        }
    }
    
    // Debug: Confirm MoveQueue ISR is being called
    static uint32_t mq_call_count = 0;
    mq_call_count++;
    if ((mq_call_count % 20000) == 0) {
        printf("[Scheduler] MoveQueue ISR called #%u\n", mq_call_count);
    }
    
    // Call user callback if registered
    if (user_callback) {
        user_callback(user_callback_data);
    }
    // -----------------------------------------------------------------------------
    // Heartbeat LED toggle (guarded)
    // -----------------------------------------------------------------------------
    #if defined(HEARTBEAT_ENABLE) && (HEARTBEAT_ENABLE)
    static uint32_t last_toggle = 0;
    static bool led_state = false;
    if ((tick_count - last_toggle) >= 500) {   // toggle every ~0.5s
        led_state = !led_state;
        gpio_put(SCHED_HEARTBEAT_PIN, led_state);
        last_toggle = tick_count;
    }
    #endif
    // Legacy stepper path disabled; MoveQueue handles stepping
}

// This runs periodically to step active motors
void scheduler_tick() {
    absolute_time_t now = get_absolute_time();

    for (auto &s : steppers) {
        if (!s.active || s.remaining == 0)
            continue;

        if (absolute_time_diff_us(now, s.next_pulse_time) <= 0) {
            // Step pulse
            gpio_put(s.step_pin, 1);
            sleep_us(2);  // 2 µs pulse width
            gpio_put(s.step_pin, 0);

            s.remaining--;
            s.interval_us += s.add_us;
            s.next_pulse_time = delayed_by_us(s.next_pulse_time, s.interval_us);

            if (s.remaining == 0) {
                s.active = false;  // done
            }
        }
    }
}
