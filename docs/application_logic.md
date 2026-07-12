# Sord Application and UI Logic

This document details the interface components, UI structure, event loop coordination, and file saving/rename behavior.

## FTXUI Interface Component Hierarchy

The UI runs on **FTXUI**'s `ScreenInteractive` in full-screen mode. The layout is arranged as a nested hierarchy of components wrapped in a stack:

1. **`root`** (`Container::Stacked`)
   * **`main_container`** (`Container::Vertical`)
     * **`title_bar`** (`Renderer` around `title_bar_container`): Holds the `Save` and `Save As` buttons, file name, and status text.
     * **`toolbar`** (`Renderer`): Displays contextual controls.
     * **`editor_area`** (`Renderer`): Dynamic viewport displaying text lines.
     * **`status_bar`** (`Renderer`): Shows editor stats (encoding, insert mode, line/col indicators).
   * **`save_overlay_maybe`** (`Maybe` wrapping `save_overlay`): Pop-up window for entering filenames when saving new documents.
   * **`save_as_overlay_maybe`** (`Maybe` wrapping `save_as_overlay`): Pop-up window for specifying absolute paths and names for new file saves.

---

## Modals & Save Workflow

The `Maybe` wrapper ensures that overlays only intercept user focus and mouse clicks when active.

```
                  [Click Save Button / Ctrl+S]
                               |
                      {document_saved_?}
                       /            \
                     Yes             No
                     /                \
             [Save Directly]     [Show Save Overlay]
                                       |
                              [Enter Filename]
                                       |
                               {Directory Exists?}
                                /             \
                              Yes              No
                              /                 \
                      [Write File]         [Show Red Error]
                            |
                     {Old Path exists 
                     & old_path != new_path?}
                            |
                   (Delete old file to 
                     simulate renaming)
```

### Path Verification
To prevent filesystem crashes, input paths are validated via `std::filesystem::exists` on their parent directories. If parent folders don't exist:
* The save is aborted.
* A red-colored message (`"Directory does not exist."`) is displayed in the dialog window.

### Simulated Renames
When executing a "Save" or "Save As" operation:
1. The new path is written successfully.
2. If the document has a pre-existing path on disk, and the user chose a different filename/path, the old file is removed using `std::filesystem::remove`.
3. The editor shifts its active buffer focus to the new target filepath.
