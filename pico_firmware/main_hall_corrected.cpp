// Corrected Hall Sensor RPM Calculation Firmware
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define PI_UART_ID uart0
#define PI_UART_TX 0
#define PI_UART_RX 1
#define PI_UART_BAUD 115200

// BLDC control pins
#define BLDC_PWM_PIN 24
#define BLDC_ENABLE_PIN 21
#define BLDC_HALL_PIN 22  // SC speed pulse from RioRand

// Global counters
uint32_t hall_count = 0;
uint32_t last_hall_count = 0;
uint32_t last_calc_time = 0;
bool last_hall_state = false;

void process_command(const char* cmd) {
    printf("CMD: '%s'\n", cmd);
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Hall_Corrected_v1.0\n");
    }
    else if (strncmp(cmd, "SET_BLDC_RPM ", 13) == 0) {
        float rpm = atof(cmd + 13);
        printf("Setting BLDC to %.1f RPM\n", rpm);
        
        if (rpm < 0 || rpm > 3000) {
            uart_puts(PI_UART_ID, "ERROR_RPM_RANGE\n");
            return;
        }
        
        float duty_cycle;
        if (rpm <= 0) {
            duty_cycle = 0.0f;
        } else if (rpm <= 100) {
            duty_cycle = 5.0f + (rpm / 100.0f) * 5.0f;
        } else {
            duty_cycle = 10.0f + ((rpm - 100.0f) / 2900.0f) * 40.0f;
        }
        
        if (duty_cycle > 50.0f) duty_cycle = 50.0f;
        
        uint32_t pwm_level = (uint32_t)(duty_cycle * 655.35f);
        pwm_set_gpio_level(BLDC_PWM_PIN, pwm_level);
        gpio_put(BLDC_ENABLE_PIN, 1);
        
        printf("BLDC set to %.1f RPM (%.1f%%)\n", rpm, duty_cycle);
        uart_puts(PI_UART_ID, "OK\n");
    }
    else if (strcmp(cmd, "STOP_BLDC") == 0) {
        pwm_set_gpio_level(BLDC_PWM_PIN, 0);
        gpio_put(BLDC_ENABLE_PIN, 0);
        printf("BLDC stopped\n");
        uart_puts(PI_UART_ID, "STOPPED\n");
    }
    else if (strcmp(cmd, "GET_HALL") == 0) {
        char response[64];
        snprintf(response, sizeof(response), "HALL:%u\n", hall_count);
        uart_puts(PI_UART_ID, response);
        
        printf("Hall sensor count: %u\n", hall_count);
    }
    else if (strcmp(cmd, "GET_HALL_RPM") == 0) {
        // CORRECTED: Calculate RPM with 3x factor (18 phase changes per revolution)
        uint32_t current_time = time_us_32();
        uint32_t elapsed_us = current_time - last_calc_time;
        
        if (elapsed_us > 1000000) {  // Calculate every 1 second
            uint32_t hall_delta = hall_count - last_hall_count;
            float elapsed_sec = elapsed_us / 1000000.0f;
            
            if (elapsed_sec > 0 && hall_delta > 0) {
                float phase_changes_per_sec = hall_delta / elapsed_sec;
                
                // CORRECTED: 18 phase changes per revolution (6 × 3)
                float rpm = (phase_changes_per_sec / 18.0f) * 60.0f;
                
                char response[64];
                snprintf(response, sizeof(response), "HALL_RPM:%.1f\n", rpm);
                uart_puts(PI_UART_ID, response);
                
                printf("Hall RPM: %.1f (%.1f phases/sec, %u delta, %.1f sec)\n", 
                       rpm, phase_changes_per_sec, hall_delta, elapsed_sec);
            } else {
                uart_puts(PI_UART_ID, "HALL_RPM:0.0\n");
            }
            
            last_hall_count = hall_count;
            last_calc_time = current_time;
        } else {
            uart_puts(PI_UART_ID, "HALL_RPM:0.0\n");
        }
    }
    else if (strcmp(cmd, "RESET_ALL") == 0) {
        hall_count = 0;
        last_hall_count = 0;
        last_calc_time = time_us_32();
        printf("All counters reset\n");
        uart_puts(PI_UART_ID, "ALL_RESET\n");
    }
    else {
        printf("Unknown command: '%s'\n", cmd);
        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
    }
}

int main() {
    stdio_init_all();
    
    printf("Corrected Hall Sensor RPM v1.0\n");
    printf("BLDC: PWM:%d, Enable:%d, Hall:%d\n", BLDC_PWM_PIN, BLDC_ENABLE_PIN, BLDC_HALL_PIN);
    printf("Hall sensor: 18 phase changes per revolution (6 × 3 factor)\n");
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    // Initialize BLDC PWM
    gpio_set_function(BLDC_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BLDC_PWM_PIN);
    pwm_set_wrap(slice_num, 65535);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_num, true);
    
    gpio_init(BLDC_ENABLE_PIN);
    gpio_set_dir(BLDC_ENABLE_PIN, GPIO_OUT);
    gpio_put(BLDC_ENABLE_PIN, 0);
    
    // Initialize Hall sensor pin
    gpio_init(BLDC_HALL_PIN);
    gpio_set_dir(BLDC_HALL_PIN, GPIO_IN);
    gpio_pull_up(BLDC_HALL_PIN);
    
    last_calc_time = time_us_32();
    last_hall_state = gpio_get(BLDC_HALL_PIN);
    
    printf("Controller Ready\n");
    printf("Commands: SET_BLDC_RPM, STOP_BLDC, GET_HALL, GET_HALL_RPM, RESET_ALL\n");
    
    char buffer[64];
    int buffer_pos = 0;
    
    while (true) {
        // POLLING: Check Hall sensor state changes
        bool current_hall_state = gpio_get(BLDC_HALL_PIN);
        if (current_hall_state != last_hall_state) {
            hall_count++;
            last_hall_state = current_hall_state;
        }
        
        // Handle UART commands
        if (uart_is_readable(PI_UART_ID)) {
            char c = uart_getc(PI_UART_ID);
            
            if (c == '\n' || c == '\r') {
                if (buffer_pos > 0) {
                    buffer[buffer_pos] = '\0';
                    process_command(buffer);
                    buffer_pos = 0;
                }
            } else if (buffer_pos < sizeof(buffer) - 1) {
                buffer[buffer_pos++] = c;
            }
        }
        
        sleep_ms(1);  // 1ms polling rate
    }
}
