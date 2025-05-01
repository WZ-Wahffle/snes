#include "ppu.h"
#include "raylib.h"
#include "spc.h"
#include "types.h"

extern cpu_t cpu;
extern ppu_t ppu;
extern spc_t spc;

uint8_t framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};

void set_pixel(uint16_t x, uint16_t y, uint32_t color) {
    ((uint32_t *)framebuffer)[x + WINDOW_WIDTH * y] = color;
}

void try_step_cpu(void) {
    static bool prev_vblank = false;
    if (cpu.remaining_clocks > 0) {
        cpu_execute();
        if (cpu.breakpoint_valid && TO_U24(cpu.pc, cpu.pbr) == cpu.breakpoint) {
            cpu.state = STATE_STOPPED;
        }
        bool curr_vblank =
            cpu.vblank_nmi_enable && cpu.memory.vblank_has_occurred;
        if (curr_vblank && !prev_vblank) {
            if (!cpu.emulation_mode)
                push_8(cpu.pbr);
            push_16(cpu.pc);
            push_8(cpu.p);
            cpu.pc = read_16(cpu.emulation_mode ? 0xfffa : 0xffea, 0);
            cpu.pbr = 0;
        } else if (!get_status_bit(STATUS_IRQOFF) && cpu.irq) {
            if (!cpu.emulation_mode)
                push_8(cpu.pbr);
            push_16(cpu.pc);
            push_8(cpu.p);
            cpu.pc = read_16(cpu.emulation_mode ? 0xfffe : 0xffee, 0);
            cpu.pbr = 0;
            cpu.irq = false;
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
    }
}

uint32_t r5g5b5_to_r8g8b8a8(uint16_t in) {
    uint32_t ret = 0xff000000;
    ret |= ((in >> 0) & 0x1f) << 3;
    ret |= ((in >> 5) & 0x1f) << 11;
    ret |= ((in >> 10) & 0x1f) << 19;
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
    case BPP_8:
        TODO("8bpp");
        break;
    default:
        UNREACHABLE_SWITCH(bpp);
    }
}
/*
void draw_bg1(uint16_t y, color_depth_t bpp) {
    uint32_t *target = (uint32_t *)(framebuffer + (WINDOW_WIDTH * 4 * y));

    uint8_t tilemap_w = ppu.bg_config[0].double_h_tilemap ? 64 : 32;
    uint8_t tilemap_h = ppu.bg_config[0].double_v_tilemap ? 64 : 32;
    uint16_t tilemap_line_pointer = ppu.bg_config[0].tilemap_addr;
    tilemap_line_pointer +=
        (((y + ppu.bg_config[0].v_scroll) / 8) % tilemap_h) * tilemap_w;
    uint16_t tilemap_line_index = 0;

    uint16_t tilemap_fetch[64] = {0};
    for (uint8_t i = 0; i < 64; i++) {
        tilemap_fetch[i] =
            TO_U16(ppu.vram[tilemap_line_pointer + tilemap_line_index * 2],
                   ppu.vram[tilemap_line_pointer + tilemap_line_index * 2 + 1]);
        tilemap_line_index++;
        if (tilemap_line_index > tilemap_w)
            tilemap_line_index %= tilemap_w;
    }

    uint8_t tile_size = ppu.bg_config[0].large_characters ? 16 : 8;
    for (uint16_t x = 0; x < WINDOW_WIDTH; x++) {
        uint8_t tilemap_idx =
            ((x + ppu.bg_config[0].h_scroll) / tile_size) % 64;
    }
}*/

void draw_obj(uint16_t y) {
    static uint8_t size_lut[8][4] = {
        {8, 8, 16, 16},   {8, 8, 32, 32},   {8, 8, 64, 64},   {16, 16, 32, 32},
        {16, 16, 64, 64}, {32, 32, 64, 64}, {16, 32, 32, 64}, {16, 32, 32, 32}};
    uint8_t draw_count = 0;

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

    for (int8_t i = 127; i >= 0; i--) {
        if (ppu.oam[i].draw_this_line) {
            int16_t sp_x = ppu.oam[i].x;
            uint8_t sp_w =
                size_lut[ppu.obj_sprite_size][ppu.oam[i].use_second_size * 2];
            uint8_t sp_h = size_lut[ppu.obj_sprite_size]
                                   [ppu.oam[i].use_second_size * 2 + 1];
            uint8_t y_off = y - ppu.oam[i].y;
            if (ppu.oam[i].flip_v)
                y_off = (sp_h - 1) - y_off;

            // The 4 bytes needed per 4bpp line of 8 pixels are prefetched
            // here. Since the maximum number of these lines per sprite is 8
            // (64 pixels / (8 pixels / line)), a maximum buffer size of 32
            // ((4 bytes / line) * 8 lines) is allocated.
            uint8_t tiles[64] = {0};
            fetch_tile_color_row(
                (ppu.oam[i].use_second_sprite_page ? name_alt : name_base) +
                    ppu.oam[i].tile_idx * 32,
                y_off, sp_w, sp_h, BPP_4, tiles);

            for (int16_t x_off = 0; x_off < sp_w; x_off++) {
                int16_t x =
                    sp_x + (ppu.oam[i].flip_h ? (sp_w - (x_off + 1)) : x_off);
                if (x < 0 || x > 255)
                    continue;
                uint32_t col = r5g5b5_to_r8g8b8a8(
                    ppu.cgram[128 + ppu.oam[i].palette * 16 + tiles[x_off]]);

                if (tiles[x_off] != 0)
                    target[x] = col;
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
            }
        }

        if (ppu.beam_x == 0 && ppu.beam_y == 225)
            cpu.memory.vblank_has_occurred = true;
        if (ppu.beam_x == 339 && ppu.beam_y == 261)
            cpu.memory.vblank_has_occurred = false;

        if (ppu.beam_x == 22 && ppu.beam_y > 0 && ppu.beam_y < 225) {
            for (uint16_t i = 0; i < 256 * 4; i++) {
                framebuffer[(ppu.beam_y - 1) * WINDOW_WIDTH * 4 + i] = 0;
            }
            draw_obj(ppu.beam_y - 1);
        }
    }
}

void ui(void) {
    SetTraceLogLevel(LOG_ERROR);
    SetTargetFPS(60);
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
        ClearBackground(BLACK);

        // cpu.remaining_clocks += 357368;
        // try_step_cpu();
        for (uint32_t i = 0; i < 341 * 262 * cpu.speed; i++) {
            switch (cpu.state) {
            case STATE_STOPPED:
                // this page intentionally left blank
                break;
            case STATE_STEPPED:
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

        if (cpu.memory.joy_auto_read) {
            cpu.memory.joy1l = 0;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) << 4;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1) << 5;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_UP) << 6;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) << 7;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) << 8;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) << 9;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) << 10;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP) << 11;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT) << 12;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT) << 13;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN) << 14;
            cpu.memory.joy1l |=
                IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) << 15;
        }

        UpdateTexture(texture, framebuffer);
        DrawTexturePro(texture, (Rectangle){0, 0, WINDOW_WIDTH, WINDOW_HEIGHT},
                       (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
                       (Vector2){0, 0}, 0.0f, WHITE);

        if (IsKeyPressed(KEY_TAB)) {
            view_debug_ui = !view_debug_ui;
        }

        if (IsKeyPressed(KEY_F10)) {
            cpu.state = STATE_RUNNING;
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
