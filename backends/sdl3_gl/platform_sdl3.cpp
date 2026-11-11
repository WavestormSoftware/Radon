// Copyright (c) 2026 Wavestorm Software
#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#include <cmath>
#include <cstring>
#include <memory>
#include <vector>
#include <mutex>
#include <string>

#include "radon_sdl3_gl.hpp"

#if defined(__ANDROID__)
#include <jni.h>
#endif
#if defined(__APPLE__) && TARGET_OS_IPHONE
extern "C" void rd_ios_haptic(int strength);  // haptics_ios.m: 1 selection, 2 light, 3 medium
#endif

namespace rd::sdl3 {

struct Platform {
    SDL_Window* window = nullptr;
    SDL_GLContext gl = nullptr;
    bool gles = false;
    bool quit = false;
    bool text_input = false;
    TextInputType text_type = TextInputType::Text;
    bool finger_active = false;
    SDL_FingerID finger = 0;  // the finger that drives the pointer
    Touch touches[kMaxTouches];
    int touch_count = 0;
    int height_before_keyboard = 0;  // window height when the soft keyboard was requested
    bool haptics = true;
    Uint64 last_ticks = 0;
    Vec2 pointer;
    bool down = false;
    bool touch = false;
    bool wait_events = false, idle = false, background = false, theme_changed = false, can_dismiss = false;
    int quiet = 0;  // polls in a row without events
    bool skip = false;
    std::vector<std::string> drops;  // dropped file paths, oldest first
    Uint64 last_draw = 0;
    SDL_Cursor* cursors[6] = {};
    std::vector<SDL_Gamepad*> pads;
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
        case SDLK_Z:
            return KeyZ;
        case SDLK_Y:
            return KeyY;
        case SDLK_PAGEUP:
            return KeyPageUp;
        case SDLK_PAGEDOWN:
            return KeyPageDown;
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
        case SDL_EVENT_DROP_FILE:
            if (e.drop.data && p->drops.size() < 64)
                p->drops.emplace_back(e.drop.data);  // bounded if never read
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
        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED: {
            const Vec2 pos{e.tfinger.x * (float)ww * k, e.tfinger.y * (float)wh * k};
            const uint64_t id = (uint64_t)e.tfinger.fingerID;
            int slot = -1;
            for (int i = 0; i < p->touch_count; i++)
                if (p->touches[i].id == id) slot = i;
            const bool up = e.type == SDL_EVENT_FINGER_UP || e.type == SDL_EVENT_FINGER_CANCELED;
            if (up) {
                if (slot >= 0) p->touches[slot] = p->touches[--p->touch_count];
            } else if (slot >= 0) {
                p->touches[slot].pos = pos;
            } else if (e.type == SDL_EVENT_FINGER_DOWN && p->touch_count < kMaxTouches) {
                p->touches[p->touch_count++] = {id, pos};
            }
            // The first finger down drives the pointer; extra fingers only feed gestures.
            if (e.type == SDL_EVENT_FINGER_DOWN && !p->finger_active) {
                p->finger_active = true;
                p->finger = e.tfinger.fingerID;
                p->pointer = pos;
                p->touch = true;
                p->down = in.pointer_pressed = true;
            } else if (p->finger_active && p->finger == e.tfinger.fingerID) {
                p->pointer = pos;
                if (up) {
                    p->finger_active = false;
                    p->down = false;
                    in.pointer_released = true;
                }
            }
            break;
        }
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
        case SDL_EVENT_GAMEPAD_ADDED:
            if (SDL_Gamepad* g = SDL_OpenGamepad(e.gdevice.which)) p->pads.push_back(g);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            for (size_t i = 0; i < p->pads.size(); i++)
                if (SDL_GetGamepadID(p->pads[i]) == e.gdevice.which) {
                    SDL_CloseGamepad(p->pads[i]);
                    p->pads.erase(p->pads.begin() + (long)i);
                    break;
                }
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {  // D-pad and face buttons drive widget focus like a keyboard
            int key = KeyNone;
            switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    key = KeyUp;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    key = KeyDown;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                    key = KeyLeft;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                    key = KeyRight;
                    break;
                case SDL_GAMEPAD_BUTTON_SOUTH:
                    key = KeySpace;
                    break;  // A: activate
                case SDL_GAMEPAD_BUTTON_EAST:
                    key = KeyEscape;
                    break;  // B: back
                case SDL_GAMEPAD_BUTTON_START:
                    key = KeyEnter;
                    break;
                case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                    key = KeyTab;
                    break;
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
                    in.keys.push({KeyTab, true, false, false});
                    break;
                default:
                    break;
            }
            if (key != KeyNone) in.keys.push({key, false, false, false});
            break;
        }
        case SDL_EVENT_TEXT_INPUT: {
            // Push whole code points only: a commit longer than the queue must not end mid-character.
            const char* c = e.text.text;
            const char* end = c + SDL_strlen(c);
            while (c < end) {
                uint32_t cp;
                const int len = utf8_decode(c, end, cp);
                if (in.text.count + len > (int)sizeof in.text.items) break;
                for (int i = 0; i < len; i++) in.text.push(c[i]);
                c += len;
            }
            break;
        }
        default:
            break;
    }
}

}  // namespace

int gamepad_count(const Platform* p) {
    return p ? (int)p->pads.size() : 0;
}

bool gamepad_state(const Platform* p, int index, GamepadState& out, float deadzone) {
    out = GamepadState{};
    if (!p || index < 0 || index >= (int)p->pads.size()) return false;
    SDL_Gamepad* g = p->pads[(size_t)index];
    auto axis = [&](SDL_GamepadAxis a) {
        float v = (float)SDL_GetGamepadAxis(g, a) / 32767.0f;
        v = v < -1 ? -1.0f : v;
        return std::fabs(v) < deadzone ? 0.0f : v;
    };
    out.connected = true;
    out.left = {axis(SDL_GAMEPAD_AXIS_LEFTX), axis(SDL_GAMEPAD_AXIS_LEFTY)};
    out.right = {axis(SDL_GAMEPAD_AXIS_RIGHTX), axis(SDL_GAMEPAD_AXIS_RIGHTY)};
    out.left_trigger = axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    out.right_trigger = axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    static const SDL_GamepadButton map[] = {
        SDL_GAMEPAD_BUTTON_SOUTH,       SDL_GAMEPAD_BUTTON_EAST,          SDL_GAMEPAD_BUTTON_WEST,
        SDL_GAMEPAD_BUTTON_NORTH,       SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
        SDL_GAMEPAD_BUTTON_BACK,        SDL_GAMEPAD_BUTTON_START,         SDL_GAMEPAD_BUTTON_LEFT_STICK,
        SDL_GAMEPAD_BUTTON_RIGHT_STICK, SDL_GAMEPAD_BUTTON_DPAD_UP,       SDL_GAMEPAD_BUTTON_DPAD_DOWN,
        SDL_GAMEPAD_BUTTON_DPAD_LEFT,   SDL_GAMEPAD_BUTTON_DPAD_RIGHT};
    for (int i = 0; i < 14; i++)
        if (SDL_GetGamepadButton(g, map[i])) out.buttons |= 1u << i;
    return true;
}

Platform* create_platform(const PlatformConfig& cfg) {
    // Touch is handled through finger events so a tap never also produces a synthetic mouse event.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");  // delivered as SDLK_AC_BACK
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) return nullptr;
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
    p->haptics = cfg.haptics;
    p->last_ticks = SDL_GetTicksNS();
    return p.release();
}

void destroy_platform(Platform* p) {
    if (!p) return;
    for (SDL_Cursor* c : p->cursors)
        if (c) SDL_DestroyCursor(c);
    for (SDL_Gamepad* g : p->pads) SDL_CloseGamepad(g);
    audio_shutdown();
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

void play_haptic(Haptic h) {
    if (h == Haptic::None) return;
#if defined(__ANDROID__)
    // View.performHapticFeedback needs no VIBRATE permission and follows the user's touch-feedback setting.
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    if (!env || !activity) return;
    jclass act = env->GetObjectClass(activity);
    jmethodID get_window = env->GetMethodID(act, "getWindow", "()Landroid/view/Window;");
    jobject window = get_window ? env->CallObjectMethod(activity, get_window) : nullptr;
    if (window) {
        jclass win = env->GetObjectClass(window);
        jmethodID get_decor = env->GetMethodID(win, "getDecorView", "()Landroid/view/View;");
        jobject view = get_decor ? env->CallObjectMethod(window, get_decor) : nullptr;
        if (view) {
            jclass vc = env->GetObjectClass(view);
            jmethodID perform = env->GetMethodID(vc, "performHapticFeedback", "(I)Z");
            // HapticFeedbackConstants: CLOCK_TICK 4, KEYBOARD_TAP 3, LONG_PRESS 0
            const jint kind = h == Haptic::Selection ? 4 : (h == Haptic::Light ? 3 : 0);
            if (perform) env->CallBooleanMethod(view, perform, kind);
            env->DeleteLocalRef(vc);
            env->DeleteLocalRef(view);
        }
        env->DeleteLocalRef(win);
        env->DeleteLocalRef(window);
    }
    env->DeleteLocalRef(act);
    env->DeleteLocalRef(activity);
    if (env->ExceptionCheck()) env->ExceptionClear();
#elif defined(__APPLE__) && TARGET_OS_IPHONE
    rd_ios_haptic((int)h);
#endif
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
#if defined(__EMSCRIPTEN__)
    p->skip = p->wait_events && p->idle && p->quiet >= 2 && SDL_GetTicksNS() - p->last_draw < 250000000;
#endif
    in.pointer = p->pointer;
    in.pointer_down = p->down;
    in.touch = p->touch;
    // The pointer's finger goes first, so touches[0] is always the pointer.
    in.touch_count = 0;
    for (int i = 0; i < p->touch_count; i++)
        if (p->finger_active && p->touches[i].id == (uint64_t)p->finger)
            in.touches[in.touch_count++] = p->touches[i];
    for (int i = 0; i < p->touch_count && in.touch_count < kMaxTouches; i++)
        if (!(p->finger_active && p->touches[i].id == (uint64_t)p->finger))
            in.touches[in.touch_count++] = p->touches[i];
    in.a11y_action = A11yAction::None;
    in.a11y_target = 0;

    SDL_Rect safe{0, 0, ww, wh};
    in.safe_area = {};
    if (SDL_GetWindowSafeArea(p->window, &safe)) {
        in.safe_area = {(float)safe.x * k, (float)safe.y * k, (float)(ww - safe.x - safe.w) * k,
                        (float)(wh - safe.y - safe.h) * k};
        if (in.safe_area.x < 0 || in.safe_area.y < 0 || in.safe_area.w < 0 || in.safe_area.h < 0)
            in.safe_area = {};
    }

    // shortcut: SDL does not report the soft keyboard's height. When the window did not shrink for it
    // (iOS, Android adjustPan), assume it covers 40% of the screen; use the real height once SDL has it.
    in.keyboard_inset = 0;
    if (p->text_input && SDL_ScreenKeyboardShown(p->window) && wh >= p->height_before_keyboard - 1)
        in.keyboard_inset = (float)wh * k * 0.4f;

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
    const TextInputType type = ctx.text_input_type();
    if (want != p->text_input || (want && type != p->text_type)) {
        if (want) {
            if (!p->text_input) SDL_GetWindowSize(p->window, nullptr, &p->height_before_keyboard);
            static const SDL_TextInputType map[] = {
                SDL_TEXTINPUT_TYPE_TEXT,   SDL_TEXTINPUT_TYPE_NUMBER,
                SDL_TEXTINPUT_TYPE_NUMBER, SDL_TEXTINPUT_TYPE_TEXT_EMAIL,
                SDL_TEXTINPUT_TYPE_TEXT,   SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_HIDDEN};
            const SDL_PropertiesID props = SDL_CreateProperties();
            SDL_SetNumberProperty(props, SDL_PROP_TEXTINPUT_TYPE_NUMBER, map[(int)type]);
            if (type != TextInputType::Text)
                SDL_SetNumberProperty(props, SDL_PROP_TEXTINPUT_CAPITALIZATION_NUMBER, SDL_CAPITALIZE_NONE);
            // Android InputType: URI keyboard (TYPE_CLASS_TEXT | VARIATION_URI), decimal number pad.
            if (type == TextInputType::Url)
                SDL_SetNumberProperty(props, SDL_PROP_TEXTINPUT_ANDROID_INPUTTYPE_NUMBER, 0x11);
            if (type == TextInputType::Decimal)
                SDL_SetNumberProperty(props, SDL_PROP_TEXTINPUT_ANDROID_INPUTTYPE_NUMBER, 0x2002);
            if (p->text_input) SDL_StopTextInput(p->window);  // switch keyboards
            SDL_StartTextInputWithProperties(p->window, props);
            SDL_DestroyProperties(props);
        } else {
            SDL_StopTextInput(p->window);
        }
        p->text_input = want;
        p->text_type = type;
    }
    if (p->haptics && ctx.haptic() != Haptic::None) play_haptic(ctx.haptic());
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
    p->last_draw = SDL_GetTicksNS();
    SDL_GL_SwapWindow(p->window);
}

bool skip_frame(const Platform* p) {
    return p->skip;
}

bool poll_dropped_file(Platform* p, std::string& path) {
    if (!p || p->drops.empty()) return false;
    path = std::move(p->drops.front());
    p->drops.erase(p->drops.begin());
    return true;
}

}  // namespace rd::sdl3
