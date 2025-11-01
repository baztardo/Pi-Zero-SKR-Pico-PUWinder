// =============================================================================
// main.cpp - COMPLETE WORKING VERSION
// Fixed integration of Scheduler, MoveQueue, TraverseController, WindingController
// =============================================================================

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <cstdio>

#include "config.h"
#include "spindle.h"
#include "traverse_controller.h"
#include "move_queue.h"
#include "scheduler.h"
#include "winding_controller.h"
#include "gcode_interface.h"
#include "communication_handler.h"
#include "diagnostic_monitor.h"

// Global instances
BLDC_MOTOR* spindle_controller = nullptr;
TraverseController* traverse_controller = nullptr;
MoveQueue* move_queue = nullptr;
Scheduler* scheduler = nullptr;
WindingController* winding_controller = nullptr;
GCodeInterface* gcode_interface = nullptr;
CommunicationHandler* communication_handler = nullptr;
DiagnosticMonitor* diagnostic_monitor = nullptr;


int main() {
    // Initialize stdio
    stdio_init_all();
    sleep_ms(2000);  // Wait for USB serial
    
    printf("\n\n");
    printf("========================================\n");
    printf("Pi Zero SKR Pico PUWinder - FIXED\n");
    printf("========================================\n\n");
    
    // Initialize spindle controller
    printf("Initializing spindle controller...\n");
    spindle_controller = new BLDC_MOTOR(SPINDLE_HALL_A_PIN);
    if (!spindle_controller) {
        printf("ERROR: Failed to create spindle controller\n");
        return -1;
    }
    spindle_controller->init();
    printf("✓ Spindle controller initialized\n");

    // Initialize move queue FIRST (Klipper-style ISR system)
    printf("Initializing move queue (ISR-driven)...\n");
    move_queue = new MoveQueue();
    if (!move_queue) {
        printf("ERROR: Failed to create move queue\n");
        return -1;
    }
    move_queue->init();
    printf("✓ Move queue initialized\n");

    // Initialize traverse controller (now with MoveQueue)
    printf("Initializing traverse controller...\n");
    traverse_controller = new TraverseController(move_queue);
    if (!traverse_controller) {
        printf("ERROR: Failed to create traverse controller\n");
        return -1;
    }
    traverse_controller->init();
    printf("✓ Traverse controller initialized\n");
    printf("  (Uses MoveQueue for real-time stepping)\n");
    printf("  (Handles stepping during winding)\n");
    
    // Initialize scheduler (20kHz ISR)
    printf("Initializing scheduler...\n");
    scheduler = new Scheduler(move_queue);
    if (!scheduler) {
        printf("ERROR: Failed to create scheduler\n");
        return -1;
    }
    if (!scheduler->start(50)) {  // 50us = 20kHz
        printf("ERROR: Failed to start scheduler\n");
        return -1;
    }
    printf("✓ Scheduler started at 20kHz\n");
    
    // Initialize winding controller
    printf("Initializing winding controller...\n");
    winding_controller = new WindingController(move_queue, spindle_controller);
    if (!winding_controller) {
        printf("ERROR: Failed to create winding controller\n");
        return -1;
    }
    winding_controller->init();
    printf("✓ Winding controller initialized\n");
    
    // Initialize G-code interface
    printf("Initializing G-code interface...\n");
    gcode_interface = new GCodeInterface(spindle_controller, traverse_controller, 
                                         move_queue, winding_controller);
    if (!gcode_interface) {
        printf("ERROR: Failed to create G-code interface\n");
        return -1;
    }
    printf("✓ G-code interface initialized\n");
    
    // Initialize communication handler
    printf("Initializing communication handler...\n");
    communication_handler = new CommunicationHandler(gcode_interface);
    if (!communication_handler) {
        printf("ERROR: Failed to create communication handler\n");
        return -1;
    }
    if (!communication_handler->init()) {
        printf("ERROR: Failed to initialize communication handler\n");
        return -1;
    }
    gcode_interface->set_communication_handler(communication_handler);
    printf("✓ Communication handler initialized\n");
    
    // Initialize diagnostic monitor
    printf("Initializing diagnostic monitor...\n");
    diagnostic_monitor = new DiagnosticMonitor(move_queue);
    printf("✓ Diagnostic monitor initialized\n");
    
    printf("\n========================================\n");
    printf("SYSTEM READY\n");
    printf("========================================\n");
    printf("Architecture:\n");
    printf("  - Scheduler ISR: 20kHz (controls MoveQueue)\n");
    printf("  - MoveQueue: Handles winding traverse steps\n");
    printf("  - TraverseController: Handles homing/manual moves\n");
    printf("  - WindingController: Coordinates winding process\n");
    printf("\nReady for commands...\n\n");
    
    // Main loop
    uint32_t last_diag_time = 0;
    
    while (true) {
        // Update winding controller (generates chunks for MoveQueue)
        if (winding_controller) {
            winding_controller->update();
        }
        
        // ✅ CRITICAL FIX: Only call generate_steps when TraverseController is moving
        // This allows homing to work while preventing conflicts during winding
        if (traverse_controller && traverse_controller->is_moving()) {
            traverse_controller->generate_steps();
        }
        
        // Handle incoming commands
        if (communication_handler) {
            communication_handler->update();
        }
        
        // Monitor queue health every second
        if (diagnostic_monitor) {
            diagnostic_monitor->update(1000);
        }
        
        // Full diagnostics every 10 seconds
        uint32_t now = time_us_32();
        if ((now - last_diag_time) > 10000000) {
            if (diagnostic_monitor) {
                diagnostic_monitor->print_full_diagnostics();
            }
            last_diag_time = now;
        }
        
        // Small delay (allows up to 10kHz main loop rate)
        sleep_us(100);
    }
    
    return 0;
}