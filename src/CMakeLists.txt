cmake_minimum_required(VERSION 3.13)

# Import the Raspberry Pi Pico SDK. This must come before project().
# Uses a pico_sdk_import.cmake placed next to this file if there is one (copy
# it from <pico-sdk>/external/), otherwise the SDK in PICO_SDK_PATH.
# The target board is not set here; select it with -DPICO_BOARD=... or in the
# Raspberry Pi Pico VS Code extension. The SDK default is the Pico.
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
    include(${CMAKE_CURRENT_LIST_DIR}/pico_sdk_import.cmake)
else()
    if (DEFINED PICO_SDK_PATH)
        set(PICO_SDK_IMPORT ${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
    elseif (DEFINED ENV{PICO_SDK_PATH})
        set(PICO_SDK_IMPORT $ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
    endif()
    if (NOT PICO_SDK_IMPORT OR NOT EXISTS ${PICO_SDK_IMPORT})
        message(FATAL_ERROR "Set PICO_SDK_PATH to the Raspberry Pi Pico SDK directory.")
    endif()
    include(${PICO_SDK_IMPORT})
endif()

project(pico_epaper_touch C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(pico_epaper_touch
    src/main.c

    board/status_led.c

    display/epd.c
    display/epd_hw.c
    display/gfx.c
    display/font5x7.c

    touch/tp370.c
    touch/touch.c

    ui/ui.c
    ui/menu.c

    demos/demo_touch_test.c
    demos/demo_keypad.c
    demos/demo_keyboard.c
    demos/demo_scribble.c
    demos/demo_gestures.c
    demos/demo_list.c
    demos/demo_tictactoe.c
    demos/demo_handwriting.c
    demos/demo_settings.c
    demos/demo_sleep.c
    demos/demo_info.c
    demos/demo_diag.c
    demos/demo_touch_accuracy.c
    demos/demo_bench.c

    demos/recog/recog.c
    demos/recog/recog_templates.c
)

# Headers are included relative to the project root, e.g. "display/epd.h".
target_include_directories(pico_epaper_touch PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(pico_epaper_touch
    pico_stdlib
    hardware_gpio
    hardware_i2c
    hardware_spi
)

# Status messages are printed over USB serial.
pico_enable_stdio_usb(pico_epaper_touch 1)
pico_enable_stdio_uart(pico_epaper_touch 0)

pico_add_extra_outputs(pico_epaper_touch)
