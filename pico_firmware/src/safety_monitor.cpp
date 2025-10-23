// =============================================================================
// safety_monitor.cpp - Comprehensive Safety System
// Purpose: Endstop monitoring, emergency stop, and stall detection
// =============================================================================

#include "safety_monitor.h"
#include "config.h"
#include "pico/stdlib.h"
#include <cstdio>

// Global instance for ISR access
static SafetyMonitor* g_safety_instance = nullptr;

// =============================================================================
// Constructor
// =============================================================================
SafetyMonitor::SafetyMonitor(MoveQueue* mq, WindingController* wc, BLDC_MOTOR* spindle)
    : move_queue(mq)
    , winding_controller(wc)
    , spindle_motor(spindle)
    , emergency_stop_triggered(false)
    , safety_enabled(true)
    , endstop_check_enabled(true)
    , stall_detection_enabled(true)
    , stall_threshold_us(1000000)  // 1 second
    , emergency_stop_button_pressed(false) {
    
    // Initialize step timing
    for (int i = 0; i < 2; i++) {
        last_step_time[i] = 0;
        last_step_count[i] = 0;
    }
    
    g_safety_instance = this;
}

// =============================================================================
// Initialize Safety System
// =============================================================================
bool SafetyMonitor::init() {
    printf("\n🛡️  Initializing Safety Monitor\n");
    printf("================================\n");
    
    // Initialize endstop pins (active low, with pull-up)
    gpio_init(Y_MIN_ENDSTOP_PIN);
    gpio_set_dir(Y_MIN_ENDSTOP_PIN, GPIO_IN);
    gpio_pull_up(Y_MIN_ENDSTOP_PIN);
    printf("✓ Y-MIN endstop: GPIO%d\n", Y_MIN_ENDSTOP_PIN);
    
    gpio_init(Y_MAX_ENDSTOP_PIN);
    gpio_set_dir(Y_MAX_ENDSTOP_PIN, GPIO_IN);
    gpio_pull_up(Y_MAX_ENDSTOP_PIN);
    printf("✓ Y-MAX endstop: GPIO%d\n", Y_MAX_ENDSTOP_PIN);
    
    // Initialize emergency stop button (active low, with pull-up)
    gpio_init(EMERGENCY_STOP_PIN);
    gpio_set_dir(EMERGENCY_STOP_PIN, GPIO_IN);
    gpio_pull_up(EMERGENCY_STOP_PIN);
    printf("✓ Emergency stop button: GPIO%d\n", EMERGENCY_STOP_PIN);
    
    // Set up interrupt for emergency stop button (falling edge)
    gpio_set_irq_enabled_with_callback(
        EMERGENCY_STOP_PIN,
        GPIO_IRQ_EDGE_FALL,
        true,
        emergency_stop_irq_handler
    );
    printf("✓ Emergency stop interrupt enabled\n");
    
    // Initialize safety status LED
    gpio_init(SAFETY_LED_PIN);
    gpio_set_dir(SAFETY_LED_PIN, GPIO_OUT);
    gpio_put(SAFETY_LED_PIN, 1);  // LED on = system safe
    printf("✓ Safety LED: GPIO%d\n", SAFETY_LED_PIN);
    
    printf("✓ Safety system initialized\n");
    printf("================================\n\n");
    
    return true;
}

// =============================================================================
// Emergency Stop IRQ Handler (button press)
// =============================================================================
void SafetyMonitor::emergency_stop_irq_handler(uint gpio, uint32_t events) {
    if (g_safety_instance && gpio == EMERGENCY_STOP_PIN) {
        g_safety_instance->emergency_stop_button_pressed = true;
        // Trigger immediate stop in next check_safety() call
    }
}

// =============================================================================
// Check Safety (called from ISR at high frequency)
// =============================================================================
void __time_critical_func(SafetyMonitor::check_safety)() {
    if (!safety_enabled) {
        return;
    }
    
    // Check emergency stop button
    if (emergency_stop_button_pressed) {
        trigger_emergency_stop(ESTOP_BUTTON);
        return;
    }
    
    // Check endstops if enabled
    if (endstop_check_enabled) {
        check_endstops();
    }
    
    // Check for stalls if enabled
    if (stall_detection_enabled) {
        check_stall_detection();
    }
}

// =============================================================================
// Check Endstops
// =============================================================================
void __time_critical_func(SafetyMonitor::check_endstops)() {
    // Check Y-MIN endstop (active low)
    if (!gpio_get(Y_MIN_ENDSTOP_PIN)) {
        if (!endstop_triggered[ENDSTOP_Y_MIN]) {
            endstop_triggered[ENDSTOP_Y_MIN] = true;
            trigger_emergency_stop(ESTOP_ENDSTOP_Y_MIN);
        }
    } else {
        endstop_triggered[ENDSTOP_Y_MIN] = false;
    }
    
    // Check Y-MAX endstop (active low)
    if (!gpio_get(Y_MAX_ENDSTOP_PIN)) {
        if (!endstop_triggered[ENDSTOP_Y_MAX]) {
            endstop_triggered[ENDSTOP_Y_MAX] = true;
            trigger_emergency_stop(ESTOP_ENDSTOP_Y_MAX);
        }
    } else {
        endstop_triggered[ENDSTOP_Y_MAX] = false;
    }
}

// =============================================================================
// Check Stall Detection
// =============================================================================
void __time_critical_func(SafetyMonitor::check_stall_detection)() {
    if (!move_queue) return;
    
    uint64_t now = time_us_64();
    
    // Check each axis for stalls
    for (uint8_t axis = 0; axis < 2; axis++) {
        // Get current step count
        uint32_t current_steps = move_queue->get_step_count(axis);
        
        // If steps changed, update timestamp
        if (current_steps != last_step_count[axis]) {
            last_step_time[axis] = now;
            last_step_count[axis] = current_steps;
        }
        
        // Check if axis is active but not stepping
        bool axis_active = move_queue->is_active(axis);
        bool queue_has_data = move_queue->has_chunk(axis);
        uint64_t time_since_step = now - last_step_time[axis];
        
        if (axis_active && queue_has_data && time_since_step > stall_threshold_us) {
            // Stall detected
            const char* axis_name = (axis == AXIS_TRAVERSE) ? "TRAVERSE" : "SPINDLE";
            printf("\n⚠️  STALL DETECTED on %s axis\n", axis_name);
            printf("  Time since last step: %llu ms\n", time_since_step / 1000);
            printf("  Queue has data: %s\n", queue_has_data ? "YES" : "NO");
            printf("  Axis active: %s\n", axis_active ? "YES" : "NO");
            
            trigger_emergency_stop(ESTOP_STALL_DETECTED);
        }
    }
}

// =============================================================================
// Trigger Emergency Stop
// =============================================================================
void SafetyMonitor::trigger_emergency_stop(EmergencyStopReason reason) {
    // Prevent repeated triggers
    if (emergency_stop_triggered) {
        return;
    }
    
    emergency_stop_triggered = true;
    emergency_stop_reason = reason;
    
    // Turn off safety LED
    gpio_put(SAFETY_LED_PIN, 0);
    
    // Print emergency stop message
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   🛑 EMERGENCY STOP TRIGGERED 🛑      ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("Reason: %s\n", get_emergency_stop_reason_string(reason));
    printf("\n");
    
    // Immediately disable all steppers
    if (move_queue) {
        move_queue->set_enable(AXIS_TRAVERSE, false);
        move_queue->set_enable(AXIS_SPINDLE, false);
        
        // Clear all queued moves
        move_queue->clear_queue(AXIS_TRAVERSE);
        move_queue->clear_queue(AXIS_SPINDLE);
        
        printf("✓ Steppers disabled and queues cleared\n");
    }
    
    // Stop spindle motor
    if (spindle_motor) {
        spindle_motor->disable();
        printf("✓ Spindle motor disabled\n");
    }
    
    // Disable winding controller
    if (winding_controller) {
        winding_controller->disable_all();
        printf("✓ Winding controller disabled\n");
    }
    
    printf("\n⚠️  System halted. Reset required to resume.\n");
    printf("════════════════════════════════════════\n\n");
}

// =============================================================================
// Get Emergency Stop Reason String
// =============================================================================
const char* SafetyMonitor::get_emergency_stop_reason_string(EmergencyStopReason reason) {
    switch (reason) {
        case ESTOP_BUTTON:           return "Emergency Stop Button Pressed";
        case ESTOP_ENDSTOP_Y_MIN:    return "Y-MIN Endstop Triggered";
        case ESTOP_ENDSTOP_Y_MAX:    return "Y-MAX Endstop Triggered";
        case ESTOP_STALL_DETECTED:   return "Motor Stall Detected";
        case ESTOP_QUEUE_OVERRUN:    return "Move Queue Overrun";
        case ESTOP_POSITION_ERROR:   return "Position Error Exceeded Limit";
        case ESTOP_COMMUNICATION:    return "Communication Timeout";
        default:                     return "Unknown Reason";
    }
}

// =============================================================================
// Check Endstop State
// =============================================================================
bool SafetyMonitor::is_endstop_triggered(EndstopType endstop) const {
    switch (endstop) {
        case ENDSTOP_Y_MIN:
            return !gpio_get(Y_MIN_ENDSTOP_PIN);
        case ENDSTOP_Y_MAX:
            return !gpio_get(Y_MAX_ENDSTOP_PIN);
        default:
            return false;
    }
}

// =============================================================================
// Reset Emergency Stop
// =============================================================================
bool SafetyMonitor::reset_emergency_stop() {
    if (!emergency_stop_triggered) {
        printf("⚠️  No emergency stop active\n");
        return true;
    }
    
    printf("\n🔄 Resetting Emergency Stop\n");
    printf("============================\n");
    
    // Check if it's safe to reset
    if (emergency_stop_button_pressed) {
        printf("❌ Cannot reset: Emergency stop button still pressed\n");
        return false;
    }
    
    if (is_endstop_triggered(ENDSTOP_Y_MIN)) {
        printf("❌ Cannot reset: Y-MIN endstop still triggered\n");
        return false;
    }
    
    if (is_endstop_triggered(ENDSTOP_Y_MAX)) {
        printf("❌ Cannot reset: Y-MAX endstop still triggered\n");
        return false;
    }
    
    // Clear emergency stop
    emergency_stop_triggered = false;
    emergency_stop_button_pressed = false;
    
    // Turn on safety LED
    gpio_put(SAFETY_LED_PIN, 1);
    
    printf("✓ Emergency stop cleared\n");
    printf("✓ System ready for operation\n");
    printf("============================\n\n");
    
    return true;
}

// =============================================================================
// Enable/Disable Safety Features
// =============================================================================
void SafetyMonitor::enable_safety(bool enable) {
    safety_enabled = enable;
    printf("%s safety monitoring\n", enable ? "✓ Enabled" : "⚠️  Disabled");
}

void SafetyMonitor::enable_endstop_check(bool enable) {
    endstop_check_enabled = enable;
    printf("%s endstop checking\n", enable ? "✓ Enabled" : "⚠️  Disabled");
}

void SafetyMonitor::enable_stall_detection(bool enable) {
    stall_detection_enabled = enable;
    printf("%s stall detection\n", enable ? "✓ Enabled" : "⚠️  Disabled");
}

// =============================================================================
// Set Stall Threshold
// =============================================================================
void SafetyMonitor::set_stall_threshold_ms(uint32_t threshold_ms) {
    stall_threshold_us = (uint64_t)threshold_ms * 1000;
    printf("✓ Stall threshold set to %u ms\n", threshold_ms);
}

// =============================================================================
// Get Safety Status
// =============================================================================
bool SafetyMonitor::is_safe() const {
    return !emergency_stop_triggered;
}

bool SafetyMonitor::is_emergency_stop_triggered() const {
    return emergency_stop_triggered;
}

EmergencyStopReason SafetyMonitor::get_emergency_stop_reason() const {
    return emergency_stop_reason;
}

// =============================================================================
// Print Safety Diagnostics
// =============================================================================
void SafetyMonitor::print_diagnostics() {
    printf("\n=== Safety Monitor Diagnostics ===\n");
    printf("Status:             %s\n", is_safe() ? "✓ SAFE" : "🛑 EMERGENCY STOP");
    
    if (emergency_stop_triggered) {
        printf("E-Stop Reason:      %s\n", 
               get_emergency_stop_reason_string(emergency_stop_reason));
    }
    
    printf("Safety Enabled:     %s\n", safety_enabled ? "YES" : "NO");
    printf("Endstop Check:      %s\n", endstop_check_enabled ? "YES" : "NO");
    printf("Stall Detection:    %s\n", stall_detection_enabled ? "YES" : "NO");
    printf("Stall Threshold:    %llu ms\n", stall_threshold_us / 1000);
    printf("\n");
    
    printf("Endstop States:\n");
    printf("  Y-MIN (GPIO%d):   %s\n", Y_MIN_ENDSTOP_PIN,
           is_endstop_triggered(ENDSTOP_Y_MIN) ? "🔴 TRIGGERED" : "✓ OK");
    printf("  Y-MAX (GPIO%d):   %s\n", Y_MAX_ENDSTOP_PIN,
           is_endstop_triggered(ENDSTOP_Y_MAX) ? "🔴 TRIGGERED" : "✓ OK");
    printf("\n");
    
    printf("Button States:\n");
    printf("  E-Stop (GPIO%d):  %s\n", EMERGENCY_STOP_PIN,
           emergency_stop_button_pressed ? "🔴 PRESSED" : "✓ OK");
    printf("\n");
    
    printf("Axis Status:\n");
    if (move_queue) {
        for (uint8_t axis = 0; axis < 2; axis++) {
            const char* name = (axis == AXIS_TRAVERSE) ? "TRAVERSE" : "SPINDLE";
            uint64_t time_since_step = time_us_64() - last_step_time[axis];
            
            printf("  %s:\n", name);
            printf("    Active:         %s\n", 
                   move_queue->is_active(axis) ? "YES" : "NO");
            printf("    Has Chunks:     %s\n",
                   move_queue->has_chunk(axis) ? "YES" : "NO");
            printf("    Last Step:      %llu ms ago\n", time_since_step / 1000);
            printf("    Total Steps:    %u\n", last_step_count[axis]);
        }
    } else {
        printf("  Move queue not initialized\n");
    }
    
    printf("===================================\n\n");
}

// =============================================================================
// Test Safety System
// =============================================================================
void SafetyMonitor::test_safety_system() {
    printf("\n========================================\n");
    printf("Testing Safety System\n");
    printf("========================================\n\n");
    
    printf("Test 1: Endstop Detection\n");
    printf("----------------------------------\n");
    printf("Y-MIN: %s\n", is_endstop_triggered(ENDSTOP_Y_MIN) ? "TRIGGERED" : "OK");
    printf("Y-MAX: %s\n", is_endstop_triggered(ENDSTOP_Y_MAX) ? "TRIGGERED" : "OK");
    
    printf("\nTest 2: Emergency Stop Button\n");
    printf("----------------------------------\n");
    printf("Please press emergency stop button...\n");
    sleep_ms(3000);
    printf("Button state: %s\n", emergency_stop_button_pressed ? "PRESSED" : "OK");
    
    printf("\nTest 3: Stall Detection\n");
    printf("----------------------------------\n");
    printf("Stall threshold: %llu ms\n", stall_threshold_us / 1000);
    printf("Detection: %s\n", stall_detection_enabled ? "ENABLED" : "DISABLED");
    
    printf("\n========================================\n");
    printf("Safety System Test Complete\n");
    printf("========================================\n\n");
}
