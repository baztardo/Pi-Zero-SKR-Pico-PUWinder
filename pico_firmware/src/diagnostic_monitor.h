// =============================================================================
// diagnostic_monitor.h - Real-time Diagnostics for MoveQueue
// Purpose: Monitor queue health from main loop without ISR printf overhead
// =============================================================================

#pragma once

#include "move_queue.h"
#include "pico/stdlib.h"
#include <cstdio>

class DiagnosticMonitor {
public:
    DiagnosticMonitor(MoveQueue* mq) 
        : move_queue(mq)
        , last_print_time(0)
        , last_depth(0)
        , last_active(false)
        , queue_full_count(0)
        , queue_empty_count(0)
    {}
    
    /**
     * @brief Update monitor - call frequently from main loop
     * Prints diagnostics every interval_ms
     */
    void update(uint32_t interval_ms = 1000) {
        uint32_t now = time_us_32();
        
        if ((now - last_print_time) < (interval_ms * 1000)) {
            return;  // Not time yet
        }
        
        last_print_time = now;
        
        // Get current state
        uint32_t depth = move_queue->get_queue_depth();
        bool active = move_queue->is_active();
        bool paused = move_queue->is_feeding_paused();
        bool estop = move_queue->is_emergency_stopped();
        
        // Track statistics
        if (depth == 127) queue_full_count++;
        if (depth == 0) queue_empty_count++;
        
        // Detect state changes
        bool depth_changed = (depth != last_depth);
        bool active_changed = (active != last_active);
        
        // Print status line
        printf("[Monitor] Depth:%3u/%d Active:%s Paused:%s E-Stop:%s",
               depth, MOVE_CHUNKS_CAPACITY,
               active ? "Y" : "N",
               paused ? "⚠" : "N",
               estop ? "🛑" : "N");
        
        // Add warnings
        if (depth == 127) {
            printf(" ⚠️ QUEUE FULL!");
        }
        if (depth > 100 && !active) {
            printf(" ⚠️ QUEUE HIGH BUT NOT ACTIVE!");
        }
        if (paused) {
            printf(" ⚠️ FEED HOLD!");
        }
        if (estop) {
            printf(" 🛑 EMERGENCY STOP!");
        }
        
        printf("\n");
        
        // Save state
        last_depth = depth;
        last_active = active;
    }
    
    /**
     * @brief Print full diagnostics
     */
    void print_full_diagnostics() {
        printf("\n");
        printf("═══════════════════════════════════════════\n");
        printf("  DIAGNOSTIC MONITOR - STATISTICS\n");
        printf("═══════════════════════════════════════════\n");
        printf("Queue Full Count:   %lu\n", queue_full_count);
        printf("Queue Empty Count:  %lu\n", queue_empty_count);
        printf("═══════════════════════════════════════════\n");
        
        // Print MoveQueue internal diagnostics
        move_queue->print_diagnostics();
    }
    
    /**
     * @brief Reset statistics
     */
    void reset() {
        queue_full_count = 0;
        queue_empty_count = 0;
        move_queue->reset_diagnostics();
    }
    
private:
    MoveQueue* move_queue;
    uint32_t last_print_time;
    uint32_t last_depth;
    bool last_active;
    uint32_t queue_full_count;
    uint32_t queue_empty_count;
};
