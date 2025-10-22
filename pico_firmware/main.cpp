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
#include "src/traverse_controller.h"
#include "src/gcode_interface.h"
#include "src/scheduler.h"
#include "src/move_queue.h"
#include "src/winding_controller.h"

// Use config.h for pin definitions

// Global controllers (Klipper-style architecture)
BLDC_MOTOR* spindle_controller = nullptr;
TraverseController* traverse_controller = nullptr;
GCodeInterface* gcode_interface = nullptr;
MoveQueue* move_queue = nullptr;
Scheduler* scheduler = nullptr;
WindingController* winding_controller = nullptr;

void process_command(const char* cmd) {
    printf("CMD: %s (len=%d)\n", cmd, strlen(cmd));
    
    if (!gcode_interface) {
        uart_puts(PI_UART_ID, "ERROR_INTERFACE_NOT_INIT\n");
        return;
    }
    
    // Parse command
    if (!gcode_interface->parse_command(cmd)) {
        uart_puts(PI_UART_ID, "ERROR_PARSE_FAILED\n");
        return;
    }
    
    // Execute command (G-code interface handles all responses)
    gcode_interface->execute_command();
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
    if (!spindle_controller) {
        printf("ERROR: Failed to create spindle controller\n");
        return -1;
    }
    spindle_controller->init();
    printf("Spindle controller initialized\n");
    
    // Initialize traverse controller
    traverse_controller = new TraverseController();
    if (!traverse_controller) {
        printf("ERROR: Failed to create traverse controller\n");
        return -1;
    }
    traverse_controller->init();
    printf("Traverse controller initialized\n");
    
    // Initialize move queue (Klipper-style)
    move_queue = new MoveQueue();
    if (!move_queue) {
        printf("ERROR: Failed to create move queue\n");
        return -1;
    }
    move_queue->init();
    printf("Move queue initialized\n");
    
    // Initialize scheduler (Klipper-style)
    scheduler = new Scheduler(move_queue);
    if (!scheduler) {
        printf("ERROR: Failed to create scheduler\n");
        return -1;
    }
    if (!scheduler->start(HEARTBEAT_US)) {
        printf("ERROR: Failed to start scheduler\n");
        return -1;
    }
    printf("Scheduler started\n");
    
    // Initialize G-code interface first
    gcode_interface = new GCodeInterface();
    if (!gcode_interface) {
        printf("ERROR: Failed to create G-code interface\n");
        return -1;
    }
    printf("G-code interface initialized\n");
    
    // Initialize winding controller (Klipper-style)
    winding_controller = new WindingController(move_queue);
    if (!winding_controller) {
        printf("ERROR: Failed to create winding controller\n");
        return -1;
    }
    winding_controller->init();
    printf("Winding controller initialized\n");
    
    printf("Pico Controller Ready (Klipper-style)\n");
    printf("Commands: G-code compatible (G0, G1, G28, M3, M4, M5, S, M6-M19, PING, VERSION)\n");
    
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
        
        // Klipper-style: Scheduler handles all stepping via ISR
        // No manual step generation needed - ISR handles everything
        
        sleep_ms(10);
    }
    
    // Cleanup (this will never be reached in normal operation)
    if (scheduler) {
        scheduler->stop();
        delete scheduler;
    }
    if (move_queue) {
        delete move_queue;
    }
    if (winding_controller) {
        delete winding_controller;
    }
    if (gcode_interface) {
        delete gcode_interface;
    }
    if (traverse_controller) {
        delete traverse_controller;
    }
    if (spindle_controller) {
        delete spindle_controller;
    }
    
    return 0;
}
