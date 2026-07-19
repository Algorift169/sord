# Sord

**Sord** is a modern, terminal-based text editor built using C++20 and the [FTXUI](https://github.com/ArthurSonzogni/FTXUI) library. It delivers a fast, responsive, and aesthetically polished text editing experience directly inside your terminal emulator.

---

## Features

- **Responsive Terminal UI**: Modern layout containing a customizable title bar, dynamic toolbar, central editor buffer area, and a status bar indicating file properties.
- **Robust Saving Engine**: Direct, user-friendly buttons for **Save** and **Save As** operations.
  - **Save**: Instantly saves changes if already linked to a file. If editing a new document, it prompts for a filename and writes it to the current directory.
  - **Save As**: Prompts for an absolute path and filename, performing path safety validation to prevent directory errors.
- **Simulated File Renaming**: Cleans up previous/untitled file buffers on disk when performing "Save As" to prevent duplicate clutter.
- **Multi-Page Document Support**: Create multiple pages with standard A4 page sizes (80×66 lines). Each page enforces a line limit with clear status bar warnings.
- **PDF Export**: Export documents to PDF with accurate page layout preservation using Haru PDF library.
- **Modular Design**: Separates domain models, input coordinators, layout rendering projections, and file writes.

---

## Directory Structure

- [docs/](file:///home/israfil/Desktop/sord/docs/): Contains detailed guides explaining Sord's internal details.
  - [architecture.md](file:///home/israfil/Desktop/sord/docs/architecture.md): High-level system structure.
  - [application_logic.md](file:///home/israfil/Desktop/sord/docs/application_logic.md): FTXUI structure and modal behaviors.
  - [editor_core.md](file:///home/israfil/Desktop/sord/docs/editor_core.md): Document buffer management and cursor actions.
  - [page_management.md](file:///home/israfil/Desktop/sord/docs/page_management.md): A4 page standards, page limits, and multi-page document handling.
  - [pdf_exporter.md](file:///home/israfil/Desktop/sord/docs/pdf_exporter.md): PDF export functionality and page layout preservation.
  - [pdf_export_font_pipeline.md](file:///home/israfil/Desktop/sord/docs/pdf_export_font_pipeline.md): Font-aware PDF exporter architecture and shaping pipeline.
  - [font_dropdown_selection.md](file:///home/israfil/Desktop/sord/docs/font_dropdown_selection.md): Font dropdown navigation and focus behavior.
  - [document_font_family_support.md](file:///home/israfil/Desktop/sord/docs/document_font_family_support.md): Document model and serialization support for per-run font families.
  - [src/](file:///home/israfil/Desktop/sord/src/): Source code (.cpp).
  - [include/](file:///home/israfil/Desktop/sord/include/): Headers (.hpp).
- [tests/](file:///home/israfil/Desktop/sord/tests/): Verification test suite.

---

## Getting Started

### Prerequisites

Sord requires a compiler supporting C++20 (e.g., GCC 10+ or Clang 10+) and FTXUI.

On Debian/Ubuntu-based systems, you can install the dependencies using:
```bash
sudo apt-get install build-essential pkg-config libftxui-dev
```

### Building the Project

Use the provided `Makefile` to compile and link the application:

```bash
make
```

### Running Sord

Start the editor by running the compiled binary:

```bash
./build/sord
```

### Running Tests

Execute the test suite to verify core document editor logic:

```bash
make test
```
