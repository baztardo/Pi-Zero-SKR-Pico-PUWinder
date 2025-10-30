// =============================================================================
// winding_controller.h - Winding Process State Machine (FIXED)
// Purpose: High-level winding sequence control and coordination
// =============================================================================

#pragma once

#include "move_queue.h"
#include "stepcompress.h"
#include "spindle.h"
#include <cstdint>

// =============================================================================
// Winding States
// =============================================================================
enum class WindingState {
    IDLE,
    RAMPING_UP,
    WINDING,
    RAMPING_DOWN,
    COMPLETE,
    ERROR
};

// =============================================================================
// Winding Parameters
// =============================================================================
struct WindingParams {
    uint32_t target_turns;
    float spindle_rpm;
    float wire_diameter_mm;
    float layer_width_mm;
    float start_position_mm;
    float ramp_time_sec;
    
    float wire_pitch_mm;
    uint32_t turns_per_layer;
    uint32_t total_layers;
    
    WindingParams()
        : target_turns(WINDING_TARGET_TURNS)
        , spindle_rpm(WINDING_SPINDLE_RPM)
        , wire_diameter_mm(WINDING_WIRE_DIA_MM)
        , layer_width_mm(WINDING_WIDTH_MM)        // From config.h
        , start_position_mm(TC_start_offset)      // From config.h (traverse start offset)
        , ramp_time_sec(WINDING_RAMP_TIME_SEC)
        , wire_pitch_mm(WINDING_WIRE_DIA_MM)
        , turns_per_layer(0)
        , total_layers(0) {
        calculate_layers();
    }
    
    void calculate_layers() {
        wire_pitch_mm = wire_diameter_mm;
        turns_per_layer = (uint32_t)(layer_width_mm / wire_pitch_mm);
        if (turns_per_layer == 0) turns_per_layer = 1;
        total_layers = (target_turns + turns_per_layer - 1) / turns_per_layer;
    }
};

// =============================================================================
// WindingController Class (FIXED)
// =============================================================================
class WindingController {
public:
    WindingController(MoveQueue* mq, BLDC_MOTOR* spindle_motor);

    void init();
    void set_parameters(const WindingParams& params);
    bool start();
    void stop();
    void update();
    WindingState get_state() const { return state; }
    uint32_t get_current_layer() const { return current_layer; }
    uint32_t get_turns_completed() const { return turns_completed; }
    float get_current_rpm() const { return current_rpm; }
    void emergency_stop();
    void reset();  // Reset controller to IDLE state for new windings

    void home_all_axes();
    void adjust_traverse_speed();
    void print_winding_metrics();

private:
    MoveQueue* move_queue;
    BLDC_MOTOR* spindle_motor;
    
    WindingState state;
    WindingParams params;
    
    uint32_t current_layer;
    uint32_t turns_completed;
    uint32_t turns_this_layer;
    float current_rpm;
    uint32_t last_rpm_update_time;
    
    bool traverse_direction;
    float current_traverse_position_mm;
    
    bool ramp_started;
    uint32_t ramp_start_time;
    
    double turn_accum;
    int8_t encoder_sign;
    double traverse_steps_emitted;
    int32_t enc_last_sync;
    int32_t enc_last_rpm;
    
    void ramp_up_spindle();
    void execute_winding();
    void ramp_down_spindle();
    void update_rpm();
    void sync_traverse_to_spindle();
    void update_display();
    
    uint32_t mm_to_steps(float mm);
    float steps_to_mm(uint32_t steps);
};
