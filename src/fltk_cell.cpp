#include "fltk_cell.h"
#include <FL/fl_draw.H>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <sstream>

namespace fltk_cell {
namespace {
struct Clip {
    explicit Clip(Rect r) { fl_push_clip(r.x, r.y, std::max(0, r.w), std::max(0, r.h)); }
    ~Clip() { fl_pop_clip(); }
};
std::size_t boundary(std::string_view text, std::size_t pos)
{
    while (pos && pos < text.size() && (static_cast<unsigned char>(text[pos]) & 0xc0) == 0x80) --pos;
    return pos;
}
std::string plain(const CellValue& value)
{
    if (auto p = std::get_if<std::string>(&value)) return *p;
    if (auto p = std::get_if<Decimal>(&value)) return p->text;
    if (auto p = std::get_if<std::int64_t>(&value)) return std::to_string(*p);
    if (auto p = std::get_if<bool>(&value)) return *p ? "true" : "false";
    if (auto p = std::get_if<double>(&value)) {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<double>::max_digits10) << *p;
        return out.str();
    }
    return std::holds_alternative<std::monostate>(value) ? "NULL" : "";
}
}

// Parse a decimal coefficient and exponent, then round its digit string without floating-point loss.
std::string formatNumber(std::string_view input, const NumberFormat& f)
{
    const std::string original(input);
    if (input.empty() || input.size() > 10000 || f.decimals < -1 || f.decimals > 100) return original;
    bool negative = false;
    std::size_t i = 0;
    if (input[i] == '+' || input[i] == '-') negative = input[i++] == '-';
    std::string digits;
    int fraction = 0;
    bool dot = false;
    while (i < input.size()) {
        char c = input[i];
        if (c == '.' && !dot) { dot = true; ++i; continue; }
        if (c < '0' || c > '9') break;
        digits += c;
        if (dot) ++fraction;
        ++i;
    }
    if (digits.empty()) return original;
    int exponent = 0;
    if (i < input.size() && (input[i] == 'e' || input[i] == 'E')) {
        ++i;
        bool minus = false;
        if (i < input.size() && (input[i] == '+' || input[i] == '-')) minus = input[i++] == '-';
        const auto start = i;
        while (i < input.size() && input[i] >= '0' && input[i] <= '9') {
            exponent = exponent * 10 + input[i++] - '0';
            if (exponent > 1000) return original;
        }
        if (i == start) return original;
        if (minus) exponent = -exponent;
    }
    if (i != input.size()) return original;
    int point = static_cast<int>(digits.size()) - fraction + exponent + (f.percent ? 2 : 0);
    if (point <= 0) { digits.insert(0, static_cast<std::size_t>(1 - point), '0'); point = 1; }
    if (point > static_cast<int>(digits.size())) digits.append(static_cast<std::size_t>(point - digits.size()), '0');
    if (f.decimals >= 0) {
        const auto count = static_cast<std::size_t>(point + f.decimals);
        const bool round = digits.size() > count && digits[count] >= '5';
        digits.resize(count, '0');
        if (round) {
            std::size_t at = digits.size();
            while (at && digits[at - 1] == '9') digits[--at] = '0';
            if (at) ++digits[at - 1];
            else { digits.insert(digits.begin(), '1'); ++point; }
        }
    }
    bool zero = digits.find_first_not_of('0') == std::string::npos;
    std::string integer = digits.substr(0, static_cast<std::size_t>(point));
    auto first = integer.find_first_not_of('0');
    integer.erase(0, first == std::string::npos ? integer.size() - 1 : first);
    if (f.grouping && f.group_separator)
        for (int pos = static_cast<int>(integer.size()) - 3; pos > 0; pos -= 3) integer.insert(static_cast<std::size_t>(pos), 1, f.group_separator);
    std::string result = (negative && !zero ? "-" : "") + f.prefix + integer;
    if (digits.size() > static_cast<std::size_t>(point)) result += f.decimal_separator + digits.substr(static_cast<std::size_t>(point));
    return result + f.suffix + (f.percent ? "%" : "");
}

std::string formatHex(const Bytes& bytes, std::size_t max_bytes, bool uppercase, bool spaces)
{
    return formatHex(BytesView{bytes.data(),bytes.size()},max_bytes,uppercase,spaces);
}
std::string formatHex(BytesView bytes, std::size_t max_bytes, bool uppercase, bool spaces)
{
    const char* hex = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    std::string out;
    const auto count = bytes.data ? std::min(bytes.size, max_bytes) : 0;
    for (std::size_t i = 0; i < count; ++i) {
        if (i && spaces) out += ' ';
        out += hex[bytes.data[i] >> 4];
        out += hex[bytes.data[i] & 15];
    }
    if (count < bytes.size) out += " ...";
    return out;
}

std::string fitText(std::string_view value, int pixels, Fl_Font font, int size)
{
    if (pixels <= 0) return {};
    fl_font(font, size);
    auto length = boundary(value, std::min<std::size_t>(value.size(), 8192));
    if (length == value.size() && fl_width(value.data(), static_cast<int>(length)) <= pixels) return std::string(value);
    const std::string suffix = "...";
    const double available = pixels - fl_width(suffix.c_str());
    if (available < 0) return {};
    std::size_t low = 0, high = length;
    while (low < high) {
        auto mid = low + (high - low + 1) / 2;
        auto end = boundary(value, mid);
        if (fl_width(value.data(), static_cast<int>(end)) <= available) low = mid;
        else high = mid - 1;
    }
    return std::string(value.substr(0, boundary(value, low))) + suffix;
}

std::string TableCellDrawer::displayText(const CellValue& value) const { return plain(value); }
void TableCellDrawer::frame(const CellContext& c) const
{
    Clip clip(c.rect);
    fl_color(c.state.selected ? c.style.selected_background : c.style.background);
    fl_rectf(c.rect.x, c.rect.y, c.rect.w, c.rect.h);
    if (c.style.grid) {
        fl_color(c.style.border);
        fl_line(c.rect.x, c.rect.y + c.rect.h - 1, c.rect.x + c.rect.w - 1, c.rect.y + c.rect.h - 1);
        fl_line(c.rect.x + c.rect.w - 1, c.rect.y, c.rect.x + c.rect.w - 1, c.rect.y + c.rect.h - 1);
    }
    if (c.state.focused && c.rect.w > 2 && c.rect.h > 2) {
        fl_color(c.style.focus);
        fl_rect(c.rect.x + 1, c.rect.y + 1, c.rect.w - 2, c.rect.h - 2);
    }
}
void TableCellDrawer::text(const CellContext& c, const std::string& value, Fl_Align alignment) const
{
    Clip clip(c.rect);
    auto foreground = c.state.selected ? c.style.selected_foreground : c.style.foreground;
    fl_color(c.state.enabled ? foreground : fl_inactive(foreground));
    auto clipped = fitText(value, c.rect.w - 2 * c.style.padding, c.style.font, c.style.font_size);
    fl_draw(clipped.c_str(), c.rect.x + c.style.padding, c.rect.y,
        std::max(0, c.rect.w - 2 * c.style.padding), c.rect.h, alignment | FL_ALIGN_INSIDE | FL_ALIGN_CLIP, nullptr, 0);
}
void TableCellTextDrawer::draw(const CellContext& c) const { frame(c); text(c, displayText(c.value), alignment); }
std::string TableCellNumberDrawer::displayText(const CellValue& value) const
{
    return std::holds_alternative<std::monostate>(value) ? "NULL" : formatNumber(plain(value), format);
}
void TableCellNumberDrawer::draw(const CellContext& c) const { frame(c); text(c, displayText(c.value), FL_ALIGN_RIGHT); }
std::string TableCellHexDrawer::displayText(const CellValue& value) const
{
    auto bytes = std::get_if<Bytes>(&value);
    if(auto view = std::get_if<BytesView>(&value)) return formatHex(*view,preview_bytes,uppercase,spaces);
    return bytes ? formatHex(*bytes, preview_bytes, uppercase, spaces) : plain(value);
}
void TableCellHexDrawer::draw(const CellContext& c) const { frame(c); text(c, displayText(c.value), FL_ALIGN_LEFT); }
void TableCellCheckBoxDrawer::draw(const CellContext& c) const
{
    frame(c);
    Clip clip(c.rect);
    const int size = std::max(0, std::min({16, c.rect.w - 4, c.rect.h - 4}));
    if (!size) return;
    const int x = c.rect.x + (c.rect.w - size) / 2, y = c.rect.y + (c.rect.h - size) / 2;
    fl_draw_box(FL_DOWN_BOX, x, y, size, size, c.style.background);
    auto color = c.state.enabled ? c.style.foreground : fl_inactive(c.style.foreground);
    fl_color(color);
    auto checked = std::get_if<bool>(&c.value);
    if (!checked) fl_line(x + 4, y + size / 2, x + size - 4, y + size / 2);
    else if (*checked) {
        fl_line(x + 3, y + size / 2, x + size / 2 - 1, y + size - 4);
        fl_line(x + size / 2 - 1, y + size - 4, x + size - 3, y + 3);
    }
}
void TableCellSvgDrawer::draw(const CellContext& c) const
{
    frame(c);
    Clip clip(c.rect);
    const auto image = std::get_if<Fl_Image*>(&c.value);
    if (!image || !*image || (*image)->w() <= 0 || (*image)->h() <= 0) return;
    const double scale = std::min(static_cast<double>(std::max(0, c.rect.w - 2 * c.style.padding)) / (*image)->w(),
        static_cast<double>(std::max(0, c.rect.h - 4)) / (*image)->h());
    const int w = static_cast<int>((*image)->w() * scale), h = static_cast<int>((*image)->h() * scale);
    if (w < 1 || h < 1) return;
    std::unique_ptr<Fl_Image> sized((*image)->copy(w, h));
    if (sized) sized->draw(c.rect.x + (c.rect.w - w) / 2, c.rect.y + (c.rect.h - h) / 2);
}
}
