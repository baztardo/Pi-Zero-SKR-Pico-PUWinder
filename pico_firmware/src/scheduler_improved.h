// =============================================================================
// scheduler_improved.h - Klipper-Style Improved Scheduler
// Purpose: Hardware timer ISR with precise timing
// =============================================================================

#pragma once

#include "move_queue.h"
#include "config.h"
#include <cstdint>

// =============================================================================
// Hardware Timer ISR Scheduler (Klipper-style)
// =============================================================================
class SchedulerImproved {
public:
    /**
     * @brief Constructor
     * @param move_queue Pointer to move queue
     */
    SchedulerImproved(MoveQueue* move_queue);
    
    /**
     * @brief Destructor
     */
    ~SchedulerImproved();
    
    /**
     * @brief Start hardware timer ISR
     * @param frequency_hz ISR frequency in Hz
     * @return true if successful
     */
    bool start_hardware_timer(uint32_t frequency_hz);
    
    /**
     * @brief Stop hardware timer ISR
     */
    void stop_hardware_timer();
    
    /**
     * @brief Check if ISR is running
     * @return true if running
     */
    bool is_running() const;
    
    /**
     * @brief Get ISR tick count
     * @return Number of ISR ticks
     */
    uint32_t get_tick_count() const;
    
    /**
     * @brief Get ISR frequency
     * @return Frequency in Hz
     */
    uint32_t get_frequency_hz() const;
    
    /**
     * @brief Register user callback
     * @param callback Function to call from ISR
     * @param user_data User data pointer
     */
    void register_callback(void (*callback)(void*), void* user_data);
    
    /**
     * @brief Hardware timer ISR handler
     */
    static void hardware_timer_isr();
    
    /**
     * @brief Get scheduler instance for ISR
     * @return Pointer to scheduler instance
     */
    static SchedulerImproved* get_instance();

private:
    MoveQueue* move_queue;
    uint32_t tick_count;
    uint32_t frequency_hz;
    bool running;
    void (*user_callback)(void*);
    void* user_callback_data;
    
    // Hardware timer configuration
    uint32_t timer_alarm_num;
    uint32_t timer_period_us;
    
    // Static instance for ISR access
    static SchedulerImproved* instance;
    
    /**
     * @brief Configure hardware timer
     * @param frequency_hz Desired frequency
     * @return true if successful
     */
    bool configure_hardware_timer(uint32_t frequency_hz);
    
    /**
     * @brief Handle ISR tick
     */
    void handle_isr_tick();
};
