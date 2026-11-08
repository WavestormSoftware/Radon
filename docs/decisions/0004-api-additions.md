# 0004: API details the plan left open

- Added `open_window`, `close_popup`, `layout_next`, `Context::{theme,dpi_scale,set_dpi_scale,warning,
  atlas_*,wants_text_input,pointer_over_ui}`, and `Input::touch` (touch has no hover and scrolls by drag).
- Clipboard hooks are `int get(void* user, char* out, int cap)` and `void set(void* user, const char*, int)`.
- `Theme` stores font handles only: a handle already carries its pixel size, so separate sizes would be
  redundant.
- `Input::safe_area` is insets (left, top, right, bottom in x, y, w, h).
- Only one popup can be open at a time. Popup height comes from the previous frame's content size.
- Closed windows keep their slot while `begin_window` is still called for them each frame.
- Draw commands carry an internal `layer` (window z) and are stable-sorted at `end_frame`; vertices are not
  moved. Rects, glyphs and solid shapes all use texture 0 so they merge into few commands.
- Windows nest only through popups (stack depth 8).
