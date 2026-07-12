# Configurations

## Overview
Sord is primarily configured through runtime behavior and environment locations rather than a dedicated settings file.

## Desktop registration and MIME integration
- Sord registers itself as a terminal application via `~/.local/share/applications/sord.desktop`.
- Desktop registration is created automatically when the editor launches with a file path or `--register`.
- Registered MIME types include plain text, Markdown, source code, JSON, XML, `application/pdf`, and `application/x-sord`.

## IPC socket location
- IPC uses a Unix socket for single-instance handoff.
- The socket path is chosen from:
  1. `$XDG_RUNTIME_DIR`
  2. `$HOME/.cache`
  3. `/tmp`
- The filename incorporates the current user ID to avoid collisions between different users.

## Environment and terminal behavior
- The desktop entry uses `Terminal=true`, ensuring Sord launches in a terminal session.
- If `$HOME` or `$XDG_RUNTIME_DIR` is unavailable, Sord falls back safely to `/tmp`.

## Build and runtime configuration notes
- Sord is built using the included `Makefile` and C++20.
- Runtime configuration mostly depends on environment variables and file system paths.
- The application does not currently expose a separate configuration file or command-line options beyond `--register`.
