#include "fltk_cell.h"
#include "fltk_cell_editor.h"
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Table.H>
#include <FL/Fl_SVG_Image.H>
#ifdef FLTK_CELL_USE_CSS
#include "fltk_cell_css.h"
#endif
#include <cstdlib>
using namespace fltk_cell;

class Gallery : public Fl_Table {
    TableCellTextDrawer text_;
    TableCellNumberDrawer number_;
    TableCellHexDrawer hex_;
    TableCellCheckBoxDrawer check_;
    TableCellSvgDrawer svg_;
    TableCellTextEditor editor_;
    TableCellCheckBoxEditor toggle_;
    int edit_row_=-1,edit_col_=-1;
    Fl_SVG_Image image_{nullptr, "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'><circle cx='12' cy='12' r='10' fill='#21875c'/></svg>"};
    std::vector<std::vector<CellValue>> data_;
#ifdef FLTK_CELL_USE_CSS
    FLTK_Css css_;
#endif
public:
    Gallery(int x, int y, int w, int h) : Fl_Table(x, y, w, h) {
#ifdef FLTK_CELL_USE_CSS
        css_.parse("cell { background: #f3f6f8; color: #203040; border: 1px solid #c8d0da; font-size: 14px; }");
#endif
        number_.format.decimal_separator = ',';
        number_.format.group_separator = '.';
        for (int row = 0; row < 30; ++row)
            data_.push_back({std::string("Editable row ") + std::to_string(row + 1), Decimal{"9007199254740993.125"},
                Bytes{0, 31, 163, 255, 0, 127}, row % 2 == 0, static_cast<Fl_Image*>(&image_)});
        rows(static_cast<int>(data_.size())); cols(5);
        col_header(1); col_header_height(28); col_width_all(200); col_width(4, 70);
        row_height_all(30); col_resize(1);
        callback([](Fl_Widget* widget, void*) { static_cast<Gallery*>(widget)->activateCell(); });
        when(FL_WHEN_RELEASE);
        end();
    }
    void activateCell() {
        if (callback_context() != CONTEXT_CELL || Fl::event_button() != FL_LEFT_MOUSE) return;
        int row = callback_row(), col = callback_col();
        if (row < 0 || col < 0) return;
        int x, y, w, h;
        if (find_cell(CONTEXT_CELL, row, col, x, y, w, h) != 0) return;
        if (col == 3) toggle_.begin(*this, {x,y,w,h}, data_[row][col], [this,row,col](auto value) {
            if (value) data_[row][col] = std::move(*value);
            redraw();
        });
        else if (col == 0 && Fl::event_clicks()) {
            edit_row_=row;edit_col_=col;
            editor_.begin(*this, {x,y,w,h}, data_[row][col], [this,row,col](auto value) {
            if (value) data_[row][col] = std::move(*value);
            redraw();
        });
        }
    }
    int handle(int event) override {
        if (editor_.active() && (event == FL_MOUSEWHEEL || event == FL_HIDE)) editor_.cancel();
        return Fl_Table::handle(event);
    }
    void resize(int x,int y,int w,int h) override {
        editor_.cancel();Fl_Table::resize(x,y,w,h);
    }
    void draw_cell(TableContext context, int row, int col, int x, int y, int w, int h) override {
        if(context==CONTEXT_RC_RESIZE){editor_.cancel();return;}
        if (context != CONTEXT_CELL && context != CONTEXT_COL_HEADER) return;
        // Fl_Table paints widgets before cells: leave the edited cell to its input overlay.
        if(context==CONTEXT_CELL && editor_.active() && row==edit_row_ && col==edit_col_) return;
        CellStyle style;
#ifdef FLTK_CELL_USE_CSS
        FLTK_Css_Node node; node.type = "cell";
        style = cellStyle(css_, node);
#endif
        if (context == CONTEXT_COL_HEADER) {
            const char* titles[] = {"Text (double-click)", "Exact decimal", "Binary preview", "Checkbox", "SVG"};
            style.background = fl_rgb_color(220, 230, 240);
            style.font = FL_HELVETICA_BOLD;
            CellValue value = std::string(titles[col]);
            text_.draw({{x,y,w,h}, value, style, {}});
            return;
        }
        const TableCellDrawer* drawer = &text_;
        if (col == 1) drawer = &number_;
        else if (col == 2) drawer = &hex_;
        else if (col == 3) drawer = &check_;
        else if (col == 4) drawer = &svg_;
        drawer->draw({{x,y,w,h}, data_[row][col], style, {is_selected(row,col) != 0, false, true}});
    }
};
int main(int argc, char** argv) {
    Fl_Double_Window window(1050, 440, "fltk-cell: drawers and independent editors");
    Gallery table(10, 10, 1030, 420);
    window.resizable(table); window.end(); window.show();
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--smoke")
        Fl::add_timeout(0.5, [](void* p) { static_cast<Fl_Double_Window*>(p)->hide(); }, &window);
    return Fl::run();
}
