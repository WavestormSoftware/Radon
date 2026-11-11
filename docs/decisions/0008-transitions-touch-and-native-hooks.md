# 0008: Transitions, touch metrics and native hooks

## Context

0.1 had hover/press blending only, desktop-sized metrics everywhere, and a backend that used the pixel
density as the DPI scale (1.0 on Android and Windows, so the UI was tiny there).

## Decisions

- **Transitions never change results.** Fades use a global alpha in the draw list (`set_alpha`), driven by
  the existing fixed animation pool. Two cases move geometry while animating: drawers slide and
  `transition_begin` offsets content by a few pixels. Both settle in about 0.2 s and are skipped with
  animations off.
- **Per-ID values live in the animation pool** (`anim_to`). A slot that was not touched on the previous
  frame restarts from its `from` value, which is what makes popups fade in again each time they open.
- **Drawers are modals.** They reuse `open_modal` / `close_modal`, the overlay window path and the
  one-modal-at-a-time rule instead of adding a second overlay system.
- **Touch is a theme, not a mode.** `Theme::touch()` raises the metrics; widgets size themselves from
  the layout cell, so no widget has touch-specific code. Density is separate (`set_dpi_scale`).
- **The backend reports the display scale** (`SDL_GetWindowDisplayScale`) and converts window
  coordinates to logical pixels. This is a behaviour change for anyone who used 0.1 on a scaled display.
- **Native features stay in the backend.** The core only exposes what it wants (`cursor()`,
  `text_input_rect()`, `animating()`, `can_dismiss()`); file dialogs, URLs and the system theme are plain
  backend functions.

## Known limits

- `text_area` has no soft wrapping; `password_box` draws at most 256 stars.
- `list_clipper` needs uniform row heights.
- A card's height lags its content by one frame.
- One finger only: no pinch or multi-touch gestures.
- No screen-reader accessibility tree.
