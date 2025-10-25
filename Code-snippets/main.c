// main.c - Pickup Winder Controller: Spindle + Traverse
#include "pico/stdlib.h"
#include "spindle.h"
#include "traverse.h"
#include <stdio.h>
#include <string.h>

// Pin definitions - adjust for your SKR Pico
#define SPINDLE_PWM_PIN     16
#define SPINDLE_DIR_PIN     17
#define SPINDLE_BRAKE_PIN   18
#define SPINDLE_HALL_PIN    19

#define TRAVERSE_STEP_PIN   20
#define TRAVERSE_DIR_PIN    21
#define TRAVERSE_ENABLE_PIN 22

spindle_t spindle;
traverse_t traverse;

// Speed map for spindle (FluidNC-style)
speed_point_t speed_map[] = {
    {0,     0},    // Stopped
    {0,     10},   // Minimum duty to start
    {100,   20},   // Slow for tension
    {500,   60},   // Medium speed
    {1000,  100}   // Maximum speed
};

// Hall sensor interrupt
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == SPINDLE_HALL_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        spindle_hall_callback(&spindle);
    }
}

// Command parser
// Spindle: CW/CCW/STOP/BRAKE/TURNS?/RESET
// Traverse: TRAV/TREL/TSTOP/TPOS?/TZERO/TENABLE
void process_command(char* cmd) {
    // Spindle commands
    if (strncmp(cmd, "CW", 2) == 0) {
        uint32_t rpm = atoi(cmd + 3);
        spindle_set_state(&spindle, SPINDLE_CW, rpm);
        printf("OK CW %lu\n", rpm);
        
    } else if (strncmp(cmd, "CCW", 3) == 0) {
        uint32_t rpm = atoi(cmd + 4);
        spindle_set_state(&spindle, SPINDLE_CCW, rpm);
        printf("OK CCW %lu\n", rpm);
        
    } else if (strcmp(cmd, "STOP") == 0) {
        spindle_stop(&spindle);
        printf("OK STOP\n");
        
    } else if (strcmp(cmd, "BRAKE") == 0) {
        spindle_brake(&spindle);
        printf("OK BRAKE\n");
        
    } else if (strcmp(cmd, "TURNS?") == 0) {
        printf("TURNS %lu\n", spindle_get_turns(&spindle));
        
    } else if (strcmp(cmd, "RESET") == 0) {
        spindle_reset_turns(&spindle);
        printf("OK RESET\n");
    
    // Traverse commands    
    } else if (strncmp(cmd, "TRAV", 4) == 0) {
        float pos_mm = atof(cmd + 5);
        traverse_move_abs(&traverse, pos_mm);
        printf("OK TRAV %.2f\n", pos_mm);
        
    } else if (strncmp(cmd, "TREL", 4) == 0) {
        float dist_mm = atof(cmd + 5);
        traverse_move_rel(&traverse, dist_mm);
        printf("OK TREL %.2f\n", dist_mm);
        
    } else if (strcmp(cmd, "TSTOP") == 0) {
        traverse_stop(&traverse);
        printf("OK TSTOP\n");
        
    } else if (strcmp(cmd, "TPOS?") == 0) {
        printf("TPOS %.3f\n", traverse_get_position_mm(&traverse));
        
    } else if (strcmp(cmd, "TZERO") == 0) {
        traverse_set_zero(&traverse);
        printf("OK TZERO\n");
        
    } else if (strncmp(cmd, "TENABLE", 7) == 0) {
        bool enable = (cmd[8] == '1');
        traverse_enable(&traverse, enable);
        printf("OK TENABLE %d\n", enable);
        
    } else {
        printf("ERR Unknown: %s\n", cmd);
    }
}

int main() {
    stdio_init_all();
    
    // Init spindle
    spindle_init(&spindle, SPINDLE_PWM_PIN, SPINDLE_DIR_PIN, 
                 SPINDLE_BRAKE_PIN, SPINDLE_HALL_PIN);
    spindle_set_speed_map(&spindle, speed_map, 5);
    spindle.spinup_ms = 2000;
    spindle.spindown_ms = 3000;
    
    // Init traverse
    traverse_init(&traverse, TRAVERSE_STEP_PIN, TRAVERSE_DIR_PIN, 
                  TRAVERSE_ENABLE_PIN);
    traverse.steps_per_mm = 100.0;
    traverse.max_rate_mm_per_min = 3000.0;
    traverse.acceleration_mm_per_sec2 = 500.0;
    traverse.max_travel_mm = 50.0;
    
    // Setup hall interrupt
    gpio_set_irq_enabled_with_callback(SPINDLE_HALL_PIN, GPIO_IRQ_EDGE_FALL, 
                                       true, &gpio_callback);
    
    printf("READY\n");
    
    // Command loop
    char buf[64];
    int idx = 0;
    
    while (1) {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    buf[idx] = 0;
                    process_command(buf);
                    idx = 0;
                }
            } else if (idx < 63) {
                buf[idx++] = c;
            }
        }
        tight_loop_contents();
    }
    
    return 0;
}
