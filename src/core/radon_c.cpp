// Copyright (c) 2026 Wavestorm Software
#include <radon/radon.hpp>
#include <radon/radon_c.h>

#include <cstring>

static_assert(sizeof(rd_vertex) == sizeof(rd::DrawVertex), "rd_vertex must match rd::DrawVertex");
static_assert(sizeof(rd_draw_command) == sizeof(rd::DrawCommand), "rd_draw_command must match");

struct rd_context {
    rd::Context ctx;
};

static rd::Context& C(rd_context* c) {
    return c->ctx;
}

extern "C" {

rd_context* rd_create(void) {
    return new rd_context();
}
void rd_destroy(rd_context* c) {
    delete c;
}
int rd_load_font(rd_context* c, const uint8_t* ttf, size_t size, float px) {
    return c ? C(c).load_font(ttf, size, px) : -1;
}
void rd_set_default_font(rd_context* c, int f) {
    C(c).set_default_font(f);
}
void rd_set_dpi_scale(rd_context* c, float s) {
    C(c).set_dpi_scale(s);
}

void rd_begin_frame(rd_context* c, const rd_input* in, float dt) {
    rd::Input i;
    i.pointer = {in->pointer_x, in->pointer_y};
    i.pointer_down = in->pointer_down != 0;
    i.pointer_pressed = in->pointer_pressed != 0;
    i.pointer_released = in->pointer_released != 0;
    i.secondary_pressed = in->secondary_pressed != 0;
    i.touch = in->touch != 0;
    i.wheel = in->wheel;
    i.screen_size = {in->screen_w, in->screen_h};
    for (int k = 0; in->keys && k < in->key_count; k++) {
        const int m = in->key_mods ? in->key_mods[k] : 0;
        i.keys.push({in->keys[k], (m & 1) != 0, (m & 2) != 0, (m & 4) != 0});
    }
    if (in->text_utf8)
        for (const char* p = in->text_utf8; *p; p++) i.text.push(*p);
    C(c).begin_frame(i, dt);
}
void rd_end_frame(rd_context* c) {
    C(c).end_frame();
}

const rd_vertex* rd_vertices(const rd_context* c, int* n) {
    *n = c->ctx.draw_list().vertex_count();
    return reinterpret_cast<const rd_vertex*>(c->ctx.draw_list().vertices());
}
const uint32_t* rd_indices(const rd_context* c, int* n) {
    *n = c->ctx.draw_list().index_count();
    return c->ctx.draw_list().indices();
}
const rd_draw_command* rd_commands(const rd_context* c, int* n) {
    *n = c->ctx.draw_list().command_count();
    return reinterpret_cast<const rd_draw_command*>(c->ctx.draw_list().commands());
}
const uint8_t* rd_atlas(const rd_context* c, int* size, uint32_t* version) {
    *size = c->ctx.atlas_size();
    *version = c->ctx.atlas_version();
    return c->ctx.atlas_pixels();
}
int rd_pointer_over_ui(const rd_context* c) {
    return c->ctx.pointer_over_ui();
}
int rd_wants_text_input(const rd_context* c) {
    return c->ctx.wants_text_input();
}
int rd_cursor(const rd_context* c) {
    return (int)c->ctx.cursor();
}

int rd_begin_window(rd_context* c, const char* title, rd_rect* r) {
    rd::Rect rect{r->x, r->y, r->w, r->h};
    const bool open = rd::begin_window(C(c), title, rect);
    *r = {rect.x, rect.y, rect.w, rect.h};
    return open;
}
void rd_end_window(rd_context* c) {
    rd::end_window(C(c));
}
void rd_layout_row(rd_context* c, int cols, const float* w, int n, float h) {
    rd::layout_row(C(c), cols, w, n, h);
}
void rd_label(rd_context* c, const char* t) {
    rd::label(C(c), t);
}
int rd_button(rd_context* c, const char* l) {
    return rd::button(C(c), l).clicked;
}
int rd_checkbox(rd_context* c, const char* l, int* v) {
    bool b = *v != 0;
    const bool changed = rd::checkbox(C(c), l, b).changed;
    *v = b;
    return changed;
}
int rd_slider(rd_context* c, const char* l, float* v, float lo, float hi) {
    return rd::slider(C(c), l, *v, lo, hi).changed;
}
int rd_textbox(rd_context* c, const char* l, char* buf, int cap) {
    if (cap <= 0) return 0;
    buf[cap - 1] = 0;
    rd::TextBuffer tb{buf, cap, (int)std::strlen(buf)};
    return rd::textbox(C(c), l, tb).changed;
}
int rd_viewport(rd_context* c, uint32_t tex, float w, float h, int flip, float* local) {
    const rd::ViewportResponse v = rd::viewport(C(c), tex, {w, h}, flip != 0);
    if (local) local[0] = v.local.x, local[1] = v.local.y;
    return (v.hovered ? 1 : 0) | (v.focused ? 2 : 0) | (v.pressed ? 4 : 0) | (v.down ? 8 : 0) |
           (v.released ? 16 : 0);
}

int rd_begin_window_ex(rd_context* c, const char* title, rd_rect* r, uint32_t flags) {
    rd::Rect rect{r->x, r->y, r->w, r->h};
    const bool open = rd::begin_window(C(c), title, rect, flags);
    *r = {rect.x, rect.y, rect.w, rect.h};
    return open;
}
void rd_open_window(rd_context* c, const char* title) {
    rd::open_window(C(c), title);
}
void rd_label_wrapped(rd_context* c, const char* t) {
    rd::label_wrapped(C(c), t);
}
int rd_button_styled(rd_context* c, const char* l, int style) {
    const rd::ButtonStyle st = style == 1   ? rd::ButtonStyle::Primary
                               : style == 2 ? rd::ButtonStyle::Ghost
                               : style == 3 ? rd::ButtonStyle::Danger
                                            : rd::ButtonStyle::Default;
    return rd::button(C(c), l, st).clicked;
}
void rd_progress_bar(rd_context* c, float f) {
    rd::progress_bar(C(c), f);
}
int rd_drag_float(rd_context* c, const char* l, float* v, float speed) {
    return rd::drag_float(C(c), l, *v, speed).changed;
}
int rd_collapsing_header(rd_context* c, const char* l, int* open) {
    bool b = *open != 0;
    const bool r = rd::collapsing_header(C(c), l, b);
    *open = b;
    return r;
}
int rd_combo(rd_context* c, const char* l, int* sel, const char* const* items, int n) {
    return rd::combo(C(c), l, *sel, items, n);
}
void rd_image(rd_context* c, uint32_t tex, float w, float h) {
    rd::image(C(c), tex, {w, h});
}

}  // extern "C"
