// =============================================================================
// main.cpp - Emergency Stop Setup (ADD THESE SECTIONS)
// =============================================================================

// ⭐ ADD THESE GLOBAL VARIABLES AT TOP OF FILE:

// Emergency stop and system state
volatile bool g_emergency_stop = false;
volatile SystemState g_system_state = STATE_IDLE;

// ⭐ ADD THIS ISR HANDLER FUNCTION:

/**
 * @brief Emergency stop interrupt handler
 * Triggered by falling edge on E_STOP_PIN
 */
void emergency_stop_isr() {
    g_emergency_stop = true;
    g_system_state = STATE_ALARM;
    
    // Optional: Set LED or alarm output here
    gpio_put(SCHED_HEARTBEAT_PIN, 1);  // Turn on LED to indicate alarm
    
    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║   🚨 EMERGENCY STOP TRIGGERED! 🚨    ║\n");
    printf("║                                       ║\n");
    printf("║  All motion STOPPED immediately!      ║\n");
    printf("║  Send M999 to reset                   ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("\n");
}

// ⭐ ADD THIS RESET FUNCTION:

/**
 * @brief Reset from emergency stop
 */
void reset_emergency_stop() {
    g_emergency_stop = false;
    g_system_state = STATE_IDLE;
    
    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║      ✅ SYSTEM RESET COMPLETE         ║\n");
    printf("║                                       ║\n");
    printf("║  Emergency stop cleared                ║\n");
    printf("║  System ready                          ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("\n");
}

// ⭐ ADD TO main() FUNCTION AFTER GPIO INITIALIZATION:

int main() {
    // ... existing initialization code ...
    
    stdio_init_all();
    printf("\n========================================\n");
    printf("Pico Winding Controller with Safety\n");
    printf("========================================\n");
    
    // ... existing component initialization ...
    
    // ⭐ NEW: Setup emergency stop button
    printf("\n[SAFETY] Configuring emergency stop...\n");
    
    gpio_init(E_STOP_PIN);
    gpio_set_dir(E_STOP_PIN, GPIO_IN);
    gpio_pull_up(E_STOP_PIN);  // Normally pulled high, triggers on LOW
    
    // Attach interrupt handler
    gpio_set_irq_enabled_with_callback(
        E_STOP_PIN, 
        GPIO_IRQ_EDGE_FALL,  // Trigger on falling edge (button press)
        true, 
        &emergency_stop_isr
    );
    
    printf("[SAFETY] Emergency stop configured on pin %d\n", E_STOP_PIN);
    printf("[SAFETY] System state: IDLE\n");
    
    // ... rest of main() ...
}

// ⭐ OPTIONAL: Add status reporting in main loop
// Add this to your main loop to show system state:

while (true) {
    // ... existing code ...
    
    // Show status on LED
    if (g_emergency_stop) {
        // Blink fast when in alarm
        static uint32_t last_blink = 0;
        if ((time_us_32() - last_blink) > 100000) {  // 100ms
            gpio_put(SCHED_HEARTBEAT_PIN, !gpio_get(SCHED_HEARTBEAT_PIN));
            last_blink = time_us_32();
        }
    }
    
    // ... rest of loop ...
}
