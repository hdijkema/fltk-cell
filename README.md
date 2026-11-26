# fltk-cell

<img src="docs/assets/logo.svg" width="80" height="80" alt="fltk-cell logo">

[Homepage](https://hdijkema.github.io/fltk-cell/) |
[API reference and usage examples](docs/API.md)

A small C++17 cell drawing and editing library for FLTK 1.4 tables.
The application owns its data and selects a drawer for each cell. The library
does not add another table model or create a widget for every cell.

## Drawers

| Class | Display |
| --- | --- |
| `TableCellTextDrawer` | Clipped UTF-8 text with ellipsis |
| `TableCellNumberDrawer` | Exact decimal formatting, grouping, precision, percent/prefix/suffix |
| `TableCellHexDrawer` | Bounded hexadecimal preview |
| `TableCellCheckBoxDrawer` | Boolean checkbox with a separate NULL state |
| `TableCellSvgDrawer` | Borrowed FLTK image, including SVG, scaled into the cell |

`TableCellDrawer` is the extension point. Drawers share background, selection,
focus, disabled-state and grid handling. `std::monostate` represents NULL; an
empty string remains empty. `Decimal` stores the original decimal text so large
database numbers never need a lossy conversion through `double`. Formatting is
for display only. Rounding is half away from zero; `decimals = -1` preserves
fractional precision.

Editing is separate: `TableCellTextEditor` overlays one `Fl_Input` on the active
cell. Enter proposes a string, Escape cancels. `TableCellCheckBoxEditor` proposes
a toggled boolean immediately. The application validates and commits values in
the callback. Read-only views simply use drawers without editors.

## Build and try

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
cmake --build build/release --parallel
ctest --test-dir build/release --output-on-failure
```

On Windows with MSVC, CMake and Ninja on PATH, use `build-release.cmd`.
Run `fltk-cell-gallery` to try the drawers, text editing and checkboxes.
`--smoke` closes the gallery automatically.

The build uses installed FLTK (`FLTK_DIR`), otherwise FLTK release sources.
CSS support defaults to on: it uses installed fltk-css or versioned release
sources. Set `FLTK_CELL_CSS_SOURCE_DIR` explicitly to use a development
checkout. Set `FLTK_CELL_WITH_CSS=OFF` for core-only use. The core
depends only on FLTK core/images and the C++ standard library.
`FLTK_CELL_BUILD_EXAMPLE` and `BUILD_TESTING` control examples and tests.

## Use in a table

```cpp
#include <fltk_cell.h>

// Inside Fl_Table::draw_cell(CONTEXT_CELL, ...); normally reuse member drawers.
fltk_cell::TableCellNumberDrawer drawer;
drawer.format.decimals = -1;
fltk_cell::CellValue value = fltk_cell::Decimal{"9007199254740993.125"};
fltk_cell::CellStyle style;
drawer.draw({{x, y, w, h}, value, style, {selected, focused, enabled}});
```

Link `fltk-cell::fltk-cell`. Installed-package consumers use:

```cmake
find_package(fltk-cell 0.1 CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE fltk-cell::fltk-cell)
```

The optional `fltk_cell_css.h` adapter provides `cellStyle(css, node)`.
It maps computed colors and basic font properties, without CSS layout or data
model changes. For an installed package request `COMPONENTS css` and link
`fltk-cell::css`; this also requires installed fltk-css.

## Lifetime and threading

- Draw and edit on the FLTK UI thread. The caller supplies coordinates, state,
  styles and redraws.
- `CellValue` owns strings and byte vectors. `BytesView` borrows a byte buffer
  to avoid copying a large blob for a small preview. Image pointers are borrowed: keep
  images alive during drawing. The SVG drawer also accepts other FLTK images.
- Cancel/destroy editors **before** their parent group. Cancel or reposition
  overlays when scrolling or resizing. The gallery demonstrates this lifetime.
  FLTK paints table children before cells: skip drawing the active edited cell
  so its input overlay remains visible.
  Destroying an editor discards pending input; keyboard editing supports Enter
  and Escape, not automatic commit on focus loss.
- `fitText` needs an initialized FLTK display/font system and bounds its preview
  work. Hex rendering expands only `preview_bytes`, not the whole blob.
- The library does not persist values, parse SQL, manage database updates or
  implement a spreadsheet/grid widget.

The first integration is pgadmin-jr's FLTK frontend. Local validation uses
Windows/MSVC; macOS/Linux have not yet been validated here.
See [LICENSE](LICENSE). Dependencies retain their own licences.

## Release versions

The current library version is **0.1.0**, identified by tag `v0.1.0`.
Dependencies use named, tested releases: FLTK **1.4.5** (`release-1.4.5`),
not a development branch or a moving `latest` reference. Local source/package
paths remain available for development. Upgrade a dependency deliberately,
run the tests and publish a new library release when appropriate.

Push the matching `v<project-version>` tag after committing the release sources.
The workflow tests the platforms first and then creates a GitHub source release.
For the first release, publish fltk-css before fltk-cell, which depends on its
`v0.1.0` release. pgadmin-jr consumes those tags after publication. Release tags
must remain immutable; do not move a published tag to different sources.
