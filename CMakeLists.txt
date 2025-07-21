# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(main C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(main main.c
        lib/aht20.c
        lib/bmp280.c
        lib/ssd1306.c
        lib/buzzer.c
        lib/matrizRGB.c
        lib/leds.c
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
pico_set_program_name(main "main")
pico_set_program_version(main "0.1")
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(main 1)
pico_enable_stdio_usb(main 1)

# Add the standard library to the buildn
target_link_libraries(main
        pico_stdlib
        hardware_i2c
        hardware_adc
        hardware_pwm
        pico_cyw43_arch_lwip_threadsafe_background)

# Add the standard include files to the build
target_include_directories(main PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Add any user requested libraries
target_link_libraries(main
)

pico_add_extra_outputs(main)
