# 0007: Wavestorm palette and the second widget family

- **Palette.** `Theme::dark()` now follows the owner's Wavestorm Draw reference: background `#0b0e12`,
  surfaces `#0f1418` / `#151a20` / `#20272f`, border `#1d242c`, accent `#ec4899` (selection tint is the
  accent mixed 28% into the surface). This supersedes the earlier placeholder accent `#4da3ff`, which is
  kept as `Theme::midnight()`. Values were sampled from the reference screenshot, not from a brand guide.
- **Customization.** Added theme fields (`selection`, `success`, `warning`, `window_radius`, `shadow_blur`,
  `focus_ring_width`, `indent`, `shadows`), `Theme::with_accent`, and scoped `push_theme`/`pop_theme`.
- **Widgets.** Radio, switch, int sliders and drags, steppers, tabs, list items and list boxes, child
  regions, tree nodes, menu bar and menus, modal dialogs, color picker, plots, spinner, badge, icons (22
  vector glyphs drawn from lines and rounded rects, so no icon font), button styles, text box placeholder.
- **Color picker** uses a saturation/value square with hue and alpha bars, not a hue wheel: the square is a
  few gradient quads; a ring needs an arc primitive the draw list does not have.
- **Not included:** multi-line text editor, context (right-click) menus, drag and drop, tables with sortable
  headers, docking. They need input (right button, drag payloads) or primitives that v0.1 does not have.
- **Modals** need `Input::screen_size`; the runners fill it.
- Child regions and popups scroll by wheel and touch drag but nest only 8 deep.
