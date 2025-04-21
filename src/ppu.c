#include "ppu.h"
#include "raylib.h"
#include "types.h"
#include "cpu.h"

extern cpu_t cpu;

void *framebuffer[WINDOW_WIDTH * WINDOW_HEIGHT * 4] = {0};

void try_step_cpu(void) {
    execute();
}

void ui(void) {
    SetTraceLogLevel(LOG_ERROR);
    SetTargetFPS(60);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "snes");
    Image framebuffer_image = {framebuffer, WINDOW_WIDTH, WINDOW_HEIGHT, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture texture = LoadTextureFromImage(framebuffer_image);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        cpu.remaining_clocks += 357368;
        try_step_cpu();

        UpdateTexture(texture, framebuffer);
        DrawTexturePro(texture, (Rectangle){0, 0, WINDOW_WIDTH, WINDOW_HEIGHT},
                       (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()},
                       (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadTexture(texture);
    CloseWindow();
}
