cmake_minimum_required(VERSION 3.13)

# Pico SDK のインポート（環境変数に依存）
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(PicoBeepMidi_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico)
set(PICO_ENABLE_TINYUSB 1)
pico_sdk_init()

add_executable(PicoBeepMidi
    ./source/main.cpp
    ./source/usb_descriptors.cpp
    ./source/SquareWave.cpp
    ./source/SinWave.cpp
    ./source/NoiseDrum.cpp
)

# PIOヘッダー生成用に正しいパスを指定
# pico_generate_pio_header(PicoBeepMidi ${CMAKE_CURRENT_LIST_DIR}/source/ws2812.pio)

target_link_libraries(PicoBeepMidi
    pico_stdlib
    pico_multicore
    hardware_pwm
    tinyusb_device
    tinyusb_board
)

target_include_directories(PicoBeepMidi PRIVATE ${CMAKE_CURRENT_LIST_DIR}/source)

pico_enable_stdio_usb(PicoBeepMidi 0)
pico_enable_stdio_uart(PicoBeepMidi 1)

pico_add_extra_outputs(PicoBeepMidi)
