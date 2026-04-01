/*
 * nsf2fmsq - NSF to FMSQ converter
 *
 * Converts NSF files to FMSQ format by:
 * 1. Running NSF through game-music-emu to capture APU register writes
 * 2. Tracking per-channel state across frames
 * 3. Generating FMSQ commands from state changes
 */

#ifndef NSF2FMSQ_H
#define NSF2FMSQ_H

#include <stdint.h>
#include <vector>
#include <string>

struct fmsq_convert_options {
    int track;          /* NSF track number (default: 0) */
    int duration_sec;   /* Recording duration in seconds (default: 60) */
    bool dump;          /* Print FMSQ commands to stdout */
};

struct fmsq_convert_result {
    std::vector<uint8_t> data;  /* FMSQ command stream (without header) */
    uint16_t frame_count;
    uint16_t loop_offset;       /* 0 = no loop */
};

/* Convert NSF file to FMSQ command stream */
bool nsf2fmsq_convert(const char* nsf_path,
                      const fmsq_convert_options& opts,
                      fmsq_convert_result& result);

/* Write FMSQ data to file */
bool fmsq_write_file(const char* path, const fmsq_convert_result& result);

#endif /* NSF2FMSQ_H */
