# Font Dropdown Selection and Focus

This document explains the font dropdown interaction improvements and keyboard/mouse selection fixes.

## What changed
The font picker overlay in the editor UI was improved to provide:

- keyboard focus tracking for dropdown items,
- arrow key navigation within the visible font list,
- clear focus and selected-font indicators,
- correct mapping from mouse clicks to font items,
- consistent dropdown closing behavior.

## Key files
- `include/app/application.hpp`
- `src/app/application.cpp`

## Behavior changes
- The font dropdown now keeps a dedicated focus index (`font_dropdown_focus_`) separate from the scroll offset.
- Arrow Up and Arrow Down move the focused item, not just the scroll position.
- The focused item is scrolled into view when necessary.
- Enter selects the currently focused font, even if it is not the first visible entry.
- The dropdown highlights the focused item with a `> ` marker and the current document font with a `✓ ` marker.
- Clicking inside the dropdown selects the corresponding font item reliably.
- Clicking outside the dropdown closes it cleanly.

## Why this matters
Users can now choose fonts more reliably from the menu, especially when the list is longer than the visible area.

Previously, the UI sometimes selected the wrong font because it always chose the first visible font or did not preserve focus. The fix makes font selection predictable and visually clearer.
