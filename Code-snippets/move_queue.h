// =============================================================================
// move_queue.h - MCU Move Queue & Step Consumer
// Purpose: Fixed-size move queue with ISR-driven step execution
// =============================================================================

#pragma once

#include "stepcompress.h"
#include "config.h"
#include <cstdint>

// =============================================================================
// MoveQueue Class
// =============================================================================
class MoveQueue {
public:
    /**
     * @brief Constructor
     */
    MoveQueue();
    
    /**
     * @brief Initialize GPIO pins for stepper control
     */
    void init();
    
    /**
     * @brief Push a step chunk to an axis queue
     * @param axis Axis identifier (AXIS_SPINDLE or AXIS_TRAVERSE)
     * @param chunk Step chunk to queue
     * @return true if successful, false if queue full
     */
    bool push_chunk(uint8_t axis, const StepChunk& chunk);
    
    /**
     * @brief Pop a chunk from an axis queue
     * @param axis Axis identifier
     * @param out Output chunk
     * @return true if successful, false if queue empty
     */
    bool pop_chunk(uint8_t axis, StepChunk& out);
    
    /**
     * @brief Check if axis has chunks in queue
     * @param axis Axis identifier
     * @return true if queue has chunks
     */
    bool has_chunk(uint8_t axis);
    
    /**
     * @brief Clear all chunks from an axis queue
     * @param axis Axis identifier
     */
    void clear_queue(uint8_t axis);
    
    /**
     * @brief ISR handler for axis stepping
     * Call this from ISR at high frequency (e.g., 10 kHz)
     * @param axis Axis identifier
     */
    void axis_isr_handler(uint8_t axis);
    
    /**
     * @brief Get current queue depth
     * @param axis Axis identifier
     * @return Number of chunks in queue
     */
    void handle_isr_tick();

    /**
     * @brief Set direction pin state
     * @param axis Axis identifier
     * @param forward true for forward, false for reverse
     */
    void set_direction(uint8_t axis, bool forward);
    
    /**
     * @brief Enable/disable motor
     * @param axis Axis identifier
     * @param enable true to enable, false to disable
     */
    void set_enable(uint8_t axis, bool enable);
    
    /**
     * @brief Check if axis is actively processing a chunk
     * @param axis Axis identifier
     * @return true if chunk is running
     */
    bool is_active(uint8_t axis);
    
    /**
     * @brief Get current step count for axis
     * @param axis Axis identifier
     * @return Total step count
     */
    int32_t get_step_count(uint8_t axis);
    
    // =========================================================================
    // ⭐ NEW: Safety and Feed Control Methods
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
    bool is_feeding_paused() const { return feeding_paused; }

private:
    // Queue storage
    StepChunk queues[NUM_AXES][MOVE_CHUNKS_CAPACITY];
    volatile uint8_t head[NUM_AXES];
    volatile uint8_t tail[NUM_AXES];
    
    // Active chunk tracking
    StepChunk active[NUM_AXES];
    bool active_running[NUM_AXES];
    uint32_t last_step_time[NUM_AXES];
    int32_t step_count[NUM_AXES];
    
    // ⭐ NEW: Feed control
    bool feeding_paused;
    
    /**
     * @brief Execute step pulse on pin
     * @param step_pin GPIO pin number
     */
    void execute_step_pulse(uint32_t step_pin);
};
