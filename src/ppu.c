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

        if (ppu.beam_x == 0 && ppu.beam_y == 225)
            cpu.memory.vblank_has_occurred = true;
        if (ppu.beam_x == 339 && ppu.beam_y == 261)
            cpu.memory.vblank_has_occurred = false;

        if (ppu.beam_x > 21 && ppu.beam_x < 278 && ppu.beam_y > 0 &&
            ppu.beam_y < 225) {
            // TODO: proper rendering I suppose

            set_pixel(ppu.beam_x - 22, ppu.beam_y - 1, 0xff000000);
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

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        // cpu.remaining_clocks += 357368;
        // try_step_cpu();
        for (uint32_t i = 0; i < 341 * 262; i++) {
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

        if (view_debug_ui) {
            cpp_imgui_render();
        }
        // view_debug_ui = true;

        EndDrawing();
    }

    cpp_end();
    UnloadTexture(texture);
    CloseWindow();
}
