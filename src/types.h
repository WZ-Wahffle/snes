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
            log_message(LOG_LEVEL_ERROR, "CPU PC: 0x%04x, bank: 0x%02x",       \
                        cpu.pc, cpu.pbr);                                      \
            log_message(LOG_LEVEL_ERROR, "SPC PC: 0x%04x", spc.pc);            \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
#define UNREACHABLE_SWITCH(val)                                                \
    ASSERT(0, "Illegal value in switch: %d, 0x%04x\n", val, val)
#define TODO(val) ASSERT(0, "%sTODO: " val "\n", "")
#define TO_U16(lsb, msb) ((uint16_t)(lsb & 0xff) | ((msb & 0xff) << 8))
#define TO_U24(lss, msb) ((uint32_t)(lss) | ((msb) << 16))
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
#define CLOCK_FREQ 21477268
#define CYCLES_PER_DOT 4

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
    SM_DIR_PAGE = 1,              // Direct Page - d
    SM_DIR_PAGEX = 2,             // X-Indexed Direct Page - d,X
    SM_DIR_PAGEY = 4,             // Y-Indexed Direct Page - d,Y
    SM_INDIRECT = 8,              // Indirect - (X)
    SM_INDIRECT_INC = 16,         // Indirect Auto-Increment - (X)+
    SM_DIR_PAGE_TO_DIR_PAGE = 32, // Direct Page to Direct Page - dd,ds
    SM_IND_PAGE_TO_IND_PAGE = 64, // Indirect Page to Indirect Page - (X),(Y)
    SM_IMM_TO_DIR_PAGE = 128,     // Immediate Data to Direct Page - d,#
    SM_DIR_PAGE_BIT = 256,        // Direct Page Bit - d.b
    SM_DIR_PAGE_BIT_REL = 512,    // Direct Page Bit Relative - d.b,r
    SM_ABS_BOOL_BIT = 1024,       // Absolute Boolean Bit - m.b
    SM_ABS = 2048,                // Absolute - a
    SM_ABS_INDX = 4096,           // Absolute X-Indexed Indirect - (a,X)
    SM_ABSX = 8192,               // X-Indexed Absolute - a,X
    SM_ABSY = 16384,              // Y-Indexed Absolute - a,Y
    SM_INDX = 32768,              // X-Indexed Indirect - (d,X)
    SM_INDY = 65536,              // Indirect Y-Indexed - (d),Y
    SM_REL = 131072,              // Relative - r
    SM_IMM = 262144,              // Immediate - #
    SM_ACC = 524288,              // Accumulator - A
    SM_IMP = 1048576,             // Implied
} spc_addressing_mode_t;

typedef enum {
    STATUS_CARRY,
    STATUS_ZERO,
    STATUS_IRQOFF,
    STATUS_BCD,
    STATUS_XNARROW,
    STATUS_MEMNARROW,
    STATUS_OVERFLOW,
    STATUS_NEGATIVE,
    STATUS_HALFCARRY = 3, // extra flags for the SPC700
    STATUS_BREAK = 4,
    STATUS_DIRECTPAGE = 5,
} status_bit_t;

typedef enum { STATE_STOPPED, STATE_STEPPED, STATE_RUNNING } emu_state_t;

typedef enum { R_C, R_X, R_Y, R_S, R_D } r_t;

typedef struct {
    const char *name;
    const uint32_t hash;
    const memory_map_mode_t mode;
} cart_hash_t;

typedef struct {
    uint8_t *rom;
    uint32_t rom_size;
    uint8_t *sram;
    uint32_t sram_size;
    uint8_t ram[0x20000];
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

    uint8_t apu_io[4];
} cpu_mmu_t;

typedef struct {
    cpu_mmu_t memory;
    double remaining_clocks;
    bool vblank_nmi_enable;
    uint8_t timer_irq;
    bool emulation_mode;

    emu_state_t state;
    bool breakpoint_valid;
    uint32_t breakpoint;

    uint16_t pc, c, x, y, d, s;
    uint8_t dbr, pbr, p;
} cpu_t;

typedef struct {
    uint8_t ram[0x10000];
} spc_mmu_t;

typedef struct {
    spc_mmu_t memory;
    uint8_t a, x, y, s, p;
    uint16_t pc;
    double remaining_clocks;
    bool breakpoint_valid;
    uint16_t breakpoint;
} spc_t;

typedef struct {
    bool force_blanking;
    uint8_t brightness;
    double remaining_clocks;
    uint16_t beam_x, beam_y;
} ppu_t;

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
EXTERNC void set_status_bit(status_bit_t bit, bool value);
EXTERNC bool get_status_bit(status_bit_t bit);
EXTERNC void spc_set_status_bit(status_bit_t bit, bool value);
EXTERNC bool spc_get_status_bit(status_bit_t bit);
EXTERNC uint32_t resolve_addr(addressing_mode_t mode);
EXTERNC uint16_t resolve_read8(addressing_mode_t mode);
EXTERNC uint16_t resolve_read16(addressing_mode_t mode, bool respect_x,
                                bool respect_m);
EXTERNC uint16_t spc_resolve_addr(spc_addressing_mode_t mode);
EXTERNC uint8_t spc_resolve_read(spc_addressing_mode_t mode);
EXTERNC void spc_resolve_write(spc_addressing_mode_t mode, uint8_t val);

EXTERNC uint8_t read_8(uint16_t addr, uint8_t bank);
EXTERNC uint16_t read_16(uint16_t addr, uint8_t bank);
EXTERNC uint32_t read_24(uint16_t addr, uint8_t bank);
EXTERNC uint16_t read_r(r_t reg);
EXTERNC void write_8(uint16_t addr, uint8_t bank, uint8_t val);
EXTERNC void write_16(uint16_t addr, uint8_t bank, uint16_t val);
EXTERNC void write_r(r_t reg, uint16_t val);
EXTERNC void push_8(uint8_t val);
EXTERNC void push_16(uint16_t val);
EXTERNC void push_24(uint32_t val);
EXTERNC uint8_t pop_8(void);
EXTERNC uint16_t pop_16(void);
EXTERNC uint32_t pop_24(void);
EXTERNC uint8_t spc_read_8(uint16_t addr);
EXTERNC uint16_t spc_read_16(uint16_t addr);
EXTERNC uint8_t spc_next_8(void);
EXTERNC void spc_write_8(uint16_t addr, uint8_t val);
EXTERNC void spc_write_16(uint16_t addr, uint16_t val);


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
