# Wiring Guide: Pi Zero ↔ SKR Pico

## UART Connection (3 wires only!)

```
Pi Zero 2 W GPIO Header      SKR Pico Debug Header
-----------------------      ---------------------
Pin 8  (GPIO 14 TX)     →    GPIO 1 (RX)
Pin 10 (GPIO 15 RX)     ←    GPIO 0 (TX)
Pin 6  (GND)            →    GND

⚠️ DO NOT CONNECT 5V WIRES!
   - Pi: Powered by USB (5V)
   - Pico: Powered by 24V PSU
   - ONLY connect: TX, RX, GND
```

## Power

- **Pi Zero**: USB power adapter (5V, 2A recommended)
- **SKR Pico**: 24V PSU (as normal for motors)
- **NEVER** cross-connect power supplies!

## Verification

After wiring, before power:
1. Check TX→RX crossover (not TX→TX!)
2. Check common GND connected
3. Verify NO 5V connections between boards
