# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
set(USERHOME $ENV{HOME})

set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(rudder C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(
    rudder 
    main.c
    rudder.c 
    usb.c
    usb_descriptors.c 
)

pico_set_program_name(rudder "rudder")
pico_set_program_version(rudder "0.1")

# Add the standard library to the build
target_link_libraries(
    rudder
    pico_stdlib
    pico_unique_id
    hardware_i2c
    hardware_adc
    hardware_dma
    tinyusb_device
    tinyusb_board
    )

pico_enable_stdio_usb(rudder 0)
pico_enable_stdio_uart(rudder 1)

# Add the standard include files to the build
target_include_directories(rudder PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(rudder)

