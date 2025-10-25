// =============================================================================
// config.h - Hardware Configuration
// =============================================================================

#pragma once

#include "pico/stdlib.h"

// =============================================================================
// Spindle (BLDC Motor) Configuration
// =============================================================================
#define SPINDLE_HALL_PIN 22
#define SPINDLE_PWM_PIN 24
#define SPINDLE_ENA_PIN 21
#define SPINDLE_FORWARD_PIN 3
#define SPINDLE_REVERSE_PIN 4
#define SPINDLE_BRAKE_PIN 4

// =============================================================================
// Traverse (Stepper Motor) Configuration  
// =============================================================================
#define TRAVERSE_STEP_PIN 6
#define TRAVERSE_DIR_PIN 5
#define TRAVERSE_ENA_PIN 7

// =============================================================================
// TMC2209 UART Configuration
// =============================================================================
#define TMC2209_TX_PIN 8
#define TMC2209_RX_PIN 9
#define TMC2209_ADDRESS 0

// =============================================================================
// Home Switch Configuration
// =============================================================================
#define Y_HOME_SWITCH_PIN 16

// =============================================================================
// ⭐ NEW: Emergency Stop Configuration
// =============================================================================
#define E_STOP_PIN 20  // Change to your actual emergency stop button pin

// =============================================================================
// ⭐ NEW: Soft Limits Configuration
// =============================================================================
#define TRAVERSE_MIN_MM 0.0f
#define TRAVERSE_MAX_MM 200.0f
#define TRAVERSE_STEPS_PER_MM 80.0f  // Adjust for your lead screw
#define TRAVERSE_MIN_STEPS 0
#define TRAVERSE_MAX_STEPS ((int32_t)(TRAVERSE_MAX_MM * TRAVERSE_STEPS_PER_MM))

// =============================================================================
// ⭐ NEW: System State Definitions
// =============================================================================
enum SystemState {
    STATE_IDLE = 0,
    STATE_HOMING = 1,
    STATE_WINDING = 2,
    STATE_HOLD = 3,
    STATE_ALARM = 4
};

// =============================================================================
// Scheduler Configuration
// =============================================================================
#define HEARTBEAT_US 1000  // 1ms = 1kHz
#define HEARTBEAT_ENABLE 1
#define SCHED_HEARTBEAT_PIN 25

// =============================================================================
// Move Queue Configuration
// =============================================================================
#define MOVE_CHUNKS_CAPACITY 16
#define NUM_AXES 2
#define AXIS_SPINDLE 0
#define AXIS_TRAVERSE 1

// =============================================================================
// Stepper Timing
// =============================================================================
#define STEP_PULSE_US 2

// =============================================================================
// Serial Configuration
// =============================================================================
#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUD_RATE 115200

// =============================================================================
// Traverse Motion Parameters
// =============================================================================
#define TRAVERSE_RAPID_SPEED 1000.0f  // mm/min
#define TRAVERSE_RAPID_ACCEL 500.0f   // mm/s²
