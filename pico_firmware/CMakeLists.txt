# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# --- Import Pico SDK ---
if (EXISTS $ENV{HOME}/.pico-sdk/sdk/2.2.0/external/pico_sdk_import.cmake)
    include($ENV{HOME}/.pico-sdk/sdk/2.2.0/external/pico_sdk_import.cmake)
elseif(DEFINED ENV{PICO_SDK_PATH})
    include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
else()
    message(FATAL_ERROR "Pico SDK import not found.")
endif()

project(pico_spindle_controller C CXX ASM)

pico_sdk_init()

add_executable(pico_spindle_controller
    main.cpp
    src/spindle.cpp
    src/BLDCRPMSensor.cpp
    src/move_queue.cpp
    src/scheduler.cpp
    src/stepcompress.cpp
    src/tmc2209.cpp
    src/winding_controller.cpp
    src/traverse_controller.cpp
    src/gcode_interface.cpp
)

target_include_directories(pico_spindle_controller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(pico_spindle_controller
    pico_stdlib
    hardware_uart
    hardware_pwm
    hardware_gpio
    hardware_timer
    hardware_irq
)

pico_enable_stdio_usb(pico_spindle_controller 1)
pico_enable_stdio_uart(pico_spindle_controller 0)

pico_add_extra_outputs(pico_spindle_controller)