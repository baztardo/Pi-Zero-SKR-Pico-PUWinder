// Fixed RPM Calculation Firmware
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

// Encoder pins
#define ENCODER_A_PIN 3
#define ENCODER_B_PIN 4
#define ENCODER_Z_PIN 25

// Global counters
volatile uint32_t a_count = 0;
volatile uint32_t b_count = 0;
volatile uint32_t z_count = 0;
volatile uint32_t hall_count = 0;
volatile uint32_t last_hall_time = 0;
uint32_t start_time = 0;
uint32_t last_hall_count = 0;
uint32_t last_calc_time = 0;

void encoder_a_isr(uint gpio, uint32_t events) {
    if (gpio == ENCODER_A_PIN) {
        a_count++;
    }
}

void encoder_b_isr(uint gpio, uint32_t events) {
    if (gpio == ENCODER_B_PIN) {
        b_count++;
    }
}

void encoder_z_isr(uint gpio, uint32_t events) {
    if (gpio == ENCODER_Z_PIN) {
        z_count++;
    }
}

void hall_isr(uint gpio, uint32_t events) {
    if (gpio == BLDC_HALL_PIN) {
        hall_count++;
        last_hall_time = time_us_32();
    }
}

void process_command(const char* cmd) {
    printf("CMD: '%s'\n", cmd);
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Fixed_RPM_v1.0\n");
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
    else if (strcmp(cmd, "GET_ENCODER") == 0) {
        char response[128];
        snprintf(response, sizeof(response), "A:%u B:%u Z:%u\n", 
                a_count, b_count, z_count);
        uart_puts(PI_UART_ID, response);
        
        printf("Encoder counts - A: %u, B: %u, Z: %u\n", 
               a_count, b_count, z_count);
    }
    else if (strcmp(cmd, "GET_HALL") == 0) {
        char response[64];
        snprintf(response, sizeof(response), "HALL:%u\n", hall_count);
        uart_puts(PI_UART_ID, response);
        
        printf("Hall sensor count: %u\n", hall_count);
    }
    else if (strcmp(cmd, "GET_HALL_RPM") == 0) {
        // FIXED: Calculate RPM from Hall sensor with proper timing
        uint32_t current_time = time_us_32();
        uint32_t elapsed_us = current_time - last_calc_time;
        
        if (elapsed_us > 100000) {  // Calculate every 100ms
            uint32_t hall_delta = hall_count - last_hall_count;
            float elapsed_sec = elapsed_us / 1000000.0f;
            
            if (elapsed_sec > 0 && hall_delta > 0) {
                float pulses_per_sec = hall_delta / elapsed_sec;
                
                // FIXED: Try different pulse rates per revolution
                // Test with 6 pulses per revolution (2x Hall sensors)
                float rpm = (pulses_per_sec / 6.0f) * 60.0f;
                
                char response[64];
                snprintf(response, sizeof(response), "HALL_RPM:%.1f\n", rpm);
                uart_puts(PI_UART_ID, response);
                
                printf("Hall RPM: %.1f (%.1f pulses/sec, %u delta)\n", rpm, pulses_per_sec, hall_delta);
            } else {
                uart_puts(PI_UART_ID, "HALL_RPM:0.0\n");
            }
            
            last_hall_count = hall_count;
            last_calc_time = current_time;
        } else {
            uart_puts(PI_UART_ID, "HALL_RPM:0.0\n");
        }
    }
    else if (strcmp(cmd, "GET_RPM") == 0) {
        // Calculate RPM from A/B encoder (1440 PPR)
        uint32_t total_pulses = a_count + b_count;
        float rpm = (total_pulses / 1440.0f) * 60.0f;
        
        char response[64];
        snprintf(response, sizeof(response), "RPM:%.1f\n", rpm);
        uart_puts(PI_UART_ID, response);
        
        printf("Encoder RPM: %.1f (from %u pulses)\n", rpm, total_pulses);
    }
    else if (strcmp(cmd, "RESET_ALL") == 0) {
        a_count = 0;
        b_count = 0;
        z_count = 0;
        hall_count = 0;
        last_hall_count = 0;
        start_time = time_us_32();
        last_calc_time = start_time;
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
    
    printf("Fixed RPM Calculation v1.0\n");
    printf("BLDC: PWM:%d, Enable:%d, Hall:%d\n", BLDC_PWM_PIN, BLDC_ENABLE_PIN, BLDC_HALL_PIN);
    printf("Encoder: A:%d, B:%d, Z:%d\n", ENCODER_A_PIN, ENCODER_B_PIN, ENCODER_Z_PIN);
    
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
    
    // Initialize encoder pins
    gpio_init(ENCODER_A_PIN);
    gpio_set_dir(ENCODER_A_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_A_PIN);
    
    gpio_init(ENCODER_B_PIN);
    gpio_set_dir(ENCODER_B_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_B_PIN);
    
    gpio_init(ENCODER_Z_PIN);
    gpio_set_dir(ENCODER_Z_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_Z_PIN);
    
    // Set up interrupts
    gpio_set_irq_enabled_with_callback(ENCODER_A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &encoder_a_isr);
    gpio_set_irq_enabled_with_callback(ENCODER_B_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &encoder_b_isr);
    gpio_set_irq_enabled_with_callback(ENCODER_Z_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &encoder_z_isr);
    gpio_set_irq_enabled_with_callback(BLDC_HALL_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &hall_isr);
    
    start_time = time_us_32();
    last_calc_time = start_time;
    
    printf("Controller Ready\n");
    printf("Commands: SET_BLDC_RPM, STOP_BLDC, GET_ENCODER, GET_HALL, GET_HALL_RPM, GET_RPM, RESET_ALL\n");
    
    char buffer[64];
    int buffer_pos = 0;
    
    while (true) {
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
        
        sleep_ms(10);
    }
}
