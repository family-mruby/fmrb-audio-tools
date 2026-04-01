/*
 * fmsq_player - FMSQ file player / WAV exporter
 *
 * Uses the Nofrendo APU emulator (via apu_if) to synthesize audio
 * from FMSQ command streams.
 *
 * Usage:
 *   fmsq_player <file.fmsq>              (SDL2 realtime playback)
 *   fmsq_player <file.fmsq> -o out.wav   (WAV file export)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <SDL2/SDL.h>

#include "apu_if.h"
#include "fmsq_player.h"

#define AUDIO_SAMPLE_RATE  15720  /* Must match APU sample rate (NTSC) */
#define AUDIO_BUFFER_SIZE  512

static volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/*
 * SDL2 audio callback.
 * Reads synthesized samples from the APU ring buffer.
 */
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
    uint16_t audio_format = 1; /* PCM */
    fwrite(&audio_format, 2, 1, f);
    fwrite(&channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);

    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
}

static int run_wav_export(fmsq_player_t *player, const char *wav_path)
{
    FILE *f = fopen(wav_path, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create '%s'\n", wav_path);
        return 1;
    }

    /* Write placeholder header (will be updated at end) */
    wav_write_header(f, 0);

    uint32_t total_samples = 0;

    printf("Exporting to %s ...\n", wav_path);

    while (g_running && player->playing) {
        fmsq_player_tick(player);

        int16_t abuffer[512];
        memset(abuffer, 0, sizeof(abuffer));
        int sample_count = apuif_process(abuffer, sizeof(abuffer) / sizeof(abuffer[0]));
        if (sample_count > 0) {
            fwrite(abuffer, sizeof(int16_t), sample_count, f);
            total_samples += sample_count;
        }
    }

    /* Rewrite header with actual data size */
    uint32_t data_size = total_samples * sizeof(int16_t);
    fseek(f, 0, SEEK_SET);
    wav_write_header(f, data_size);
    fclose(f);

    float duration = (float)total_samples / AUDIO_SAMPLE_RATE;
    printf("Done: %u samples, %.2fs, %u bytes -> %s\n",
           total_samples, duration, data_size, wav_path);

    return 0;
}

static int run_sdl2_playback(fmsq_player_t *player, const char *fmsq_path)
{
    /* Initialize SDL2 audio */
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    /* Open SDL2 audio device */
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
    printf("Playing %s ... (Ctrl+C to stop)\n", fmsq_path);

    const Uint32 frame_interval_ms = 16;

    while (g_running && player->playing) {
        Uint32 frame_start = SDL_GetTicks();

        fmsq_player_tick(player);

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
        fprintf(stderr, "Usage: %s <file.fmsq> [-o output.wav]\n", argv[0]);
        return 1;
    }

    const char *fmsq_path = argv[1];
    const char *wav_path = NULL;

    /* Parse -o option */
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            wav_path = argv[i + 1];
            break;
        }
    }

    signal(SIGINT, signal_handler);

    /* Initialize APU */
    apuif_init();
    apuif_write_reg(0x4015, 0x0F);

    /* Load FMSQ file */
    fmsq_player_t player;
    if (fmsq_player_load(&player, fmsq_path) != 0) {
        return 1;
    }
    fmsq_player_reset(&player);

    int ret;
    if (wav_path) {
        ret = run_wav_export(&player, wav_path);
    } else {
        ret = run_sdl2_playback(&player, fmsq_path);
    }

    printf("Playback finished. Frames played: %u\n", player.frames_played);
    fmsq_player_free(&player);

    return ret;
}
