// Copyright (c) 2026 Wavestorm Software
#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#include <cstring>
#include <memory>
#include <mutex>
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
    bool wait_events = false, idle = false, background = false, theme_changed = false, can_dismiss = false;
    int quiet = 0;  // polls in a row without events
    SDL_Cursor* cursors[6] = {};
    Cursor cursor = Cursor::Arrow;
    Rect text_area;
};

namespace {

int map_key(SDL_Keycode k) {
    switch (k) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            return KeyEnter;
        case SDLK_BACKSPACE:
            return KeyBackspace;
        case SDLK_DELETE:
            return KeyDelete;
        case SDLK_LEFT:
            return KeyLeft;
        case SDLK_RIGHT:
            return KeyRight;
        case SDLK_UP:
            return KeyUp;
        case SDLK_DOWN:
            return KeyDown;
        case SDLK_HOME:
            return KeyHome;
        case SDLK_END:
            return KeyEnd;
        case SDLK_TAB:
            return KeyTab;
        case SDLK_ESCAPE:
        case SDLK_AC_BACK:  // Android back button: closes popups, dialogs and the keyboard
            return KeyEscape;
        case SDLK_SPACE:
            return KeySpace;
        case SDLK_A:
            return KeyA;
        case SDLK_C:
            return KeyC;
        case SDLK_V:
            return KeyV;
        case SDLK_X:
            return KeyX;
        default:
            return KeyNone;
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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        es ? SDL_GL_CONTEXT_PROFILE_ES : SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (cfg.resizable) flags |= SDL_WINDOW_RESIZABLE;
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
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

// The dialog callback can run on another thread, and after the platform is gone: keep it static.
std::mutex g_dialog_mutex;
std::string g_dialog_path;
bool g_dialog_done = false;

void SDLCALL dialog_done(void*, const char* const* files, int) {
    std::lock_guard<std::mutex> lock(g_dialog_mutex);
    g_dialog_path = (files && files[0]) ? files[0] : "";
    g_dialog_done = true;
}

/// Window coordinates (mouse, touch, safe area) -> logical pixels.
float window_to_logical(const Platform* p) {
    const float density = SDL_GetWindowPixelDensity(p->window);
    return (density > 0 ? density : 1.0f) / dpi_scale(p);
}

void handle_event(Platform* p, Input& in, const SDL_Event& e, float k, int ww, int wh) {
    switch (e.type) {
        case SDL_EVENT_QUIT:
            p->quit = true;
            break;
        case SDL_EVENT_DID_ENTER_BACKGROUND:
            p->background = true;
            break;
        case SDL_EVENT_WILL_ENTER_FOREGROUND:
        case SDL_EVENT_DID_ENTER_FOREGROUND:
            p->background = false;
            break;
        case SDL_EVENT_SYSTEM_THEME_CHANGED:
            p->theme_changed = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            p->pointer = {e.motion.x * k, e.motion.y * k};
            p->touch = false;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            p->pointer = {e.button.x * k, e.button.y * k};
            p->touch = false;
            if (e.button.button == SDL_BUTTON_LEFT) {
                p->down = in.pointer_pressed = true;
                SDL_CaptureMouse(true);  // keep receiving the drag outside the window
            } else if (e.button.button == SDL_BUTTON_RIGHT) {
                in.secondary_pressed = true;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.button == SDL_BUTTON_LEFT) {
                p->pointer = {e.button.x * k, e.button.y * k};
                p->down = false;
                in.pointer_released = true;
                SDL_CaptureMouse(false);
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            in.wheel += e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -e.wheel.y : e.wheel.y;
            break;
        case SDL_EVENT_FINGER_DOWN:
            if (!p->finger_active) {
                p->finger_active = true;
                p->finger = e.tfinger.fingerID;
                p->pointer = {e.tfinger.x * (float)ww * k, e.tfinger.y * (float)wh * k};
                p->touch = true;
                p->down = in.pointer_pressed = true;
            }
            break;
        case SDL_EVENT_FINGER_MOTION:
            if (p->finger_active && p->finger == e.tfinger.fingerID)
                p->pointer = {e.tfinger.x * (float)ww * k, e.tfinger.y * (float)wh * k};
            break;
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED:
            if (p->finger_active && p->finger == e.tfinger.fingerID) {
                p->pointer = {e.tfinger.x * (float)ww * k, e.tfinger.y * (float)wh * k};
                p->finger_active = false;
                p->down = false;
                in.pointer_released = true;
            }
            break;
        case SDL_EVENT_KEY_DOWN: {
            if (e.key.key == SDLK_AC_BACK && !p->can_dismiss) {
                p->quit = true;  // nothing to close: back leaves the app, as Android users expect
                break;
            }
            int key = map_key(e.key.key);
            if (key == KeyNone) break;
            const SDL_Keymod m = e.key.mod;
            bool ctrl = (m & SDL_KMOD_CTRL) != 0;
#if defined(__APPLE__)
            ctrl = ctrl || (m & SDL_KMOD_GUI) != 0;  // Cmd+C/V/X/A
#endif
            in.keys.push({key, (m & SDL_KMOD_SHIFT) != 0, ctrl, (m & SDL_KMOD_ALT) != 0});
            break;
        }
        case SDL_EVENT_TEXT_INPUT:
            for (const char* c = e.text.text; *c; c++) in.text.push(*c);
            break;
        default:
            break;
    }
}

}  // namespace

Platform* create_platform(const PlatformConfig& cfg) {
    // Touch is handled through finger events so a tap never also produces a synthetic mouse event.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");  // delivered as SDLK_AC_BACK
    if (!SDL_Init(SDL_INIT_VIDEO)) return nullptr;
    std::unique_ptr<Platform> p(new Platform);
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
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
    p->wait_events = cfg.wait_events;
    p->last_ticks = SDL_GetTicksNS();
    return p.release();
}

void destroy_platform(Platform* p) {
    if (!p) return;
    for (SDL_Cursor* c : p->cursors)
        if (c) SDL_DestroyCursor(c);
    if (p->gl) SDL_GL_DestroyContext(p->gl);
    if (p->window) SDL_DestroyWindow(p->window);
    SDL_Quit();
    delete p;
}

float dpi_scale(const Platform* p) {
    // The display scale is what the OS wants UI multiplied by: 2 on Retina, 1.5 on a 150% Windows
    // or Linux desktop, 2.6 on a dense phone. (Pixel density alone is 1 on Android and Windows.)
    float d = SDL_GetWindowDisplayScale(p->window);
    if (d <= 0) d = SDL_GetWindowPixelDensity(p->window);
    return d > 0 ? d : 1.0f;
}

bool is_gles(const Platform* p) {
    return p->gles;
}
void request_quit(Platform* p) {
    p->quit = true;
}

void framebuffer_size(const Platform* p, int& w, int& h) {
    SDL_GetWindowSizeInPixels(p->window, &w, &h);
}

void logical_size(const Platform* p, int& w, int& h) {
    framebuffer_size(p, w, h);
    const float d = dpi_scale(p);
    w = (int)((float)w / d);
    h = (int)((float)h / d);
}

bool system_dark(const Platform*) {
    return SDL_GetSystemTheme() != SDL_SYSTEM_THEME_LIGHT;
}

bool system_theme_changed(Platform* p) {
    const bool c = p->theme_changed;
    p->theme_changed = false;
    return c;
}

bool open_url(const char* url) {
    return SDL_OpenURL(url);
}

void show_file_dialog(Platform* p, FileDialog kind, const char* where) {
    if (kind == FileDialog::Open)
        SDL_ShowOpenFileDialog(dialog_done, nullptr, p->window, nullptr, 0, where, false);
    else if (kind == FileDialog::Save)
        SDL_ShowSaveFileDialog(dialog_done, nullptr, p->window, nullptr, 0, where);
    else
        SDL_ShowOpenFolderDialog(dialog_done, nullptr, p->window, where, false);
}

bool poll_file_dialog(Platform*, std::string& path) {
    std::lock_guard<std::mutex> lock(g_dialog_mutex);
    if (!g_dialog_done) return false;
    g_dialog_done = false;
    path = g_dialog_path;
    return true;
}

void attach(Platform* p, Context& ctx) {
    ctx.set_dpi_scale(dpi_scale(p));
    ctx.set_clipboard_callbacks(clipboard_get, clipboard_set, nullptr);
}

bool poll(Platform* p, Input& in, float& dt) {
    in.pointer_pressed = in.pointer_released = in.secondary_pressed = false;
    in.wheel = 0;
    in.keys.clear();
    in.text.clear();

    bool waited = false;
#if !defined(__EMSCRIPTEN__)
    if (p->wait_events && p->idle && p->quiet >= 2) {
        // Nothing is moving: sleep until input arrives. The timeout keeps the caret blinking.
        SDL_WaitEventTimeout(nullptr, 250);
        waited = true;
    }
#endif
    int ww = 1, wh = 1;
    SDL_GetWindowSize(p->window, &ww, &wh);
    const float k = window_to_logical(p);
    SDL_Event e;
    int events = 0;
    while (SDL_PollEvent(&e)) {
        events++;
        handle_event(p, in, e, k, ww, wh);
    }
#if !defined(__EMSCRIPTEN__)
    // In the background (mobile) the app must not draw: block until it returns.
    // shortcut: iOS can suspend before this runs; add an SDL event watch if GL calls in the
    // background ever show up in crash logs.
    while (p->background && !p->quit) {
        if (SDL_WaitEvent(&e)) handle_event(p, in, e, k, ww, wh);
        waited = true;
    }
#endif
    p->quiet = events ? 0 : p->quiet + 1;
    in.pointer = p->pointer;
    in.pointer_down = p->down;
    in.touch = p->touch;

    SDL_Rect safe{0, 0, ww, wh};
    in.safe_area = {};
    if (SDL_GetWindowSafeArea(p->window, &safe)) {
        in.safe_area = {(float)safe.x * k, (float)safe.y * k, (float)(ww - safe.x - safe.w) * k,
                        (float)(wh - safe.y - safe.h) * k};
        if (in.safe_area.x < 0 || in.safe_area.y < 0 || in.safe_area.w < 0 || in.safe_area.h < 0)
            in.safe_area = {};
    }

    Uint64 now = SDL_GetTicksNS();
    dt = (float)((double)(now - p->last_ticks) * 1e-9);
    p->last_ticks = now;
    if (dt > 0.1f && !(waited && dt < 0.3f)) dt = 0.1f;  // a stall must not fast-forward animations
    return !p->quit;
}

void finish_frame(Platform* p, Context& ctx) {
    const bool want = ctx.wants_text_input();
    if (want) {  // tell the IME / soft keyboard where the field is, in window coordinates
        const Rect r = ctx.text_input_rect();
        if (r.x != p->text_area.x || r.y != p->text_area.y || r.w != p->text_area.w ||
            r.h != p->text_area.h) {
            const float k = 1.0f / window_to_logical(p);
            const SDL_Rect area{(int)(r.x * k), (int)(r.y * k), (int)(r.w * k), (int)(r.h * k)};
            SDL_SetTextInputArea(p->window, &area, 0);
            p->text_area = r;
        }
    }
    if (want != p->text_input) {
        if (want)
            SDL_StartTextInput(p->window);
        else
            SDL_StopTextInput(p->window);
        p->text_input = want;
    }
    if (ctx.cursor() != p->cursor) {
        static const SDL_SystemCursor map[] = {SDL_SYSTEM_CURSOR_DEFAULT,   SDL_SYSTEM_CURSOR_TEXT,
                                               SDL_SYSTEM_CURSOR_POINTER,   SDL_SYSTEM_CURSOR_EW_RESIZE,
                                               SDL_SYSTEM_CURSOR_NS_RESIZE, SDL_SYSTEM_CURSOR_NWSE_RESIZE};
        p->cursor = ctx.cursor();
        SDL_Cursor*& c = p->cursors[(int)p->cursor];
        if (!c) c = SDL_CreateSystemCursor(map[(int)p->cursor]);
        if (c) SDL_SetCursor(c);
    }
    p->idle = !ctx.animating();
    p->can_dismiss = ctx.can_dismiss();
    SDL_GL_SwapWindow(p->window);
}

}  // namespace rd::sdl3
