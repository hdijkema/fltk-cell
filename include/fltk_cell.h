#pragma once
#include <FL/Fl_Image.H>
#include <FL/Enumerations.H>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace fltk_cell {
struct Rect { int x = 0, y = 0, w = 0, h = 0; };
struct Decimal { std::string text; };
using Bytes = std::vector<std::uint8_t>;
// Borrowed for the duration of draw/displayText; avoids copying a blob just to preview it.
struct BytesView { const std::uint8_t* data = nullptr; std::size_t size = 0; };
// Image pointers are borrowed: the caller keeps the image alive throughout drawing.
using CellValue = std::variant<std::monostate, std::string, bool, std::int64_t, double, Decimal, Bytes, BytesView, Fl_Image*>;
struct CellStyle {
    Fl_Color background = FL_WHITE, foreground = FL_BLACK, border = FL_LIGHT2;
    Fl_Color selected_background = FL_SELECTION_COLOR, selected_foreground = FL_WHITE;
    Fl_Color focus = FL_DARK_BLUE;
    Fl_Font font = FL_HELVETICA;
    int font_size = 14, padding = 6;
    bool grid = true;
};
struct CellState { bool selected = false, focused = false, enabled = true; };
struct CellContext { Rect rect; const CellValue& value; const CellStyle& style; CellState state; };
struct NumberFormat {
    int decimals = 2; // -1 preserves source fractional digits; otherwise 0..100.
    char decimal_separator = '.', group_separator = ',';
    bool grouping = true, percent = false;
    std::string prefix, suffix;
};
// Exact decimal formatting, rounded half away from zero for display only. Source values never change.
// Invalid/overlong numeric input is returned unchanged; no implicit double conversion is performed.
std::string formatNumber(std::string_view value, const NumberFormat& format = {});
// Render at most max_bytes and a truncation marker, without expanding the entire binary value.
std::string formatHex(const Bytes& value, std::size_t max_bytes = 24, bool uppercase = true, bool spaces = true);
std::string formatHex(BytesView value, std::size_t max_bytes = 24, bool uppercase = true, bool spaces = true);
// Return the longest code-point-aligned prefix fitting the available pixels, with ellipsis when needed.
// Requires the GUI thread and an initialized FLTK display/font system.
std::string fitText(std::string_view text, int pixels, Fl_Font font, int font_size);

// Stateless drawing interface. Caller owns data/style and calls only on the FLTK UI thread.
// Implementations clip every operation to rect; they never update data, own widgets or schedule redraws.
class TableCellDrawer {
public:
    virtual ~TableCellDrawer() = default;
    virtual void draw(const CellContext&) const = 0;
    virtual std::string displayText(const CellValue&) const;
protected:
    void frame(const CellContext&) const;
    void text(const CellContext&, const std::string&, Fl_Align alignment) const;
};
class TableCellTextDrawer : public TableCellDrawer {
public:
    Fl_Align alignment = FL_ALIGN_LEFT;
    void draw(const CellContext&) const override;
};
class TableCellNumberDrawer : public TableCellDrawer {
public:
    NumberFormat format;
    std::string displayText(const CellValue&) const override;
    void draw(const CellContext&) const override;
};
class TableCellHexDrawer : public TableCellDrawer {
public:
    std::size_t preview_bytes = 24;
    bool uppercase = true, spaces = true;
    std::string displayText(const CellValue&) const override;
    void draw(const CellContext&) const override;
};
class TableCellCheckBoxDrawer : public TableCellDrawer {
public:
    void draw(const CellContext&) const override;
};
class TableCellSvgDrawer : public TableCellDrawer {
public:
    void draw(const CellContext&) const override;
};
}
