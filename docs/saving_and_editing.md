# Saving and Editing

## Overview
Sord provides standard document editing plus explicit save and save-as workflows. The editor preserves typed changes and tracks save state for the current buffer.

## Editing model
- The document model is implemented by `include/editor/document.hpp` and `src/editor/document.cpp`.
- Text content is stored as pages containing lines.
- Cursor navigation and editing operations are handled by the `Document` class.
- Editing actions include character insertion, newline insertion, backspace, and cursor movement.

## Save workflow
- `Save` writes the current buffer to the existing document path.
- If the document has no path, the save action opens the save dialog.
- `Save As` always prompts for a target path.
- The save dialog accepts a filename or path string.

## Implementation details
- Save management is implemented in `include/app/save_manager.hpp` and `src/app/save_manager.cpp`.
- `SaveManager::save` performs file writes and returns success/failure.
- `SaveManager::normalize_path` ensures paths are cleaned before use.
- `Application::save_document` and `Application::save_document_as` manage editor state after saving.

## Document state tracking
- `document_saved_` indicates whether the current buffer is stored on disk.
- `document_modified_` indicates unsaved changes.
- After a successful save, the title and status update accordingly.
- Save operations schedule a short saved-state indicator visible in the UI.

## User experience
- The title bar includes `Save` and `Save As` buttons.
- Ctrl+S triggers save behavior from the application event handler.
- Save dialogs show validation feedback when directories or paths are invalid.
- If the user saves to a new path, the old file is removed to avoid duplicate temporary buffers.
