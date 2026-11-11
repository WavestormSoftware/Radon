// Copyright (c) 2026 Wavestorm Software
// Radon UI drawn by raylib, with a raylib 3D scene shown in a panel. --screenshot out.png renders a few
// frames, saves and exits.
#include <radon_raylib.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
    const char* shot = nullptr;
    for (int i = 1; i + 1 < argc; i++)
        if (!std::strcmp(argv[i], "--screenshot")) shot = argv[i + 1];
    InitWindow(640, 420, "Radon + raylib");
    SetTargetFPS(60);
    rd::rl::Renderer* rr = rd::rl::create_renderer();
    rd::Context ui;
    // Font: $RD_FONT, the bundled one, or a common system font. The buffer must outlive the context.
    const char* paths[] = {std::getenv("RD_FONT"),
                           RD_FONT_PATH,
                           "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf",
                           "C:/Windows/Fonts/segoeui.ttf",
                           "/System/Library/Fonts/Helvetica.ttc"};
    int font_size = 0;
    unsigned char* font = nullptr;
    for (const char* p : paths)
        if (p && FileExists(p) && (font = LoadFileData(p, &font_size))) break;
    if (font)
        ui.set_default_font(ui.load_font(font, (size_t)font_size, 16.0f));
    else
        std::fprintf(stderr, "no font found; set RD_FONT=/path/to/font.ttf\n");

    RenderTexture2D rt = LoadRenderTexture(320, 200);
    rd::rl::set_texture(rr, 1, rt.texture);

    rd::Rect win{20, 20, 360, 340};
    int clicks = 0;
    bool spin = true;
    float angle = 0;
    for (int frame = 0; !WindowShouldClose(); frame++) {
        if (spin) angle += GetFrameTime();
        BeginTextureMode(rt);
        ClearBackground({24, 28, 40, 255});
        Camera3D cam{{3, 3, 3}, {0, 0, 0}, {0, 1, 0}, 45.0f, CAMERA_PERSPECTIVE};
        BeginMode3D(cam);
        DrawCubeV({0, 0, 0}, {1.5f + 0.3f * std::sin(angle), 1.5f, 1.5f}, ORANGE);
        DrawCubeWiresV({0, 0, 0}, {1.5f + 0.3f * std::sin(angle), 1.5f, 1.5f}, MAROON);
        DrawGrid(10, 1.0f);
        EndMode3D();
        EndTextureMode();

        rd::Input in;
        rd::rl::fill_input(in);
        rd::rl::apply_dpi(ui);
        ui.begin_frame(in, GetFrameTime());
        if (rd::begin_window(ui, "Radon in raylib", win)) {
            rd::layout_row(ui, 1, nullptr, 0, 0);
            char text[64];
            std::snprintf(text, sizeof text, "clicks: %d", clicks);
            rd::label(ui, text);
            if (rd::button(ui, "Click me", rd::ButtonStyle::Primary).clicked) clicks++;
            rd::checkbox(ui, "Spin", spin);
            rd::viewport(ui, 1, {320, 200}, true);
            rd::end_window(ui);
        }
        ui.end_frame();

        BeginDrawing();
        ClearBackground({12, 14, 20, 255});
        rd::rl::render(rr, ui);
        EndDrawing();
        if (shot && frame == 5) {
            Image img = LoadImageFromScreen();
            ExportImage(img, shot);
            UnloadImage(img);
            break;
        }
    }
    UnloadRenderTexture(rt);
    rd::rl::destroy_renderer(rr);
    CloseWindow();
    return 0;
}
