
cmake_minimum_required(VERSION 3.16)

# Project configuration
set(target_name zeal-lines)

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
    src/game.c
    src/input.c
    src/entropy.c
    src/render.c
    src/play.c
    src/save.c
    src/sfx.c
    src/scores.c
    src/gameover.c
    src/splash.c
    src/assets_generated.c
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
    zvb_sound
)

# Generate Zeal OS output files
zos_add_outputs(${target_name})

# Copy the runtime assets next to the binary. This runs on every build (not
# only when the executable is relinked), so replacing an asset is enough to
# update bin/.
set(ASSET_COPY_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/assets/zeal-tiles.dat"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/zeal-tiles.dat")

foreach(splash_name splash.ztp splash.zts splash.ztm)
    if(EXISTS "${CMAKE_SOURCE_DIR}/assets/${splash_name}")
        list(APPEND ASSET_COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/assets/${splash_name}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${splash_name}")
    endif()
endforeach()

add_custom_target(copy_runtime_assets ALL
    ${ASSET_COPY_COMMANDS}
    COMMENT "Copying runtime assets to bin/"
)
add_dependencies(${target_name} copy_runtime_assets)
