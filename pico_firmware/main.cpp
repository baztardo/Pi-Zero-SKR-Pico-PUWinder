// Pi-Pico UART Controller with BLDC Support
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include <cstring>
#include <cstdio>

// UART config (Pi communication)
#define PI_UART_ID uart0
#define PI_UART_TX_PIN 0
#define PI_UART_RX_PIN 1
#define PI_UART_BAUD 115200

// BLDC config (YOUR pin assignments!)
#define BLDC_PWM_PIN 24      // PWM to EP-0172 SC input
#define BLDC_HALL_PIN 22     // Hall sensor input from EP-0172
#define BLDC_POWER_PIN 21    // HotBed MOSFET (motor power enable)

uint bldc_pwm_slice;
float bldc_target_rpm = 0;

void bldc_init() {
    // Init PWM for speed control
    gpio_set_function(BLDC_PWM_PIN, GPIO_FUNC_PWM);
    bldc_pwm_slice = pwm_gpio_to_slice_num(BLDC_PWM_PIN);
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);
    pwm_init(bldc_pwm_slice, &config, true);
    pwm_set_gpio_level(BLDC_PWM_PIN, 0);
    
    // Init Hall sensor input
    gpio_init(BLDC_HALL_PIN);
    gpio_set_dir(BLDC_HALL_PIN, GPIO_IN);
    
    // Init power enable (HotBed MOSFET)
    gpio_init(BLDC_POWER_PIN);
    gpio_set_dir(BLDC_POWER_PIN, GPIO_OUT);
    gpio_put(BLDC_POWER_PIN, 0);  // Off initially
    
    printf("BLDC init: PWM=GPIO%d, Hall=GPIO%d, Power=GPIO%d\n", 
           BLDC_PWM_PIN, BLDC_HALL_PIN, BLDC_POWER_PIN);
}

void bldc_set_rpm(float rpm) {
    bldc_target_rpm = rpm;
    
    if (rpm > 0) {
        gpio_put(BLDC_POWER_PIN, 1);  // Enable power
        
        // Linear mapping: 0-3000 RPM = 0-100% duty
        float duty = (rpm / 3000.0f) * 100.0f;
        if (duty > 100.0f) duty = 100.0f;
        
        uint16_t level = (uint16_t)((duty / 100.0f) * 65535);
        pwm_set_gpio_level(BLDC_PWM_PIN, level);
        
        printf("BLDC: %.1f RPM (%.1f%% duty, power ON)\n", rpm, duty);
    } else {
        gpio_put(BLDC_POWER_PIN, 0);  // Disable power
        pwm_set_gpio_level(BLDC_PWM_PIN, 0);
        printf("BLDC: stopped (power OFF)\n");
    }
}

void bldc_stop() {
    pwm_set_gpio_level(BLDC_PWM_PIN, 0);
    gpio_put(BLDC_POWER_PIN, 0);  // Power off
    bldc_target_rpm = 0;
    printf("BLDC: emergency stop\n");
}

char cmd_buffer[128];
int cmd_pos = 0;

void process_command(const char* cmd) {
    printf("CMD: %s\n", cmd);
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Pico_BLDC_v1.0\n");
    }
    else if (strncmp(cmd, "SET_BLDC_RPM ", 13) == 0) {
        float rpm = atof(cmd + 13);
        bldc_set_rpm(rpm);
        uart_puts(PI_UART_ID, "ok\n");
    }
    else if (strcmp(cmd, "GET_BLDC_RPM") == 0) {
        char response[32];
        snprintf(response, sizeof(response), "%.1f\n", bldc_target_rpm);
        uart_puts(PI_UART_ID, response);
    }
    else if (strcmp(cmd, "STOP_BLDC") == 0) {
        bldc_stop();
        uart_puts(PI_UART_ID, "ok\n");
    }
    else {
        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    // Init UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX_PIN, GPIO_FUNC_UART);
    
    // Init BLDC
    bldc_init();
    
    printf("\n=================================\n");
    printf("Pico BLDC Controller v1.0\n");
    printf("UART: GPIO 0/1 @ %d baud\n", PI_UART_BAUD);
    printf("BLDC PWM: GPIO %d\n", BLDC_PWM_PIN);
    printf("BLDC Hall: GPIO %d\n", BLDC_HALL_PIN);
    printf("BLDC Power: GPIO %d (HotBed)\n", BLDC_POWER_PIN);
    printf("=================================\n\n");
    
    uart_puts(PI_UART_ID, "Pico_BLDC_Ready\n");
    
    while (true) {
        if (uart_is_readable(PI_UART_ID)) {
            char c = uart_getc(PI_UART_ID);
            
            if (c == '\n' || c == '\r') {
                if (cmd_pos > 0) {
                    cmd_buffer[cmd_pos] = '\0';
                    process_command(cmd_buffer);
                    cmd_pos = 0;
                }
            }
            else if (cmd_pos < sizeof(cmd_buffer) - 1) {
                cmd_buffer[cmd_pos++] = c;
            }
        }
        sleep_ms(1);
    }
}
