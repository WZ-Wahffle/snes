#include "ppu.h"
#include "cpu.h"
#include "raylib.h"
#include "types.h"
#include "ui.h"

extern cpu_t cpu;
extern ppu_t ppu;

void *framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};

void set_pixel(uint16_t x, uint16_t y, uint32_t color) {
    ((uint32_t *)framebuffer)[x + WINDOW_WIDTH * y] = color;
}

void try_step_cpu(void) {
    if (cpu.remaining_clocks > 0) {
        execute();
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
                cpu.remaining_clocks = 1;
                cpu.state = STATE_STOPPED;
                try_step_cpu();
                try_step_ppu();
                break;
            case STATE_RUNNING:
                cpu.remaining_clocks += CYCLES_PER_DOT;
                ppu.remaining_clocks += CYCLES_PER_DOT;
                while (cpu.remaining_clocks > 0 && cpu.state != STATE_STOPPED) {
                    try_step_cpu();
                }
                try_step_ppu();
                break;
            }
        }

        if (IsKeyPressed(KEY_TAB)) {
            view_debug_ui = !view_debug_ui;
        }

        if (view_debug_ui) {
            cpp_imgui_render();
        }

        UpdateTexture(texture, framebuffer);
        DrawTexturePro(texture, (Rectangle){0, 0, WINDOW_WIDTH, WINDOW_HEIGHT},
                       (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
                       (Vector2){0, 0}, 0.0f, WHITE);

        EndDrawing();
    }

    cpp_end();
    UnloadTexture(texture);
    CloseWindow();
}
