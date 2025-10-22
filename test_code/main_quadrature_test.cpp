// Quadrature Encoder Test Firmware
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <cstring>
#include <cstdio>

#define PI_UART_ID uart0
#define PI_UART_TX 0
#define PI_UART_RX 1
#define PI_UART_BAUD 115200

// Encoder pins
#define ENCODER_A_PIN 2
#define ENCODER_B_PIN 3
#define ENCODER_Z_PIN 4
#define ENCODER_PULSE_PIN 22  // Your current pin

// BLDC control
#define BLDC_PWM_PIN 24
#define BLDC_ENABLE_PIN 21

// Global counters
volatile uint32_t pulse_count = 0;
volatile uint32_t a_count = 0;
volatile uint32_t b_count = 0;
volatile uint32_t z_count = 0;

void pulse_isr(uint gpio, uint32_t events) {
    if (gpio == ENCODER_PULSE_PIN) {
        pulse_count++;
    }
}

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

void process_command(const char* cmd) {
    printf("CMD: '%s'\n", cmd);
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Quadrature_Test_v1.0\n");
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
        snprintf(response, sizeof(response), "PULSE:%u A:%u B:%u Z:%u\n", 
                pulse_count, a_count, b_count, z_count);
        uart_puts(PI_UART_ID, response);
        
        printf("Encoder counts - Pulse: %u, A: %u, B: %u, Z: %u\n", 
               pulse_count, a_count, b_count, z_count);
    }
    else if (strcmp(cmd, "RESET_ENCODER") == 0) {
        pulse_count = 0;
        a_count = 0;
        b_count = 0;
        z_count = 0;
        printf("All encoder counters reset\n");
        uart_puts(PI_UART_ID, "ENCODER_RESET\n");
    }
    else {
        printf("Unknown command: '%s'\n", cmd);
        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
    }
}

int main() {
    stdio_init_all();
    
    printf("Quadrature Encoder Test v1.0\n");
    
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
    
    // Initialize encoder pins
    gpio_init(ENCODER_PULSE_PIN);
    gpio_set_dir(ENCODER_PULSE_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_PULSE_PIN);
    
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
    gpio_set_irq_enabled_with_callback(ENCODER_PULSE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &pulse_isr);
    gpio_set_irq_enabled_with_callback(ENCODER_A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &encoder_a_isr);
    gpio_set_irq_enabled_with_callback(ENCODER_B_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &encoder_b_isr);
    gpio_set_irq_enabled_with_callback(ENCODER_Z_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &encoder_z_isr);
    
    printf("Quadrature Encoder Test Ready\n");
    printf("Pins: Pulse:%d A:%d B:%d Z:%d\n", ENCODER_PULSE_PIN, ENCODER_A_PIN, ENCODER_B_PIN, ENCODER_Z_PIN);
    printf("Commands: SET_BLDC_RPM, STOP_BLDC, GET_ENCODER, RESET_ENCODER\n");
    
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
