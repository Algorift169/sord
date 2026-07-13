# Page Management and A4 Standard

## Overview

Sord enforces standard A4 page dimensions to ensure consistency between terminal editing and PDF export. Each page is limited to **66 lines** (standard A4 height in terminal format) and **80 characters wide** (standard terminal width).

## A4 Page Standard

### Dimensions
- **Width**: 80 characters
- **Height**: 66 lines
- **Format**: Portrait orientation (terminal/text-based)

These dimensions are defined in `sord::layout::PageLayout`:

```cpp
static constexpr std::size_t A4_WIDTH = 80;   // characters
static constexpr std::size_t A4_HEIGHT = 66;  // lines
```

## Page Limit Enforcement

### How It Works

Each page has a maximum line capacity of **66 lines**. When a user reaches the page limit:

1. **Cursor reaches line 66** → Status bar displays a warning
2. **User attempts to press Enter** → The keypress is ignored (blocked)
3. **User must create a new page** → Click the `+` button or use the page add function
4. **New page created** → Cursor moves to line 1 of the new page

### Implementation

**Document Class** (`sord::editor::Document`):
- `PAGE_LINE_LIMIT` constant = 66
- `insert_newline()` method checks page capacity before creating new lines:
  ```cpp
  if (cursor_row_ + 1 >= PAGE_LINE_LIMIT) {
      return;  // Prevents new line - page is full
  }
  ```
- `page_line_limit()` getter provides access to the limit

**Editor Renderer** (`sord::renderer::EditorRenderer`):
- `render_status_bar()` displays page fill status
- Shows warning when `cursor_row + 1 >= page_limit`

### Status Bar Feedback

The status bar displays:
```
Ln X, Col Y  Page N  [Optional Warning]
```

**Example when page is full:**
```
Ln 66, Col 1  Page 1  ⚠ Page Full - Please add a new page
```

**Example when page has space:**
```
Ln 45, Col 12  Page 2
```

## Page Transitions

### Moving Between Pages

Users can navigate between pages using arrow keys:
- **Up arrow at top line** → Moves to previous page (last line)
- **Down arrow at last line** → Moves to next page (first line)
- **Left/Right arrow** → Normal column navigation

### Adding Pages

Pages are created using the application UI (typically a `+` button):
1. New page is appended to document
2. Cursor automatically moves to line 1 of new page
3. Page numbering updates in status bar

## PDF Export Compatibility

The A4 page dimensions ensure consistency when exporting to PDF:

1. **Terminal dimensions** (80×66) map to **A4 paper size**
2. **PDF exporter** creates one A4 page per document page
3. **Content layout** is preserved exactly
4. **Page breaks** occur at document page boundaries

**PDF Settings**:
- Page size: A4 (8.27" × 11.69")
- Orientation: Portrait
- Margins: 1 inch (72pt) on all sides
- Font: Helvetica, 12pt

## User Experience

### Clear Boundaries
- Users always know the page capacity (66 lines)
- Status bar provides clear warning when page is full
- No surprise page breaks or auto-scrolling

### Intuitive Workflow
1. Type naturally on the page
2. See warning when approaching limit
3. Click `+` to create new page
4. Continue typing on next page

### Consistent Across Formats
- **Terminal**: Visual page feedback
- **PDF Export**: Exact same page layout
- **Saving**: Page structure preserved

## Technical Details

### Page Management Components

1. **PageLayout** (`include/layout/page_layout.hpp`)
   - Stores page dimensions
   - Calculates content area (accounting for margins)
   - Provides A4 constants

2. **Document** (`include/editor/document.hpp`)
   - Manages page collection via `PageManager`
   - Enforces page line limits during text insertion
   - Tracks current page and cursor position

3. **PageManager** (`include/editor/page_manager.hpp`)
   - Maintains vector of pages
   - Handles page add/remove operations
   - Provides page access methods

4. **EditorRenderer** (`include/renderer/editor_renderer.hpp`)
   - Renders visible content with page context
   - Displays page separators between pages
   - Shows status bar with page information

### Page Rendering

The renderer combines content from all pages:
- Renders content lines for each page
- Inserts page separator (`_________`) between pages
- Displays visible window to user
- Updates status bar with current page info

## Limitations and Future Improvements

### Current Limitations
- Page line limit is fixed at 66 (cannot be customized)
- Page width is fixed at 80 characters
- No page-specific formatting or styling

### Potential Enhancements
- Configurable page dimensions per document
- Page templates with different layouts
- Automatic page break handling during paste operations
- Page navigation toolbar
- Page thumbnail preview

## Testing

### Unit Tests
- `test_page_layout.cpp`: Verify page dimension calculations
- `test_page_manager_basics.cpp`: Page creation and management
- `test_page_manager_add_remove.cpp`: Page operations

### Integration Tests
- Verify line limit enforcement during typing
- Test page transitions with arrow keys
- Validate PDF export with multiple pages
- Check status bar updates
