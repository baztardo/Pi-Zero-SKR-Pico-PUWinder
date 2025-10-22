// Safety E-Stop Firmware
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

// Safety pins
#define E_STOP_PIN 5        // Hardware E-Stop (active LOW)
#define BLDC_PWM_PIN 24     // BLDC control
#define BLDC_ENABLE_PIN 21  // BLDC enable
#define BLDC_STOP_PIN 6     // BLDC controller stop (active LOW)
#define BLDC_BRAKE_PIN 7    // BLDC controller brake (active HIGH)

// Safety state
bool e_stop_active = false;
bool motor_enabled = false;

void e_stop_isr(uint gpio, uint32_t events) {
    if (gpio == E_STOP_PIN) {
        e_stop_active = true;
        motor_enabled = false;
        
        // Emergency stop all motors
        pwm_set_gpio_level(BLDC_PWM_PIN, 0);
        gpio_put(BLDC_ENABLE_PIN, 0);
        gpio_put(BLDC_STOP_PIN, 0);  // Stop BLDC controller
        gpio_put(BLDC_BRAKE_PIN, 1); // Apply brake
        
        printf("EMERGENCY STOP ACTIVATED!\n");
    }
}

void process_command(const char* cmd) {
    printf("CMD: '%s'\n", cmd);
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Safety_v1.0\n");
    }
    else if (strcmp(cmd, "GET_SAFETY_STATUS") == 0) {
        char response[64];
        snprintf(response, sizeof(response), "E_STOP:%s MOTOR:%s\n", 
                e_stop_active ? "ACTIVE" : "OK", 
                motor_enabled ? "ON" : "OFF");
        uart_puts(PI_UART_ID, response);
    }
    else if (strcmp(cmd, "RESET_E_STOP") == 0) {
        if (gpio_get(E_STOP_PIN) == 1) {  // E-Stop button released
            e_stop_active = false;
            printf("E-Stop reset\n");
            uart_puts(PI_UART_ID, "E_STOP_RESET\n");
        } else {
            uart_puts(PI_UART_ID, "E_STOP_STILL_ACTIVE\n");
        }
    }
    else if (strncmp(cmd, "SET_BLDC_RPM ", 13) == 0) {
        if (e_stop_active) {
            uart_puts(PI_UART_ID, "ERROR_E_STOP_ACTIVE\n");
            return;
        }
        
        float rpm = atof(cmd + 13);
        if (rpm < 0 || rpm > 3000) {
            uart_puts(PI_UART_ID, "ERROR_RPM_RANGE\n");
            return;
        }
        
        // Convert RPM to PWM
        float duty_cycle = 5.0f + (rpm / 3000.0f) * 45.0f;
        if (duty_cycle > 50.0f) duty_cycle = 50.0f;
        
        uint32_t pwm_level = (uint32_t)(duty_cycle * 655.35f);
        pwm_set_gpio_level(BLDC_PWM_PIN, pwm_level);
        
        gpio_put(BLDC_ENABLE_PIN, 1);
        gpio_put(BLDC_STOP_PIN, 1);   // Enable BLDC controller
        gpio_put(BLDC_BRAKE_PIN, 0);  // Release brake
        
        motor_enabled = true;
        printf("BLDC set to %.1f RPM (%.1f%%)\n", rpm, duty_cycle);
        uart_puts(PI_UART_ID, "OK\n");
    }
    else if (strcmp(cmd, "STOP_BLDC") == 0) {
        pwm_set_gpio_level(BLDC_PWM_PIN, 0);
        gpio_put(BLDC_ENABLE_PIN, 0);
        gpio_put(BLDC_STOP_PIN, 0);   // Stop BLDC controller
        gpio_put(BLDC_BRAKE_PIN, 0);  // Release brake
        
        motor_enabled = false;
        printf("BLDC stopped\n");
        uart_puts(PI_UART_ID, "STOPPED\n");
    }
    else {
        printf("Unknown command: '%s'\n", cmd);
        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
    }
}

int main() {
    stdio_init_all();
    
    printf("Safety E-Stop Control v1.0\n");
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    // Initialize E-Stop pin
    gpio_init(E_STOP_PIN);
    gpio_set_dir(E_STOP_PIN, GPIO_IN);
    gpio_pull_up(E_STOP_PIN);
    
    // Initialize BLDC control pins
    gpio_set_function(BLDC_PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BLDC_PWM_PIN);
    pwm_set_wrap(slice_num, 65535);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_num, true);
    
    gpio_init(BLDC_ENABLE_PIN);
    gpio_set_dir(BLDC_ENABLE_PIN, GPIO_OUT);
    gpio_put(BLDC_ENABLE_PIN, 0);
    
    gpio_init(BLDC_STOP_PIN);
    gpio_set_dir(BLDC_STOP_PIN, GPIO_OUT);
    gpio_put(BLDC_STOP_PIN, 0);
    
    gpio_init(BLDC_BRAKE_PIN);
    gpio_set_dir(BLDC_BRAKE_PIN, GPIO_OUT);
    gpio_put(BLDC_BRAKE_PIN, 0);
    
    // Set up E-Stop interrupt
    gpio_set_irq_enabled_with_callback(E_STOP_PIN, GPIO_IRQ_EDGE_FALL, 
                                     true, &e_stop_isr);
    
    printf("Safety System Ready\n");
    printf("E-Stop Pin: %d, BLDC Pins: PWM:%d, Enable:%d, Stop:%d, Brake:%d\n", 
           E_STOP_PIN, BLDC_PWM_PIN, BLDC_ENABLE_PIN, BLDC_STOP_PIN, BLDC_BRAKE_PIN);
    
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
