# PDF Export Font Pipeline

This document describes the font-aware PDF export architecture introduced in the exporter redesign.

## What changed
The PDF export system was refactored to separate shaping, font management, and rendering into dedicated components:

- `FontCache`: resolves font family names to actual font files, loads and caches font resources, and provides a single source of truth for PDF font embedding.
- `Shaper`: shapes Unicode text using HarfBuzz and returns glyph metadata only. It does not draw anything.
- `PDFExporter`: becomes the renderer. It creates pages, performs line breaking, positions glyphs, selects fonts, and saves the final PDF through libHaru.

## Key files
- `include/exporter/font_cache.hpp`
- `src/exporter/font_cache.cpp`
- `include/exporter/shaper.hpp`
- `src/exporter/shaper.cpp`
- `include/exporter/glyph_run.hpp`
- `src/exporter/pdf_exporter.hpp`
- `src/exporter/pdf_exporter.cpp`

## Design
The new export flow follows a clear pipeline:

1. The document model exposes a sequence of `TextRun` values. Each run preserves its original `style.font_family`.
2. `PDFExporter` creates a libHaru PDF and a `FontCache` instance.
3. For each styled run, `PDFExporter` asks `FontCache` for a matching `FontResource`.
4. `Shaper` takes the font resource, the run text, and the requested font size, then returns a `GlyphRun` containing:
   - glyph IDs
   - cluster membership
   - x and y offsets
   - x and y advances
   - run ascender and descender
5. `PDFExporter` uses the embedded PDF font and the shaped glyph positions to place text into the document.
6. If a requested font cannot be loaded, a fallback font is selected only during export. The document model remains unchanged.

## Responsibilities by module
- `FontCache`
  - Initializes FreeType once.
  - Loads each font file only once.
  - Creates `hb_font_t*` and `HPDF_Font` handles for each loaded font.
  - Caches font resources keyed by file path.
  - Falls back to a system default font only when export requires it.

- `Shaper`
  - Shapes Unicode text through HarfBuzz.
  - Returns a `GlyphRun` with all layout information.
  - Never calls libHaru or FreeType drawing APIs.

- `PDFExporter`
  - Establishes page size and margins.
  - Breaks text into lines using glyph advances.
  - Renders styled runs in the correct font.
  - Saves the PDF file and reports precise libHaru errors.

## Why this matters
This architecture fixes the prior export problem where font selection was lost or replaced by generic PDF fonts, especially for complex scripts.

With the new design:
- selected document fonts are preserved in the export path,
- multiple fonts may coexist in the same paragraph,
- glyph placement is derived from HarfBuzz advances rather than character counting,
- the shaping stage no longer performs rendering,
- font resources are reused instead of reloaded repeatedly.

## Compatibility notes
The export path now supports document fonts by family name, deferred fallback, and proper shaping for Unicode text. It is prepared for future styling enhancements like bold, italic, underline, and color.
