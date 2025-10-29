// =============================================================================
// scheduler.h - Hardware Timer ISR Scheduler (FIXED)
// Purpose: Manage high-frequency ISR for step timing
// =============================================================================

#pragma once

#include "pico/stdlib.h"
#include "move_queue.h"
#include <cstdint>

// Forward declarations
class MoveQueue;

// =============================================================================
// Scheduler Class
// =============================================================================
class Scheduler {
public:
    /**
     * @brief Constructor
     * @param move_queue Pointer to move queue instance
     */
    Scheduler(MoveQueue* move_queue);
    
    /**
     * @brief Initialize and start the hardware timer ISR
     * @param interval_us ISR interval in microseconds (default 50µs = 20kHz)
     * @return true if successful
     */
    bool start(uint32_t interval_us = 50);
    
    /**
     * @brief Stop the hardware timer ISR
     */
    void stop();
    
    /**
     * @brief Check if scheduler is running
     * @return true if ISR is active
     */
    bool is_running() const;
    
    /**
     * @brief Get ISR execution count
     * @return Number of times ISR has executed
     */
    uint32_t get_tick_count() const;
    
    /**
     * @brief Get ISR frequency in Hz
     * @return Frequency in Hz
     */
    uint32_t get_frequency_hz() const;
    
    /**
     * @brief Register a callback function to be called from ISR
     * @param callback Function pointer
     * @param user_data User data passed to callback
     */
    void register_callback(void (*callback)(void*), void* user_data);
    
    /**
     * @brief ISR handler - called from hardware alarm callback
     * MUST BE FAST - NO PRINTF ALLOWED
     */
    void handle_isr();

private:
    MoveQueue* move_queue;
    volatile uint32_t tick_count;
    uint32_t interval_us;
    bool running;
    
    void (*user_callback)(void*);
    void* user_callback_data;
};
