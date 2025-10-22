// Debug PWM Firmware
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define PI_UART_ID uart0
#define PI_UART_TX 0
#define PI_UART_RX 1
#define PI_UART_BAUD 115200

// BLDC PWM config
#define BLDC_PWM_PIN 24
#define BLDC_ENABLE_PIN 21

void process_command(const char* cmd) {
    printf("CMD: '%s'\n", cmd);
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Debug_PWM_v1.0\n");
    }
    else if (strncmp(cmd, "SET_BLDC_RPM ", 13) == 0) {
        float rpm = atof(cmd + 13);
        printf("Setting BLDC to %.1f RPM\n", rpm);
        
        if (rpm < 0 || rpm > 3000) {
            uart_puts(PI_UART_ID, "ERROR_RPM_RANGE\n");
            return;
        }
        
        // Convert RPM to PWM duty cycle (0-100%)
        float duty_cycle = (rpm / 3000.0f) * 100.0f;
        if (duty_cycle > 100.0f) duty_cycle = 100.0f;
        
        printf("Duty cycle: %.1f%%\n", duty_cycle);
        
        // Set PWM duty cycle (0-65535 for 16-bit)
        uint32_t pwm_level = (uint32_t)(duty_cycle * 655.35f);
        pwm_set_gpio_level(BLDC_PWM_PIN, pwm_level);
        
        printf("PWM level set to: %u (%.1f%%)\n", pwm_level, duty_cycle);
        
        // Also set enable pin high
        gpio_put(BLDC_ENABLE_PIN, 1);
        printf("Enable pin set HIGH\n");
        
        uart_puts(PI_UART_ID, "OK\n");
    }
    else if (strcmp(cmd, "STOP_BLDC") == 0) {
        pwm_set_gpio_level(BLDC_PWM_PIN, 0);
        gpio_put(BLDC_ENABLE_PIN, 0);
        printf("BLDC stopped - PWM=0, Enable=LOW\n");
        uart_puts(PI_UART_ID, "STOPPED\n");
    }
    else if (strcmp(cmd, "TEST_PWM") == 0) {
        // Test with fixed PWM levels
        printf("Testing PWM levels...\n");
        
        // Test 10% duty cycle
        pwm_set_gpio_level(BLDC_PWM_PIN, 6553);  // 10%
        gpio_put(BLDC_ENABLE_PIN, 1);
        printf("PWM set to 10%% (6553)\n");
        uart_puts(PI_UART_ID, "PWM_10_PERCENT\n");
    }
    else {
        printf("Unknown command: '%s'\n", cmd);
        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
    }
}

int main() {
    stdio_init_all();
    
    printf("Debug PWM Control v1.0\n");
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    // Initialize BLDC PWM
    gpio_set_function(BLDC_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BLDC_PWM_PIN);
    pwm_set_wrap(slice_num, 65535);  // 16-bit resolution
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_num, true);
    
    // Initialize BLDC enable pin
    gpio_init(BLDC_ENABLE_PIN);
    gpio_set_dir(BLDC_ENABLE_PIN, GPIO_OUT);
    gpio_put(BLDC_ENABLE_PIN, 0);  // Start with enable LOW
    
    printf("BLDC Controller Ready\n");
    printf("PWM Pin: %d, Enable Pin: %d\n", BLDC_PWM_PIN, BLDC_ENABLE_PIN);
    printf("Commands: PING, VERSION, SET_BLDC_RPM <rpm>, STOP_BLDC, TEST_PWM\n");
    
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
