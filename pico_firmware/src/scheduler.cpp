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

// Global pointer to scheduler instance for static callback
static Scheduler* g_scheduler_instance = nullptr;

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
    gpio_init(SCHED_HEARTBEAT_PIN);
    gpio_set_dir(SCHED_HEARTBEAT_PIN, GPIO_OUT);
    gpio_put(SCHED_HEARTBEAT_PIN, 0);
    
    interval_us = 50; // 20kHz = 50us period
    
    // Claim hardware alarm 0
    hardware_alarm_claim(0);
    
    // Set up callback
    hardware_alarm_set_callback(0, [](uint alarm_num) {
        // Clear interrupt
        hw_clear_bits(&timer_hw->intr, 1u << alarm_num);
        // Schedule next (absolute time prevents drift)
        timer_hw->alarm[alarm_num] = timer_hw->timerawl + 50;
        // Call ISR
        if (g_scheduler_instance) {
            g_scheduler_instance->handle_isr();
        }
    });
    
    // Start first interrupt
    timer_hw->alarm[0] = timer_hw->timerawl + 50;
    hw_set_bits(&timer_hw->inte, 1u << 0);
    irq_set_enabled(TIMER_IRQ_0, true);
    
    running = true;
    printf("Hardware timer started at 20kHz\n");
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
    // check_endstops(move_queue);  // TODO: Implement endstop checking
    // Encoder now updated by Core 1 (see main.cpp core1_entry)
    // Removed: spindle_encoder->update() to reduce Core 0 ISR load
    
    // Process move queues for both axes
    if (move_queue) {
        move_queue->handle_isr_tick();
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
