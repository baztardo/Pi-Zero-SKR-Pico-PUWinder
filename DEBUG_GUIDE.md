# Pico Probe Debugging Guide

## Hardware Setup

### SWD Connections
Connect your Pico Probe to the target Pico:

| Pico Probe | Target Pico | Function |
|------------|-------------|----------|
| SWDIO      | GPIO 2      | Serial Wire Data I/O |
| SWCLK      | GPIO 3      | Serial Wire Clock |
| GND        | GND         | Ground |
| 3.3V       | 3.3V        | Power (optional) |

**Note:** UART0 (GPIO 0,1) remains connected to Pi Zero for communication.

## Software Setup

### Terminal 1: Start OpenOCD
```bash
./debug_pico.sh
```

### Terminal 2: Start GDB
```bash
cd pico_firmware/build
arm-none-eabi-gdb -x ../../debug_commands.gdb pico_spindle_controller.elf
```

## Debugging Current Issues

### 1. WIND Command Not Responding

**Set breakpoints:**
```gdb
(gdb) break parse_command
(gdb) break execute_wind
(gdb) break send_response
```

**Test sequence:**
1. Send WIND command from Pi Zero
2. Check if `parse_command` is called
3. Check if `execute_wind` is called
4. Check if `send_response` is called

**Debug commands:**
```gdb
(gdb) print_status
(gdb) print_uart_buffer
(gdb) info locals
```

### 2. RPM Not Updating

**Set breakpoints:**
```gdb
(gdb) break handle_pulse
(gdb) break calculate_rpm
(gdb) break get_rpm
```

**Test sequence:**
1. Start spindle motor
2. Check if `handle_pulse` is called (hall sensor ISR)
3. Check if `calculate_rpm` is called
4. Check `filtered_rpm` value

**Debug commands:**
```gdb
(gdb) print_spindle
(gdb) info registers
```

### 3. Homing Not Working

**Set breakpoints:**
```gdb
(gdb) break home
(gdb) break generate_steps
(gdb) break gpio_put
```

**Test sequence:**
1. Send G28 command
2. Check if `home` is called
3. Check if `generate_steps` is called
4. Check if GPIO pins are being toggled

**Debug commands:**
```gdb
(gdb) print_traverse
(gdb) monitor gpio readall
```

### 4. UART Communication Issues

**Set breakpoints:**
```gdb
(gdb) break uart_putc
(gdb) break uart_getc
(gdb) break process_command
```

**Test sequence:**
1. Send any command from Pi Zero
2. Check if `uart_getc` receives data
3. Check if `process_command` is called
4. Check if `uart_putc` sends response

## Useful GDB Commands

### Basic Commands
- `continue` - Continue execution
- `step` - Step one instruction
- `next` - Step one line
- `finish` - Run until function returns
- `break function_name` - Set breakpoint
- `delete breakpoint_number` - Remove breakpoint
- `info breakpoints` - List all breakpoints

### Variable Inspection
- `print variable_name` - Print variable value
- `info locals` - Show local variables
- `info args` - Show function arguments
- `x/10x &array` - Examine memory as hex
- `x/10s &string` - Examine memory as string

### Custom Commands (defined in debug_commands.gdb)
- `print_status` - Show system status
- `print_uart_buffer` - Show UART buffer
- `print_spindle` - Show spindle status
- `print_traverse` - Show traverse status

## Common Issues and Solutions

### Issue: "No symbol table loaded"
**Solution:** Make sure to load the .elf file:
```gdb
(gdb) file pico_spindle_controller.elf
(gdb) load
```

### Issue: "Cannot access memory"
**Solution:** The target might be running. Halt it first:
```gdb
(gdb) monitor reset halt
```

### Issue: Breakpoints not working
**Solution:** Make sure the firmware is loaded and running:
```gdb
(gdb) load
(gdb) continue
```

## Debugging Workflow

1. **Start OpenOCD** in Terminal 1
2. **Start GDB** in Terminal 2
3. **Set relevant breakpoints** for the issue you're debugging
4. **Continue execution** and trigger the issue
5. **Inspect variables** when breakpoint hits
6. **Step through code** to find the problem
7. **Fix the code** and rebuild
8. **Repeat** until issue is resolved

## Tips

- Use `monitor reset halt` to restart the target
- Use `load` to reload the firmware after changes
- Use `info breakpoints` to see all active breakpoints
- Use `delete` to remove unwanted breakpoints
- Use `continue` to resume execution after inspection
