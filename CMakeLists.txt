cmake_minimum_required(VERSION 3.17)
if(NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if(NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Set PICO_SDK_PATH to an installed Pico SDK with TinyUSB submodule")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(PicoGMMidi C CXX ASM)
# Fixed 250 MHz runtime clock: keep Flash at 31.25 MHz (also applies to boot2).
add_compile_definitions(PICO_FLASH_SPI_CLKDIV=8)

# SDK and language settings.
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
if(NOT PICO_RP2040)
    message(FATAL_ERROR "This project currently targets RP2040 Pico only")
endif()

# Use a separate build directory per board. Pin settings live in BoardConfig.h.
if(NOT PICO_BOARD STREQUAL "pico" AND NOT PICO_BOARD STREQUAL "waveshare_rp2040_zero")
    message(FATAL_ERROR "Supported boards: pico, waveshare_rp2040_zero")
endif()

# Firmware sources.
add_executable(PicoGMMidi
    source/main.cpp source/usb_descriptors.cpp
    source/MidiParser.cpp source/Synth.cpp source/SoundBank.cpp source/SynthSamples.cpp
    source/Cmu800Tone.cpp source/Cmu800Envelope.cpp source/PsgDrum.cpp
    source/BoardControls.cpp source/ModeStorage.cpp)
# Keep the final two 4 KiB sectors out of the executable and UF2 payload.
set(PICO_FLASH_SIZE_BYTES_STRING "2088960")
configure_file(${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/pico_flash_region.template.ld
    ${CMAKE_CURRENT_BINARY_DIR}/PicoGMMidi/pico_flash_region.ld)
pico_add_linker_script_override_path(PicoGMMidi ${CMAKE_CURRENT_BINARY_DIR}/PicoGMMidi FILES pico_flash_region.ld)
# Keep the physical 2 MiB SDK limit: hardware_flash must still access settings.
# Generate for both boards; BoardConfig.h controls whether the source uses it.
pico_generate_pio_header(PicoGMMidi ${CMAKE_CURRENT_LIST_DIR}/source/mode_led.pio)
target_include_directories(PicoGMMidi PRIVATE source)
target_compile_options(PicoGMMidi PRIVATE -O3 -Wall -Wextra -Werror)
target_link_libraries(PicoGMMidi PRIVATE pico_stdlib pico_multicore pico_unique_id
    hardware_pwm hardware_dma hardware_clocks hardware_pio hardware_flash hardware_vreg tinyusb_device)
pico_enable_stdio_usb(PicoGMMidi 0)
pico_enable_stdio_uart(PicoGMMidi 0)
pico_add_extra_outputs(PicoGMMidi)

