# Klipper-Style Architecture Implementation

## Overview
This document describes the proper Klipper-style architecture implementation for the Pi Zero + Pico winding machine system.

## Architecture Components

### 1. **Scheduler** (ISR-based timing)
- **Purpose**: Hardware timer ISR for precise timing
- **Function**: Calls MoveQueue::handle_isr_tick() at high frequency
- **Integration**: `scheduler->start(HEARTBEAT_US)` in main.cpp
- **Klipper-style**: Uses repeating timer ISR for step generation

### 2. **MoveQueue** (Step generation and queuing)
- **Purpose**: ISR-driven step execution with move queuing
- **Function**: `axis_isr_handler()` for each axis, alternating between spindle/traverse
- **Integration**: `move_queue->init()` and `move_queue->handle_isr_tick()`
- **Klipper-style**: Fixed-size queues with ISR-driven step pulses

### 3. **StepCompressor** (Smooth movement)
- **Purpose**: Converts high-level moves to step chunks
- **Function**: `compress_trapezoid()` and `compress_constant_velocity()`
- **Integration**: Used by WindingController to generate step chunks
- **Klipper-style**: Trapezoidal velocity profiles with acceleration

### 4. **WindingController** (High-level logic)
- **Purpose**: Winding process state machine and move generation
- **Function**: `home_traverse()`, `move_to_start()`, `execute_winding()`
- **Integration**: Uses StepCompressor to generate moves for MoveQueue
- **Klipper-style**: High-level process control with move generation

### 5. **GCodeInterface** (Command processing)
- **Purpose**: Parse and execute G-code commands
- **Function**: `execute_g0_g1()`, `execute_g28()`, `execute_m17_m18()`
- **Integration**: Uses WindingController and MoveQueue for hardware control
- **Klipper-style**: G-code command processing with hardware integration

## Data Flow

```
Pi Zero (High-level) → UART → Pico (Low-level)
├── G-code commands
└── Pico main.cpp
    ├── GCodeInterface (parse commands)
    ├── WindingController (high-level logic)
    ├── StepCompressor (move generation)
    ├── MoveQueue (step queuing)
    └── Scheduler (ISR timing)
        └── Hardware (step pulses)
```

## Component Integration

### Main.cpp Initialization
```cpp
// Initialize move queue (Klipper-style)
move_queue = new MoveQueue();
move_queue->init();

// Initialize scheduler (Klipper-style)
scheduler = new Scheduler(move_queue);
scheduler->start(HEARTBEAT_US);

// Initialize winding controller (Klipper-style)
winding_controller = new WindingController(move_queue);
winding_controller->init();
```

### G-code Command Flow
```
G1 Y10 F50 → GCodeInterface::execute_g0_g1()
    ↓
WindingController::move_to_start()
    ↓
StepCompressor::compress_trapezoid()
    ↓
MoveQueue::push_chunk()
    ↓
Scheduler::handle_isr() → MoveQueue::handle_isr_tick()
    ↓
Hardware step pulses
```

## Key Differences from Manual Implementation

### ❌ **REMOVED (Manual/Duplicate)**
- **TraverseController::generate_steps()** - Manual step generation
- **Scheduler::scheduler_tick()** - Timer-based step generation
- **Duplicate Pi Zero winding controller** - Removed `winding_controller.py`

### ✅ **KEPT (Klipper-style)**
- **MoveQueue::axis_isr_handler()** - ISR-based step generation
- **Scheduler::handle_isr()** - ISR timing
- **StepCompressor** - Smooth movement generation
- **WindingController** - High-level process control

## Benefits of Klipper-Style Architecture

### 1. **Precise Timing**
- ISR-based step generation ensures precise timing
- No jitter from main loop delays
- Hardware timer guarantees step intervals

### 2. **Smooth Movement**
- StepCompressor generates smooth velocity profiles
- Trapezoidal acceleration/deceleration
- No step skipping or stuttering

### 3. **Efficient Queuing**
- MoveQueue handles step queuing efficiently
- Alternating axis processing balances load
- Fixed-size queues prevent memory issues

### 4. **Modular Design**
- Clear separation of concerns
- Each component has single responsibility
- Easy to debug and maintain

### 5. **Real-time Performance**
- ISR-based processing for real-time requirements
- No blocking operations in main loop
- Predictable timing behavior

## Testing

### Test Scripts
- **`test_klipper_architecture.py`** - Comprehensive architecture testing
- **`test_gcode_interface.py`** - G-code command testing
- **`test_winding_sequence.py`** - Complete winding process testing

### Test Coverage
- ✅ Scheduler ISR timing
- ✅ MoveQueue step generation
- ✅ WindingController high-level logic
- ✅ StepCompressor smooth movement
- ✅ G-code Interface command processing
- ✅ Hardware integration (spindle + traverse)

## Conclusion

The Klipper-style architecture provides:
- **Precise timing** through ISR-based step generation
- **Smooth movement** through StepCompressor
- **Efficient queuing** through MoveQueue
- **High-level control** through WindingController
- **G-code compatibility** through GCodeInterface

This implementation maintains the working Klipper methods while providing a clean, modular, and efficient architecture for the winding machine system.
