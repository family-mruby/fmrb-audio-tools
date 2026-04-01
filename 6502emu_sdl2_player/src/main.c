/*
 * 6502 NSF Player - Direct NSF playback via 6502 CPU + APU emulator
 *
 * Usage:
 *   nsf_player <file.nsf> [-t track] [-o output.wav]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <SDL2/SDL.h>

#include "apu_if.h"
#include "nsf_player.h"

#define AUDIO_SAMPLE_RATE  15720  /* Must match APU sample rate (NTSC) */
#define AUDIO_BUFFER_SIZE  512

static volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* SDL2 audio callback */
static void audio_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;
    int samples = len / sizeof(int16_t);
    int16_t *out = (int16_t *)stream;
    apuif_ring_read(out, samples);
}

/* --- WAV file writer --- */

static void wav_write_header(FILE *f, uint32_t data_size)
{
    uint16_t channels = 1;
    uint32_t sample_rate = AUDIO_SAMPLE_RATE;
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * channels * bits_per_sample / 8;
    uint16_t block_align = channels * bits_per_sample / 8;
    uint32_t chunk_size = 36 + data_size;

    fwrite("RIFF", 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    fwrite("fmt ", 1, 4, f);
    uint32_t fmt_size = 16;
    fwrite(&fmt_size, 4, 1, f);
    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, f);
    fwrite(&channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
}

static int run_wav_export(nsf_player_t *player, const char *wav_path, int duration_sec)
{
    FILE *f = fopen(wav_path, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create '%s'\n", wav_path);
        return 1;
    }

    wav_write_header(f, 0);

    uint32_t total_samples = 0;
    int total_frames = duration_sec * 60;

    printf("Exporting %d seconds to %s ...\n", duration_sec, wav_path);

    for (int frame = 0; frame < total_frames && g_running; frame++) {
        nsf_player_tick(player);

        int16_t abuffer[512];
        memset(abuffer, 0, sizeof(abuffer));
        int sample_count = apuif_process(abuffer, sizeof(abuffer) / sizeof(abuffer[0]));
        if (sample_count > 0) {
            fwrite(abuffer, sizeof(int16_t), sample_count, f);
            total_samples += sample_count;
        }
    }

    uint32_t data_size = total_samples * sizeof(int16_t);
    fseek(f, 0, SEEK_SET);
    wav_write_header(f, data_size);
    fclose(f);

    float duration = (float)total_samples / AUDIO_SAMPLE_RATE;
    printf("Done: %u samples, %.2fs -> %s\n", total_samples, duration, wav_path);

    return 0;
}

static int run_sdl2_playback(nsf_player_t *player, const char *nsf_path)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = AUDIO_SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = AUDIO_BUFFER_SIZE;
    want.callback = audio_callback;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    printf("Audio: %dHz, %d ch, buffer=%d samples\n",
           have.freq, have.channels, have.samples);

    SDL_PauseAudioDevice(dev, 0);
    printf("Playing %s (song %d/%d) ... Ctrl+C to stop\n",
           nsf_path, player->current_song + 1, player->header.total_songs);

    const Uint32 frame_interval_ms = 16;

    while (g_running) {
        Uint32 frame_start = SDL_GetTicks();

        nsf_player_tick(player);

        int16_t abuffer[512];
        memset(abuffer, 0, sizeof(abuffer));
        int sample_count = apuif_process(abuffer, sizeof(abuffer) / sizeof(abuffer[0]));
        if (sample_count > 0) {
            apuif_audio_write(abuffer, sample_count, 1);
        }

        Uint32 elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < frame_interval_ms) {
            SDL_Delay(frame_interval_ms - elapsed);
        }
    }

    SDL_PauseAudioDevice(dev, 1);
    SDL_CloseAudioDevice(dev);
    SDL_Quit();

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.nsf> [-t track] [-o output.wav]\n", argv[0]);
        return 1;
    }

    const char *nsf_path = argv[1];
    const char *wav_path = NULL;
    int track = 0;
    int duration_sec = 60;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            track = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            wav_path = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            duration_sec = atoi(argv[++i]);
        }
    }

    signal(SIGINT, signal_handler);

    /* Initialize APU */
    apuif_init();

    /* Load NSF */
    nsf_player_t player;
    if (nsf_player_load(&player, nsf_path) != 0) {
        return 1;
    }

    /* Start song */
    if (nsf_player_start(&player, track) != 0) {
        nsf_player_free(&player);
        return 1;
    }

    int ret;
    if (wav_path) {
        ret = run_wav_export(&player, wav_path, duration_sec);
    } else {
        ret = run_sdl2_playback(&player, nsf_path);
    }

    printf("Frames played: %u\n", player.frames_played);
    nsf_player_free(&player);

    return ret;
}
