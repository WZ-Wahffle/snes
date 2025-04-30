#include "ppu.h"
#include "raylib.h"
#include "spc.h"
#include "types.h"

extern cpu_t cpu;
extern ppu_t ppu;
extern spc_t spc;

void *framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};

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
        } else if (!get_status_bit(STATUS_IRQOFF) && cpu.irq) {
            if (!cpu.emulation_mode)
                push_8(cpu.pbr);
            push_16(cpu.pc);
            push_8(cpu.p);
            cpu.pc = read_16(cpu.emulation_mode ? 0xfffe : 0xffee, 0);
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

void draw_obj(uint16_t x, uint16_t y) {
    static uint8_t size_lut[8][4] = {
        {8, 8, 16, 16},   {8, 8, 32, 32},   {8, 8, 64, 64},   {16, 16, 32, 32},
        {16, 16, 64, 64}, {32, 32, 64, 64}, {16, 32, 32, 64}, {16, 32, 32, 32}};
    for (int8_t sprite_idx = 127; sprite_idx >= 0; sprite_idx--) {
        int16_t sp_x = ppu.oam[sprite_idx].x;
        int16_t sp_y = ppu.oam[sprite_idx].y;
        uint8_t sp_w = size_lut[ppu.obj_sprite_size]
                               [ppu.oam[sprite_idx].use_second_size * 2];
        uint8_t sp_h = size_lut[ppu.obj_sprite_size]
                               [ppu.oam[sprite_idx].use_second_size * 2 + 1];
        if (IN_INTERVAL(x, sp_x, sp_x + sp_w) &&
            IN_INTERVAL(y, sp_y, sp_y + sp_h)) {
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

        if (ppu.beam_x > 21 && ppu.beam_x < 278 && ppu.beam_y > 0 &&
            ppu.beam_y < 225) {

            uint16_t drawing_x = ppu.beam_x - 22;
            uint16_t drawing_y = ppu.beam_y - 1;

            draw_obj(drawing_x, drawing_y);
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
