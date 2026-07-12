# PDF Exporter

## Overview
Sord includes a built-in PDF export feature that converts the current document into a PDF file using the Haru PDF library.

## Implementation
The PDF exporter lives in `include/exporter/pdf_exporter.hpp` and `src/exporter/pdf_exporter.cpp`.

Key behavior:
- Creates a new PDF document.
- Adds one page to the PDF for each editor page.
- Writes each document line as wrapped paragraph text.
- Saves the generated PDF to disk.

## How it works
1. `PDFExporter::Export` initializes the PDF document and font.
2. For each document page, it creates a new PDF page sized to A4.
3. It renders each line with word wrapping inside the page margins.
4. It saves the resulting PDF file to the requested output path.

## Error handling
- The exporter returns `false` on failure and provides a diagnostic message via the `error` string.
- Typical failure points include PDF creation, page creation, and file save errors.

## User experience
- Users trigger export through the editor UI via the `Export PDF` button.
- A dialog appears for the target file path.
- The exporter defaults to adding a `.pdf` extension if one is not already provided.

## Integration
- The exporter is used by `src/app/application.cpp` when the export dialog is confirmed.
- It receives the current `sord::editor::Document` and an output path.
- If export succeeds, the dialog closes. If it fails, an error message is shown.
