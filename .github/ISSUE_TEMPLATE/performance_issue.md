---
name: Performance Issue
about: Report performance problems or optimization requests
title: '[PERF] '
labels: 'performance'
assignees: ''

---

**Performance Issue Description**
A clear description of the performance problem.

**Current Performance**
- **Spindle RPM**: [e.g., 1000 RPM]
- **Traverse Speed**: [e.g., 50 mm/min]
- **Response Time**: [e.g., 2.5 seconds]
- **Memory Usage**: [e.g., 45MB]

**Expected Performance**
- **Target RPM**: [e.g., 2000 RPM]
- **Target Speed**: [e.g., 100 mm/min]
- **Target Response**: [e.g., 1.0 seconds]
- **Target Memory**: [e.g., 30MB]

**Hardware Configuration**
```
# Pi Zero specs
CPU: [e.g., ARM1176JZF-S 1GHz]
RAM: [e.g., 512MB]
Storage: [e.g., 32GB SD Card Class 10]

# SKR Pico specs
MCU: [e.g., RP2040 133MHz]
RAM: [e.g., 264KB]
Flash: [e.g., 2MB]
```

**Software Configuration**
```
# Python version
python --version

# Pico SDK version
# Check in pico_firmware/CMakeLists.txt

# Dependencies
pip list | grep -E "(pyserial|pytest)"
```

**Profiling Data**
```
# If you have profiling data, paste it here
# Use: python -m cProfile test_spindle.py
# Or: python -m memory_profiler test_spindle.py
```

**Steps to Reproduce**
1. Start the system
2. Run command: `python test_spindle.py`
3. Observe performance issue
4. Check system resources

**Additional Context**
- When did this start happening?
- Has it always been slow?
- Any recent changes?
- System load during issue?
