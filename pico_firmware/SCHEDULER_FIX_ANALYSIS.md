# SCHEDULER ISR ANALYSIS - Root Cause Found

## The Problem
ISR is only called ONCE, then never again. Queue fills up, traverse doesn't move.

## Root Cause
**MIXING TIMER APIs** - The scheduler uses Pico SDK's `hardware_alarm_set_target()` to start the timer, but then tries to manually re-arm it using raw hardware registers `timer_hw->alarm[alarm_num] = timer_hw->timerawl + 50`.

This causes conflicts because:
1. SDK expects to manage the alarm via its own functions
2. Direct register writes bypass SDK's internal tracking
3. The alarm doesn't re-arm correctly after the first interrupt

## Evidence from Code
```cpp
// Line 70: Start using SDK
hardware_alarm_set_target(0, delayed_by_us(get_absolute_time(), 50));

// Line 25: Try to re-arm using raw registers  
timer_hw->alarm[alarm_num] = timer_hw->timerawl + 50;  // ❌ DOESN'T WORK
```

## Solution Options

### Option 1: Pure SDK Approach (RECOMMENDED)
Use `hardware_alarm_set_target()` consistently:

```cpp
void scheduler_alarm_callback(uint alarm_num) {
    if (g_scheduler_instance) {
        g_scheduler_instance->handle_isr();
    }
    // Re-arm using SDK
    hardware_alarm_set_target(alarm_num, delayed_by_us(get_absolute_time(), 50));
}
```

**Pros:** Clean, SDK-managed, reliable
**Cons:** Slight overhead from SDK functions

### Option 2: Pure Hardware Approach  
Use raw register access consistently:

```cpp
void scheduler_alarm_callback(uint alarm_num) {
    // Clear interrupt FIRST
    hw_clear_bits(&timer_hw->intr, 1u << alarm_num);
    
    if (g_scheduler_instance) {
        g_scheduler_instance->handle_isr();
    }
    
    // Re-arm using hardware registers
    timer_hw->alarm[alarm_num] = time_us_32() + 50;  // Use current time, not timerawl
}

// Start also needs to use raw registers
bool Scheduler::start(uint32_t interval) {
    hardware_alarm_claim(0);
    hardware_alarm_set_callback(0, scheduler_alarm_callback);
    
    // Enable alarm using raw registers
    timer_hw->alarm[0] = time_us_32() + 50;
    hw_set_bits(&timer_hw->inte, 1u << 0);  // Enable interrupt
    
    running = true;
    return true;
}
```

**Pros:** Maximum performance, no SDK overhead
**Cons:** More complex, bypasses SDK safety

### Option 3: Use add_repeating_timer (SIMPLEST)
```cpp
static repeating_timer_t timer;

bool Scheduler::start(uint32_t interval) {
    return add_repeating_timer_us(-50, [](repeating_timer_t *rt) {
        if (g_scheduler_instance) {
            g_scheduler_instance->handle_isr();
        }
        return true;  // Keep repeating
    }, nullptr, &timer);
}
```

**Pros:** SDK handles everything, guaranteed to repeat
**Cons:** Cannot use lambda directly (needs proper callback)

## Recommended Fix
Use **Option 1** (Pure SDK) - it's the most reliable and maintainable approach.

## Implementation
See scheduler_CLEAN.cpp for the complete working implementation.

