// Copyright (c) 2026 Wavestorm Software
#include <SDL3/SDL.h>

#include <cstring>
#include <memory>
#include <string>

#include "radon_sdl3_gl.hpp"

namespace rd::sdl3 {

struct Platform {
    SDL_Window* window = nullptr;
    SDL_GLContext gl = nullptr;
    bool gles = false;
    bool quit = false;
    bool text_input = false;
    bool finger_active = false;
    SDL_FingerID finger = 0;
    Uint64 last_ticks = 0;
    Vec2 pointer;
    bool down = false;
    bool touch = false;
};

namespace {

int map_key(SDL_Keycode k) {
    switch (k) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER: return KeyEnter;
        case SDLK_BACKSPACE: return KeyBackspace;
        case SDLK_DELETE: return KeyDelete;
        case SDLK_LEFT: return KeyLeft;
        case SDLK_RIGHT: return KeyRight;
        case SDLK_UP: return KeyUp;
        case SDLK_DOWN: return KeyDown;
        case SDLK_HOME: return KeyHome;
        case SDLK_END: return KeyEnd;
        case SDLK_TAB: return KeyTab;
        case SDLK_ESCAPE: return KeyEscape;
        case SDLK_SPACE: return KeySpace;
        case SDLK_A: return KeyA;
        case SDLK_C: return KeyC;
        case SDLK_V: return KeyV;
        case SDLK_X: return KeyX;
        default: return KeyNone;
    }
}

int clipboard_get(void*, char* out, int cap) {
    char* t = SDL_GetClipboardText();
    if (!t) return 0;
    int n = (int)SDL_strlen(t);
    if (n > cap) n = cap;
    std::memcpy(out, t, (size_t)n);
    SDL_free(t);
    return n;
}

void clipboard_set(void*, const char* text, int len) {
    SDL_SetClipboardText(std::string(text, (size_t)len).c_str());
}

bool make_window(Platform* p, const PlatformConfig& cfg, bool es) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, es ? 0 : 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, es ? SDL_GL_CONTEXT_PROFILE_ES : SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (cfg.resizable) flags |= SDL_WINDOW_RESIZABLE;
#if defined(__ANDROID__) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE))
    flags |= SDL_WINDOW_FULLSCREEN;
#endif
    p->window = SDL_CreateWindow(cfg.title, cfg.width, cfg.height, flags);
    if (!p->window) return false;
    p->gl = SDL_GL_CreateContext(p->window);
    if (!p->gl) {
        SDL_DestroyWindow(p->window);
        p->window = nullptr;
        return false;
    }
    p->gles = es;
    return true;
}

}  // namespace

Platform* create_platform(const PlatformConfig& cfg) {
    // Touch is handled through finger events so a tap never also produces a synthetic mouse event.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    if (!SDL_Init(SDL_INIT_VIDEO)) return nullptr;
    std::unique_ptr<Platform> p(new Platform);
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__) || (defined(__APPLE__) && defined(TARGET_OS_IPHONE))
    const bool prefer_es = true;
#else
    const bool prefer_es = false;
#endif
    if (!make_window(p.get(), cfg, prefer_es) && !make_window(p.get(), cfg, !prefer_es)) {
        SDL_Quit();
        return nullptr;
    }
    SDL_GL_MakeCurrent(p->window, p->gl);
    SDL_GL_SetSwapInterval(cfg.vsync ? 1 : 0);
    p->last_ticks = SDL_GetTicksNS();
    return p.release();
}

void destroy_platform(Platform* p) {
    if (!p) return;
    if (p->gl) SDL_GL_DestroyContext(p->gl);
    if (p->window) SDL_DestroyWindow(p->window);
    SDL_Quit();
    delete p;
}

float dpi_scale(const Platform* p) {
    float d = SDL_GetWindowPixelDensity(p->window);
    return d > 0 ? d : 1.0f;
}

bool is_gles(const Platform* p) { return p->gles; }
void request_quit(Platform* p) { p->quit = true; }

void framebuffer_size(const Platform* p, int& w, int& h) { SDL_GetWindowSizeInPixels(p->window, &w, &h); }

void attach(Platform* p, Context& ctx) {
    ctx.set_dpi_scale(dpi_scale(p));
    ctx.set_clipboard_callbacks(clipboard_get, clipboard_set, nullptr);
}

bool poll(Platform* p, Input& in, float& dt) {
    in.pointer_pressed = in.pointer_released = false;
    in.wheel = 0;
    in.keys.clear();
    in.text.clear();

    int ww = 1, wh = 1;
    SDL_GetWindowSize(p->window, &ww, &wh);
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT: p->quit = true; break;
            case SDL_EVENT_MOUSE_MOTION:
                p->pointer = {e.motion.x, e.motion.y};
                p->touch = false;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    p->pointer = {e.button.x, e.button.y};
                    p->touch = false;
                    p->down = in.pointer_pressed = true;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    p->pointer = {e.button.x, e.button.y};
                    p->down = false;
                    in.pointer_released = true;
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                in.wheel += e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -e.wheel.y : e.wheel.y;
                break;
            case SDL_EVENT_FINGER_DOWN:
                if (!p->finger_active) {
                    p->finger_active = true;
                    p->finger = e.tfinger.fingerID;
                    p->pointer = {e.tfinger.x * (float)ww, e.tfinger.y * (float)wh};
                    p->touch = true;
                    p->down = in.pointer_pressed = true;
                }
                break;
            case SDL_EVENT_FINGER_MOTION:
                if (p->finger_active && p->finger == e.tfinger.fingerID)
                    p->pointer = {e.tfinger.x * (float)ww, e.tfinger.y * (float)wh};
                break;
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_CANCELED:
                if (p->finger_active && p->finger == e.tfinger.fingerID) {
                    p->pointer = {e.tfinger.x * (float)ww, e.tfinger.y * (float)wh};
                    p->finger_active = false;
                    p->down = false;
                    in.pointer_released = true;
                }
                break;
            case SDL_EVENT_KEY_DOWN: {
                int k = map_key(e.key.key);
                if (k == KeyNone) break;
                const SDL_Keymod m = e.key.mod;
                bool ctrl = (m & SDL_KMOD_CTRL) != 0;
#if defined(__APPLE__)
                ctrl = ctrl || (m & SDL_KMOD_GUI) != 0;
#endif
                in.keys.push({k, (m & SDL_KMOD_SHIFT) != 0, ctrl, (m & SDL_KMOD_ALT) != 0});
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
                for (const char* c = e.text.text; *c; c++) in.text.push(*c);
                break;
            default: break;
        }
    }
    in.pointer = p->pointer;
    in.pointer_down = p->down;
    in.touch = p->touch;

    SDL_Rect safe{0, 0, ww, wh};
    in.safe_area = {};
    if (SDL_GetWindowSafeArea(p->window, &safe)) {
        in.safe_area = {(float)safe.x, (float)safe.y, (float)(ww - safe.x - safe.w), (float)(wh - safe.y - safe.h)};
        if (in.safe_area.x < 0 || in.safe_area.y < 0 || in.safe_area.w < 0 || in.safe_area.h < 0) in.safe_area = {};
    }

    Uint64 now = SDL_GetTicksNS();
    dt = (float)((double)(now - p->last_ticks) * 1e-9);
    p->last_ticks = now;
    if (dt > 0.1f) dt = 0.1f;  // a stall must not fast-forward animations
    return !p->quit;
}

void finish_frame(Platform* p, Context& ctx) {
    const bool want = ctx.wants_text_input();
    if (want != p->text_input) {
        if (want) SDL_StartTextInput(p->window);
        else SDL_StopTextInput(p->window);
        p->text_input = want;
    }
    SDL_GL_SwapWindow(p->window);
}

}  // namespace rd::sdl3
