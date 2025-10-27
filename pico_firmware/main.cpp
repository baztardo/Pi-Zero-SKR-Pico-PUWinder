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
#include "src/communication_handler.h"
#include "src/scheduler.h"
#include "src/move_queue.h"
#include "src/winding_controller.h"

// Use config.h for pin definitions

// Global controllers (Klipper-style architecture)
BLDC_MOTOR* spindle_controller = nullptr;
TraverseController* traverse_controller = nullptr;
GCodeInterface* gcode_interface = nullptr;
CommunicationHandler* communication_handler = nullptr;
MoveQueue* move_queue = nullptr;
Scheduler* scheduler = nullptr;
WindingController* winding_controller = nullptr;


int main() {
    stdio_init_all();
    
    printf("Pico Spindle Controller %s\n", FIRMWARE_VERSION);
    printf("Build: %s\n", VERSION_DATE);
    printf("Pico_Spindle_Ready\n");
    
    // Initialize spindle controller (handles all spindle GPIO/PWM setup)
    spindle_controller = new BLDC_MOTOR(SPINDLE_HALL_A_PIN);
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
    
    // Initialize winding controller (Klipper-style)
    winding_controller = new WindingController(move_queue, spindle_controller);
    if (!winding_controller) {
        printf("ERROR: Failed to create winding controller\n");
        return -1;
    }
    winding_controller->init();
    printf("Winding controller initialized\n");
    
    // Initialize G-code interface with controller references
    gcode_interface = new GCodeInterface(spindle_controller, traverse_controller, move_queue, winding_controller);
    if (!gcode_interface) {
        printf("ERROR: Failed to create G-code interface\n");
        return -1;
    }
    printf("G-code interface initialized with controller references\n");
    
    // Initialize communication handler
    communication_handler = new CommunicationHandler(gcode_interface);
    if (!communication_handler) {
        printf("ERROR: Failed to create communication handler\n");
        return -1;
    }
    if (!communication_handler->init()) {
        printf("ERROR: Failed to initialize communication handler\n");
        return -1;
    }
    printf("Communication handler initialized\n");
    
    // Connect communication handler to G-code interface
    gcode_interface->set_communication_handler(communication_handler);
    
    printf("Pico Controller Ready (Klipper-style)\n");
    printf("Commands: G-code compatible (G0, G1, G28, M3, M4, M5, S, M6-M19, PING, VERSION)\n");
    printf("Winding System: Traverse homing, spindle sync, emergency stop ready\n");
    printf("Ready for commands...\n");
    
    while (true) {
        // Update controllers
        if (winding_controller) {
            winding_controller->update();
        }
        
        // Handle communication
        if (communication_handler) {
            communication_handler->update();
        }
        
        // Scheduler handles all stepping via ISR
        sleep_ms(10);
    }
    
    // Cleanup (this will never be reached in normal operation)
    if (scheduler) {
        scheduler->stop();
        delete scheduler;
    }
    if (communication_handler) {
        delete communication_handler;
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