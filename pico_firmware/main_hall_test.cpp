// Hall Sensor Test Firmware
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <cstring>
#include <cstdio>

#define PI_UART_ID uart0
#define PI_UART_TX 0
#define PI_UART_RX 1
#define PI_UART_BAUD 115200

// Hall sensor pin
#define HALL_PIN 22

// Global counters
volatile uint32_t hall_pulse_count = 0;
volatile uint32_t last_hall_time = 0;
uint32_t start_time = 0;

void hall_isr(uint gpio, uint32_t events) {
    if (gpio == HALL_PIN) {
        hall_pulse_count++;
        last_hall_time = time_us_32();
    }
}

void process_command(const char* cmd) {
    printf("CMD: '%s'\n", cmd);
    
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Hall_Test_v1.0\n");
    }
    else if (strcmp(cmd, "START_HALL_TEST") == 0) {
        hall_pulse_count = 0;
        start_time = time_us_32();
        printf("Hall sensor test started\n");
        uart_puts(PI_UART_ID, "HALL_TEST_STARTED\n");
    }
    else if (strcmp(cmd, "GET_HALL_COUNT") == 0) {
        uint32_t current_time = time_us_32();
        uint32_t elapsed_us = current_time - start_time;
        float elapsed_sec = elapsed_us / 1000000.0f;
        
        char response[64];
        snprintf(response, sizeof(response), "PULSES:%u TIME:%.2fs\n", 
                hall_pulse_count, elapsed_sec);
        uart_puts(PI_UART_ID, response);
        
        printf("Hall pulses: %u in %.2f seconds\n", hall_pulse_count, elapsed_sec);
    }
    else if (strcmp(cmd, "RESET_HALL") == 0) {
        hall_pulse_count = 0;
        start_time = time_us_32();
        printf("Hall counter reset\n");
        uart_puts(PI_UART_ID, "HALL_RESET\n");
    }
    else {
        printf("Unknown command: '%s'\n", cmd);
        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
    }
}

int main() {
    stdio_init_all();
    
    printf("Hall Sensor Test v1.0\n");
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    // Initialize Hall sensor pin
    gpio_init(HALL_PIN);
    gpio_set_dir(HALL_PIN, GPIO_IN);
    gpio_pull_up(HALL_PIN);
    
    // Set up interrupt for Hall sensor
    gpio_set_irq_enabled_with_callback(HALL_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                     true, &hall_isr);
    
    printf("Hall Sensor Ready on GPIO %d\n", HALL_PIN);
    printf("Commands: START_HALL_TEST, GET_HALL_COUNT, RESET_HALL\n");
    
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
