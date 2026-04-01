/*
 * nsf2fmsq - NSF to FMSQ converter
 *
 * Captures APU register writes from NSF emulation and converts them
 * to FMSQ event stream using REG_WRITE commands.
 *
 * All APU register writes are preserved in exact order.
 * Frame boundaries are determined by PLAY_START events from the logger.
 */

#include "nsf2fmsq.h"
#include "fmsq_format.h"
#include "gme.h"
#include "Apu_Logger.h"

#include <cstdio>
#include <cstring>
#include <vector>

/* Emit WAIT command(s) for given number of frames */
static void emit_wait(std::vector<uint8_t>& out, int frames)
{
    while (frames > 0) {
        int w = (frames > FMSQ_MAX_WAIT_FRAMES) ? FMSQ_MAX_WAIT_FRAMES : frames;
        out.push_back(FMSQ_MAKE_WAIT(w));
        frames -= w;
    }
}

/* Emit REG_WRITE commands for all APU writes in a frame range */
static int emit_frame_reg_writes(std::vector<uint8_t>& out,
                                  const std::vector<apu_log_entry_t>& entries,
                                  size_t start, size_t end)
{
    int count = 0;
    for (size_t i = start; i < end; i++) {
        const auto& e = entries[i];
        if (e.event_type != APU_EVENT_WRITE) continue;

        uint16_t addr = e.addr;
        if (addr < 0x4000 || addr > 0x4017) continue;

        uint8_t offset = (uint8_t)(addr - 0x4000);
        out.push_back(FMSQ_CMD_REG_WRITE(offset));
        out.push_back(e.data);
        count++;
    }
    return count;
}

bool nsf2fmsq_convert(const char* nsf_path,
                      const fmsq_convert_options& opts,
                      fmsq_convert_result& result)
{
    /* Initialize APU logger */
    init_apu_logger();
    Apu_Logger* logger = get_apu_logger();
    if (!logger) {
        fprintf(stderr, "[ERROR] Failed to initialize APU logger\n");
        return false;
    }
    logger->set_enabled(true);
    logger->clear();

    /* Open NSF file */
    printf("[LOG] Opening NSF file: %s\n", nsf_path);
    Music_Emu* emu = nullptr;
    const int sample_rate = 44100;
    gme_err_t err = gme_open_file(nsf_path, &emu, sample_rate);
    if (err) {
        fprintf(stderr, "[ERROR] gme_open_file failed: %s\n", err);
        return false;
    }

    int track_count = gme_track_count(emu);
    if (opts.track >= track_count) {
        fprintf(stderr, "[ERROR] Track %d not found (file has %d tracks)\n",
                opts.track, track_count);
        gme_delete(emu);
        return false;
    }

    printf("[LOG] NSF: %d tracks, recording track %d for %d seconds\n",
           track_count, opts.track, opts.duration_sec);

    /* Start track -- this runs INIT and captures initial APU writes */
    err = gme_start_track(emu, opts.track);
    if (err) {
        fprintf(stderr, "[ERROR] gme_start_track failed: %s\n", err);
        gme_delete(emu);
        return false;
    }

    printf("[LOG] After INIT: %zu entries captured\n", logger->entry_count());

    /* Run entire emulation to capture all APU writes */
    int duration_ms = opts.duration_sec * 1000;
    const int chunk_size = 1024;
    short buf[chunk_size];

    printf("[LOG] Running emulation for %d seconds...\n", opts.duration_sec);
    while (gme_tell(emu) < duration_ms) {
        err = gme_play(emu, chunk_size, buf);
        if (err) {
            fprintf(stderr, "[ERROR] gme_play failed: %s\n", err);
            break;
        }
    }

    const auto& entries = logger->get_entries();
    printf("[LOG] Emulation complete: %zu total entries\n", entries.size());

    /* Count event types */
    {
        int counts[5] = {};
        for (const auto& e : entries) {
            if (e.event_type < 5) counts[e.event_type]++;
        }
        printf("[LOG] Events: WRITE=%d, INIT_START=%d, INIT_END=%d, PLAY_START=%d, PLAY_END=%d\n",
               counts[0], counts[1], counts[2], counts[3], counts[4]);
    }

    gme_delete(emu);

    /* Convert entries to FMSQ */
    result.data.clear();
    result.frame_count = 0;
    result.loop_offset = 0;

    /* Find INIT_END to separate INIT from PLAY */
    size_t init_end_idx = 0;
    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].event_type == APU_EVENT_INIT_END) {
            init_end_idx = i;
            break;
        }
    }

    /* Emit INIT writes */
    int init_writes = emit_frame_reg_writes(result.data, entries, 0, init_end_idx);
    printf("[LOG] INIT: %d register writes\n", init_writes);

    /* Build frame index from PLAY_START markers */
    std::vector<size_t> frame_starts;
    for (size_t i = init_end_idx; i < entries.size(); i++) {
        if (entries[i].event_type == APU_EVENT_PLAY_START) {
            frame_starts.push_back(i + 1);
        }
    }

    printf("[LOG] Found %zu PLAY frames\n", frame_starts.size());

    /* Process each frame */
    int pending_wait = 0;
    int total_reg_writes = init_writes;

    for (size_t fi = 0; fi < frame_starts.size(); fi++) {
        size_t frame_begin = frame_starts[fi];
        size_t frame_end;
        if (fi + 1 < frame_starts.size()) {
            /* Find the PLAY_START entry itself (one before frame_starts[fi+1]) */
            frame_end = frame_starts[fi + 1] - 1;
        } else {
            frame_end = entries.size();
        }

        /* Count writes in this frame */
        int write_count = 0;
        for (size_t j = frame_begin; j < frame_end; j++) {
            if (entries[j].event_type == APU_EVENT_WRITE &&
                entries[j].addr >= 0x4000 && entries[j].addr <= 0x4017) {
                write_count++;
            }
        }

        if (write_count > 0) {
            /* Emit accumulated WAIT */
            if (pending_wait > 0) {
                emit_wait(result.data, pending_wait);
                pending_wait = 0;
            }
            int n = emit_frame_reg_writes(result.data, entries, frame_begin, frame_end);
            total_reg_writes += n;
            pending_wait = 1;
        } else {
            pending_wait++;
        }

        result.frame_count++;
    }

    /* Emit trailing WAIT */
    if (pending_wait > 0) {
        emit_wait(result.data, pending_wait);
    }

    /* END */
    result.data.push_back(FMSQ_CMD_END);

    printf("[LOG] Result: %d frames, %d reg writes, %zu bytes\n",
           result.frame_count, total_reg_writes, result.data.size());

    /* Dump if requested */
    if (opts.dump) {
        printf("\n--- FMSQ Command Dump ---\n");
        size_t pc = 0;
        int frame_num = 0;
        while (pc < result.data.size()) {
            uint8_t cmd = result.data[pc];
            printf("%04zx: ", pc);

            if (FMSQ_IS_WAIT(cmd)) {
                printf("WAIT %d\n", FMSQ_WAIT_FRAMES(cmd));
                frame_num += FMSQ_WAIT_FRAMES(cmd);
                pc++;
            } else if (FMSQ_IS_REG_WRITE(cmd)) {
                uint16_t addr = FMSQ_REG_ADDR(cmd);
                uint8_t data = result.data[pc + 1];
                printf("REG $%04X <- 0x%02X\n", addr, data);
                pc += 2;
            } else if (cmd == FMSQ_CMD_END) {
                printf("END\n");
                pc++;
            } else if (cmd == FMSQ_CMD_LOOP) {
                uint16_t offset = result.data[pc+1] | (result.data[pc+2] << 8);
                printf("LOOP -> 0x%04x\n", offset);
                pc += 3;
            } else {
                printf("UNKNOWN 0x%02x\n", cmd);
                pc++;
            }
        }
        printf("--- End Dump ---\n");
    }

    return true;
}
