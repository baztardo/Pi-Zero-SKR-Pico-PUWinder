// ============================================================================
// SKR-Pico v1.0 Verified Pin Assignments
// Verified by Steve (hardware tested) – October 2025
// ============================================================================
// Notes:
//  - All pins are 3.3V logic only.
//  - MOSFET outputs (IO17,18,20,21,23) are low-side switches, not logic pins.
// ============================================================================

#pragma once
// ============================================================================
// UART0 (pins 0 TX, 1 RX) 
// ============================================================================
// #define UART0_TX_PIN  0  // I2C SDA
// #define UART0_RX_PIN  1  // I2C SCL

// ---------------------------------------------------------------------------
// Stepper Drivers
// ---------------------------------------------------------------------------

// X Axis
#define X_EN_PIN        12  // 
#define X_STEP_PIN      11  // 
#define X_DIR_PIN       10  // 

// Y Axis
#define Y_EN_PIN        7  // Traverse enable
#define Y_STEP_PIN      6  // Traverse step
#define Y_DIR_PIN       5  // Traverse direction

// Z Axis
#define Z_EN_PIN        2   // not used
#define Z_STEP_PIN      19  // not used
#define Z_DIR_PIN       28  // not used

// E0 Axis (aux driver)
#define E0_EN_PIN       15  // not used
#define E0_STEP_PIN     14  // not used
#define E0_DIR_PIN      13  // not used

// ---------------------------------------------------------------------------
// TMC2209 UART (shared bus)
// ---------------------------------------------------------------------------
#define TMC_UART_TX_PIN 8     // RP2040 TX → Drivers
#define TMC_UART_RX_PIN 9     // RP2040 RX ← Drivers
#define TMC_UART_BAUD   115200

// ---------------------------------------------------------------------------
// Endstops 
// ---------------------------------------------------------------------------
#define X_STOP_PIN      3   // 
#define Y_STOP_PIN      4   // 
#define Z_STOP_PIN      25  // 
#define E0_STOP_PIN     16  // home switch

// ---------------------------------------------------------------------------
// ADC Inputs (Temperature or analog sensors)
// ---------------------------------------------------------------------------
#define THB_PIN         26
#define TH0_PIN         27

// ---------------------------------------------------------------------------
// General IO
// ---------------------------------------------------------------------------
#define PROBE_PIN       22    // Digital input (safe)
#define SERVO_PIN       29    // PWM capable (3.3V logic)
#define RGB_PIN         24    // WS2812 or digital output
#define Laser           20    // can not use fan3 and laser
// ---------------------------------------------------------------------------
// MOSFET-controlled Outputs (NOT logic-safe)
// ---------------------------------------------------------------------------
// These pins sink ground through a mossfet – cannot source 3.3V logic
// Can be used for the LED indicator and also switched power 12/24
#define FAN1_PIN        17
#define FAN2_PIN        18
#define FAN3_PIN        20
#define HE_PIN          23
#define HB_PIN          21

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------
#define BOARD_NAME      "BTT SKR-Pico v1.0"
#define MCU_NAME        "RP2040"
#define FIRMWARE_NAME   "PU-Winder"


// ---------------------------------------------------------------------------
// # Mechanical Parameters
// ---------------------------------------------------------------------------

stepper_motor:
  steps_per_rev: 200
  microsteps: 16
  total_steps_per_rev: 3200

lead_screw:
  pitch_mm: 2
  threads_per_mm: 0.5

# Calibrated Values
calibration:
  test_steps: 10000
  test_distance_mm: 1.63
  steps_per_mm: 6135
  theoretical_steps_per_mm: 1600
  ratio: 3.83

# Homing Parameters
homing:
  phase1_max_distance_mm: 81
  phase1_speed_mm_per_sec: 20
  phase2_backoff_mm: 8
  phase2_speed_mm_per_sec: 2
  phase3_target_mm: 22
  phase3_speed_mm_per_sec: 10