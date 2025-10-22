// Pi-Pico UART Controller with Spindle Support
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "src/spindle.h"
#include "src/config.h"
#include "src/version.h"

// Use config.h for pin definitions

// Global spindle controller
BLDC_MOTOR* spindle_controller = nullptr;

void process_command(const char* cmd) {
    printf("CMD: %s (len=%d)\n", cmd, strlen(cmd));
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        char version_str[64];
        snprintf(version_str, sizeof(version_str), "Pico_Spindle_%s\n", FIRMWARE_VERSION);
        uart_puts(PI_UART_ID, version_str);
    }
    else if (strncmp(cmd, "SET_SPINDLE_RPM ", 16) == 0) {
        float rpm = atof(cmd + 16);
        printf("Setting spindle to %.1f RPM\n", rpm);
        
        if (rpm < 0 || rpm > 3000) {
            uart_puts(PI_UART_ID, "ERROR_RPM_RANGE\n");
            return;
        }
        
        // Convert RPM to PWM duty cycle (0-100%)
        float duty_cycle = (rpm / 3000.0f) * 100.0f;
        if (duty_cycle > 100.0f) duty_cycle = 100.0f;
        
        printf("Duty cycle: %.1f%%\n", duty_cycle);
        
        // Set PWM duty cycle (16-bit: 0-65535)
        uint32_t pwm_value = (uint32_t)(duty_cycle * 65535.0f / 100.0f);
        pwm_set_gpio_level(SPINDLE_PWM_PIN, pwm_value);
        
        uart_puts(PI_UART_ID, "OK\n");
    }
    else if (strcmp(cmd, "GET_SPINDLE_RPM") == 0) {
        // Read current RPM from Hall sensor
        if (spindle_controller) {
            float current_rpm = spindle_controller->get_rpm();
            char response[32];
            snprintf(response, sizeof(response), "RPM:%.1f\n", current_rpm);
            uart_puts(PI_UART_ID, response);
        } else {
            uart_puts(PI_UART_ID, "RPM:0.0\n");
        }
    }
    else if (strcmp(cmd, "STOP_SPINDLE") == 0) {
        // Stop PWM output
        uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
        uint chan = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
        pwm_set_chan_level(slice_num, chan, 0);
        uart_puts(PI_UART_ID, "STOPPED\n");
    }
    else {
        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
    }
}

int main() {
    stdio_init_all();
    
    printf("Pico Spindle Controller %s\n", FIRMWARE_VERSION);
    printf("Build: %s\n", VERSION_DATE);
    printf("Pico_Spindle_Ready\n");
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    // Initialize spindle PWM
    gpio_set_function(SPINDLE_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
    uint chan = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
    pwm_set_wrap(slice_num, 65535);  // 16-bit resolution
    pwm_set_chan_level(slice_num, chan, 0);  // Start at 0% duty cycle
    pwm_set_enabled(slice_num, true);
    
    // Initialize spindle enable pin
    gpio_init(SPINDLE_ENABLE_PIN);
    gpio_set_dir(SPINDLE_ENABLE_PIN, GPIO_OUT);
    gpio_put(SPINDLE_ENABLE_PIN, 1);  // Enable spindle
    
    // Initialize spindle speed pulse reader
    spindle_controller = new BLDC_MOTOR(SPINDLE_HALL_PIN);
    spindle_controller->init();  // Initialize the controller
    
    printf("Spindle Controller Ready\n");
    printf("Commands: PING, VERSION, SET_SPINDLE_RPM <rpm>, GET_SPINDLE_RPM, STOP_SPINDLE\n");
    
    char buffer[64];
    int buffer_pos = 0;
    
    while (true) {
        // Check for UART data
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
