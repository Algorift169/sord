# Sord Editor Core and Document State

This document explains the core engine of Sord, focusing on document buffers, cursor navigation, and text updates.

## Document State (`sord::editor::Document`)

The **`Document`** represents the underlying file contents in memory. 

* **Line Buffer**: Managed as a `std::vector<std::string>`, where each element is a line.
* **Selection State**: Tracks text selection ranges.
* **File Properties**: Stores the file title, modified indicator flags, and helper getters.

---

## Input Orchestration (`sord::editor::Editor`)

The **`Editor`** serves as the API gateway between external keyboard layout events and the underlying `Document`.

### Event Dispatching
Keyboard events captured in the FTXUI interactive loop are forwarded to the editor:
* **Characters**: Inserted directly at the cursor's coordinate index.
* **Arrows**: Shift coordinate positions (clamped to line count and length).
* **Backspace/Delete**: Dispatches removals at `cursor_index - 1` or `cursor_index`.
* **Return**: Splits the string at the cursor, inserting a new vector element below.

---

## Render Projection (`sord::renderer::EditorRenderer`)

To avoid performance costs, the renderer performs clipping by calculating visible ranges:
* **Viewport Clipping**: Computes visible lines from the buffer based on vertical/horizontal editor size constraints.
* **Status Formatting**: Assembles state summaries (such as cursor coords `Ln X, Col Y`, encoding `UTF-8`, and input modes `INSERT`/`NORMAL`) to render into the footer.
