/*
 * Compatibility stubs for building apu_emu outside ESP-IDF.
 *
 * Provides minimal implementations of ESP-IDF functions referenced
 * by nes_apu.c (only in dead debug code paths).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* esp_timer_get_time() stub - returns microseconds */
int64_t esp_timer_get_time(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* Nofrendo log stubs */
int log_init(void) { return 0; }
void log_shutdown(void) {}

int log_print(const char *string)
{
    return printf("%s", string);
}

int log_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = vprintf(format, args);
    va_end(args);
    return ret;
}

void log_chain_logfunc(int (*logfunc)(const char *string))
{
    (void)logfunc;
}

void log_assert(int expr, int line, const char *file, char *msg)
{
    if (!expr) {
        fprintf(stderr, "ASSERT failed at %s:%d", file, line);
        if (msg) fprintf(stderr, ": %s", msg);
        fprintf(stderr, "\n");
        abort();
    }
}

/* Nofrendo memguard stubs (non-debug path) */
bool mem_debug = false;

void *_my_malloc(int size)
{
    return malloc(size);
}

void _my_free(void **data)
{
    if (data && *data) {
        free(*data);
        *data = NULL;
    }
}

char *_my_strdup(const char *string)
{
    return strdup(string);
}

void mem_cleanup(void) {}
void mem_checkblocks(void) {}
void mem_checkleaks(void) {}
