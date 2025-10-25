# C++ API Reference

## ⚡ Pi Zero SKR Pico PUWinder C++ Firmware API

**AI-Generated Documentation** - Last Updated: 2025-10-25 03:51:01

### 📋 Overview

The C++ firmware runs on the SKR Pico and provides real-time control of the winding machine hardware. It includes ISR-driven step generation, G-code parsing, and safety features.

### 🏗️ Architecture

#### Main Components
- **main.cpp**: Application entry point and initialization
- **gcode_interface.cpp**: G-code command parser and executor
- **spindle.cpp**: BLDC motor control with Hall sensor feedback
- **traverse_controller.cpp**: Stepper motor control
- **move_queue.cpp**: Move queue with ISR-driven execution
- **scheduler.cpp**: Hardware timer ISR scheduler

### 🔧 Core Classes

#### BLDC_MOTOR
Controls the spindle BLDC motor with Hall sensor feedback.

```cpp
#include "spindle.h"

// Create spindle controller
BLDC_MOTOR* spindle_controller = new BLDC_MOTOR(SPINDLE_HALL_PIN);
spindle_controller->init();

// Get RPM
float rpm = spindle_controller->get_rpm();

// Set direction
spindle_controller->set_direction(DIRECTION_CW);

// Set brake
spindle_controller->set_brake(true);
```

#### TraverseController
Controls the traverse stepper motor.

```cpp
#include "traverse_controller.h"

// Create traverse controller
TraverseController* traverse_controller = new TraverseController();
traverse_controller->init();

// Move to position
traverse_controller->move_to_position(50.0f);

// Home axis
traverse_controller->home();

// Get current position
float position = traverse_controller->get_current_position();
```

#### GCodeInterface
Parses and executes G-code commands from the Pi Zero.

```cpp
#include "gcode_interface.h"

// Create G-code interface
GCodeInterface* gcode_interface = new GCodeInterface();

// Parse command
if (gcode_interface->parse_command("M3 S500")) {
    // Execute command
    gcode_interface->execute_command();
}
```

#### MoveQueue
Manages move chunks with ISR-driven execution.

```cpp
#include "move_queue.h"

// Create move queue
MoveQueue* move_queue = new MoveQueue();
move_queue->init();

// Push move chunk
StepChunk chunk;
// ... populate chunk ...
move_queue->push_chunk(AXIS_TRAVERSE, chunk);

// Check if axis is active
bool active = move_queue->is_active(AXIS_TRAVERSE);
```

#### Scheduler
Hardware timer ISR scheduler for real-time control.

```cpp
#include "scheduler.h"

// Create scheduler
Scheduler* scheduler = new Scheduler(move_queue);

// Start scheduler
scheduler->start(HEARTBEAT_US);

// Check if running
bool running = scheduler->is_running();
```

### 🎯 G-code Commands

#### Movement Commands
```cpp
// G0/G1 - Linear interpolation
gcode_interface->parse_command("G1 Y50 F1000");

// G28 - Home all axes
gcode_interface->parse_command("G28");

// G4 - Dwell with planner sync
gcode_interface->parse_command("G4 P1000");
```

#### Spindle Commands
```cpp
// M3 - Spindle clockwise
gcode_interface->parse_command("M3 S500");

// M4 - Spindle counter-clockwise
gcode_interface->parse_command("M4 S1000");

// M5 - Stop spindle
gcode_interface->parse_command("M5");

// S - Set spindle speed
gcode_interface->parse_command("S1500");
```

#### Safety Commands
```cpp
// M0 - Feed hold
gcode_interface->parse_command("M0");

// M1 - Resume from hold
gcode_interface->parse_command("M1");

// M112 - Emergency stop
gcode_interface->parse_command("M112");

// M410 - Quick stop
gcode_interface->parse_command("M410");

// M999 - Reset from emergency stop
gcode_interface->parse_command("M999");
```

### 🛡️ Safety Features

#### Move Queue Safety
```cpp
// Pause feeding
move_queue->pause_feeding();

// Resume feeding
move_queue->resume_feeding();

// Check if feeding is paused
bool paused = move_queue->is_feeding_paused();

// Emergency stop
move_queue->emergency_stop();

// Check if emergency stop is active
bool emergency = move_queue->is_emergency_stopped();
```

#### Spindle Safety
```cpp
// Set spindle brake
spindle_controller->set_brake(true);

// Check brake status
bool brake = spindle_controller->get_brake();

// Set direction
spindle_controller->set_direction(DIRECTION_CW);
```

### ⚡ ISR and Real-time Control

#### Hardware Timer ISR
```cpp
// Scheduler ISR handler
void scheduler_isr() {
    // Process move queues
    if (move_queue) {
        move_queue->handle_isr_tick();
    }
    
    // Update encoder
    if (spindle_controller) {
        spindle_controller->handle_pulse();
    }
}
```

#### Step Generation
```cpp
// Traverse step ISR
void traverse_step_isr() {
    if (traverse_controller) {
        traverse_controller->stepper_step();
    }
}
```

### 📊 Status and Monitoring

#### Machine Status
```cpp
// Get spindle status
float rpm = spindle_controller->get_rpm();
bool running = !spindle_controller->get_brake();

// Get traverse status
float position = traverse_controller->get_current_position();
bool homed = traverse_controller->is_homed();

// Get move queue status
uint32_t depth = move_queue->get_queue_depth(AXIS_TRAVERSE);
bool active = move_queue->is_active(AXIS_TRAVERSE);
```

#### Scheduler Status
```cpp
// Get scheduler information
bool running = scheduler->is_running();
uint32_t ticks = scheduler->get_tick_count();
```

### 🔧 Hardware Configuration

#### Pin Definitions
```cpp
// From config.h
#define SPINDLE_PWM_PIN 24
#define SPINDLE_ENABLE_PIN 21
#define SPINDLE_HALL_PIN 22
#define SPINDLE_DIR_PIN 4
#define SPINDLE_BRAKE_PIN 3

#define TRAVERSE_STEP_PIN 6
#define TRAVERSE_DIR_PIN 5
#define TRAVERSE_ENA_PIN 7
#define TRAVERSE_HOME_PIN 16
```

#### PWM Configuration
```cpp
// Initialize PWM
gpio_set_function(SPINDLE_PWM_PIN, GPIO_FUNC_PWM);
uint slice_num = pwm_gpio_to_slice_num(SPINDLE_PWM_PIN);
uint chan = pwm_gpio_to_channel(SPINDLE_PWM_PIN);
pwm_set_wrap(slice_num, 65535);
pwm_set_chan_level(slice_num, chan, 0);
pwm_set_enabled(slice_num, true);
```

### 🧪 Testing and Validation

#### Unit Testing
```cpp
// Test spindle control
void test_spindle() {
    BLDC_MOTOR* spindle = new BLDC_MOTOR(SPINDLE_HALL_PIN);
    spindle->init();
    
    // Test direction
    spindle->set_direction(DIRECTION_CW);
    assert(spindle->get_direction() == DIRECTION_CW);
    
    // Test brake
    spindle->set_brake(true);
    assert(spindle->get_brake() == true);
}
```

#### Integration Testing
```cpp
// Test G-code interface
void test_gcode() {
    GCodeInterface* gcode = new GCodeInterface();
    
    // Test parsing
    assert(gcode->parse_command("M3 S500"));
    assert(gcode->get_current_command() == TOKEN_M3);
    
    // Test execution
    assert(gcode->execute_command());
}
```

### 📈 Performance Optimization

#### ISR Optimization
```cpp
// Minimize ISR execution time
void scheduler_isr() {
    // Only essential operations in ISR
    move_queue->handle_isr_tick();
    
    // Defer non-critical operations
    if (user_callback) {
        user_callback(user_callback_data);
    }
}
```

#### Memory Management
```cpp
// Efficient move queue
class MoveQueue {
private:
    StepChunk queues[2][128];  // Fixed-size arrays
    volatile uint16_t head[2];
    volatile uint16_t tail[2];
};
```

### 🔄 Error Handling

#### Command Validation
```cpp
// Validate G-code parameters
bool GCodeInterface::validate_parameters() {
    if (params.has_S && (params.S < 0 || params.S > 3000)) {
        set_error("RPM out of range (0-3000)");
        return false;
    }
    return true;
}
```

#### Hardware Error Handling
```cpp
// Check hardware status
bool check_hardware() {
    if (!spindle_controller) {
        printf("ERROR: Spindle controller not initialized
");
        return false;
    }
    return true;
}
```

### 📚 Examples

#### Complete Initialization
```cpp
int main() {{
    stdio_init_all();
    
    // Initialize UART
    uart_init(PI_UART_ID, PI_UART_BAUD);
    gpio_set_function(PI_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PI_UART_RX, GPIO_FUNC_UART);
    
    // Initialize spindle
    spindle_controller = new BLDC_MOTOR(SPINDLE_HALL_PIN);
    spindle_controller->init();
    
    // Initialize traverse
    traverse_controller = new TraverseController();
    traverse_controller->init();
    
    // Initialize move queue
    move_queue = new MoveQueue();
    move_queue->init();
    
    // Initialize scheduler
    scheduler = new Scheduler(move_queue);
    scheduler->start(HEARTBEAT_US);
    
    // Initialize G-code interface
    gcode_interface = new GCodeInterface();
    
    printf("Pico Spindle Controller Ready\n");
    
    // Main loop
    while (true) {{
        // Process UART commands
        if (uart_is_readable(PI_UART_ID)) {{
            char buffer[256];
            uart_read_blocking(PI_UART_ID, buffer, 1);
            // ... process command ...
        }}
    }}
}}
```

---
*This documentation is automatically generated by AI and updated on every push.*
