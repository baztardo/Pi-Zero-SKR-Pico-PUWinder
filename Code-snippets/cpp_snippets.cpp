/*
 * C++ Code Snippets for Pi Zero SKR Pico PUWinder
 * Useful code patterns and examples for Pico firmware development
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// =============================================================================
// Configuration Constants
// =============================================================================

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define SPINDLE_PWM_PIN 2
#define SPINDLE_DIR_PIN 3
#define SPINDLE_BRAKE_PIN 4
#define SPINDLE_HALL_PIN 5

#define TRAVERSE_STEP_PIN 6
#define TRAVERSE_DIR_PIN 7
#define TRAVERSE_ENA_PIN 8
#define TRAVERSE_HOME_PIN 9

#define MAX_RPM 3000.0f
#define PWM_FREQ 1000.0f
#define PWM_WRAP 65535

// =============================================================================
// UART Communication Snippets
// =============================================================================

void uart_init_snippet() {
    // Initialize UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    printf("UART initialized at %d baud\n", BAUD_RATE);
}

void uart_send_command(const char* command) {
    // Send command via UART
    uart_puts(UART_ID, command);
    uart_puts(UART_ID, "\n");
    printf("Sent: %s\n", command);
}

void uart_read_response(char* buffer, size_t buffer_size) {
    // Read response from UART
    size_t bytes_read = 0;
    char c;
    
    while (bytes_read < buffer_size - 1) {
        if (uart_is_readable(UART_ID)) {
            c = uart_getc(UART_ID);
            if (c == '\n' || c == '\r') {
                break;
            }
            buffer[bytes_read++] = c;
        }
    }
    buffer[bytes_read] = '\0';
}

// =============================================================================
// PWM Control Snippets
// =============================================================================

void pwm_init_snippet() {
    // Initialize PWM for spindle control
    gpio_set_function(SPINDLE_PWM_PIN, GPIO_FUNC_PWM);
    
    uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
    uint channel = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
    
    // Set PWM frequency
    pwm_set_clkdiv(slice_num, 125.0f / PWM_FREQ);
    pwm_set_wrap(slice_num, PWM_WRAP);
    pwm_set_chan_level(slice_num, channel, 0);
    pwm_set_enabled(slice_num, true);
    
    printf("PWM initialized on pin %d\n", SPINDLE_PWM_PIN);
}

void set_spindle_pwm(float duty_percent) {
    // Set spindle PWM duty cycle
    uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
    uint channel = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
    
    // Clamp duty cycle to 0-100%
    duty_percent = fmaxf(0.0f, fminf(100.0f, duty_percent));
    
    // Convert to PWM level
    uint16_t pwm_level = (uint16_t)((duty_percent / 100.0f) * PWM_WRAP);
    
    pwm_set_chan_level(slice_num, channel, pwm_level);
    printf("Set spindle PWM to %.1f%% (level: %d)\n", duty_percent, pwm_level);
}

// =============================================================================
// Spindle Control Snippets
// =============================================================================

typedef struct {
    float current_rpm;
    float target_rpm;
    bool is_running;
    bool direction;  // true = CW, false = CCW
    uint32_t pulse_count;
    uint32_t last_pulse_time;
} spindle_state_t;

static spindle_state_t spindle_state = {0};

void spindle_init() {
    // Initialize spindle control
    gpio_init(SPINDLE_DIR_PIN);
    gpio_set_dir(SPINDLE_DIR_PIN, GPIO_OUT);
    
    gpio_init(SPINDLE_BRAKE_PIN);
    gpio_set_dir(SPINDLE_BRAKE_PIN, GPIO_OUT);
    gpio_put(SPINDLE_BRAKE_PIN, 1);  // Brake on initially
    
    // Initialize hall sensor input
    gpio_init(SPINDLE_HALL_PIN);
    gpio_set_dir(SPINDLE_HALL_PIN, GPIO_IN);
    gpio_pull_up(SPINDLE_HALL_PIN);
    
    pwm_init_snippet();
    
    printf("Spindle controller initialized\n");
}

void set_spindle_rpm(float rpm, bool direction) {
    // Set spindle RPM and direction
    spindle_state.target_rpm = fmaxf(0.0f, fminf(MAX_RPM, rpm));
    spindle_state.direction = direction;
    
    if (spindle_state.target_rpm > 0) {
        // Set direction
        gpio_put(SPINDLE_DIR_PIN, direction);
        
        // Release brake
        gpio_put(SPINDLE_BRAKE_PIN, 0);
        
        // Calculate PWM duty cycle (simplified)
        float duty_percent = (spindle_state.target_rpm / MAX_RPM) * 100.0f;
        set_spindle_pwm(duty_percent);
        
        spindle_state.is_running = true;
        printf("Spindle set to %.1f RPM, direction: %s\n", 
               spindle_state.target_rpm, direction ? "CW" : "CCW");
    } else {
        stop_spindle();
    }
}

void stop_spindle() {
    // Stop spindle
    set_spindle_pwm(0.0f);
    gpio_put(SPINDLE_BRAKE_PIN, 1);  // Apply brake
    spindle_state.is_running = false;
    spindle_state.current_rpm = 0.0f;
    spindle_state.target_rpm = 0.0f;
    printf("Spindle stopped\n");
}

// =============================================================================
// Stepper Control Snippets
// =============================================================================

typedef struct {
    float current_position;
    float target_position;
    bool is_moving;
    float feed_rate;  // mm/min
    uint32_t step_count;
} stepper_state_t;

static stepper_state_t stepper_state = {0};

void stepper_init() {
    // Initialize stepper motor control
    gpio_init(TRAVERSE_STEP_PIN);
    gpio_set_dir(TRAVERSE_STEP_PIN, GPIO_OUT);
    
    gpio_init(TRAVERSE_DIR_PIN);
    gpio_set_dir(TRAVERSE_DIR_PIN, GPIO_OUT);
    
    gpio_init(TRAVERSE_ENA_PIN);
    gpio_set_dir(TRAVERSE_ENA_PIN, GPIO_OUT);
    gpio_put(TRAVERSE_ENA_PIN, 0);  // Enable stepper
    
    // Initialize home switch
    gpio_init(TRAVERSE_HOME_PIN);
    gpio_set_dir(TRAVERSE_HOME_PIN, GPIO_IN);
    gpio_pull_up(TRAVERSE_HOME_PIN);
    
    printf("Stepper controller initialized\n");
}

void stepper_step() {
    // Generate step pulse
    gpio_put(TRAVERSE_STEP_PIN, 1);
    sleep_us(10);  // Step pulse width
    gpio_put(TRAVERSE_STEP_PIN, 0);
    stepper_state.step_count++;
}

void stepper_move_to(float position, float feed_rate) {
    // Move stepper to position
    stepper_state.target_position = position;
    stepper_state.feed_rate = feed_rate;
    
    float distance = position - stepper_state.current_position;
    bool direction = distance > 0;
    
    gpio_put(TRAVERSE_DIR_PIN, direction);
    
    // Calculate number of steps (assuming 200 steps/mm)
    uint32_t steps = (uint32_t)fabsf(distance * 200.0f);
    
    // Calculate step delay based on feed rate
    uint32_t step_delay_us = (uint32_t)(60000000.0f / (feed_rate * 200.0f));
    
    printf("Moving %.2f mm at %.1f mm/min (%d steps)\n", 
           distance, feed_rate, steps);
    
    for (uint32_t i = 0; i < steps; i++) {
        stepper_step();
        sleep_us(step_delay_us);
    }
    
    stepper_state.current_position = position;
    stepper_state.is_moving = false;
}

bool stepper_home() {
    // Home stepper motor
    printf("Homing stepper...\n");
    
    // Move towards home switch
    gpio_put(TRAVERSE_DIR_PIN, 0);  // Move towards home
    
    while (!gpio_get(TRAVERSE_HOME_PIN)) {
        stepper_step();
        sleep_us(1000);  // Slow homing speed
    }
    
    stepper_state.current_position = 0.0f;
    stepper_state.target_position = 0.0f;
    stepper_state.step_count = 0;
    
    printf("Stepper homed\n");
    return true;
}

// =============================================================================
// G-code Processing Snippets
// =============================================================================

typedef struct {
    char command;
    int number;
    float x, y, z;
    float s, f;
    bool has_x, has_y, has_z, has_s, has_f;
} gcode_t;

bool parse_gcode(const char* line, gcode_t* gcode) {
    // Parse G-code line
    memset(gcode, 0, sizeof(gcode_t));
    
    // Skip whitespace
    while (*line == ' ' || *line == '\t') line++;
    
    // Parse command
    if (*line == 'G' || *line == 'M') {
        gcode->command = *line++;
        gcode->number = atoi(line);
        
        // Skip to parameters
        while (*line && *line != ' ') line++;
        
        // Parse parameters
        while (*line) {
            while (*line == ' ') line++;
            if (!*line) break;
            
            char param = *line++;
            float value = atof(line);
            
            switch (param) {
                case 'X': gcode->x = value; gcode->has_x = true; break;
                case 'Y': gcode->y = value; gcode->has_y = true; break;
                case 'Z': gcode->z = value; gcode->has_z = true; break;
                case 'S': gcode->s = value; gcode->has_s = true; break;
                case 'F': gcode->f = value; gcode->has_f = true; break;
            }
            
            // Skip to next parameter
            while (*line && *line != ' ') line++;
        }
        
        return true;
    }
    
    return false;
}

void execute_gcode(const gcode_t* gcode) {
    // Execute parsed G-code
    if (gcode->command == 'G') {
        switch (gcode->number) {
            case 0:
            case 1:  // Linear movement
                if (gcode->has_y) {
                    stepper_move_to(gcode->y, gcode->has_f ? gcode->f : 1000.0f);
                }
                break;
                
            case 28:  // Home all axes
                stepper_home();
                break;
        }
    }
    else if (gcode->command == 'M') {
        switch (gcode->number) {
            case 3:  // Spindle CW
                set_spindle_rpm(gcode->has_s ? gcode->s : 1000.0f, true);
                break;
                
            case 4:  // Spindle CCW
                set_spindle_rpm(gcode->has_s ? gcode->s : 1000.0f, false);
                break;
                
            case 5:  // Stop spindle
                stop_spindle();
                break;
        }
    }
}

// =============================================================================
// Interrupt Service Routines
// =============================================================================

void hall_sensor_isr(uint gpio, uint32_t events) {
    // Hall sensor interrupt for RPM measurement
    if (gpio == SPINDLE_HALL_PIN) {
        uint32_t current_time = time_us_32();
        uint32_t pulse_interval = current_time - spindle_state.last_pulse_time;
        
        if (pulse_interval > 1000) {  // Debounce
            spindle_state.pulse_count++;
            spindle_state.last_pulse_time = current_time;
            
            // Calculate RPM (assuming 1 pulse per revolution)
            if (pulse_interval > 0) {
                spindle_state.current_rpm = 60000000.0f / pulse_interval;
            }
        }
    }
}

// =============================================================================
// Main Control Loop
// =============================================================================

void main_control_loop() {
    char uart_buffer[256];
    gcode_t gcode;
    
    printf("Pi Zero SKR Pico PUWinder - Main Control Loop\n");
    printf("Ready for commands...\n");
    
    while (true) {
        // Check for UART commands
        if (uart_is_readable(UART_ID)) {
            uart_read_response(uart_buffer, sizeof(uart_buffer));
            
            if (parse_gcode(uart_buffer, &gcode)) {
                printf("Executing: %s\n", uart_buffer);
                execute_gcode(&gcode);
            } else if (strcmp(uart_buffer, "PING") == 0) {
                uart_send_command("PONG");
            } else if (strcmp(uart_buffer, "VERSION") == 0) {
                uart_send_command("Pico_UART_Test_v1.0");
            } else {
                uart_send_command("UNKNOWN");
            }
        }
        
        // Update status
        static uint32_t last_status_time = 0;
        uint32_t current_time = time_us_32();
        
        if (current_time - last_status_time > 1000000) {  // Every 1 second
            printf("Status - Spindle: %.1f RPM, Position: %.2f mm\n", 
                   spindle_state.current_rpm, stepper_state.current_position);
            last_status_time = current_time;
        }
        
        sleep_ms(10);
    }
}

// =============================================================================
// Initialization and Main Function
// =============================================================================

int main() {
    // Initialize stdio
    stdio_init_all();
    
    // Initialize hardware
    uart_init_snippet();
    spindle_init();
    stepper_init();
    
    // Set up interrupts
    gpio_set_irq_enabled_with_callback(SPINDLE_HALL_PIN, GPIO_IRQ_EDGE_RISE, 
                                      true, &hall_sensor_isr);
    
    printf("\n=== Pi Zero SKR Pico PUWinder ===\n");
    printf("Firmware Version: 1.0\n");
    printf("Hardware: SKR Pico v1.0\n");
    printf("Ready for operation!\n\n");
    
    // Start main control loop
    main_control_loop();
    
    return 0;
}
