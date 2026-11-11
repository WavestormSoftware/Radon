# Accessibility

Radon records an accessibility tree each frame when `Config::a11y` is on. A backend forwards it to the
platform screen reader; Radon itself ships no platform bridge yet.

```cpp
rd::Config cfg;
cfg.a11y = true;            // off by default: recording costs a little per widget
rd::Context ui(cfg);
// ... after end_frame:
int n = 0;
const rd::A11yNode* nodes = ui.a11y_nodes(&n);
for (int i = 0; i < n; i++) announce(nodes[i]);
```

## Nodes

Every focusable widget gets a node, and so do windows, labels, headings, wrapped text, alerts, images and
charts. A node is plain data, valid until the next `begin_frame`:

| Field | |
|---|---|
| `id` | The widget ID. Stable across frames as long as the label (and ID stack) is. |
| `parent` | The ID of the window it is in, 0 at the top level. |
| `role` | `A11yRole`: `Window`, `Button`, `Checkbox`, `Radio`, `Switch`, `Slider`, `TextField`, `ComboBox`, `ListItem`, `Tab`, `Link`, `Text`, `Heading`, `Image`, `ProgressBar`, `TreeItem`, `MenuItem`, `Table`, `Row`. |
| `rect` | Bounds in logical pixels. |
| `label` | The visible label (text after `##` is dropped), UTF-8, up to 47 bytes. |
| `value` | The current value as text: `"on"`/`"off"`, `"0.50"`, the selected item, a field's text (never a password). |
| `flags` | `Focusable`, `Focused`, `Checked`, `Selected`, `Disabled`, `Expanded`, `Collapsed`. |

The pool holds `Config::max_a11y_nodes` (1024) nodes; past that `error()` turns true and the rest of the
frame is not recorded.

## Actions

A screen reader drives the UI through `Input`:

```cpp
in.a11y_action = rd::A11yAction::Activate;   // or Focus, Increment, Decrement
in.a11y_target = node.id;
```

`Activate` clicks the widget (a button fires, a checkbox toggles, a tab is selected). `Focus` moves keyboard
focus to it. `Increment` / `Decrement` focus it and send Right / Left, which sliders, steppers, ratings
and range sliders understand. Clear the action after one frame (the SDL backend does).

## Forwarding to a platform

| Platform | API | Mapping |
|---|---|---|
| Linux | AT-SPI (D-Bus) | One accessible object per node; `role` to `ATSPI_ROLE_*`; `Activate` from the `Action` interface. |
| Windows | UI Automation | A fragment root for the window, one provider per node, `Invoke` / `Toggle` / `RangeValue` patterns. |
| macOS / iOS | `NSAccessibility` / `UIAccessibilityElement` | One element per node with `accessibilityFrame` from `rect` (convert to screen points). |
| Android | `AccessibilityNodeProvider` on the SDL view | Virtual views keyed by `id`; `performAction` sets `a11y_action`. |

Because the tree is rebuilt every frame, diff it by `id` and send change events only for nodes whose
`value`, `flags` or `rect` changed.
