cmake_minimum_required(VERSION 3.16)
project(nsf_6502_player LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# Paths
set(APU_EMU_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../fmruby-graphics-audio/components/apu_emu)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Executable
add_executable(nsf_6502_player
    src/main.c
    src/nes_cpu.c
    src/nsf_player.c
    src/compat_stubs.c
    # APU emulator (from fmruby-graphics-audio)
    ${APU_EMU_DIR}/src/apu_if_linux.c
    ${APU_EMU_DIR}/src/nofrendo/nes_apu.c
)

target_include_directories(nsf_6502_player PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${APU_EMU_DIR}/include
    ${APU_EMU_DIR}/src
    ${APU_EMU_DIR}/src/nofrendo
    ${SDL2_INCLUDE_DIRS}
)

target_compile_definitions(nsf_6502_player PRIVATE
    __linux__
)

target_compile_options(nsf_6502_player PRIVATE
    ${SDL2_CFLAGS_OTHER}
    -Wall -Wextra
    -Wno-unused-parameter
)

target_link_libraries(nsf_6502_player PRIVATE
    ${SDL2_LIBRARIES}
)

# esp_timer.h stub
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/esp_stubs)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/esp_stubs/esp_timer.h
    "/* stub */\n"
    "#ifndef ESP_TIMER_H_STUB\n"
    "#define ESP_TIMER_H_STUB\n"
    "#include <stdint.h>\n"
    "int64_t esp_timer_get_time(void);\n"
    "#endif\n"
)

target_include_directories(nsf_6502_player PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/esp_stubs
)
