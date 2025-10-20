cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pico_bldc_controller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_bldc_controller
    main.cpp
    src/bldc_speed_pulse.cpp
)

target_include_directories(pico_bldc_controller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(pico_bldc_controller
    pico_stdlib
    hardware_uart
    hardware_pwm
    hardware_gpio
)

pico_enable_stdio_usb(pico_bldc_controller 1)
pico_enable_stdio_uart(pico_bldc_controller 0)
pico_add_extra_outputs(pico_bldc_controller)
