// Minimal Pico UART test - responds to PING with PONG
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <cstring>
#include <cstdio>

#define PI_UART_ID uart0
#define PI_UART_TX_PIN 0
#define PI_UART_RX_PIN 1
#define PI_UART_BAUD 115200

char cmd_buffer[128];
int cmd_pos = 0;

void process_command(const char* cmd) {
    if (strcmp(cmd, "PING") == 0) {
        uart_puts(PI_UART_ID, "PONG\n");
    }
    else if (strcmp(cmd, "VERSION") == 0) {
        uart_puts(PI_UART_ID, "Pico_UART_Test_v1.0\n");
    }
    else {
        uart_puts(PI_UART_ID, "UNKNOWN\n");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    // Initialize UART for Pi communication
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX_PIN, GPIO_FUNC_UART);
    
    printf("Pico UART Test Ready\n");
    printf("Listening on UART0 (GPIO 0/1) @ %d baud\n", PI_UART_BAUD);
    uart_puts(PI_UART_ID, "Pico_Ready\n");
    
    while (true) {
        // Read from UART
        if (uart_is_readable(PI_UART_ID)) {
            char c = uart_getc(PI_UART_ID);
            
            if (c == '\n' || c == '\r') {
                if (cmd_pos > 0) {
                    cmd_buffer[cmd_pos] = '\0';
                    printf("Received: %s\n", cmd_buffer);
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
