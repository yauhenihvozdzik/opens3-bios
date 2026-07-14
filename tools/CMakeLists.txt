cmake_minimum_required(VERSION 3.16)
project(OPENS3_Tools)

# Common settings for C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Determine the repository root directory (one level up from the tools/ directory)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Automatically define the independent path to the ABI headers
set(ABI_INCLUDE_DIR "${PROJECT_ROOT}/components/boot_manager/include")

# ==============================================================================
# 1. HOST UTILITY COMPILATION (OPENS3_loader for PC)
# ==============================================================================
add_executable(OPENS3_loader OPENS3_loader.cpp)

# Post-build command to copy the compiled loader to the root of the tools/ directory
add_custom_command(TARGET OPENS3_loader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:OPENS3_loader> ${CMAKE_CURRENT_SOURCE_DIR}/
    COMMENT "Copied OPENS3_loader to tools/ directory."
)

# ==============================================================================
# 2. TARGET PAYLOAD COMPILATION (for ESP32-C6 RISC-V)
# ==============================================================================

# Search for the ESP-IDF Xtensa cross-compiler for ESP32-S3
find_program(XTENSA_GCC xtensa-esp32s3-elf-gcc)
find_program(XTENSA_OBJCOPY xtensa-esp32s3-elf-objcopy)

if(XTENSA_GCC AND XTENSA_OBJCOPY)
    message(STATUS "Found ESP-IDF Xtensa Toolchain: ${XTENSA_GCC}")
    message(STATUS "Using ABI Include Path: ${ABI_INCLUDE_DIR}")

    set(PAYLOAD_SRC "${CMAKE_CURRENT_SOURCE_DIR}/example/payload.c")
    set(PAYLOAD_LD  "${CMAKE_CURRENT_SOURCE_DIR}/payload.ld")
    set(PAYLOAD_ELF "${CMAKE_CURRENT_BINARY_DIR}/payload.elf")
    set(PAYLOAD_BIN "${CMAKE_CURRENT_SOURCE_DIR}/payload.bin")

    # Compile bare-metal Xtensa payload for ESP32-S3
    add_custom_command(
        OUTPUT ${PAYLOAD_BIN}
        COMMAND ${XTENSA_GCC} -mlongcalls -Os -fPIC -fno-jump-tables -nostdlib
                             -I${ABI_INCLUDE_DIR}
                             -Wl,-T,${PAYLOAD_LD}
                             -o ${PAYLOAD_ELF} ${PAYLOAD_SRC}
        COMMAND ${XTENSA_OBJCOPY} -O binary ${PAYLOAD_ELF} ${PAYLOAD_BIN}
        DEPENDS ${PAYLOAD_SRC} ${PAYLOAD_LD}
        COMMENT "Compiling bare-metal Xtensa Payload for OPENS3 BIOS..."
    )

    add_custom_target(payload ALL DEPENDS ${PAYLOAD_BIN})
else()
    message(WARNING "Xtensa ESP32-S3 Toolchain NOT FOUND! Run ESP-IDF export.sh first!")
endif()
