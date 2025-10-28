// =============================================================================
// move_queue.h - MCU Move Queue & Step Consumer (FIXED)
// Purpose: Fixed-size move queue with ISR-driven step execution for traverse axis
// Key changes: Added diagnostic methods, removed ISR printf
// =============================================================================

#pragma once

#include "stepcompress.h"
#include "config.h"
#include <cstdint>

// =============================================================================
// MoveQueue Class - Traverse Stepper Only
// =============================================================================
class MoveQueue {
public:
    /**
     * @brief Constructor
     */
    MoveQueue();
    
    /**
     * @brief Initialize GPIO pins for traverse stepper control
     */
    void init();
    
    /**
     * @brief Push a step chunk to traverse queue
     * @param chunk Step chunk to queue
     * @return true if successful, false if queue full
     */
    bool push_chunk(const StepChunk& chunk);
    
    /**
     * @brief Pop a chunk from traverse queue
     * @param out Output chunk
     * @return true if successful, false if queue empty
     */
    bool pop_chunk(StepChunk& out);
    
    /**
     * @brief Check if traverse has chunks in queue
     * @return true if queue has chunks
     */
    bool has_chunk();
    
    /**
     * @brief Clear all chunks from traverse queue
     */
    void clear_queue();
    
    /**
     * @brief ISR handler for traverse stepping
     * Call this from ISR at high frequency (e.g., 20 kHz)
     * NO PRINTF ALLOWED IN THIS FUNCTION
     */
    void traverse_isr_handler();

    /**
     * @brief Set traverse direction pin state
     * @param forward true for forward, false for reverse
     */
    void set_direction(bool forward);
    
    /**
     * @brief Enable/disable traverse motor
     * @param enable true to enable motor
     */
    void set_enable(bool enable);
    
    /**
     * @brief Check if traverse is currently executing steps
     * @return true if actively stepping
     */
    bool is_active();
    
    /**
     * @brief Get total steps executed on traverse since init
     * @return Total step count
     */
    int32_t get_step_count();

    /**
     * @brief Get current queue depth
     * @return Number of chunks in queue
     */ 
    uint32_t get_queue_depth() const;
    
    // =========================================================================
    // FluidNC-style Safety and Feed Control Methods
    // =========================================================================
    
    /**
     * @brief Pause accepting new moves (feed hold)
     */
    void pause_feeding();
    
    /**
     * @brief Resume accepting new moves
     */
    void resume_feeding();
    
    /**
     * @brief Check if feeding is paused
     * @return true if paused
     */
    bool is_feeding_paused() const;
    
    /**
     * @brief Emergency stop all movement
     */
    void emergency_stop();
    
    /**
     * @brief Check if emergency stop is active
     * @return true if emergency stop is active
     */
    bool is_emergency_stopped() const;
    
    // =========================================================================
    // NEW: Diagnostic Methods - Call from main loop, NOT from ISR
    // =========================================================================
    
    /**
     * @brief Print comprehensive diagnostics
     * Shows ISR stats, queue state, safety flags, and active chunk info
     * MUST be called from main loop, NOT from ISR
     */
    void print_diagnostics();
    
    /**
     * @brief Reset diagnostic counters
     * Useful for measuring performance over specific intervals
     */
    void reset_diagnostics();

private:
    StepChunk queue[128];  // MOVE_CHUNKS_CAPACITY
    volatile uint16_t head;
    volatile uint16_t tail;
    
    StepChunk active;
    bool active_running;
    uint32_t last_step_time;
    int32_t step_count;
    
    // FluidNC-style safety and feed control
    bool feeding_paused;
    bool emergency_stop_active;
    
    /**
     * @brief Execute a step pulse on traverse step pin
     * Called from ISR - must be fast
     */
    void execute_step_pulse();
};
