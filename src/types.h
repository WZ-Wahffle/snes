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

typedef enum {
    LOG_LEVEL_VERBOSE,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} log_level_t;

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
#define TO_U24(lss, msb) ((lss) | ((msb) << 16))
#define U16_LOBYTE(val) ((val) & 0xff)
#define U16_HIBYTE(val) (((val) >> 8) & 0xff)
#define U24_LOSHORT(val) ((val) & 0xffff)
#define U24_HIBYTE(val) (((val) >> 16) & 0xff)
#define IN_INTERVAL(val, min, max) ((val) >= (min) && (val) < (max))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 224

typedef enum { LOROM, HIROM, EXHIROM } memory_map_mode_t;

typedef enum {
    AM_ABS = 1,                // Absolute - a
    AM_INDX = 2,               // Absolute Indexed Indirect - (a,x)
    AM_ABSX = 4,               // Absolute Indexed with X - a,x
    AM_ABSY = 8,               // Absolute Indexed with Y - a,y
    AM_IND = 16,               // Absolute Indirect - (a)
    AM_ABSX_L = 32,            // Absolute Long Indexed With X - al,x
    AM_ABS_L = 64,             // Absolute Long - al
    AM_ACC = 128,              // Accumulator - A
    AM_BLK = 256,              // Block Move - xyc
    AM_INDX_DIR = 512,         // Direct Indexed Indirect - (d,x)
    AM_ZBKX_DIR = 1024,        // Direct Indexed with X - d,x
    AM_ZBKY_DIR = 2048,        // Direct Indexed with Y - d,y
    AM_INDY_DIR = 4096,        // Direct Indirect Indexed - (d),y
    AM_INDY_DIR_L = 8192,      // Direct Indirect Long Indexed - [d],y
    AM_IND_DIR_L = 16384,      // Direct Indirect Long - [d]
    AM_IND_DIR = 32768,        // Direct Indirect - (d)
    AM_DIR = 65536,            // Direct - d
    AM_IMM = 131072,           // Immediate - #
    AM_IMP = 262144,           // Implied - i
    AM_PC_REL_L = 524288,      // Program Counter Relative Long - rl
    AM_PC_REL = 1048576,       // Program Counter Relative - r
    AM_STK = 2097152,          // Stack - s
    AM_STK_REL = 4194304,      // Stack Relative - d,s
    AM_STK_REL_INDY = 8388608, // Stack Relative Indirect Indexed - (d,s),y
} addressing_mode_t;

typedef enum {
    STATUS_CARRY,
    STATUS_ZERO,
    STATUS_IRQOFF,
    STATUS_BCD,
    STATUS_XWIDE,
    STATUS_MEMWIDE,
    STATUS_OVERFLOW,
    STATUS_NEGATIVE
} status_bit_t;

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

    struct dma_t {
        bool dma_enable;
        bool hdma_enable;
        bool direction;
        bool indirect_hdma;
        uint8_t addr_inc_mode;
        uint8_t transfer_pattern;
        uint8_t b_bus_addr;
        uint32_t dma_src_addr;
        uint32_t dma_byte_count;
        uint32_t hdma_current_address;
        bool hdma_repeat;
        uint8_t scanlines_left;
    } dmas[8];
} cpu_mmu_t;

typedef struct {
    cpu_mmu_t memory;
    double remaining_clocks;
    bool vblank_nmi_enable;
    uint8_t timer_irq;

    uint16_t pc, c, x, y, d, s;
    uint8_t dbr, pbr, p;
} cpu_t;

static void log_message(log_level_t level, char *message, ...) {
#ifdef LOG_LEVEL
    va_list va_args;
    va_start(va_args, message);
    if (level >= LOG_LEVEL) {
        switch (level) {
        case LOG_LEVEL_VERBOSE:
            printf("[VERB] ");
            vprintf(message, va_args);
            printf("\n");
            break;
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
