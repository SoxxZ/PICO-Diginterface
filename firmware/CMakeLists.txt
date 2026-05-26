cmake_minimum_required(VERSION 3.19)

include(pico_sdk_import.cmake)

project(PICO-Diginterface C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(PICO-Diginterface
    main.c
    usb_descriptors.c
    audio_pdm_output.c
    adc_input.c
    ptt.c
)

pico_generate_pio_header(PICO-Diginterface ${CMAKE_CURRENT_LIST_DIR}/pdm_output.pio)

target_include_directories(PICO-Diginterface PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(PICO-Diginterface
    pico_stdlib
    hardware_adc
    hardware_dma
    hardware_pio
    tinyusb_device
    tinyusb_board
)

# Patch tinyusb RP2040 DCD: replace with upstream master transfer engine (state-machine based)
# + bitmap DPRAM allocator (PR tinyusb#1802). Fixes ISO endpoint hangs on SET_INTERFACE.
# See: https://github.com/hathach/tinyusb/issues/3213
set(TINYUSB_PORT_DIR "${PICO_SDK_PATH}/lib/tinyusb/src/portable/raspberrypi/rp2040")

set(TINYUSB_DCD_FILE "${TINYUSB_PORT_DIR}/dcd_rp2040.c")
if(EXISTS "${TINYUSB_DCD_FILE}")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/patches/dcd_rp2040.c"
    "${TINYUSB_DCD_FILE}"
    COPYONLY
  )
  message(STATUS "Patched tinyusb dcd_rp2040.c with master transfer engine + bitmap allocator")
endif()

set(TINYUSB_USB_C_FILE "${TINYUSB_PORT_DIR}/rp2040_usb.c")
if(EXISTS "${TINYUSB_USB_C_FILE}")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/patches/rp2040_usb.c"
    "${TINYUSB_USB_C_FILE}"
    COPYONLY
  )
  message(STATUS "Patched tinyusb rp2040_usb.c with master transfer engine")
endif()

set(TINYUSB_USB_H_FILE "${TINYUSB_PORT_DIR}/rp2040_usb.h")
if(EXISTS "${TINYUSB_USB_H_FILE}")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/patches/rp2040_usb.h"
    "${TINYUSB_USB_H_FILE}"
    COPYONLY
  )
  message(STATUS "Patched tinyusb rp2040_usb.h with master struct + transfer_type field")
endif()

pico_add_extra_outputs(PICO-Diginterface)
