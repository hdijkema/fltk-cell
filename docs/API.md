# fltk-cell API and examples

[README](../README.md) · [Source](https://github.com/hdijkema/fltk-cell)

The library draws typed values inside table cells and supplies separate editors.
It does not own a table model or create one widget per cell. Namespace:
`fltk_cell`. Drawing and editing require the FLTK GUI thread.

## How the API works

Your table owns its values. The library supplies reusable drawing objects and
separate editors; the normal FLTK table callbacks decide when to use them.

| Class | Construction, use and behavior |
| --- | --- |
| `TableCellDrawer` | Abstract drawing interface. `draw()` paints one cell; `displayText()` supplies a textual representation. Subclasses can reuse `frame()` and `text()`. |
| `TableCellTextDrawer`, `TableCellNumberDrawer`, `TableCellHexDrawer`, `TableCellCheckBoxDrawer`, `TableCellSvgDrawer` | Default-construct the drawer you need and reuse it. Set its public formatting options, then call `draw()` from `Fl_Table::draw_cell`. Drawers borrow the supplied value/style for that call and do not change the model. |
| `TableCellTextEditor` | Default-construct one persistent editor. `begin()` overlays an input on the active cell; Enter proposes a string and Escape cancels. Use `active()`, `resize()` and `cancel()` when the table moves or the edit ends. |
| `TableCellCheckBoxEditor` | Default-construct, then call `begin()` to propose a toggled boolean immediately. The application receives the proposal through the same completion callback. |
| `TableCellEditor` | Abstract editing interface with `begin()` and `cancel()`. Applications validate and store proposed values in the completion callback. |

Draw on the GUI thread using the rectangle FLTK supplies. Reuse drawers as
members of your table/controller. Keep an editor alive while its callback is
pending and cancel/destroy it before its parent. Skip drawing the cell covered
by a text editor's input overlay.

## Complete example: typed values in an Fl_Table

Save as `main.cpp`; see [Headers and linking](#headers-and-linking) for CMake
integration. This small table owns its values and reuses one number drawer and
one text drawer. The decimal value retains precision beyond the exact integer
range of a `double`.

```cpp
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Table.H>
#include <fltk_cell.h>
#include <array>

// Draw application-owned values using reusable typed drawers.
class ExampleTable : public Fl_Table
{
public:
    // Create a one-column table; no external data or database is accessed.
    ExampleTable(int x, int y, int w, int h) : Fl_Table(x, y, w, h)
    {
        rows(2);
        cols(1);
        col_width_all(300);
        row_height_all(34);
        number_.format.decimals = -1;
        end();
    }

protected:
    // FLTK supplies cell geometry; drawers handle clipping and the frame.
    void draw_cell(TableContext context, int row, int, int x, int y,
                   int w, int h) override
    {
        if (context != CONTEXT_CELL) return;
        fltk_cell::CellContext cell{ {x, y, w, h}, values_[row], style_, {} };
        if (row == 0) number_.draw(cell);
        else text_.draw(cell);
    }

private:
    std::array<fltk_cell::CellValue, 2> values_{
        fltk_cell::Decimal{"9007199254740993.125"}, std::string("Hello, FLTK")};
    fltk_cell::CellStyle style_;
    fltk_cell::TableCellNumberDrawer number_;
    fltk_cell::TableCellTextDrawer text_;
};

int main()
{
    Fl_Double_Window window(480, 240, "Cell example");
    auto* table = new ExampleTable(10, 10, 460, 220);
    window.end();
    window.resizable(table);
    window.show();
    return Fl::run();
}
```

## Class API

### Drawers

`TableCellDrawer` is the stateless extension interface. Every drawer clips to
the supplied rectangle, uses the common cell frame, and leaves data and redraw
scheduling to the caller.

| Type | Public operations and fields |
| --- | --- |
| `TableCellDrawer` | `virtual ~TableCellDrawer() = default`; pure `virtual void draw(const CellContext&) const = 0`; `virtual std::string displayText(const CellValue&) const`. |
| `TableCellTextDrawer` | `Fl_Align alignment = FL_ALIGN_LEFT`; overrides `draw`; inherits textual conversion. |
| `TableCellNumberDrawer` | `NumberFormat format`; overrides `displayText` and `draw`. |
| `TableCellHexDrawer` | `std::size_t preview_bytes = 24`; `bool uppercase = true, spaces = true`; overrides `displayText` and `draw`. |
| `TableCellCheckBoxDrawer` | Overrides `draw` for boolean and NULL display. |
| `TableCellSvgDrawer` | Overrides `draw` for a borrowed `Fl_Image*` scaled into the cell. |

Subclasses can use the protected helpers
`void frame(const CellContext&) const` and
`void text(const CellContext&, const std::string&, Fl_Align) const`.
Override `displayText` when a custom display also needs matching clipboard or
editor text. Reuse drawers rather than constructing a widget for each cell.

### Editors

Declared in [fltk_cell_editor.h](../include/fltk_cell_editor.h).

`CellEditFinished` is
`std::function<void(std::optional<CellValue>)>`.
A value proposes a commit; `std::nullopt` means cancellation. The application
validates and stores the proposed value, then redraws its table.

| Type | Public operation | Contract |
| --- | --- | --- |
| `TableCellEditor` | `virtual ~TableCellEditor() = default` | Abstract editor interface. |
| `TableCellEditor` | `virtual void begin(Fl_Group& parent, Rect rect, const CellValue& value, CellEditFinished finished) = 0` | Edit one value, using application-provided parent, geometry and completion callback. |
| `TableCellEditor` | `virtual void cancel() = 0` | Cancel pending work. |
| `TableCellTextEditor` | `TableCellTextEditor()`; `~TableCellTextEditor() override` | Own one temporary `Fl_Input`; destruction discards pending input. |
| `TableCellTextEditor` | `void begin(Fl_Group&, Rect, const CellValue&, CellEditFinished) override` | Cancel a previous edit, create the input overlay, fill it using textual conversion and focus it. |
| `TableCellTextEditor` | `void cancel() override` | Finish an active edit with cancellation. |
| `TableCellTextEditor` | `void resize(Rect rect)` | Reposition an active overlay after resize/scroll. |
| `TableCellTextEditor` | `bool active() const` | Whether an input overlay exists. |
| `TableCellCheckBoxEditor` | `void begin(Fl_Group&, Rect, const CellValue&, CellEditFinished) override` | Immediately propose the opposite boolean; a non-boolean value proposes `true`. No input widget is created. |
| `TableCellCheckBoxEditor` | `void cancel() override` | No pending work; no operation. |

Text editing accepts Enter and cancels on Escape; focus loss does not commit.
Destroy/cancel editors before their parent group. Cancel or reposition overlays
on scroll/resize. FLTK paints table children before cells, so skip drawing the
cell currently covered by the input overlay.

## Proposing a checkbox edit

Inside a table controller, keep `value`, `table` and the editor alive for the
operation. `begin` calls the callback synchronously for this editor:

```cpp
fltk_cell::CellValue value = false;
fltk_cell::TableCellCheckBoxEditor editor;
editor.begin(table, {x, y, w, h}, value,
    [&](std::optional<fltk_cell::CellValue> proposed) {
        if (!proposed) return;
        value = *proposed;
        table.redraw();
    });
```

For text editing use a persistent `TableCellTextEditor` member instead, retaining
the callback's captured objects until commit or cancellation.

## Optional CSS adapter

Declared in [fltk_cell_css.h](../include/fltk_cell_css.h):

```cpp
fltk_cell::CellStyle cellStyle(const FLTK_Css& css, FLTK_Css_Node node);
```

It calls `css.style(node)` and copies background, foreground, border,
selection background, font size and basic generic-family/bold/italic font
mapping into a `CellStyle`. Other fields keep their defaults. It retains no
stylesheet reference and does not perform CSS layout. For example, with an
existing stylesheet:

```cpp
FLTK_Css_Node node;
node.type = "cell";
node.classes = {"numeric"};
const auto style = fltk_cell::cellStyle(css, node);
```

## Formatting functions

| Declaration | Result and preconditions |
| --- | --- |
| `std::string formatNumber(std::string_view value, const NumberFormat& format = {})` | Exact decimal display formatting, rounding half away from zero. Source text is unchanged. Invalid/overlong numeric input is returned unchanged. |
| `std::string formatHex(const Bytes& value, std::size_t max_bytes = 24, bool uppercase = true, bool spaces = true)` | Format a bounded hexadecimal preview with a truncation marker. |
| `std::string formatHex(BytesView value, std::size_t max_bytes = 24, bool uppercase = true, bool spaces = true)` | The same bounded preview without copying the byte buffer. |
| `std::string fitText(std::string_view text, int pixels, Fl_Font font, int font_size)` | Return a code-point-aligned prefix fitting the width, adding ellipsis when needed. Requires an initialized FLTK display/font system and the GUI thread. |

## Supporting data types

These public value and configuration types supply inputs and results for the
operations above. Their fields and defaults are listed here for reference.

### Values and context

Declared in [fltk_cell.h](../include/fltk_cell.h).

| Type | Public fields or alternatives |
| --- | --- |
| `Rect` | `int x = 0, y = 0, w = 0, h = 0`; drawing or editor rectangle. |
| `Decimal` | `std::string text`; original numeric text, avoiding lossy conversion through `double`. |
| `Bytes` | Alias for `std::vector<std::uint8_t>`; owns its byte storage. |
| `BytesView` | `const std::uint8_t* data = nullptr; std::size_t size = 0;`; borrows bytes for drawing/formatting. Supply valid storage for the full call. |
| `CellValue` | `std::variant<std::monostate, std::string, bool, std::int64_t, double, Decimal, Bytes, BytesView, Fl_Image*>`. Strings/byte vectors are owned; image pointers are borrowed. |
| `CellState` | `bool selected = false, focused = false, enabled = true`. |
| `CellContext` | `Rect rect; const CellValue& value; const CellStyle& style; CellState state;`; borrowed value/style must remain valid throughout the draw call. |

`std::monostate` means NULL. An empty string is a distinct value. Keep an image
alive for every call that draws it; `TableCellSvgDrawer` also accepts non-SVG
FLTK images.

#### CellStyle

| Public field | Default |
| --- | --- |
| `Fl_Color background` | `FL_WHITE` |
| `Fl_Color foreground` | `FL_BLACK` |
| `Fl_Color border` | `FL_LIGHT2` |
| `Fl_Color selected_background` | `FL_SELECTION_COLOR` |
| `Fl_Color selected_foreground` | `FL_WHITE` |
| `Fl_Color focus` | `FL_DARK_BLUE` |
| `Fl_Font font` | `FL_HELVETICA` |
| `int font_size` | `14` |
| `int padding` | `6` |
| `bool grid` | `true` |

#### NumberFormat

| Public field | Default and meaning |
| --- | --- |
| `int decimals` | `2`; use `-1` to retain source fractional digits, otherwise 0–100. |
| `char decimal_separator` | `'.'` |
| `char group_separator` | `','` |
| `bool grouping` | `true` |
| `bool percent` | `false`; scale the display as a percentage. |
| `std::string prefix`, `suffix` | Empty display decorations. |

## Headers and linking

Use `<fltk_cell.h>` for values, formatting and drawers,
`<fltk_cell_editor.h>` for editing, and `<fltk_cell_css.h>` for the optional CSS
adapter.

```cmake
cmake_minimum_required(VERSION 3.21)
project(cell-example LANGUAGES CXX)
find_package(fltk-cell 0.1 CONFIG REQUIRED)
add_executable(example main.cpp)
target_compile_features(example PRIVATE cxx_std_17)
target_link_libraries(example PRIVATE fltk-cell::fltk-cell)
```

For the CSS adapter, request `find_package(fltk-cell 0.1 CONFIG REQUIRED
COMPONENTS css)` instead and link `fltk-cell::css`. This requires installed
fltk-css as well.

## Library build options

`FLTK_CELL_WITH_CSS=OFF` builds the core without CSS. With CSS enabled,
`FLTK_CELL_CSS_SOURCE_DIR` selects an explicit development checkout.
`FLTK_CELL_BUILD_EXAMPLE` and `BUILD_TESTING` control the gallery and tests.
The core depends on FLTK core/images and the C++ standard library.

## Building the example

Save the complete C++ program as `main.cpp` and the CMake block as
`CMakeLists.txt` in an empty example directory. Set `CMAKE_PREFIX_PATH` to your
installed library and FLTK prefixes, for example:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/sdk/widgets;C:/sdk/fltk"
cmake --build build --config Release
```

Use your actual installation paths and a Visual Studio developer terminal on
Windows. Match architecture, runtime and Release/Debug configuration. See the
[README](../README.md) for library build and installation instructions.

This reference describes the public headers in this repository. It covers the
library's declared API; inherited FLTK methods and private implementation helpers
are outside its scope. Keep it aligned with header changes. Windows is the local
validation target; this reference does not establish macOS/Linux runtime or
clean-machine distribution validation.
