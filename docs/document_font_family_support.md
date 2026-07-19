# Document Font Family Support

This document describes the font family preservation work that allows styled text runs to retain their selected fonts.

## What changed
The editor document model and save/load pipeline were extended to support per-run font families and run-based serialization.

## Key files
- `include/editor/document.hpp`
- `src/editor/document.cpp`
- `include/editor/font_family.hpp`
- `include/exporter/pdf_exporter.hpp`
- `src/app/save_manager.cpp`

## Document model updates
- `TextRun` already encapsulated the text content and a `Style` object.
- The document model was updated to keep history entries as styled page runs instead of plain strings.
- A new `set_pages_from_runs()` API was added to load rich document content from serialized `TextRun` data.
- The editor now preserves `TextRun.style.font_family` across save/load and undo history.

## Serialization changes
- The save manager was upgraded from plain line-based storage to a versioned JSON format.
- The file format now writes `pages -> paragraphs -> runs` and includes `style.font_family` for every text run.
- This means the document is no longer reduced to bare lines during save and load; styling metadata survives round trip.

## PDF exporter integration
- `PDFExporter` now accepts styled runs and routes them through the font-aware rendering pipeline.
- The exporter reads `run.style.font_family` directly and does not alter the document's font metadata.

## Why this matters
This work ensures that font choice is not lost when the document is stored or exported.

With styled runs preserved in the document model, different font families can coexist on the same line or across pages, and the PDF exporter can faithfully reproduce the user's formatting intent.
