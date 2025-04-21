#ifndef TYPES_H_
#define TYPES_H_

#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { LOG_LEVEL_INFO, LOG_LEVEL_WARNING, LOG_LEVEL_ERROR } log_level_t;

static void log_message(log_level_t level, char *message, ...);

// custom assertion macro to allow registering exit callback
#define ASSERT(val, msg, ...)                                                  \
    do {                                                                       \
        if (!(val)) {                                                          \
            log_message(LOG_LEVEL_ERROR, "%s:%d: Assertion failed:", __FILE__, \
                        __LINE__);                                             \
            log_message(LOG_LEVEL_ERROR, msg, __VA_ARGS__);                    \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
#define UNREACHABLE_SWITCH(val)                                                \
    ASSERT(0, "Illegal value in switch: %d, 0x%04x\n", val, val)
#define TODO(val) ASSERT(0, "%sTODO: " val "\n", "")
#define TO_U16(lsb, msb) ((lsb & 0xff) | ((msb & 0xff) << 8))
#define LOBYTE(val) ((val) & 0xff)
#define HIBYTE(val) (((val) >> 8) & 0xff)
#define IN_INTERVAL(val, min, max) ((val) >= (min) && (val) < (max))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 224

typedef enum { LOROM, HIROM, EXHIROM } memory_map_mode_t;

typedef struct {
    const char *name;
    const uint32_t hash;
    const memory_map_mode_t mode;
} cart_hash_t;

typedef struct {
    uint8_t *rom;
    uint32_t rom_size;
    uint8_t *ram;
    uint32_t ram_size;
    memory_map_mode_t mode;
} cpu_mmu_t;

typedef struct {
    cpu_mmu_t memory;
    double remaining_clocks;
} cpu_t;

static void log_message(log_level_t level, char *message, ...) {
#ifdef LOG_LEVEL
    va_list va_args;
    va_start(va_args, message);
    if (level >= LOG_LEVEL) {
        switch (level) {
        case LOG_LEVEL_INFO:
            printf("\033[32m[INFO] ");
            vprintf(message, va_args);
            printf("\033[0m\n");
            break;
        case LOG_LEVEL_WARNING:
            printf("\033[33m[WARN] ");
            vprintf(message, va_args);
            printf("\033[0m\n");
            break;
        case LOG_LEVEL_ERROR:
            printf("\033[31m[FAIL] ");
            vprintf(message, va_args);
            printf("\033[0m\n");
            break;
        }
    }
#endif
}

#endif
