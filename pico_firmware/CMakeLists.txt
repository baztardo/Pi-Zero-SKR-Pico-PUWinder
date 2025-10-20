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
# Prefer VSCode-provided path; otherwise try PICO_SDK_PATH or local file
if (EXISTS $ENV{HOME}/.pico-sdk/sdk/2.2.0/external/pico_sdk_import.cmake)
    include($ENV{HOME}/.pico-sdk/sdk/2.2.0/external/pico_sdk_import.cmake)
elseif(DEFINED ENV{PICO_SDK_PATH})
    include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
else()
    message(FATAL_ERROR "Pico SDK import not found. Set PICO_SDK_PATH or install VSCode Pico SDK.")
endif()

project(pico_uart_test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_uart_test
    main.cpp
)

target_link_libraries(pico_uart_test
    pico_stdlib
    hardware_uart
)

pico_enable_stdio_usb(pico_uart_test 1)
pico_enable_stdio_uart(pico_uart_test 0)
pico_add_extra_outputs(pico_uart_test)
