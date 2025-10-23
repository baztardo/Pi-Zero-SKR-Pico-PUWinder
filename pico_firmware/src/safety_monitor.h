// =============================================================================
// safety_monitor.h - Safety System Header
// =============================================================================

#pragma once

#include "move_queue.h"
#include "winding_controller.h"
#include "spindle.h"
#include <cstdint>

// Emergency stop reasons
enum EmergencyStopReason {
    ESTOP_BUTTON,
    ESTOP_ENDSTOP_Y_MIN,
    ESTOP_ENDSTOP_Y_MAX,
    ESTOP_STALL_DETECTED,
    ESTOP_QUEUE_OVERRUN,
    ESTOP_POSITION_ERROR,
    ESTOP_COMMUNICATION
};

// Endstop types
enum EndstopType {
    ENDSTOP_Y_MIN,
    ENDSTOP_Y_MAX
};

// =============================================================================
// SafetyMonitor Class
// =============================================================================
class SafetyMonitor {
public:
    /**
     * @brief Constructor
     */
    SafetyMonitor(MoveQueue* move_queue, 
                  WindingController* winding_controller,
                  BLDC_MOTOR* spindle_motor);
    
    /**
     * @brief Initialize safety system
     */
    bool init();
    
    /**
     * @brief Check safety (called from ISR)
     */
    void check_safety();
    
    /**
     * @brief Trigger emergency stop
     */
    void trigger_emergency_stop(EmergencyStopReason reason);
    
    /**
     * @brief Reset emergency stop
     */
    bool reset_emergency_stop();
    
    /**
     * @brief Check if endstop is triggered
     */
    bool is_endstop_triggered(EndstopType endstop) const;
    
    /**
     * @brief Check if system is safe
     */
    bool is_safe() const;
    
    /**
     * @brief Check if emergency stop is triggered
     */
    bool is_emergency_stop_triggered() const;
    
    /**
     * @brief Get emergency stop reason
     */
    EmergencyStopReason get_emergency_stop_reason() const;
    
    /**
     * @brief Enable/disable safety features
     */
    void enable_safety(bool enable);
    void enable_endstop_check(bool enable);
    void enable_stall_detection(bool enable);
    
    /**
     * @brief Set stall threshold
     */
    void set_stall_threshold_ms(uint32_t threshold_ms);
    
    /**
     * @brief Print diagnostics
     */
    void print_diagnostics();
    
    /**
     * @brief Test safety system
     */
    void test_safety_system();
    
private:
    MoveQueue* move_queue;
    WindingController* winding_controller;
    BLDC_MOTOR* spindle_motor;
    
    volatile bool emergency_stop_triggered;
    EmergencyStopReason emergency_stop_reason;
    
    bool safety_enabled;
    bool endstop_check_enabled;
    bool stall_detection_enabled;
    
    uint64_t stall_threshold_us;
    uint64_t last_step_time[2];  // spindle, traverse
    uint32_t last_step_count[2];
    
    bool endstop_triggered[2];  // Y_MIN, Y_MAX
    volatile bool emergency_stop_button_pressed;
    
    void check_endstops();
    void check_stall_detection();
    
    static void emergency_stop_irq_handler(uint gpio, uint32_t events);
    const char* get_emergency_stop_reason_string(EmergencyStopReason reason);
};
