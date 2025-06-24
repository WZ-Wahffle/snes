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
#define R5G5B5_TO_R8G8B8A8(val) ()
#define IN_INTERVAL(val, min, max) ((val) >= (min) && (val) < (max))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#define SWAP_ENDIAN(a) ((((a)>>24)&0xff) | \
                    (((a)<<8)&0xff0000) | \
                    (((a)>>8)&0xff00) | \
                    (((a)<<24)&0xff000000)

#define WINDOW_WIDTH 256
#define WINDOW_HEIGHT 224
#define CLOCK_FREQ 21477268
#define CYCLES_PER_DOT 4
#define SAMPLE_RATE 32000.f

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
    SM_Y = 2097152,               // Y register - y
    SM_ABS_BOOL_BIT_INV = 4194304 // Absolute Boolean Bit Inverted - /m.b
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

typedef enum {
    STATE_STOPPED,
    STATE_CPU_STEPPED,
    STATE_SPC_STEPPED,
    STATE_RUNNING
} emu_state_t;

typedef enum { R_C, R_X, R_Y, R_S, R_D } r_t;

typedef enum { BPP_2 = 2, BPP_4 = 4, BPP_8 = 8 } color_depth_t;

typedef enum { ATTACK, DECAY, SUSTAIN, RELEASE } adsr_state_t;

typedef struct {
    const char *name;
    const uint32_t hash;
    const memory_map_mode_t mode;
} cart_hash_t;

typedef struct {
    uint32_t line;
    char bp_inter[7];
    bool valid;
    bool read, write, execute;
} breakpoint_t;

typedef struct {
    uint8_t *rom;
    uint32_t rom_size;
    uint8_t *sram;
    uint32_t sram_size;
    uint8_t ram[0x20000];
    uint32_t ramaddr;
    memory_map_mode_t mode;
    uint8_t coprocessor;

    struct dsp1_t {
        bool command_happened;
        uint8_t command;
        uint8_t read_bytes_r, written_bytes_r, read_bytes, written_bytes;
        uint8_t operands[14];
        uint8_t outputs[6];
    } dsp1;

    struct dma_t {
        bool hdma_enable;
        bool direction;
        bool indirect_hdma;
        uint8_t addr_inc_mode;
        uint8_t transfer_pattern;
        uint8_t b_bus_addr;
        uint32_t dma_src_addr;
        uint32_t dma_byte_count;
        uint32_t hdma_current_address;
        bool hdma_waiting;
        bool hdma_repeat;
        uint8_t scanlines_left;
    } dmas[8];

    uint8_t apu_io[4];
    bool vblank_has_occurred;
    bool timer_has_occurred;

    uint8_t mul_factor_a, mul_factor_b;
    uint16_t dividend;
    uint8_t divisor;
    bool doing_div;

    bool joy_auto_read, joy_latch_pending;
    uint16_t joy1l, joy1h, joy1l_latched, joy1h_latched;
    uint16_t joy2l, joy2h, joy2l_latched, joy2h_latched;
    uint8_t joy1_shift_idx, joy2_shift_idx;
} cpu_mmu_t;

typedef struct {
    cpu_mmu_t memory;
    double remaining_clocks;
    bool vblank_nmi_enable;
    uint8_t timer_irq;
    bool emulation_mode;
    bool irq;
    bool brk;
    bool cop;

    char *file_name;

    emu_state_t state;
    double speed;

    breakpoint_t *breakpoints;
    uint32_t breakpoints_size;

    uint8_t opcode_history[0x10000];
    uint32_t pc_history[0x10000];
    uint16_t history_idx;

    uint16_t pc, c, x, y, d, s;
    uint8_t dbr, pbr, p;
} cpu_t;

typedef struct {
    int8_t vol_left, vol_right;
    uint16_t pitch;
    uint8_t sample_source_directory;
    bool adsr_enable;
    uint8_t a_rate, d_rate, s_level, s_rate;
    uint8_t gain;
    uint16_t t;
    float points_passed_since_refill;
    bool key_on, key_off, playing;

    uint8_t envx;
    int8_t outx;

    int16_t envelope;
    adsr_state_t adsr_state;
    uint8_t remaining_values_in_block;
    uint8_t clear_out_count;
    uint8_t refill_idx;
    int16_t sample_buffer[12];
    int16_t prev_sample;
    int16_t prev_prev_sample;
    bool loop, end;
    uint8_t left_shift;
    uint8_t filter;
    uint16_t sample_addr;
    uint16_t loop_addr;

    bool should_end;

    bool mute_override;
    int16_t output;
} dsp_channel_t;

typedef struct {
    uint8_t ram[0x10000];
    struct spc_timer_t {
        bool enable;
        uint8_t timer;
        uint8_t counter;
        uint8_t timer_internal;
    } timers[3];
    AudioStream stream;
    dsp_channel_t channels[8];
    uint8_t coefficients[8];
    int8_t vol_left, vol_right, echo_left, echo_right;
    uint8_t echo_feedback, echo_delay;
    uint8_t pitch_mod_enable;
    uint8_t use_noise;
    uint8_t echo_enable;
    uint8_t sample_source_directory_page;
    uint8_t echo_start_address;
    uint8_t key_on, key_off;

    bool mute_voices;
    bool mute_all;
    bool disable_echo_write;
    uint8_t noise_freq;

    uint8_t dsp_addr;
} spc_mmu_t;

typedef struct {
    spc_mmu_t memory;
    uint8_t a, x, y, s, p;
    uint16_t pc;
    double remaining_clocks;
    bool breakpoint_valid;
    uint16_t breakpoint;
    bool enable_ipl;
    bool brk;
} spc_t;

typedef struct {
    bool enable_bg_override[4];
    bool enable_obj_override;

    bool force_blanking;
    uint8_t brightness;
    uint8_t address_increment_amount, address_remapping;
    bool address_increment_mode;
    double remaining_clocks;
    uint16_t beam_x, beam_y;
    uint8_t mosaic_size;
    uint16_t h_timer_target, v_timer_target;

    uint8_t display_config;
    struct {
        bool large_characters;
        bool enable_mosaic;
        bool double_h_tilemap, double_v_tilemap;
        uint16_t tilemap_addr;
        uint16_t tiledata_addr;
        uint16_t h_scroll;
        uint16_t v_scroll;
        uint8_t mask_logic;
        bool window_1_enable, window_2_enable;
        bool main_window_enable, sub_window_enable;
        bool window_1_invert, window_2_invert;
        bool main_screen_enable, sub_screen_enable;
        bool color_math_enable;
    } bg_config[4];
    uint8_t bg_scroll_latch;

    uint8_t bg_mode;
    bool mode_1_bg3_prio;

    uint8_t obj_window_mask_logic;
    uint8_t col_window_mask_logic;
    bool obj_window_1_enable, obj_window_2_enable;
    bool col_window_1_enable, col_window_2_enable;
    bool obj_main_window_enable, obj_sub_window_enable;
    bool obj_window_1_invert, obj_window_2_invert;
    bool col_window_1_invert, col_window_2_invert;
    uint8_t window_1_l, window_1_r, window_2_l, window_2_r;
    bool obj_main_screen_enable, obj_sub_screen_enable;
    bool obj_color_math_enable;
    bool backdrop_color_math_enable;
    bool half_color_math;
    bool color_math_subtract;
    uint16_t fixed_color;

    bool direct_color_mode;
    bool addend_subscreen;
    uint8_t sub_window_transparent_region;
    uint8_t main_window_black_region;

    bool mode_7_flip_h, mode_7_flip_v, mode_7_non_tilemap_fill,
        mode_7_tilemap_repeat;
    uint16_t mode_7_center_x, mode_7_center_y;
    int16_t mul_factor_1, mul_factor_2;
    uint16_t c_7_buffer, d_7_buffer;
    float a_7, b_7, c_7, d_7;
    uint8_t mode_7_latch;

    uint16_t vram_addr;
    uint8_t vram[0x10000];

    uint8_t cgram_addr;
    bool cgram_latched;
    uint8_t cgram_latch;
    uint16_t cgram[0x100];

    uint8_t obj_sprite_size;
    uint8_t obj_name_select;
    uint8_t obj_name_base_address;
    uint16_t oam_addr;
    bool oam_priority_rotation;
    uint8_t oam_latch;
    struct {
        int16_t x;
        int16_t y;
        uint8_t tile_idx;
        bool use_second_sprite_page;
        bool use_second_size;
        uint8_t palette;
        uint8_t priority;
        bool flip_h, flip_v;
        bool draw_this_line;
    } oam[128];
} ppu_t;

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif
EXTERNC uint32_t lo_rom_resolve(uint32_t addr, bool log);
EXTERNC uint32_t hi_rom_resolve(uint32_t addr, bool log);
EXTERNC uint32_t ex_hi_rom_resolve(uint32_t addr, bool log);
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
EXTERNC uint8_t read_8_no_log(uint16_t addr, uint8_t bank);
EXTERNC uint16_t read_16(uint16_t addr, uint8_t bank);
EXTERNC uint32_t read_24(uint16_t addr, uint8_t bank);
EXTERNC uint16_t read_r(r_t reg);
EXTERNC uint8_t next_8(void);
EXTERNC uint16_t next_16(void);
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
EXTERNC uint8_t spc_read_8_no_log(uint16_t addr);
EXTERNC uint16_t spc_read_16(uint16_t addr);
EXTERNC uint8_t spc_next_8(void);
EXTERNC uint16_t spc_next_16(void);
EXTERNC void spc_write_8(uint16_t addr, uint8_t val);
EXTERNC void spc_write_16(uint16_t addr, uint16_t val);
EXTERNC void spc_push_8(uint8_t val);
EXTERNC void spc_push_16(uint16_t val);
EXTERNC uint8_t spc_pop_8(void);
EXTERNC uint16_t spc_pop_16(void);

void dsp1_write(uint8_t);

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
