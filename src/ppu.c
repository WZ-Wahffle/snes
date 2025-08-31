#include "ppu.h"
#include "raylib.h"
#include "spc.h"
#include "types.h"

extern cpu_t cpu;
extern ppu_t ppu;
extern spc_t spc;

uint8_t framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};
uint8_t subscreen[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};
uint8_t priority[WINDOW_WIDTH * WINDOW_HEIGHT] = {0};
uint8_t priority_sub[WINDOW_WIDTH * WINDOW_HEIGHT] = {0};
bool use_color_math[WINDOW_WIDTH] = {0};

#define GAMEPAD_UP GAMEPAD_BUTTON_LEFT_FACE_UP
#define GAMEPAD_DOWN GAMEPAD_BUTTON_LEFT_FACE_DOWN
#define GAMEPAD_LEFT GAMEPAD_BUTTON_LEFT_FACE_LEFT
#define GAMEPAD_RIGHT GAMEPAD_BUTTON_LEFT_FACE_RIGHT

#define GAMEPAD_A GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
#define GAMEPAD_B GAMEPAD_BUTTON_RIGHT_FACE_DOWN
#define GAMEPAD_X GAMEPAD_BUTTON_RIGHT_FACE_UP
#define GAMEPAD_Y GAMEPAD_BUTTON_RIGHT_FACE_LEFT

#define GAMEPAD_START GAMEPAD_BUTTON_MIDDLE_RIGHT
#define GAMEPAD_SELECT GAMEPAD_BUTTON_MIDDLE_LEFT

#define GAMEPAD_R GAMEPAD_BUTTON_RIGHT_TRIGGER_1
#define GAMEPAD_L GAMEPAD_BUTTON_LEFT_TRIGGER_1

void set_pixel(uint16_t x, uint16_t y, uint32_t color) {
    ((uint32_t *)framebuffer)[x + WINDOW_WIDTH * y] = color;
}

void try_step_cpu(void) {
    static bool prev_vblank = false;
    if (cpu.remaining_clocks > 0) {
        cpu_execute();
        for (uint32_t i = 0; i < cpu.breakpoints_size; i++) {
            if (cpu.breakpoints[i].valid && cpu.breakpoints[i].execute &&
                TO_U24(cpu.pc, cpu.pbr) == cpu.breakpoints[i].line) {
                cpu.state = STATE_STOPPED;
                break;
            }
        }
        bool any_interrupt_happened = true;
        bool curr_vblank =
            cpu.vblank_nmi_enable && cpu.memory.vblank_has_occurred;
        if (curr_vblank && !prev_vblank) {
            if (!cpu.emulation_mode)
                push_8(cpu.pbr);
            push_16(cpu.pc);
            push_8(cpu.p);
            cpu.pc = read_16(cpu.emulation_mode ? 0xfffa : 0xffea, 0);
            cpu.pbr = 0;
        } else if (cpu.brk) {
            cpu.brk = false;
            if (!cpu.emulation_mode)
                push_8(cpu.pbr);
            push_16(cpu.pc + 1);
            if (cpu.emulation_mode)
                set_status_bit(STATUS_BREAK, true);
            push_8(cpu.p);
            set_status_bit(STATUS_IRQOFF, true);
            set_status_bit(STATUS_BCD, false);
            cpu.pc = read_16(cpu.emulation_mode ? 0xfffe : 0xffe6, 0);
            cpu.pbr = 0;
        } else if (cpu.cop) {
            cpu.cop = false;
            if (!cpu.emulation_mode)
                push_8(cpu.pbr);
            push_16(cpu.pc + 1);
            push_8(cpu.p);
            set_status_bit(STATUS_IRQOFF, true);
            set_status_bit(STATUS_BCD, false);
            cpu.pc = read_16(cpu.emulation_mode ? 0xfff4 : 0xffe4, 0);
            cpu.pbr = 0;
        } else if (!get_status_bit(STATUS_IRQOFF) && cpu.irq) {
            if (!cpu.emulation_mode)
                push_8(cpu.pbr);
            push_16(cpu.pc);
            push_8(cpu.p);
            cpu.pc = read_16(cpu.emulation_mode ? 0xfffe : 0xffee, 0);
            cpu.pbr = 0;
            cpu.irq = false;
        } else {
            any_interrupt_happened = false;
        }
        if (cpu.waiting && any_interrupt_happened) {
            cpu.waiting = false;
        }
        prev_vblank = curr_vblank;
    }
}

void try_step_spc(void) {
    if (spc.remaining_clocks > 0) {
        spc_execute();
        if (spc.breakpoint_valid && spc.pc == spc.breakpoint) {
            cpu.state = STATE_STOPPED;
        }
        if (spc.brk) {
            spc.brk = false;
            spc_push_16(spc.pc);
            spc_push_8(spc.p);
            spc_set_status_bit(STATUS_BREAK, true);
            spc_set_status_bit(STATUS_IRQOFF, false);
            spc.pc = spc_read_16(0xffde);
        }
    }
}

uint16_t r8g8b8a8_to_r5g5b5(uint32_t in) {
    uint16_t ret = 0;
    ret |= ((in >> 3) & 0x1f) << 0;
    ret |= ((in >> 11) & 0x1f) << 5;
    ret |= ((in >> 19) & 0x1f) << 10;
    return ret;
}

uint32_t r5g5b5_to_r8g8b8a8(uint16_t in) {
    return r5g5b5_components_to_r8g8b8a8(in >> 0, in >> 5, in >> 10);
}

uint32_t r5g5b5_components_to_r8g8b8a8(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t ret = 0xff000000;
    ret |= (uint32_t)((r & 0x1f)) << 3;
    ret |= (uint32_t)((g & 0x1f)) << 11;
    ret |= (uint32_t)((b & 0x1f)) << 19;
    return ret;
}

uint32_t brightness_adjust(uint32_t col) {
    uint32_t ret = 0xff000000;
    ret |= (uint32_t)((col & 0x0000ff) * (ppu.brightness / 15.f)) & 0x0000ff;
    ret |= (uint32_t)((col & 0x00ff00) * (ppu.brightness / 15.f)) & 0x00ff00;
    ret |= (uint32_t)((col & 0xff0000) * (ppu.brightness / 15.f)) & 0xff0000;
    return ret;
}

void fetch_tile_color_row(uint16_t tile_vram_offset, uint8_t y_offset,
                          uint8_t width, uint8_t height, color_depth_t bpp,
                          uint8_t out[width]) {
    ASSERT(height > y_offset,
           "Tried indexing tile of height %d out of bounds at y = %d", height,
           y_offset);
    switch (bpp) {
    case BPP_2: {
        uint16_t adj_tile_vram_offset =
            tile_vram_offset + (y_offset % 8) * 2 + (y_offset / 8) * 256;
        for (uint8_t i = 0; i < width / 8; i++) {
            out[i * 8 + 0] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 7) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 7) & 1) << 1);
            out[i * 8 + 1] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 6) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 6) & 1) << 1);
            out[i * 8 + 2] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 5) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 5) & 1) << 1);
            out[i * 8 + 3] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 4) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 4) & 1) << 1);
            out[i * 8 + 4] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 3) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 3) & 1) << 1);
            out[i * 8 + 5] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 2) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 2) & 1) << 1);
            out[i * 8 + 6] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 1) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 1) & 1) << 1);
            out[i * 8 + 7] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 0) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 0) & 1) << 1);
            adj_tile_vram_offset += 16;
        }
    } break;
    case BPP_4: {
        uint16_t adj_tile_vram_offset =
            tile_vram_offset + (y_offset % 8) * 2 + (y_offset / 8) * 512;
        for (uint8_t i = 0; i < width / 8; i++) {
            out[i * 8 + 0] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 7) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 7) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 7) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 7) & 1) << 3);
            out[i * 8 + 1] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 6) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 6) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 6) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 6) & 1) << 3);
            out[i * 8 + 2] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 5) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 5) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 5) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 5) & 1) << 3);
            out[i * 8 + 3] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 4) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 4) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 4) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 4) & 1) << 3);
            out[i * 8 + 4] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 3) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 3) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 3) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 3) & 1) << 3);
            out[i * 8 + 5] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 2) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 2) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 2) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 2) & 1) << 3);
            out[i * 8 + 6] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 1) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 1) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 1) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 1) & 1) << 3);
            out[i * 8 + 7] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 0) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 0) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 0) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 0) & 1) << 3);
            adj_tile_vram_offset += 32;
        }
    } break;
    case BPP_8: {
        uint16_t adj_tile_vram_offset =
            tile_vram_offset + (y_offset % 8) * 2 + (y_offset / 8) * 1024;
        for (uint8_t i = 0; i < width / 8; i++) {
            out[i * 8 + 0] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 7) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 7) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 7) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 7) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 7) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 7) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 7) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 7) & 1) << 7);
            out[i * 8 + 1] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 6) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 6) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 6) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 6) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 6) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 6) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 6) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 6) & 1) << 7);
            out[i * 8 + 2] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 5) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 5) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 5) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 5) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 5) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 5) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 5) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 5) & 1) << 7);
            out[i * 8 + 3] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 4) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 4) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 4) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 4) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 4) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 4) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 4) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 4) & 1) << 7);
            out[i * 8 + 4] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 3) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 3) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 3) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 3) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 3) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 3) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 3) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 3) & 1) << 7);
            out[i * 8 + 5] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 2) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 2) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 2) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 2) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 2) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 2) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 2) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 2) & 1) << 7);
            out[i * 8 + 6] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 1) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 1) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 1) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 1) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 1) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 1) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 1) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 1) & 1) << 7);
            out[i * 8 + 7] =
                (((ppu.vram[adj_tile_vram_offset + 0] >> 0) & 1) << 0) |
                (((ppu.vram[adj_tile_vram_offset + 1] >> 0) & 1) << 1) |
                (((ppu.vram[adj_tile_vram_offset + 16] >> 0) & 1) << 2) |
                (((ppu.vram[adj_tile_vram_offset + 17] >> 0) & 1) << 3) |
                (((ppu.vram[adj_tile_vram_offset + 32] >> 0) & 1) << 4) |
                (((ppu.vram[adj_tile_vram_offset + 33] >> 0) & 1) << 5) |
                (((ppu.vram[adj_tile_vram_offset + 48] >> 0) & 1) << 6) |
                (((ppu.vram[adj_tile_vram_offset + 49] >> 0) & 1) << 7);
            adj_tile_vram_offset += 64;
        }
    } break;
    default:
        UNREACHABLE_SWITCH(bpp);
    }
}

void draw_bg(uint8_t bg_idx, uint16_t y, color_depth_t bpp, uint8_t low_prio,
             uint8_t high_prio) {
    if (!ppu.bg_config[bg_idx].main_screen_enable &&
        !ppu.bg_config[bg_idx].sub_screen_enable)
        return;
    if (ppu.enable_bg_override[bg_idx])
        return;
    uint16_t screen_y = y;
    if (ppu.bg_config[bg_idx].enable_mosaic) {
        y -= y % (ppu.mosaic_size + 1);
    }
    uint32_t *target =
        (uint32_t *)(framebuffer + (WINDOW_WIDTH * 4 * screen_y));
    uint32_t *target_sub =
        (uint32_t *)(subscreen + (WINDOW_WIDTH * 4 * screen_y));
    uint8_t tilemap_w = ppu.bg_config[bg_idx].double_h_tilemap ? 64 : 32;
    uint8_t tilemap_h = ppu.bg_config[bg_idx].double_v_tilemap ? 64 : 32;
    uint16_t tilemap_line_pointer = ppu.bg_config[bg_idx].tilemap_addr;
    uint8_t tile_size = ppu.bg_config[bg_idx].large_characters ? 16 : 8;
    uint16_t window_height = tilemap_h * tile_size;
    uint16_t window_width = tilemap_w * tile_size;
    uint8_t tile_index_v =
        (((y + ppu.bg_config[bg_idx].v_scroll) % window_height) / tile_size);
    if (tile_index_v > 31 && tilemap_w == 64 && tilemap_h == 64) {
        tilemap_line_pointer += 0x1000;
    }
    tilemap_line_pointer += (tile_index_v % 32) * 2 * 32;
    uint16_t tilemap_line_index = 0;

    uint16_t tilemap_fetch[64] = {0};
    for (uint8_t i = 0; i < 64; i++) {
        tilemap_fetch[i] = TO_U16(
            ppu.vram[tilemap_line_pointer + (tilemap_line_index % 32) * 2 +
                     (tilemap_line_index / 32) * 0x800],
            ppu.vram[tilemap_line_pointer + (tilemap_line_index % 32) * 2 +
                     (tilemap_line_index / 32) * 0x800 + 1]);
        tilemap_line_index++;
        tilemap_line_index %= tilemap_w;
    }

    uint8_t out[1024] = {0};
    for (uint8_t tile_idx = 0; tile_idx < tilemap_w; tile_idx++) {
        uint8_t tile_y_off = (y + ppu.bg_config[bg_idx].v_scroll) % tile_size;
        if (tilemap_fetch[tile_idx] >> 15)
            tile_y_off = tile_size - 1 - tile_y_off;
        fetch_tile_color_row(ppu.bg_config[bg_idx].tiledata_addr +
                                 (tilemap_fetch[tile_idx] & 0x3ff) *
                                     ((bpp * tile_size * tile_size) / 8),
                             tile_y_off, tile_size, tile_size, bpp,
                             out + tile_idx * tile_size);
    }

    for (uint16_t screen_x = 0; screen_x < WINDOW_WIDTH; screen_x++) {
        uint16_t x = screen_x;
        if (ppu.bg_config[bg_idx].enable_mosaic) {
            x -= x % (ppu.mosaic_size + 1);
        }
        uint8_t tilemap_idx =
            (((x + ppu.bg_config[bg_idx].h_scroll) % window_width) /
             tile_size) %
            64;
        bool prio = tilemap_fetch[tilemap_idx] & 0x2000;

        bool window_1 = false;
        if (ppu.bg_config[bg_idx].window_1_enable) {
            window_1 = IN_INTERVAL(screen_x, ppu.window_1_l, ppu.window_1_r) ^
                       ppu.bg_config[bg_idx].window_1_invert;
        }
        bool window_2 = false;
        if (ppu.bg_config[bg_idx].window_2_enable) {
            window_2 = IN_INTERVAL(screen_x, ppu.window_2_l, ppu.window_2_r) ^
                       ppu.bg_config[bg_idx].window_2_invert;
        }

        bool blocked;
        if (!ppu.bg_config[bg_idx].window_1_enable &&
            !ppu.bg_config[bg_idx].window_2_enable) {
            blocked = false;
        } else if (ppu.bg_config[bg_idx].window_1_enable &&
                   !ppu.bg_config[bg_idx].window_2_enable) {
            blocked = window_1;
        } else if (!ppu.bg_config[bg_idx].window_1_enable &&
                   ppu.bg_config[bg_idx].window_2_enable) {
            blocked = window_2;
        } else
            switch (ppu.bg_config[bg_idx].mask_logic) {
            case 0:
                blocked = window_1 || window_2;
                break;
            case 1:
                blocked = window_1 && window_2;
                break;
            case 2:
                blocked = window_1 ^ window_2;
                break;
            case 3:
                blocked = window_1 == window_2;
                break;
            default:
                UNREACHABLE_SWITCH(ppu.bg_config[bg_idx].mask_logic);
            }
        uint8_t tile_x_off = (x + ppu.bg_config[bg_idx].h_scroll) % tile_size;
        if ((tilemap_fetch[tilemap_idx] >> 14) & 1)
            tile_x_off = tile_size - 1 - tile_x_off;
        uint8_t palette = (tilemap_fetch[tilemap_idx] >> 10) & 0b111;

        if (ppu.bg_config[bg_idx].main_screen_enable &&
            priority[screen_y * WINDOW_WIDTH + screen_x] <
                (prio ? high_prio : low_prio)) {
            if (blocked && ppu.bg_config[bg_idx].main_window_enable) {
                target[screen_x] = 0xff000000;
                priority[screen_y * WINDOW_WIDTH + screen_x] =
                    prio ? high_prio : low_prio;
                continue;
            }

            if (out[tilemap_idx * tile_size + tile_x_off] != 0) {
                target[screen_x] = brightness_adjust(
                    ppu.cgram[palette * bpp * bpp +
                              out[tilemap_idx * tile_size + tile_x_off]]);
                priority[screen_y * WINDOW_WIDTH + screen_x] =
                    prio ? high_prio : low_prio;
                use_color_math[screen_x] =
                    ppu.bg_config[bg_idx].color_math_enable;
            }
        }

        if (ppu.bg_config[bg_idx].sub_screen_enable &&
            priority_sub[screen_y * WINDOW_WIDTH + screen_x] <
                (prio ? high_prio : low_prio)) {
            if (blocked && ppu.bg_config[bg_idx].sub_window_enable) {
                // target_sub[screen_x] = 0xff000000;
                // priority_sub[screen_y * WINDOW_WIDTH + screen_x] =
                //     prio ? high_prio : low_prio;
                continue;
            }

            if (out[tilemap_idx * tile_size + tile_x_off] != 0) {
                target_sub[screen_x] = brightness_adjust(
                    ppu.cgram[palette * bpp * bpp +
                              out[tilemap_idx * tile_size + tile_x_off]]);
                priority_sub[screen_y * WINDOW_WIDTH + screen_x] =
                    prio ? high_prio : low_prio;
            }
        }
    }
}

void draw_bg_1_mode_7(int16_t y) {
    if (!ppu.bg_config[0].main_screen_enable &&
        !ppu.bg_config[0].sub_screen_enable)
        return;
    if (ppu.enable_bg_override[0])
        return;
    int16_t screen_y = y;
    uint32_t *target =
        (uint32_t *)(framebuffer + (WINDOW_WIDTH * 4 * screen_y));
    uint32_t *target_sub =
        (uint32_t *)(subscreen + (WINDOW_WIDTH * 4 * screen_y));
    for (int16_t screen_x = 0; screen_x < WINDOW_WIDTH; screen_x++) {
        int16_t x = ppu.mode_7_center_x;
        y = ppu.mode_7_center_y;
        x += (screen_x - ppu.mode_7_center_x) * ppu.a_7 +
             (screen_y - ppu.mode_7_center_y) * ppu.b_7;
        y += (screen_x - ppu.mode_7_center_x) * ppu.c_7 +
             (screen_y - ppu.mode_7_center_y) * ppu.d_7;
        if (x < 0 || y < 0 || x >= 1024 || y >= 1024) {
            if (ppu.mode_7_tilemap_repeat) {
                x %= 1024;
                y %= 1024;
            } else {
                if (ppu.mode_7_non_tilemap_fill) {
                    x = 0;
                    y = 0;
                } else {
                    continue;
                }
            }
        }
        if (ppu.bg_config[0].enable_mosaic) {
            y -= y % (ppu.mosaic_size + 1);
        }
        if (ppu.bg_config[0].enable_mosaic) {
            x -= x % (ppu.mosaic_size + 1);
        }
        uint16_t tile_number = (y / 8) * 128 + (x / 8);
        uint16_t tile_idx = ppu.vram[tile_number * 2];
        uint8_t palette_idx =
            ppu.vram[tile_idx * 128 + (2 * (x % 8)) + (2 * (y % 8) * 8) + 1];
        target[screen_x] = ppu.cgram[palette_idx];
        target_sub[screen_x] = ppu.cgram[palette_idx];
    }
}

void draw_obj(uint16_t y) {
    static uint8_t size_lut[8][4] = {
        {8, 8, 16, 16},   {8, 8, 32, 32},   {8, 8, 64, 64},   {16, 16, 32, 32},
        {16, 16, 64, 64}, {32, 32, 64, 64}, {16, 32, 32, 64}, {16, 32, 32, 32}};
    uint8_t draw_count = 0;
    if (ppu.enable_obj_override)
        return;

    // step 1: collecting (lower index, higher priority)
    for (uint8_t i = 0; i < 128; i++) {
        int16_t sp_x = ppu.oam[i].x;
        int16_t sp_y = ppu.oam[i].y;
        uint8_t sp_w =
            size_lut[ppu.obj_sprite_size][ppu.oam[i].use_second_size * 2];
        uint8_t sp_h =
            size_lut[ppu.obj_sprite_size][ppu.oam[i].use_second_size * 2 + 1];
        ppu.oam[i].draw_this_line = IN_INTERVAL(y, sp_y, sp_y + sp_h) &&
                                    IN_INTERVAL(sp_x, 0, WINDOW_WIDTH - sp_w) &&
                                    draw_count++ < 32;
    }

    uint16_t name_base = ppu.obj_name_base_address << 14;
    uint16_t name_alt = name_base + ((ppu.obj_name_select + 1) << 13);

    // step 2: drawing (lower index, higher priority)
    uint32_t *target = (uint32_t *)(framebuffer + (WINDOW_WIDTH * 4 * y));
    uint32_t *target_sub = (uint32_t *)(subscreen + (WINDOW_WIDTH * 4 * y));

    for (uint8_t i = 0; i < 128; i++) {
        if (ppu.oam[i].draw_this_line) {
            int16_t sp_x = ppu.oam[i].x;
            uint8_t sp_w =
                size_lut[ppu.obj_sprite_size][ppu.oam[i].use_second_size * 2];
            uint8_t sp_h = size_lut[ppu.obj_sprite_size]
                                   [ppu.oam[i].use_second_size * 2 + 1];
            uint8_t y_off = y - ppu.oam[i].y;
            if (ppu.oam[i].flip_v)
                y_off = (sp_h - 1) - y_off;

            uint8_t tiles[64] = {0};
            fetch_tile_color_row(
                (ppu.oam[i].use_second_sprite_page ? name_alt : name_base) +
                    ppu.oam[i].tile_idx * 32,
                y_off, sp_w, sp_h, BPP_4, tiles);

            uint8_t prio = ppu.oam[i].priority * 3 + 2;
            for (int16_t x_off = 0; x_off < sp_w; x_off++) {
                int16_t x =
                    sp_x + (ppu.oam[i].flip_h ? (sp_w - (x_off + 1)) : x_off);

                bool window_1 = false;
                if (ppu.obj_window_1_enable) {
                    window_1 = IN_INTERVAL(x, ppu.window_1_l, ppu.window_1_r) ^
                               ppu.obj_window_1_invert;
                }
                bool window_2 = false;
                if (ppu.obj_window_2_enable) {
                    window_2 = IN_INTERVAL(x, ppu.window_2_l, ppu.window_2_r) ^
                               ppu.obj_window_2_invert;
                }

                bool blocked;
                if (!ppu.obj_window_1_enable && !ppu.obj_window_2_enable) {
                    blocked = false;
                } else if (ppu.obj_window_1_enable &&
                           !ppu.obj_window_2_enable) {
                    blocked = window_1;
                } else if (!ppu.obj_window_1_enable &&
                           ppu.obj_window_2_enable) {
                    blocked = window_2;
                } else
                    switch (ppu.obj_window_mask_logic) {
                    case 0:
                        blocked = window_1 || window_2;
                        break;
                    case 1:
                        blocked = window_1 && window_2;
                        break;
                    case 2:
                        blocked = window_1 ^ window_2;
                        break;
                    case 3:
                        blocked = window_1 == window_2;
                        break;
                    default:
                        UNREACHABLE_SWITCH(ppu.obj_window_mask_logic);
                    }

                if (ppu.obj_main_screen_enable &&
                    priority[y * WINDOW_WIDTH + x] < prio) {
                    if (x < 0 || x > 255)
                        continue;
                    if (blocked && ppu.obj_main_window_enable) {
                        target[x] = 0xff000000;
                        priority[y * WINDOW_WIDTH + x] = prio;
                        continue;
                    }

                    if (tiles[x_off] != 0) {
                        uint32_t col = brightness_adjust(
                            ppu.cgram[128 + ppu.oam[i].palette * 16 +
                                      tiles[x_off]]);
                        target[x] = col;
                        priority[y * WINDOW_WIDTH + x] = prio;
                        use_color_math[x] = ppu.obj_color_math_enable;
                    }
                }

                if (ppu.obj_sub_screen_enable &&
                    priority_sub[y * WINDOW_WIDTH + x] < prio) {
                    if (x < 0 || x > 255) {
                        continue;
                    }
                    if (blocked && ppu.obj_sub_window_enable) {
                        // target_sub[x] = 0xff000000;
                        // priority_sub[y * WINDOW_WIDTH + x] = prio;
                        continue;
                    }

                    if (tiles[x_off] != 0) {
                        uint32_t col = brightness_adjust(
                            ppu.cgram[128 + ppu.oam[i].palette * 16 +
                                      tiles[x_off]]);
                        target_sub[x] = col;
                        priority_sub[y * WINDOW_WIDTH + x] = prio;
                    }
                }
            }
        }
    }
}

void try_step_ppu(void) {
    if (ppu.remaining_clocks > 0) {
        ppu.remaining_clocks -= CYCLES_PER_DOT;

        ppu.beam_x++;
        if (ppu.beam_x == 340) {
            ppu.beam_x = 0;
            ppu.beam_y++;
            if (ppu.beam_y == 262) {
                ppu.beam_y = 0;
            }
        }

        if (cpu.timer_irq) {
            if ((cpu.timer_irq == 1 && ppu.beam_x == ppu.h_timer_target) ||
                (cpu.timer_irq == 2 && ppu.beam_y == ppu.v_timer_target &&
                 ppu.beam_x == 0) ||
                (cpu.timer_irq == 3 && ppu.beam_y == ppu.v_timer_target &&
                 ppu.beam_x == ppu.h_timer_target)) {
                cpu.irq = true;
                cpu.memory.timer_has_occurred = true;
            }
        }

        if (ppu.beam_x == 0 && ppu.beam_y == 225) {
            cpu.memory.vblank_has_occurred = true;
            if (cpu.break_next_frame && cpu.state == STATE_RUNNING) {
                cpu.state = STATE_STOPPED;
                cpu.break_next_frame = false;
            }
        }
        if (ppu.beam_x == 339 && ppu.beam_y == 261)
            cpu.memory.vblank_has_occurred = false;

        if (ppu.beam_x == 278 && ppu.beam_y > 0 && ppu.beam_y < 225) {
            for (uint8_t i = 0; i < 8; i++) {
                if (cpu.memory.dmas[i].hdma_enable) {
                    if (cpu.memory.dmas[i].scanlines_left == 0) {
                        uint32_t addr = cpu.memory.dmas[i].hdma_current_address;
                        uint8_t next =
                            read_8(U24_LOSHORT(addr), U24_HIBYTE(addr));
                        addr = TO_U24(U24_LOSHORT(addr) + 1, U24_HIBYTE(addr));
                        if (next == 0) {
                            cpu.memory.dmas[i].hdma_enable = false;
                            continue;
                        }
                        cpu.memory.dmas[i].hdma_repeat = next & 0x80;
                        cpu.memory.dmas[i].scanlines_left = next & 0x7f;
                        cpu.memory.dmas[i].hdma_current_address = addr;
                        cpu.memory.dmas[i].hdma_waiting = false;
                        if (cpu.memory.dmas[i].indirect_hdma) {
                            cpu.memory.dmas[i].dma_byte_count = TO_U24(
                                read_16(U24_LOSHORT(addr), U24_HIBYTE(addr)),
                                U24_HIBYTE(cpu.memory.dmas[i].dma_byte_count));
                            addr =
                                TO_U24(U24_LOSHORT(addr) + 2, U24_HIBYTE(addr));
                            cpu.memory.dmas[i].hdma_current_address = addr;
                        }
                    }

                    static uint8_t transfer_patterns[8][5] = {
                        {1, 0, 0, 0, 0}, {2, 0, 1, 0, 0}, {2, 0, 0, 0, 0},
                        {4, 0, 0, 1, 1}, {4, 0, 1, 2, 3}, {4, 0, 1, 0, 1},
                        {2, 0, 0, 0, 0}, {4, 0, 0, 1, 1},
                    };
                    if (cpu.memory.dmas[i].indirect_hdma) {
                        if (cpu.memory.dmas[i].hdma_repeat ||
                            !cpu.memory.dmas[i].hdma_waiting) {
                            uint8_t count =
                                transfer_patterns[cpu.memory.dmas[i]
                                                      .transfer_pattern][0];
                            uint32_t addr = cpu.memory.dmas[i].dma_byte_count;
                            for (uint8_t j = 0; j < count; j++) {
                                uint8_t to_write =
                                    read_8(U24_LOSHORT(addr), U24_HIBYTE(addr));
                                if (cpu.memory.dmas[i].addr_inc_mode == 0)
                                    addr = TO_U24(U24_LOSHORT(addr) + 1,
                                                  U24_HIBYTE(addr));
                                if (cpu.memory.dmas[i].addr_inc_mode == 2)
                                    addr = TO_U24(U24_LOSHORT(addr) - 1,
                                                  U24_HIBYTE(addr));

                                write_8(
                                    0x2100 + cpu.memory.dmas[i].b_bus_addr +
                                        transfer_patterns[cpu.memory.dmas[i]
                                                              .transfer_pattern]
                                                         [1 + j],
                                    0, to_write);
                            }
                            cpu.memory.dmas[i].dma_byte_count = addr;
                            if (!cpu.memory.dmas[i].hdma_repeat)
                                cpu.memory.dmas[i].hdma_waiting = true;
                        }
                    } else {
                        if (cpu.memory.dmas[i].hdma_repeat ||
                            !cpu.memory.dmas[i].hdma_waiting) {
                            uint8_t count =
                                transfer_patterns[cpu.memory.dmas[i]
                                                      .transfer_pattern][0];
                            uint32_t addr =
                                cpu.memory.dmas[i].hdma_current_address;
                            for (uint8_t j = 0; j < count; j++) {
                                uint8_t to_write =
                                    read_8(U24_LOSHORT(addr), U24_HIBYTE(addr));
                                if (cpu.memory.dmas[i].addr_inc_mode == 0)
                                    addr = TO_U24(U24_LOSHORT(addr) + 1,
                                                  U24_HIBYTE(addr));
                                if (cpu.memory.dmas[i].addr_inc_mode == 2)
                                    addr = TO_U24(U24_LOSHORT(addr) - 1,
                                                  U24_HIBYTE(addr));
                                write_8(
                                    0x2100 + cpu.memory.dmas[i].b_bus_addr +
                                        transfer_patterns[cpu.memory.dmas[i]
                                                              .transfer_pattern]
                                                         [1 + j],
                                    0, to_write);
                            }
                            cpu.memory.dmas[i].hdma_current_address = addr;
                            if (!cpu.memory.dmas[i].hdma_repeat)
                                cpu.memory.dmas[i].hdma_waiting = true;
                        }
                    }

                    cpu.memory.dmas[i].scanlines_left--;
                }
            }
        }

        if (ppu.beam_x == 22 && ppu.beam_y > 0 && ppu.beam_y < 225) {
            if (ppu.force_blanking)
                return;
            for (uint16_t i = 0; i < WINDOW_WIDTH; i++) {
                ((uint32_t *)framebuffer)[(ppu.beam_y - 1) * WINDOW_WIDTH + i] =
                    ppu.fixed_color_24bit;
                ((uint32_t *)subscreen)[(ppu.beam_y - 1) * WINDOW_WIDTH + i] =
                    ppu.fixed_color_24bit;
                priority[(ppu.beam_y - 1) * WINDOW_WIDTH + i] = 0;
                priority_sub[(ppu.beam_y - 1) * WINDOW_WIDTH + i] = 0;
                use_color_math[i] = 0;
            }
            if (ppu.bg_mode == 0) {
                draw_bg(0, ppu.beam_y - 1, BPP_2, 7, 10);
                draw_bg(1, ppu.beam_y - 1, BPP_2, 6, 9);
                draw_bg(2, ppu.beam_y - 1, BPP_2, 1, 4);
                draw_bg(3, ppu.beam_y - 1, BPP_2, 0, 3);
            }
            if (ppu.bg_mode == 1) {
                draw_bg(0, ppu.beam_y - 1, BPP_4, 7, 10);
                draw_bg(1, ppu.beam_y - 1, BPP_4, 6, 9);
                draw_bg(2, ppu.beam_y - 1, BPP_2, 1,
                        ppu.mode_1_bg3_prio ? 12 : 4);
            }
            if (ppu.bg_mode == 2) {
                draw_bg(0, ppu.beam_y - 1, BPP_4, 4, 10);
                draw_bg(1, ppu.beam_y - 1, BPP_4, 1, 7);
            }
            if (ppu.bg_mode == 3) {
                draw_bg(0, ppu.beam_y - 1, BPP_8, 4, 10);
                draw_bg(1, ppu.beam_y - 1, BPP_4, 1, 7);
            }
            if (ppu.bg_mode == 4) {
                draw_bg(0, ppu.beam_y - 1, BPP_8, 4, 10);
                draw_bg(1, ppu.beam_y - 1, BPP_2, 1, 7);
            }
            if (ppu.bg_mode == 7) {
                draw_bg_1_mode_7(ppu.beam_y - 1);
            }
            draw_obj(ppu.beam_y - 1);
            for (uint16_t i = 0; i < WINDOW_WIDTH; i++) {
                if (false) {
                    bool window_1 = false;
                    if (ppu.col_window_1_enable) {
                        window_1 =
                            IN_INTERVAL(i, ppu.window_1_l, ppu.window_1_r) ^
                            ppu.col_window_1_invert;
                    }
                    bool window_2 = false;
                    if (ppu.col_window_2_enable) {
                        window_2 =
                            IN_INTERVAL(i, ppu.window_2_l, ppu.window_2_r) ^
                            ppu.col_window_2_invert;
                    }
                    bool blocked;
                    if (!ppu.col_window_1_enable && !ppu.col_window_2_enable) {
                        blocked = false;
                    } else if (ppu.col_window_1_enable &&
                               !ppu.col_window_2_enable) {
                        blocked = window_1;
                    } else if (!ppu.col_window_1_enable &&
                               ppu.col_window_2_enable) {
                        blocked = window_2;
                    } else
                        switch (ppu.col_window_mask_logic) {
                        case 0:
                            blocked = window_1 || window_2;
                            break;
                        case 1:
                            blocked = window_1 && window_2;
                            break;
                        case 2:
                            blocked = window_1 ^ window_2;
                            break;
                        case 3:
                            blocked = window_1 == window_2;
                            break;
                        default:
                            UNREACHABLE_SWITCH(ppu.col_window_mask_logic);
                        }

                    switch (ppu.main_window_black_region) {
                    case 0:
                        // this page intentionally left blank
                        break;
                    case 1:
                        if (!blocked) {
                            ((uint32_t *)
                                 framebuffer)[(ppu.beam_y - 1) * WINDOW_WIDTH +
                                              i] = 0xff000000;
                            continue;
                        }
                        break;
                    case 2:
                        if (blocked) {
                            ((uint32_t *)
                                 framebuffer)[(ppu.beam_y - 1) * WINDOW_WIDTH +
                                              i] = 0xff000000;
                            continue;
                        }
                        break;
                    case 3:
                        ((uint32_t *)
                             framebuffer)[(ppu.beam_y - 1) * WINDOW_WIDTH + i] =
                            0xff000000;
                        continue;
                    }
                    switch (ppu.sub_window_transparent_region) {
                    case 0:
                        // this page intentionally left blank
                        break;
                    case 1:
                        if (!blocked) {
                            continue;
                        }
                        break;
                    case 2:
                        if (blocked) {
                            continue;
                        }
                        break;
                    case 3:
                        continue;
                    }

                    int32_t main =
                        ((uint32_t *)
                             framebuffer)[(ppu.beam_y - 1) * WINDOW_WIDTH + i] &
                        0xffffff;
                    int32_t to_add =
                        ppu.addend_subscreen
                            ? ((uint32_t *)
                                   subscreen)[(ppu.beam_y - 1) * WINDOW_WIDTH +
                                              i] &
                                  0xffffff
                            : ppu.fixed_color_24bit;
                    int32_t r, g, b;
                    if (ppu.color_math_subtract) {
                        r = ((main >> 3) & 0xff) - ((to_add >> 3) & 0xff);
                        g = ((main >> 11) & 0xff) - ((to_add >> 11) & 0xff);
                        b = ((main >> 19) & 0xff) - ((to_add >> 19) & 0xff);

                    } else {
                        r = ((main >> 3) & 0xff) + ((to_add >> 3) & 0xff);
                        g = ((main >> 11) & 0xff) + ((to_add >> 11) & 0xff);
                        b = ((main >> 19) & 0xff) + ((to_add >> 19) & 0xff);
                    }
                    if (ppu.half_color_math) {
                        r /= 2;
                        g /= 2;
                        b /= 2;
                    }
                    r = MIN(255, MAX(0, r));
                    g = MIN(255, MAX(0, g));
                    b = MIN(255, MAX(0, b));
                    ((uint32_t *)
                         framebuffer)[(ppu.beam_y - 1) * WINDOW_WIDTH + i] =
                        r | (g << 8) | (b << 16) | 0xff000000;
                }

                if (priority[(ppu.beam_y - 1) * WINDOW_WIDTH + i] == 0) {
                    ((uint32_t *)
                         framebuffer)[(ppu.beam_y - 1) * WINDOW_WIDTH + i] =
                        ((uint32_t *)
                             subscreen)[(ppu.beam_y - 1) * WINDOW_WIDTH + i];
                }
            }
        }
    }
}

void ui(void) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(WINDOW_WIDTH * 4, WINDOW_HEIGHT * 4, "snes");
    Image framebuffer_image = {framebuffer, WINDOW_WIDTH, WINDOW_HEIGHT, 1,
                               PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture texture = LoadTextureFromImage(framebuffer_image);
    cpp_init();

    bool view_debug_ui = false;

    ppu.v_timer_target = 0x1ff;
    ppu.h_timer_target = 0x1ff;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(SWAP_ENDIAN(ppu.fixed_color_24bit))));

        for (uint32_t i = 0;
             i < 341 * 262 * cpu.speed * (GetFrameTime() / 0.0166f); i++) {
            switch (cpu.state) {
            case STATE_STOPPED:
                // this page intentionally left blank
                break;
            case STATE_CPU_STEPPED:
                ppu.remaining_clocks += (-cpu.remaining_clocks) + 1;
                spc.remaining_clocks += (-cpu.remaining_clocks) + 1;
                cpu.remaining_clocks = 1;
                cpu.state = STATE_STOPPED;
                try_step_cpu();
                try_step_ppu();
                while (spc.remaining_clocks > 0) {
                    try_step_spc();
                }
                break;
            case STATE_SPC_STEPPED:
                ppu.remaining_clocks += (-spc.remaining_clocks) + 1;
                cpu.remaining_clocks += (-spc.remaining_clocks) + 1;
                spc.remaining_clocks = 1;
                cpu.state = STATE_STOPPED;
                try_step_spc();
                try_step_ppu();
                while (spc.remaining_clocks > 0) {
                    try_step_cpu();
                }
                break;
            case STATE_RUNNING:
                cpu.remaining_clocks += CYCLES_PER_DOT;
                ppu.remaining_clocks += CYCLES_PER_DOT;
                spc.remaining_clocks += CYCLES_PER_DOT;
                while (cpu.remaining_clocks > 0 && cpu.state != STATE_STOPPED) {
                    try_step_cpu();
                }
                while (spc.remaining_clocks > 0 && cpu.state != STATE_STOPPED) {
                    try_step_spc();
                }
                try_step_ppu();
                break;
            }
        }

        cpu.memory.joy1l = 0;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_R) << 4;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_L) << 5;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_X) << 6;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_A) << 7;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_RIGHT) << 8;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_LEFT) << 9;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_DOWN) << 10;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_UP) << 11;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_START) << 12;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_SELECT) << 13;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_Y) << 14;
        cpu.memory.joy1l |= IsGamepadButtonDown(0, GAMEPAD_B) << 15;

        cpu.memory.joy1l |=
            (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) > 0.5) << 8;
        cpu.memory.joy1l |=
            (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) < -0.5) << 9;
        cpu.memory.joy1l |=
            (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) > 0.5) << 10;
        cpu.memory.joy1l |=
            (GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) < -0.5) << 11;
        cpu.memory.joy_latch_pending = false;

        UpdateTexture(texture, framebuffer);
        DrawTexturePro(texture, (Rectangle){0, 0, WINDOW_WIDTH, WINDOW_HEIGHT},
                       (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
                       (Vector2){0, 0}, 0.0f, WHITE);

        if (IsKeyPressed(KEY_TAB)) {
            view_debug_ui = !view_debug_ui;
        }

        if (IsKeyPressed(KEY_F10)) {

            cpu.state =
                cpu.state == STATE_RUNNING ? STATE_STOPPED : STATE_RUNNING;
        }

        if (IsKeyPressed(KEY_END)) {
            cpu.speed *= 2;
        }

        if (IsKeyPressed(KEY_HOME)) {
            cpu.speed /= 2;
        }

        if (view_debug_ui) {
            cpp_imgui_render();
        }

        DrawFPS(0, 0);

        EndDrawing();
    }

    cpp_end();
    UnloadTexture(texture);
    CloseWindow();
}
