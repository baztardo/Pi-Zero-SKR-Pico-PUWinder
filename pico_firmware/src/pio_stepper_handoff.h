/**
 * @file pio_stepper_handoff.h
 * @brief PIO stepper with proper GPIO handoff for homing/winding coexistence
 * 
 * Key feature: Can release/reclaim GPIO to allow TraverseController direct access
 */

#ifndef PIO_STEPPER_HANDOFF_H
#define PIO_STEPPER_HANDOFF_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"

class PIOStepperHandoff {
public:
    PIOStepperHandoff(uint step_pin, uint dir_pin);
    ~PIOStepperHandoff();
    
    // PIO control - call these to activate/deactivate PIO
    void claim_gpio();      // Take control of STEP pin from GPIO
    void release_gpio();    // Release STEP pin back to GPIO control
    bool is_pio_active();   // Check if PIO owns the pin
    
    // Stepping interface (only works when PIO is active)
    void set_direction(bool forward);
    bool queue_step(uint32_t interval_us);
    uint32_t get_fifo_level();
    bool can_queue_step();
    void emergency_stop();
    
    uint64_t get_steps_queued() const { return steps_queued; }

private:
    PIO pio;
    uint sm;
    uint offset;
    uint step_pin;
    uint dir_pin;
    bool pio_owns_pin;      // Track who owns the STEP pin
    bool enabled;
    uint64_t steps_queued;
    
    static constexpr float PIO_CLOCK_DIV = 125.0f;
};

#endif // PIO_STEPPER_HANDOFF_H

