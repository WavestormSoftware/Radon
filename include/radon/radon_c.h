/* Copyright (c) 2026 Wavestorm Software */
/* C API for engines and languages that cannot use the C++ headers (Unity, Unreal, Godot, FFI).
 * Frame: rd_begin_frame -> widgets -> rd_end_frame -> read the draw list and render it yourself.
 * The vertex/command layouts are the ones in docs/backends.md. Only a core widget set is wrapped;
 * it grows on request. Not thread-safe: use one context from one thread. */
#ifndef RADON_C_H
#define RADON_C_H
#include <stddef.h>
#include <stdint.h>

/* Build Radon as a shared library with -DRD_C_BUILD_SHARED, and consume it with -DRD_C_SHARED. */
#if defined(_WIN32) && (defined(RD_C_BUILD_SHARED) || defined(RD_C_SHARED))
#ifdef RD_C_BUILD_SHARED
#define RD_API __declspec(dllexport)
#else
#define RD_API __declspec(dllimport)
#endif
#elif defined(RD_C_BUILD_SHARED)
#define RD_API __attribute__((visibility("default")))
#else
#define RD_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rd_context rd_context;

typedef struct { float x, y, w, h; } rd_rect;

/* One frame of input. Positions in logical pixels, top-left origin. */
typedef struct {
    float pointer_x, pointer_y;
    int pointer_down, pointer_pressed, pointer_released, secondary_pressed, touch;
    float wheel;
    float screen_w, screen_h;
    const int* keys;        /* rd_key values pressed this frame (shift/ctrl/alt in key_mods), may be NULL */
    const int* key_mods;    /* bit0 shift, bit1 ctrl, bit2 alt; same length as keys */
    int key_count;
    const char* text_utf8;  /* typed text this frame, NUL-terminated, may be NULL */
} rd_input;

enum {
    RD_KEY_NONE = 0, RD_KEY_ENTER, RD_KEY_BACKSPACE, RD_KEY_DELETE, RD_KEY_LEFT, RD_KEY_RIGHT, RD_KEY_UP,
    RD_KEY_DOWN, RD_KEY_HOME, RD_KEY_END, RD_KEY_TAB, RD_KEY_ESCAPE, RD_KEY_SPACE, RD_KEY_A, RD_KEY_C,
    RD_KEY_V, RD_KEY_X
};

/* Same memory layout as rd::DrawVertex (44 bytes) and rd::DrawCommand (32 bytes). */
typedef struct {
    float pos[2], uv[2];
    uint8_t color[4];
    float local[2], shape[4];
} rd_vertex;
typedef struct {
    uint32_t texture;     /* 0 = glyph atlas, else your own id (see rd_viewport / rd_image) */
    float clip[4];        /* x, y, w, h in logical px */
    uint32_t index_offset, index_count, layer;
} rd_draw_command;

RD_API rd_context* rd_create(void);
RD_API void rd_destroy(rd_context* c);
/* Font data must outlive the context. Returns the font handle or -1. Call before the first frame. */
RD_API int rd_load_font(rd_context* c, const uint8_t* ttf, size_t size, float pixel_size);
RD_API void rd_set_default_font(rd_context* c, int font);
RD_API void rd_set_dpi_scale(rd_context* c, float scale);

RD_API void rd_begin_frame(rd_context* c, const rd_input* in, float dt_seconds);
RD_API void rd_end_frame(rd_context* c);

/* Draw list, valid until the next rd_begin_frame. */
RD_API const rd_vertex* rd_vertices(const rd_context* c, int* count);
RD_API const uint32_t* rd_indices(const rd_context* c, int* count);
RD_API const rd_draw_command* rd_commands(const rd_context* c, int* count);
/* Glyph atlas: 8-bit coverage, size x size. Re-upload when the version changes. */
RD_API const uint8_t* rd_atlas(const rd_context* c, int* size, uint32_t* version);
RD_API int rd_pointer_over_ui(const rd_context* c);
RD_API int rd_wants_text_input(const rd_context* c);
RD_API int rd_cursor(const rd_context* c); /* 0 arrow, 1 text, 2 hand, 3 resize EW, 4 resize NS, 5 resize NWSE */

/* Widgets. Strings are UTF-8, NUL-terminated. Window rect is read and written (move/resize). */
RD_API int rd_begin_window(rd_context* c, const char* title, rd_rect* rect); /* call rd_end_window only if 1 */
RD_API void rd_end_window(rd_context* c);
RD_API void rd_layout_row(rd_context* c, int columns, const float* widths, int count, float height);
RD_API void rd_label(rd_context* c, const char* text);
RD_API int rd_button(rd_context* c, const char* label);                    /* 1 when clicked */
RD_API int rd_checkbox(rd_context* c, const char* label, int* value);      /* 1 when changed */
RD_API int rd_slider(rd_context* c, const char* label, float* value, float min, float max); /* 1 when changed */
/* Edits `buf` in place (capacity includes the NUL). 1 when the text changed. */
RD_API int rd_textbox(rd_context* c, const char* label, char* buf, int capacity);
/* WindowFlags bits as in rd::WindowFlags: 1 movable, 2 resizable, 4 closable, 8 scrollable, 16 title bar, 32 border. */
RD_API int rd_begin_window_ex(rd_context* c, const char* title, rd_rect* rect, uint32_t flags);
RD_API void rd_open_window(rd_context* c, const char* title); /* reopen a window closed with its X */
RD_API void rd_label_wrapped(rd_context* c, const char* text);
/* style: 0 default, 1 primary, 2 ghost, 3 danger */
RD_API int rd_button_styled(rd_context* c, const char* label, int style);
RD_API void rd_progress_bar(rd_context* c, float fraction);
RD_API int rd_drag_float(rd_context* c, const char* label, float* value, float speed);
/* Returns 1 while open. */
RD_API int rd_collapsing_header(rd_context* c, const char* label, int* open);
/* items: array of `count` UTF-8 strings. Returns 1 when the selection changed. */
RD_API int rd_combo(rd_context* c, const char* label, int* selected, const char* const* items, int count);
/* Draws your texture (uv = full image). */
RD_API void rd_image(rd_context* c, uint32_t texture, float w, float h);
/* Shows your texture; result flags are bits: 1 hovered, 2 focused, 4 pressed, 8 down, 16 released.
 * local_xy receives the pointer relative to the view. */
RD_API int rd_viewport(rd_context* c, uint32_t texture, float w, float h, int flip_y, float* local_xy);

#ifdef __cplusplus
}
#endif
#endif
