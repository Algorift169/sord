# Architectural Upgrade

## Overview
This document describes recent architecture upgrades that improve Sord's editor session handling and file integration.

## Modular application structure
- `src/app/application.cpp` contains the application lifecycle, UI composition, and IPC server functionality.
- `include/app/application.hpp` defines the application API and state members.
- Editor state is separated into `include/editor/document.hpp` and `src/editor/document.cpp`.
- Rendering is handled by `include/renderer/editor_renderer.hpp`, `src/renderer/editor_renderer.cpp`, and `src/renderer/page_renderer.cpp`.

## IPC and single-instance handoff
- Sord now supports desktop file launch handoff into an existing terminal session.
- The main process attempts to open a Unix socket and listen for open requests.
- New invocations send file-open requests to the running socket if available.
- This architecture avoids multiple terminal windows and preserves the active editor session.

## File management and save model
- The application model tracks current file path, save state, and document modification state.
- Save and save-as dialogs are implemented with FTXUI overlay components.
- The editor document is normalized into pages and lines, then serialized for saving.

## Rendering improvements
- `EditorRenderer` exposes `render_visible_lines(width, height, offset)` to support scrolling.
- `PageRenderer` renders page content into a vector of visible lines for the terminal viewport.
- Scroll offset is maintained by `Application` to keep the cursor in view.

## Future upgrade considerations
- Add persistent user settings or config file support.
- Extend IPC support to handle multiple files and window focus.
- Improve PDF export pagination and formatting.
- Add syntax-aware rendering for source code and Markdown.
