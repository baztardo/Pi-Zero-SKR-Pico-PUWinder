/**
 * @file pio_stepper.cpp
 * @brief Implementation of PIO stepper with GPIO handoff
 */

#include "pio_stepper.h"
#include "pio_stepper.pio.h"
#include <stdio.h>

PIOStepper::PIOStepper(uint step_pin, uint dir_pin)
    : step_pin(step_pin)
    , dir_pin(dir_pin)
    , pio_active(false)
    , steps_queued(0)
{
    // Choose PIO instance
    pio = pio0;
    
    // Claim a state machine (will panic if none available)
    sm = pio_claim_unused_sm(pio, true);
    
    // Load PIO program
    offset = pio_add_program(pio, &pio_stepper_program);
    
    // Initialize PIO program (but don't enable yet)
    pio_stepper_program_init(pio, sm, offset, step_pin);
    
    // Immediately disable it - we'll enable when winding starts
    pio_sm_set_enabled(pio, sm, false);
    
    // Initialize DIR pin as regular GPIO
    gpio_init(dir_pin);
    gpio_set_dir(dir_pin, GPIO_OUT);
    gpio_put(dir_pin, 0);
    
    printf("[PIOStepper] Initialized on PIO%d SM%d (INACTIVE)\n", 
           pio_get_index(pio), sm);
    printf("[PIOStepper] STEP=GPIO%d (currently GPIO mode), DIR=GPIO%d\n", 
           step_pin, dir_pin);
}

PIOStepper::~PIOStepper() {
    // Make sure we're deactivated
    if (pio_active) {
        deactivate();
    }
    
    // Remove program
    pio_remove_program(pio, &pio_stepper_program, offset);
    
    // Release state machine
    pio_sm_unclaim(pio, sm);
    
    printf("[PIOStepper] Cleaned up\n");
}

void PIOStepper::activate() {
    if (pio_active) {
        printf("[PIOStepper] Already active!\n");
        return;
    }
    
    printf("[PIOStepper] 🔄 Activating PIO mode (taking control of GPIO%d)\n", step_pin);
    
    // Reclaim GPIO for PIO
    pio_stepper_reclaim_gpio(pio, sm, step_pin);
    
    pio_active = true;
    steps_queued = 0;
    
    printf("[PIOStepper] ✓ PIO mode ACTIVE - ready for high-speed stepping\n");
}

void PIOStepper::deactivate() {
    if (!pio_active) {
        return;  // Already inactive
    }
    
    printf("[PIOStepper] 🔄 Deactivating PIO mode (releasing GPIO%d)\n", step_pin);
    
    // Release GPIO from PIO
    pio_stepper_release_gpio(pio, sm, step_pin);
    
    pio_active = false;
    
    printf("[PIOStepper] ✓ PIO mode INACTIVE - GPIO available for homing\n");
}

void PIOStepper::set_direction(bool forward) {
    gpio_put(dir_pin, forward ? 0 : 1);
}

bool PIOStepper::queue_step(uint32_t interval_us) {
    if (!pio_active) {
        return false;  // Not in PIO mode
    }
    
    // Check if FIFO has space
    if (pio_sm_is_tx_fifo_full(pio, sm)) {
        return false;
    }
    
    // Queue the step interval
    pio_sm_put(pio, sm, interval_us);
    
    steps_queued++;
    return true;
}

uint32_t PIOStepper::get_fifo_level() {
    if (!pio_active) {
        return 0;
    }
    
    // With FIFO join, we have 8 words depth
    // Return available space
    int level = pio_sm_get_tx_fifo_level(pio, sm);
    return 8 - level;
}

bool PIOStepper::can_queue_step() {
    if (!pio_active) {
        return false;
    }
    
    return !pio_sm_is_tx_fifo_full(pio, sm);
}

void PIOStepper::feed_step() {
    // For compatibility with ISR code - queue immediate step
    queue_step(0);
}

void PIOStepper::emergency_stop() {
    if (!pio_active) {
        return;
    }

    // Stop state machine
    pio_sm_set_enabled(pio, sm, false);

    // Clear FIFO
    pio_sm_clear_fifos(pio, sm);

    // Restart state machine
    pio_sm_restart(pio, sm);
    pio_sm_set_enabled(pio, sm, true);

    printf("[PIOStepper] Emergency stop - FIFO cleared\n");
}

bool PIOStepper::is_running() const {
    if (!pio_active) {
        return false;
    }
    
    // Check if FIFO has data or state machine is executing
    return !pio_sm_is_tx_fifo_empty(pio, sm);
}
