// Debug UART Test
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <cstring>
#include <cstdio>

#define PI_UART_ID uart0
#define PI_UART_TX 0
#define PI_UART_RX 1
#define PI_UART_BAUD 115200

int main() {
    stdio_init_all();
    
    printf("DEBUG_FIRMWARE_v1.0\n");
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    printf("UART Ready - Debug Mode\n");
    
    char buffer[64];
    int buffer_pos = 0;
    
    while (true) {
        if (uart_is_readable(PI_UART_ID)) {
            char c = uart_getc(PI_UART_ID);
            printf("Got char: %c (0x%02x)\n", c, c);
            
            if (c == '\n' || c == '\r') {
                if (buffer_pos > 0) {
                    buffer[buffer_pos] = '\0';
                    printf("Full command: '%s'\n", buffer);
                    
                    if (strcmp(buffer, "PING") == 0) {
                        uart_puts(PI_UART_ID, "PONG\n");
                    }
                    else if (strcmp(buffer, "VERSION") == 0) {
                        uart_puts(PI_UART_ID, "DEBUG_v1.0\n");
                    }
                    else if (strncmp(buffer, "SET_BLDC_RPM ", 13) == 0) {
                        printf("BLDC command detected!\n");
                        uart_puts(PI_UART_ID, "BLDC_OK\n");
                    }
                    else {
                        printf("Unknown command: '%s'\n", buffer);
                        uart_puts(PI_UART_ID, "ERROR_UNKNOWN_CMD\n");
                    }
                    
                    buffer_pos = 0;
                }
            } else if (buffer_pos < sizeof(buffer) - 1) {
                buffer[buffer_pos++] = c;
            }
        }
        
        sleep_ms(10);
    }
}
