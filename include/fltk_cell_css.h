#pragma once
#include "fltk_cell.h"
#include <fltk_css.h>

namespace fltk_cell {
// Translate a resolved CSS node into drawing values. No stylesheet is retained by a drawer.
inline CellStyle cellStyle(const FLTK_Css& css, FLTK_Css_Node node)
{
    const auto source = css.style(node);
    CellStyle result;
    result.background = source.background;
    result.foreground = source.color;
    result.border = source.border_color;
    result.selected_background = source.selection_color;
    result.font_size = source.font_size;
    result.font = source.font_family == "monospace" ? FL_COURIER : source.font_family == "serif" ? FL_TIMES : FL_HELVETICA;
    if (source.bold) result.font = static_cast<Fl_Font>(result.font + 1);
    if (source.italic) result.font = static_cast<Fl_Font>(result.font + 2);
    return result;
}
}
