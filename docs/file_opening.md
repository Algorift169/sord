# File Opening

## Overview
Sord supports opening files from the editor UI and by launching the application with a file path argument.

## Desktop file integration
- A `.desktop` file is created at `~/.local/share/applications/sord.desktop`.
- The desktop entry registers standard text MIME types and `application/pdf`.
- The desktop entry launches Sord with the selected file path in the terminal.

## Single-instance handoff
- Sord uses a Unix domain socket stored in `XDG_RUNTIME_DIR` or `~/.cache`.
- When a second invocation includes a file path, the new process sends the path to the running instance.
- If the running instance accepts the request, the second process exits immediately.
- If no instance is running, Sord starts normally and opens the requested file.

## In-app file open UI
- The `Open` button on the toolbar launches a file-open dialog.
- Users may type a file path to open.
- The editor validates existence and readability before loading the file.

## Loading files
- File loading logic is implemented in `src/app/application.cpp`.
- Files are read in binary mode and normalized into text lines.
- Supported line endings include `\n` and `\r\n`.
- Opened documents are displayed in the current editor buffer and marked as saved.

## Behavior
- If a file is opened from the desktop entry while Sord is already running, the content arrives via IPC and the editor loads it into the active session.
- The editor preserves the running terminal session and does not create a second window when handoff succeeds.
