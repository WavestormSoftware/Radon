// Copyright (c) 2026 Wavestorm Software
#include "../internal.hpp"

namespace rd {
using namespace detail;

float titlebar_height(const Context& ctx) {
    return ctx.theme().cell_height + 8;
}

void window_background(Context& ctx, const Paint& fill) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const Rect r{0, 0, s.in.screen_size.x, s.in.screen_size.y};
    const float rad = s.in.window_maximized ? 0.0f : t.window_radius;
    s.dl.fill(r, fill.empty() ? pick(t.window_paint, t.background) : fill, rad);
    s.dl.rect(r, t.border, rad, t.border_width);
}

bool titlebar_begin(Context& ctx, std::string_view title, uint32_t flags, TextureId icon) {
    State& s = state(ctx);
    const Theme& t = s.cfg.theme;
    const float W = s.in.screen_size.x, H = titlebar_height(ctx);
    const bool focused = s.in.window_focused, maxed = s.in.window_maximized;
    const Rect bar{0, 0, W, H};
    s.chrome.caption = bar;
    s.tb_active = true;

    s.dl.fill(bar, pick(t.window_paint, t.surface), Corners::top(maxed ? 0.0f : t.window_radius));
    s.dl.rect({0, H - 1, W, 1}, t.border);

    // Buttons, right to left: close, maximize / restore, minimize.
    const float bw = 46;
    float edge = W;
    auto button = [&](Icon ic, const char* name, bool close, WindowCommand cmd) {
        Rect r{edge - bw, 0, bw, H};
        edge -= bw;
        const ID id = get_id(s, name);
        Response rs = update_control(ctx, id, r, false, false);
        a11y_node(s, A11yRole::Button, name);
        float press = 0;
        const float h = animate(s, id, rs.hovered, s.active == id, &press);
        const Color base = close ? Color{0xe8, 0x3a, 0x4a, 255} : t.surface_active;
        s.dl.rect({r.x + 3, r.y + 5, r.w - 6, r.h - 10}, with_alpha(base, clamp01(h * 0.85f + press * 0.15f)),
                  t.corner_radius);
        const Color ink = (close && h > 0.5f) ? Color{255, 255, 255, 255} : (focused ? t.text : t.text_muted);
        draw_icon(ctx, ic, {r.x + (r.w - 18) / 2, (H - 18) / 2, 18, 18}, ink);
        if (rs.clicked) s.wcmd = cmd;
    };
    button(Icon::Close, "Close window", true, WindowCommand::Close);
    if (!(flags & TitlebarNoMaximize))
        button(maxed ? Icon::Restore : Icon::Maximize, maxed ? "Restore window" : "Maximize window", false,
               maxed ? WindowCommand::Restore : WindowCommand::Maximize);
    if (!(flags & TitlebarNoMinimize)) button(Icon::Minimize, "Minimize window", false, WindowCommand::Minimize);

    float x = 12;
    if (icon) {
        s.dl.image(icon, {x, (H - 18) / 2, 18, 18}, {0, 0, 1, 1}, {255, 255, 255, 255});
        x += 26;
    }
    const Color ink = focused ? t.text : t.text_muted;
    const float tw = text_size(s, t.font_body, title).x;
    if (flags & TitlebarCenterTitle) {
        draw_text_centered(s, t.font_body, {x, 0, edge - x, H}, title, ink, true);
    } else {
        draw_text_centered(s, t.font_body, {x, 0, tw, H}, title, ink, false);
        x += tw + 16;
    }

    s.tb_rect = {x, 0, std::fmax(0.0f, edge - x), H};
    if (!begin_window(ctx, "##titlebar", s.tb_rect, WindowTransparent | WindowNoPadding)) {
        s.tb_active = false;
        return false;
    }
    spacing(ctx, (H - t.cell_height) / 2);
    return true;
}

void titlebar_end(Context& ctx) {
    end_window(ctx);
    state(ctx).tb_active = false;
}

}  // namespace rd
