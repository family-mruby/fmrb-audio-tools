cmake_minimum_required(VERSION 3.16)
project(nsf2fmsq LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# gme library (NSF subset)
set(GME_SRCS
    gme/gme.cpp
    gme/Gme_File.cpp
    gme/Music_Emu.cpp
    gme/Classic_Emu.cpp
    gme/Data_Reader.cpp
    gme/Blip_Buffer.cpp
    gme/Multi_Buffer.cpp
    gme/Effects_Buffer.cpp
    gme/Fir_Resampler.cpp
    gme/Dual_Resampler.cpp
    gme/M3u_Playlist.cpp
    gme/Nsf_Emu.cpp
    gme/Nes_Cpu.cpp
    gme/Nes_Apu.cpp
    gme/Nes_Oscs.cpp
    gme/Apu_Logger.cpp
    # Expansion chips
    gme/Nes_Vrc6_Apu.cpp
    gme/Nes_Vrc7_Apu.cpp
    gme/Nes_Fds_Apu.cpp
    gme/Nes_Namco_Apu.cpp
    gme/Nes_Fme7_Apu.cpp
    gme/ext/emu2413.c
    gme/ext/panning.c
    gme/gme_type_stubs.cpp
)

add_library(gme_static STATIC ${GME_SRCS})
target_include_directories(gme_static PUBLIC gme)
target_compile_definitions(gme_static PRIVATE
    GME_APU_LOGGER
    BLARGG_LITTLE_ENDIAN=1
    USE_GME_NSF=1
)
target_compile_options(gme_static PRIVATE -w)

# nsf2fmsq tool
add_executable(nsf2fmsq
    src/main.cpp
    src/nsf2fmsq.cpp
    src/fmsq_writer.cpp
)
target_include_directories(nsf2fmsq PRIVATE src gme)
target_link_libraries(nsf2fmsq gme_static)
target_compile_definitions(nsf2fmsq PRIVATE GME_APU_LOGGER)
