/**
 * @file pio_stepper.h
 * @brief Hardware-accelerated stepper control with GPIO handoff capability
 * 
 * This module allows:
 * 1. PIO hardware stepping during winding (100kHz+ capable)
 * 2. Regular GPIO stepping during homing (TraverseController)
 * 3. Clean handoff between the two modes
 */

#ifndef PIO_STEPPER_H
#define PIO_STEPPER_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"

class PIOStepper {
public:
    /**
     * @brief Constructor - does NOT claim GPIO yet!
     * @param step_pin GPIO pin for STEP output
     * @param dir_pin GPIO pin for DIR output (regular GPIO, not PIO)
     */
    PIOStepper(uint step_pin, uint dir_pin);
    
    /**
     * @brief Destructor - releases all resources
     */
    ~PIOStepper();
    
    /**
     * @brief Activate PIO mode (take control of STEP pin)
     * Call this BEFORE winding starts
     */
    void activate();
    
    /**
     * @brief Deactivate PIO mode (release STEP pin back to GPIO)
     * Call this AFTER winding completes, BEFORE homing
     */
    void deactivate();
    
    /**
     * @brief Check if PIO is currently active
     */
    bool is_active() const { return pio_active; }
    
    /**
     * @brief Set direction (works in both modes)
     * @param forward true for forward, false for reverse
     */
    void set_direction(bool forward);
    
    /**
     * @brief Queue a step with specified interval
     * Only works when PIO is active!
     * @param interval_us Microseconds to wait before pulse
     * @return true if queued, false if FIFO full or not active
     */
    bool queue_step(uint32_t interval_us);
    
    /**
     * @brief Get available FIFO space
     * @return Number of steps that can be queued (0-8)
     */
    uint32_t get_fifo_level();
    
    /**
     * @brief Check if can queue another step
     */
    bool can_queue_step();
    
    /**
     * @brief Emergency stop - clear FIFO and stop immediately
     */
    void emergency_stop();
    
    /**
     * @brief Check if PIO is currently running (FIFO not empty)
     */
    bool is_running() const;
    
    /**
     * @brief Get total steps queued since activation
     */
    uint64_t get_steps_queued() const { return steps_queued; }
    
    /**
     * @brief Reset step counter
     */
    void reset_step_counter() { steps_queued = 0; }

private:
    PIO pio;                // PIO instance (PIO0 or PIO1)
    uint sm;                // State machine number (0-3)
    uint offset;            // Program offset in PIO memory
    uint step_pin;          // STEP pin GPIO
    uint dir_pin;           // DIR pin GPIO (regular GPIO)
    bool pio_active;        // Is PIO currently controlling the pin?
    uint64_t steps_queued;  // Total steps queued to PIO
};

#endif // PIO_STEPPER_H
