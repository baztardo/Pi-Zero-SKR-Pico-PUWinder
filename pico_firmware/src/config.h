// =============================================================================
// config.h - Hardware Configuration and Constants
// Purpose: Central configuration for all hardware pins and system parameters
// =============================================================================

#pragma once

#include <cstdint>

// NOTE: Version info moved to src/version.h

// =============================================================================
// PIN DEFINITIONS (SKR Pico)
// =============================================================================

// UART config (Pi communication)
#define PI_UART_ID uart0
#define PI_UART_TX 1
#define PI_UART_RX 0
#define PI_UART_BAUD 115200

// BLDC PWM config
#define SPINDLE_PWM_PIN 24
#define SPINDLE_ENABLE_PIN 21    // Enable/disable power to the spindle
#define SPINDLE_HALL_A_PIN 22    // Hall sensor A (3-phase BLDC)
#define SPINDLE_HALL_B_PIN 23    // Hall sensor B (3-phase BLDC)
#define SPINDLE_HALL_C_PIN 25    // Hall sensor C (3-phase BLDC)
#define SPINDLE_DIR_PIN       3  // Set LOW to change direction
#define SPINDLE_BRAKE_PIN     4  // Set HIGH to brake

// Traverse Stepper Motor
#define TRAVERSE_STEP_PIN   6
#define TRAVERSE_DIR_PIN    5
#define TRAVERSE_ENA_PIN    7
#define TRAVERSE_HOME_PIN   16
#define TRAVERSE_DIR_INVERT  0   // set 1 if traverse moves the wrong way
#define TRAVERSE_CURRENT_MA 250     // Traverse motor RMS current (mA)
// Traverse Lead Screw
#define TRAVERSE_PITCH_MM      5.0f    // Lead screw pitch (mm per revolution)n
#define MIN_TRAVERSE_POS_MM     0.0f    // Minimum traverse position
#define HOMING_SPEED_MM_PER_SEC 5.0f    // Homing speed

// Traverse parameters
#define Y_STEPS_PER_MM          80.0
#define Y_MAX_ACCEL             100.0   // mm/s²
#define Y_MAX_VELOCITY          200.0   // mm/s
#define Y_MAX_POSITION_MM       200.0   // Soft limit (calculated)
#define Y_MIN_POSITION_MM       0.0     // Home position

// Home switch (your one endstop)
#define Y_HOME_SWITCH_PIN       16

// Soft limits enabled
#define USE_SOFT_LIMITS         1

// =============================================================================
// TMC2209 UART (Shared bus)
// =============================================================================

// TMC2209 UART (Shared bus)
#define TMC_UART_TX_PIN     8
#define TMC_UART_RX_PIN     9
#define TMC_UART_BAUD       115200
#define HOLD_CURRENT_PERCENT 30     // 30% of run current when holding
#define POWER_DOWN_DELAY     20      // Delay before reducing to hold current (x 0.1s)
#define TRAVERSE_MICROSTEPS 16      // 16x for traverse (PRECISION - slower but accurate)


// TMC2209 Microstepping values:
// 0 = Full step (1x)
// 1 = Half step (2x)
// 2 = 4x    ← Spindle ACTUALLY uses this (SKR Pico hardware config)
// 3 = 8x
// 4 = 16x   ← Traverse uses this
// 5 = 32x
// 6 = 64x
// 7 = 128x
// 8 = 256x

// =============================================================================
// WINDING PARAMETERS (Easy configuration!)
// =============================================================================
#define WINDING_TARGET_TURNS    1000    // Total turns to wind
#define WINDING_SPINDLE_RPM     120.0f  // **SPINDLE** speed (RPM) - Start conservative!
#define WINDING_WIRE_DIA_MM     0.064f  // Wire diameter (mm) - 43 AWG
#define WINDING_WIDTH_MM        50.0f   // Winding width (mm)
#define WINDING_OFFSET_MM       0.0f    // Offset from center of winding (mm)
#define WINDING_START_POS_MM    20.0f   // Start position from home (mm)
#define WINDING_RAMP_TIME_SEC   10.0f   // Ramp up/down time (was 5s - too fast!)

// =============================================================================
// MOTION PARAMETERS (Defaults)
// =============================================================================
#define DEFAULT_MAX_VELOCITY    1000.0  // steps/sec
#define DEFAULT_ACCELERATION    2000.0  // steps/sec²
#define DEFAULT_JERK            5000.0  // steps/sec³ (future use)

// Traverse Motion Speeds
#define TRAVERSE_HOMING_SPEED   1500    // steps/sec for homing
#define TRAVERSE_RAPID_SPEED    3000    // steps/sec for rapid moves
#define TRAVERSE_RAPID_ACCEL    5000    // steps/sec² for rapid moves
#define TRAVERSE_MIN_WINDING_SPEED 1000 // Minimum speed during winding (steps/sec)

// =============================================================================
// SYSTEM CONSTANTS
// =============================================================================
#define NUM_AXES                2       // Number of axes (spindle + traverse)
#define AXIS_SPINDLE           0       // Spindle axis index
#define AXIS_TRAVERSE          1       // Traverse axis index

// Move queue configuration
#define MOVE_CHUNKS_CAPACITY    128     // Maximum chunks per axis
#define STEP_PULSE_US          2       // Step pulse width (microseconds)

// Spindle stepper pins (if using stepper instead of BLDC)
#define SPINDLE_STEP_PIN       11      // Spindle step pin
#define SPINDLE_ENA_PIN        12      // Spindle enable pin

// TMC2209 configuration
#define R_SENSE                 0.11f   // Sense resistor value (ohms)

// Debug pin
#define DEBUG_PIN              25      // Debug LED pin

// Scheduler configuration
#define HEARTBEAT_US           100     // Scheduler heartbeat interval (microseconds)
#define SCHED_HEARTBEAT_PIN    25      // Scheduler heartbeat LED pin

// =============================================================================
// BLDC MOTOR CONFIGURATION
// =============================================================================

// BLDC Motor Direction
#define BLDC_DIRECTION_CW      1       // Clockwise direction
#define BLDC_DIRECTION_CCW     0       // Counter-clockwise direction

// BLDC Motor Default Settings
#define BLDC_DEFAULT_PPR       36      // Default pulses per revolution (3-phase BLDC: 6 states × 3 phases × 2 edges = 36 edges per electrical revolution)
#define BLDC_DEBOUNCE_US       100     // Debounce time in microseconds
#define BLDC_SMOOTH_ALPHA      0.1f    // Default smoothing factor (0.1 = 10% new, 90% old)

// BLDC Motor Timing
#define BLDC_RPM_CALC_INTERVAL 1000000 // RPM calculation interval (1 second in microseconds)
#define BLDC_MIN_PULSE_DT_US   100     // Minimum pulse interval (debounce)

// BLDC Motor Status
#define BLDC_STATUS_READY      0
#define BLDC_STATUS_RUNNING    1
#define BLDC_STATUS_STOPPED    2
#define BLDC_STATUS_ERROR      3

