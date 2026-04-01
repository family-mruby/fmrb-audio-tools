cmake_minimum_required(VERSION 3.16)
project(fmsq_player LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# Paths
set(APU_EMU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../fmruby-graphics-audio/components/apu_emu)
set(FMSQ_FORMAT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Executable
add_executable(fmsq_player
    main.c
    fmsq_player.c
    compat_stubs.c
    # APU emulator (used as-is from fmruby-graphics-audio)
    ${APU_EMU_DIR}/src/apu_if_linux.c
    ${APU_EMU_DIR}/src/nofrendo/nes_apu.c
)

target_include_directories(fmsq_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APU_EMU_DIR}/include
    ${APU_EMU_DIR}/src
    ${APU_EMU_DIR}/src/nofrendo
    ${FMSQ_FORMAT_DIR}
    ${SDL2_INCLUDE_DIRS}
)

# __linux__ is needed for apu_if_linux.c selection in apu_if.h
target_compile_definitions(fmsq_player PRIVATE
    __linux__
)

target_compile_options(fmsq_player PRIVATE
    ${SDL2_CFLAGS_OTHER}
    -Wall -Wextra
    -Wno-unused-parameter
)

target_link_libraries(fmsq_player PRIVATE
    ${SDL2_LIBRARIES}
)

# esp_timer.h stub header (provide empty header so nes_apu.c can include it)
# We create a shim directory with a stub header
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/esp_stubs)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/esp_stubs/esp_timer.h
    "/* stub for standalone build */\n"
    "#ifndef ESP_TIMER_H_STUB\n"
    "#define ESP_TIMER_H_STUB\n"
    "#include <stdint.h>\n"
    "int64_t esp_timer_get_time(void);\n"
    "#endif\n"
)

target_include_directories(fmsq_player PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/esp_stubs
)
