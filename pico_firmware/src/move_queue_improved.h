// =============================================================================
// move_queue_improved.h - Klipper-Style Improved Move Queue
// Purpose: Priority-based move queue with efficient processing
// =============================================================================

#pragma once

#include "stepcompress.h"
#include "config.h"
#include <cstdint>
#include <queue>

// =============================================================================
// Move Priority Levels
// =============================================================================
enum MovePriority {
    PRIORITY_EMERGENCY = 0,   // Emergency stop
    PRIORITY_HIGH = 1,        // High priority moves
    PRIORITY_NORMAL = 2,      // Normal moves
    PRIORITY_LOW = 3          // Low priority moves
};

// =============================================================================
// Enhanced Step Chunk with Priority
// =============================================================================
struct StepChunkPriority {
    StepChunk chunk;
    MovePriority priority;
    uint32_t timestamp;
    uint32_t deadline_us;
    bool is_urgent;
};

// =============================================================================
// Improved MoveQueue Class (Klipper-style)
// =============================================================================
class MoveQueueImproved {
public:
    /**
     * @brief Constructor
     */
    MoveQueueImproved();
    
    /**
     * @brief Destructor
     */
    ~MoveQueueImproved();
    
    /**
     * @brief Initialize GPIO pins for stepper control
     */
    void init();
    
    /**
     * @brief Push a step chunk with priority
     * @param axis Axis identifier
     * @param chunk Step chunk to queue
     * @param priority Priority level
     * @param deadline_us Deadline in microseconds
     * @return true if successful
     */
    bool push_chunk_priority(
        uint8_t axis, 
        const StepChunk& chunk, 
        MovePriority priority,
        uint32_t deadline_us = 0);
    
    /**
     * @brief Pop highest priority chunk
     * @param axis Axis identifier
     * @param out Output chunk
     * @return true if successful
     */
    bool pop_chunk_priority(uint8_t axis, StepChunkPriority& out);
    
    /**
     * @brief Check if axis has chunks in queue
     * @param axis Axis identifier
     * @return true if queue has chunks
     */
    bool has_chunk(uint8_t axis) const;
    
    /**
     * @brief Get queue depth for axis
     * @param axis Axis identifier
     * @return Number of chunks in queue
     */
    uint32_t get_queue_depth(uint8_t axis) const;
    
    /**
     * @brief Clear all chunks from an axis queue
     * @param axis Axis identifier
     */
    void clear_queue(uint8_t axis);
    
    /**
     * @brief Set direction pin state
     * @param axis Axis identifier
     * @param forward true for forward, false for reverse
     */
    void set_direction(uint8_t axis, bool forward);
    
    /**
     * @brief Set enable pin state
     * @param axis Axis identifier
     * @param enable true to enable, false to disable
     */
    void set_enable(uint8_t axis, bool enable);
    
    /**
     * @brief Check if axis is active
     * @param axis Axis identifier
     * @return true if active
     */
    bool is_active(uint8_t axis) const;
    
    /**
     * @brief Get step count for axis
     * @param axis Axis identifier
     * @return Number of steps executed
     */
    int32_t get_step_count(uint8_t axis) const;
    
    /**
     * @brief Priority-based ISR handler
     * @param axis Axis identifier
     */
    void axis_isr_handler_priority(uint8_t axis);
    
    /**
     * @brief Priority-based ISR tick handler
     */
    void handle_isr_tick_priority();
    
    /**
     * @brief Process urgent moves first
     */
    void process_urgent_moves();
    
    /**
     * @brief Get queue statistics
     * @param axis Axis identifier
     * @return Statistics structure
     */
    struct QueueStats {
        uint32_t total_chunks;
        uint32_t urgent_chunks;
        uint32_t high_priority_chunks;
        uint32_t normal_priority_chunks;
        uint32_t low_priority_chunks;
        uint32_t max_queue_depth;
        uint32_t average_queue_depth;
    };
    
    QueueStats get_queue_stats(uint8_t axis) const;

private:
    // Priority queues for each axis
    std::priority_queue<StepChunkPriority> priority_queues[NUM_AXES];
    
    // Queue statistics
    uint32_t queue_depths[NUM_AXES];
    uint32_t max_queue_depths[NUM_AXES];
    uint32_t total_chunks_processed[NUM_AXES];
    
    // Active chunk tracking
    StepChunkPriority active_chunks[NUM_AXES];
    bool active_running[NUM_AXES];
    uint32_t last_step_time[NUM_AXES];
    int32_t step_count[NUM_AXES];
    
    // GPIO pin states
    bool direction_states[NUM_AXES];
    bool enable_states[NUM_AXES];
    
    /**
     * @brief Execute step pulse
     * @param step_pin GPIO pin for step pulse
     */
    void execute_step_pulse(uint32_t step_pin);
    
    /**
     * @brief Update queue statistics
     * @param axis Axis identifier
     */
    void update_queue_stats(uint8_t axis);
    
    /**
     * @brief Check for deadline violations
     * @param axis Axis identifier
     * @return true if deadline violated
     */
    bool check_deadline_violation(uint8_t axis) const;
};
