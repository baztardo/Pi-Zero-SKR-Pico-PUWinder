# Comprehensive Implementation Plan
## Klipper-Style Winding Machine Improvements

---

## 🎯 Executive Summary

Based on analysis of `CODEBASE_ERRORS_FOUND.md`, `COMPREHENSIVE_ERROR_ANALYSIS.md`, and `KLIPPER_ARCHITECTURE.md`, this plan addresses the **4 critical improvements** needed to achieve true Klipper-level performance:

1. **Hardware Timer ISR** (Precision timing)
2. **Enhanced StepCompressor** (Smooth motion)
3. **Complete G1 Implementation** (Functional movement)
4. **Safety Features** (Robust operation)

---

## 📊 Current Status Assessment

### ✅ What's Working
- **Compilation**: All code compiles successfully
- **Basic Architecture**: Klipper-style components in place
- **Component Integration**: MoveQueue, Scheduler, StepCompressor connected
- **Error Handling**: Null checks and error propagation added
- **Memory Management**: Proper cleanup implemented

### ❌ What Needs Improvement

| Issue | Impact | Priority | Status |
|-------|--------|----------|--------|
| **Software Timer ISR** | Jitter, imprecise timing | 🔥 CRITICAL | In Progress |
| **Simple Bisect Algorithm** | 10% velocity spikes | 🔥 CRITICAL | Not Started |
| **Incomplete G1 Command** | No actual movement | 🔥 CRITICAL | Not Started |
| **Missing Safety Features** | Unsafe operation | ⚠️ HIGH | Not Started |

---

## 🔥 Phase 1: Critical Performance Fixes

### 1.1 Hardware Timer ISR Implementation

**Problem**: Currently using `add_repeating_timer_us()` software timer
**Goal**: Replace with RP2040 hardware timer alarm for precise timing

#### Implementation Steps

```cpp
// File: pico_firmware/src/scheduler.cpp

// STEP 1: Replace software timer with hardware alarm
bool Scheduler::start_hardware_timer(uint32_t frequency_hz) {
    // Calculate timer period in microseconds
    timer_period_us = 1000000 / frequency_hz;
    
    // Use hardware timer alarm 0
    hardware_alarm_claim(0);
    hardware_alarm_set_callback(0, hardware_timer_isr);
    
    // Set to 20kHz (50us period) for precise step timing
    hardware_alarm_set_target(0, time_us_64() + timer_period_us);
    
    running = true;
    return true;
}

// STEP 2: Implement hardware ISR callback
void Scheduler::hardware_timer_isr() {
    // Acknowledge alarm
    hw_clear_bits(&timer_hw->intr, 1u << 0);
    
    // Schedule next interrupt
    hardware_alarm_set_target(0, time_us_64() + g_scheduler_instance->timer_period_us);
    
    // Call move queue handler
    g_scheduler_instance->handle_isr();
}

// STEP 3: Add atomic operations for thread safety
void Scheduler::handle_isr() {
    // Atomic increment
    __atomic_add_fetch(&tick_count, 1, __ATOMIC_RELAXED);
    
    // Process move queues with critical section
    uint32_t save = save_and_disable_interrupts();
    if (move_queue) {
        move_queue->handle_isr_tick();
    }
    restore_interrupts(save);
}
```

**Testing Requirements**:
- Measure ISR jitter with oscilloscope (target: <1μs)
- Verify 20kHz timing accuracy
- Test under load (full queues, high step rates)
- Validate atomic operations prevent race conditions

**Expected Results**:
- ✅ Precise 20kHz timing (50μs period ±0.5μs)
- ✅ No jitter from main loop delays
- ✅ Consistent step pulse intervals
- ✅ Real-time performance guaranteed

---

### 1.2 Enhanced StepCompressor with Bisect Algorithm

**Problem**: Simple binary search causes velocity spikes
**Goal**: Implement Klipper's bisect algorithm with velocity spike detection

#### Implementation Steps

```cpp
// File: pico_firmware/src/stepcompress.cpp

// STEP 1: Implement Klipper-style bisect algorithm
StepChunk StepCompressor::compress_bisect_add(
    const std::vector<uint64_t>& times,
    size_t start,
    size_t end,
    double max_err_us)
{
    StepChunk result{0, 0, 0};
    if (end <= start) return result;
    
    // Use least-squares fitting instead of simple midpoint
    size_t best_end = start + 1;
    double best_error = 1e9;
    
    // Binary search for optimal chunk size
    size_t left = start + 1;
    size_t right = std::min(end, start + 64); // Max 64 steps per chunk
    
    while (left <= right) {
        size_t mid = (left + right) / 2;
        
        uint32_t interval;
        int32_t add;
        double max_error;
        
        if (fit_chunk_least_squares(times, start, mid, interval, add, max_error)) {
            if (max_error < max_err_us && max_error < best_error) {
                best_error = max_error;
                best_end = mid;
                result.interval_us = interval;
                result.add_us = add;
                result.count = mid - start;
                
                // Try larger chunk
                left = mid + 1;
            } else {
                // Error too large, try smaller chunk
                right = mid - 1;
            }
        } else {
            right = mid - 1;
        }
    }
    
    return result;
}

// STEP 2: Implement least-squares fitting
bool StepCompressor::fit_chunk_least_squares(
    const std::vector<uint64_t>& times,
    size_t start, size_t end,
    uint32_t& interval, int32_t& add, double& max_error)
{
    size_t N = end - start;
    if (N < 1) return false;
    
    // Model: time[k] = t0 + interval*k + add*k*(k-1)/2
    // Solve using normal equations for least squares
    
    double S11 = 0, S12 = 0, S22 = 0, Sy1 = 0, Sy2 = 0;
    uint64_t t0 = (start == 0) ? 0 : times[start - 1];
    
    for (size_t i = 0; i < N; i++) {
        double k = (double)(i + 1);
        double x1 = k;
        double x2 = k * (k - 1.0) / 2.0;
        double y = (double)((int64_t)times[start + i] - (int64_t)t0);
        
        S11 += x1 * x1;
        S12 += x1 * x2;
        S22 += x2 * x2;
        Sy1 += x1 * y;
        Sy2 += x2 * y;
    }
    
    // Solve 2x2 system
    double det = S11 * S22 - S12 * S12;
    if (fabs(det) < 1e-12) return false;
    
    double interval_f = (S22 * Sy1 - S12 * Sy2) / det;
    double add_f = (-S12 * Sy1 + S11 * Sy2) / det;
    
    // Calculate maximum error
    max_error = 0.0;
    for (size_t i = 0; i < N; i++) {
        double k = (double)(i + 1);
        double predicted = (double)t0 + interval_f * k + add_f * k * (k - 1.0) / 2.0;
        double error = fabs((double)times[start + i] - predicted);
        max_error = std::max(max_error, error);
    }
    
    interval = (uint32_t)llround(interval_f);
    add = (int32_t)llround(add_f);
    
    return true;
}

// STEP 3: Add velocity spike detection
bool StepCompressor::detect_velocity_spikes(
    const std::vector<StepChunk>& chunks,
    double max_spike_sps)
{
    if (chunks.size() < 2) return true;
    
    for (size_t i = 1; i < chunks.size(); i++) {
        const auto& prev = chunks[i-1];
        const auto& curr = chunks[i];
        
        // Calculate velocity at chunk boundary
        double v_prev_end = 1e6 / (prev.interval_us + prev.add_us * (prev.count - 1));
        double v_curr_start = 1e6 / curr.interval_us;
        
        double spike = fabs(v_curr_start - v_prev_end);
        
        if (spike > max_spike_sps) {
            printf("WARNING: Velocity spike %.1f steps/sec between chunks\n", spike);
            return false;
        }
    }
    
    return true;
}

// STEP 4: Optimize chunk boundaries
void StepCompressor::optimize_chunk_boundaries(
    std::vector<StepChunk>& chunks,
    double max_spike_sps)
{
    // Iteratively adjust chunk boundaries to minimize spikes
    bool improved = true;
    int iterations = 0;
    
    while (improved && iterations < 10) {
        improved = false;
        iterations++;
        
        for (size_t i = 1; i < chunks.size(); i++) {
            auto& prev = chunks[i-1];
            auto& curr = chunks[i];
            
            // Try moving boundary by ±1 step
            if (prev.count > 1) {
                // Try moving one step from prev to curr
                prev.count--;
                curr.count++;
                curr.interval_us -= prev.add_us; // Adjust start
                
                if (detect_velocity_spikes(chunks, max_spike_sps)) {
                    improved = true;
                } else {
                    // Revert
                    prev.count++;
                    curr.count--;
                    curr.interval_us += prev.add_us;
                }
            }
        }
    }
}
```

**Testing Requirements**:
- Generate test moves with varying velocities
- Measure velocity spikes at chunk boundaries
- Compare before/after optimization
- Verify timing accuracy remains <20μs

**Expected Results**:
- ✅ Velocity spikes reduced from ~10% to <1%
- ✅ Smoother motion profiles
- ✅ Better step timing accuracy
- ✅ Optimized chunk boundaries

---

### 1.3 Complete G1 Command Implementation

**Problem**: G1 commands don't actually move the traverse
**Goal**: Connect G1 to StepCompressor and MoveQueue

#### Implementation Steps

```cpp
// File: pico_firmware/src/gcode_interface.cpp

bool GCodeInterface::execute_g0_g1(const char* command) {
    double target_y = current_y;
    double feedrate = current_feedrate;
    
    // Parse parameters
    parse_parameters(command, "YF", &target_y, &feedrate);
    
    // Calculate move parameters
    double distance_mm = fabs(target_y - current_y);
    double velocity_mms = feedrate / 60.0; // Convert mm/min to mm/s
    
    // Convert to steps (Y_STEPS_PER_MM from config.h)
    uint32_t total_steps = (uint32_t)(distance_mm * Y_STEPS_PER_MM);
    double start_velocity_sps = 0.0; // Start from rest
    double cruise_velocity_sps = velocity_mms * Y_STEPS_PER_MM;
    double accel_spss = Y_MAX_ACCEL * Y_STEPS_PER_MM; // steps/s²
    
    // Generate step chunks using StepCompressor
    std::vector<StepChunk> chunks;
    StepCompressor::compress_trapezoid_into(
        chunks,
        total_steps,
        start_velocity_sps,
        cruise_velocity_sps,
        accel_spss
    );
    
    // Detect and report velocity spikes
    if (!StepCompressor::detect_velocity_spikes(chunks, 100.0)) {
        printf("WARNING: Velocity spikes detected, optimizing...\n");
        StepCompressor::optimize_chunk_boundaries(chunks, 100.0);
    }
    
    // Set traverse direction
    bool dir = (target_y > current_y);
    winding_controller->set_traverse_direction(dir);
    
    // Push chunks to move queue
    for (const auto& chunk : chunks) {
        if (!move_queue->push_chunk(AXIS_TRAVERSE, chunk)) {
            set_error("Move queue full");
            return false;
        }
    }
    
    // Update current position
    current_y = target_y;
    
    printf("✓ G1 Y%.3f F%.1f - Generated %zu chunks, %u total steps\n",
           target_y, feedrate, chunks.size(), total_steps);
    
    return true;
}
```

**Testing Requirements**:
- Test short moves (1mm, 10mm)
- Test long moves (100mm, full travel)
- Test various feedrates (10-200 mm/min)
- Verify actual hardware movement
- Measure position accuracy

**Expected Results**:
- ✅ Traverse moves to commanded position
- ✅ Smooth acceleration/deceleration
- ✅ Accurate feedrate control
- ✅ Position accuracy ±0.1mm

---

### 1.4 Safety Features Implementation

**Problem**: No endstop monitoring, emergency stop, or stall detection
**Goal**: Add comprehensive safety features

#### Implementation Steps

```cpp
// File: pico_firmware/src/safety_monitor.h

class SafetyMonitor {
public:
    void init();
    void check_safety(); // Called from ISR
    
    bool is_endstop_triggered(uint8_t axis);
    void trigger_emergency_stop();
    bool is_stall_detected(uint8_t axis);
    
private:
    volatile bool emergency_stop_triggered;
    uint32_t last_step_time[2]; // spindle, traverse
    uint32_t stall_threshold_us;
};

// File: pico_firmware/src/safety_monitor.cpp

void SafetyMonitor::check_safety() {
    // Check endstops (active low)
    if (!gpio_get(Y_MIN_ENDSTOP_PIN)) {
        printf("EMERGENCY: Y-MIN endstop triggered!\n");
        trigger_emergency_stop();
    }
    
    if (!gpio_get(Y_MAX_ENDSTOP_PIN)) {
        printf("EMERGENCY: Y-MAX endstop triggered!\n");
        trigger_emergency_stop();
    }
    
    // Check for stalls (no steps for >1 second)
    uint64_t now = time_us_64();
    for (uint8_t axis = 0; axis < 2; axis++) {
        if (now - last_step_time[axis] > stall_threshold_us) {
            if (move_queue->is_active(axis)) {
                printf("WARNING: Stall detected on axis %d\n", axis);
                // Could trigger emergency stop or just warn
            }
        }
    }
}

void SafetyMonitor::trigger_emergency_stop() {
    emergency_stop_triggered = true;
    
    // Immediately disable steppers
    move_queue->set_enable(AXIS_SPINDLE, false);
    move_queue->set_enable(AXIS_TRAVERSE, false);
    
    // Clear all queues
    move_queue->clear_queue(AXIS_SPINDLE);
    move_queue->clear_queue(AXIS_TRAVERSE);
    
    // Stop spindle motor
    spindle_controller->disable();
    
    printf("🛑 EMERGENCY STOP TRIGGERED 🛑\n");
}

// Integrate into Scheduler ISR
void Scheduler::handle_isr() {
    tick_count++;
    
    // Check safety FIRST before processing moves
    if (safety_monitor) {
        safety_monitor->check_safety();
    }
    
    // Only process moves if no emergency stop
    if (!safety_monitor->is_emergency_stop_triggered()) {
        if (move_queue) {
            move_queue->handle_isr_tick();
        }
    }
}
```

**Testing Requirements**:
- Test endstop triggering (both min/max)
- Test emergency stop button
- Test stall detection
- Verify immediate stop response
- Test recovery after emergency stop

**Expected Results**:
- ✅ Endstops trigger immediate stop
- ✅ Emergency stop within <10ms
- ✅ Stall detection works reliably
- ✅ Safe recovery after stop

---

## ⚡ Phase 2: Performance Optimization

### 2.1 Priority-Based Move Queue

**Current**: Simple alternating between axes
**Improvement**: Priority-based processing with deadlines

```cpp
enum MovePriority {
    PRIORITY_URGENT = 0,   // Emergency moves
    PRIORITY_HIGH = 1,     // Time-critical moves
    PRIORITY_NORMAL = 2,   // Regular moves
    PRIORITY_LOW = 3       // Background moves
};

struct PriorityChunk {
    StepChunk chunk;
    MovePriority priority;
    uint32_t deadline_us; // Time by which move must start
};

bool MoveQueue::push_chunk_priority(
    uint8_t axis,
    const StepChunk& chunk,
    MovePriority priority,
    uint32_t deadline_us)
{
    // Insert into priority queue instead of FIFO
    // Process urgent moves first, then by deadline
}
```

### 2.2 Token-Based G-code Parser

**Current**: String comparison with `strncmp()`
**Improvement**: Lookup table with token-based parsing

```cpp
enum GCodeToken {
    TOKEN_G0, TOKEN_G1, TOKEN_G28,
    TOKEN_M17, TOKEN_M18, TOKEN_M84,
    // ... etc
};

static const std::unordered_map<std::string, GCodeToken> command_lookup = {
    {"G0", TOKEN_G0},
    {"G1", TOKEN_G1},
    {"G28", TOKEN_G28},
    // ... etc
};

bool parse_command(const char* command, GCodeToken& token) {
    // Extract command (first 2-3 chars)
    // Lookup in table (O(1) instead of O(n))
    // Return token for switch statement
}
```

### 2.3 Queue Statistics & Monitoring

```cpp
struct QueueStats {
    uint32_t total_chunks_processed;
    uint32_t queue_overruns;
    uint32_t deadline_violations;
    uint32_t max_queue_depth;
    double average_queue_depth;
};

QueueStats get_queue_stats(uint8_t axis);
```

---

## 🚀 Phase 3: Advanced Features

### 3.1 Input Shaping
- Vibration reduction for smoother winding
- ZV, MZV, or EI shaping algorithms

### 3.2 Adaptive Feedrate
- Adjust speed based on wire tension
- Prevent wire breakage

### 3.3 Enhanced Homing
- Multi-point homing for calibration
- Automatic tension calibration

---

## 📋 Testing Strategy

### Unit Tests
- [ ] StepCompressor chunk generation
- [ ] Velocity spike detection
- [ ] Hardware timer timing accuracy
- [ ] G-code parameter parsing
- [ ] Safety monitor endstop detection

### Integration Tests
- [ ] Complete G1 move sequence
- [ ] Winding cycle with G-code
- [ ] Emergency stop during movement
- [ ] Queue overflow handling
- [ ] Multi-axis coordination

### Performance Tests
- [ ] ISR timing jitter measurement
- [ ] Velocity spike analysis
- [ ] Step pulse timing accuracy
- [ ] G-code parsing speed
- [ ] Queue processing latency

### Hardware Tests
- [ ] Actual traverse movement
- [ ] Spindle synchronization
- [ ] Endstop triggering
- [ ] Position accuracy verification
- [ ] Long-duration reliability

---

## 🎯 Success Metrics

| Metric | Current | Target | Test Method |
|--------|---------|--------|-------------|
| **ISR Jitter** | ±10μs | <1μs | Oscilloscope |
| **Velocity Spikes** | ~10% | <1% | Motion analysis |
| **Position Accuracy** | Unknown | ±0.1mm | Dial indicator |
| **Emergency Stop Time** | N/A | <10ms | Timing test |
| **G-code Latency** | Unknown | <1ms | Profiling |
| **Queue Depth** | Unknown | <90% | Statistics |

---

## 📅 Implementation Timeline

### Week 1: Hardware Timer ISR
- Days 1-2: Implement hardware timer
- Days 3-4: Test timing accuracy
- Days 5-6: Integrate with move queue
- Day 7: Validation and debugging

### Week 2: Enhanced StepCompressor
- Days 1-3: Implement bisect algorithm
- Days 4-5: Add velocity spike detection
- Days 6-7: Optimize chunk boundaries

### Week 3: Complete G1 & Safety
- Days 1-3: Implement G1 movement
- Days 4-5: Add safety monitoring
- Days 6-7: Integration testing

### Week 4: Optimization & Testing
- Days 1-3: Priority queue & parser
- Days 4-5: Performance testing
- Days 6-7: Documentation & cleanup

---

## 🔧 Development Tools

### Required Hardware
- Oscilloscope (measure ISR timing)
- Logic analyzer (debug step pulses)
- Dial indicator (position accuracy)
- Power supply (testing)

### Software Tools
- Serial monitor (debugging)
- Profiler (performance analysis)
- Unit test framework
- CI/CD pipeline

---

## 📚 Key Files to Modify

| File | Changes | Priority |
|------|---------|----------|
| `scheduler.cpp` | Hardware timer ISR | 🔥 CRITICAL |
| `stepcompress.cpp` | Bisect algorithm | 🔥 CRITICAL |
| `gcode_interface.cpp` | Complete G1 | 🔥 CRITICAL |
| `safety_monitor.cpp` | Safety features | ⚠️ HIGH |
| `move_queue.cpp` | Priority queue | ⚡ MEDIUM |
| `gcode_parser.cpp` | Token-based parser | ⚡ MEDIUM |

---

## ✅ Acceptance Criteria

### Phase 1 (Critical)
- [ ] Hardware timer ISR working with <1μs jitter
- [ ] Velocity spikes reduced to <1%
- [ ] G1 commands move traverse accurately
- [ ] Safety features prevent crashes

### Phase 2 (Optimization)
- [ ] Priority queue improves responsiveness
- [ ] G-code parsing <1ms latency
- [ ] Queue statistics available
- [ ] Performance metrics logged

### Phase 3 (Advanced)
- [ ] Input shaping reduces vibration
- [ ] Adaptive feedrate works
- [ ] Enhanced homing calibrates system

---

## 🎓 Resources & References

1. **Klipper Source Code**
   - `klippy/chelper/stepcompress.c` - Bisect algorithm
   - `klippy/mcu.py` - Move queue handling
   - `klippy/gcode.py` - G-code parsing

2. **RP2040 Documentation**
   - Hardware timer alarms
   - Atomic operations
   - Interrupt handling

3. **Testing Resources**
   - Step timing analysis
   - Motion profiling
   - Safety validation

---

## 📞 Support & Escalation

### Issue Triage
- **CRITICAL**: Stops compilation or causes crashes → Fix immediately
- **HIGH**: Affects functionality or performance → Fix within 1 week
- **MEDIUM**: Optimization or enhancement → Fix within 1 month
- **LOW**: Nice-to-have features → Backlog

### Known Limitations
- Maximum step rate: 20kHz (hardware limit)
- Queue depth: 64 chunks per axis
- G-code buffer: 256 characters
- Endstop debounce: 1ms

---

## 🏁 Conclusion

This comprehensive plan addresses all identified issues from the codebase analysis:

1. **Hardware Timer ISR** - Eliminates jitter, enables precise timing
2. **Enhanced StepCompressor** - Reduces velocity spikes, smooths motion
3. **Complete G1 Implementation** - Enables actual traverse movement
4. **Safety Features** - Prevents hardware damage and ensures safe operation

Following this plan will result in a production-ready, Klipper-level winding machine controller with:
- ✅ Precise timing (<1μs jitter)
- ✅ Smooth motion (<1% velocity spikes)
- ✅ Full G-code support
- ✅ Comprehensive safety features
- ✅ Real-time performance guarantees

**Ready to implement? Let's start with Phase 1.1: Hardware Timer ISR!** 🚀
