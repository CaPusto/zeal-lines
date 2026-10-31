
cmake_minimum_required(VERSION 3.16)

# Project configuration
set(target_name zeal_lines)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Zeal OS toolchain
set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

# Enable Zeal Video Board
zos_use(zvb)

# Initialize ZGDK
include($ENV{ZGDK_PATH}/cmake/zgdk_init.cmake)

project(Zeal-Lines C ASM)

# Application sources
add_executable(${target_name}
    src/main.c
)

# SDK dependencies
find_package(ZVB_SDK REQUIRED CONFIG
    PATHS "$ENV{ZVB_SDK_PATH}/cmake"
    NO_DEFAULT_PATH
)

find_package(ZGDK REQUIRED CONFIG
    PATHS "$ENV{ZGDK_PATH}/cmake"
    NO_DEFAULT_PATH
)

target_link_libraries(${target_name} PUBLIC
    zgdk
    zvb_gfx
)

# Generate Zeal OS output files
zos_add_outputs(${target_name})

