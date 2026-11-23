#include "fltk_cell_editor.h"
#include <FL/Fl.H>
#include <FL/Fl_Table.H>

namespace fltk_cell {
struct TableCellTextEditor::Input : Fl_Input {
    TableCellTextEditor& editor;
    Input(Rect r, TableCellTextEditor& owner) : Fl_Input(r.x, r.y, r.w, r.h), editor(owner) {}
    int handle(int event) override {
        if (event == FL_KEYDOWN && (Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter)) {
            editor.finish(true);
            return 1;
        }
        if (event == FL_KEYDOWN && Fl::event_key() == FL_Escape) { editor.cancel(); return 1; }
        return Fl_Input::handle(event);
    }
};
TableCellTextEditor::TableCellTextEditor() = default;
TableCellTextEditor::~TableCellTextEditor() { finished_ = {}; cancel(); }
void TableCellTextEditor::begin(Fl_Group& parent, Rect rect, const CellValue& value, CellEditFinished finished)
{
    cancel();
    auto* previous = Fl_Group::current();
    Fl_Group::current(nullptr);
    input_ = std::make_unique<Input>(rect, *this);
    Fl_Group::current(previous);
    // Fl_Table hides Fl_Group::add to insert into its inner scrolling group.
    if(auto* table=dynamic_cast<Fl_Table*>(&parent)) table->add(*input_);
    else parent.add(*input_);
    finished_ = std::move(finished);
    TableCellTextDrawer drawer;
    input_->value(drawer.displayText(value).c_str());
    input_->take_focus();
    input_->insert_position(input_->size(), 0);
    parent.redraw();
}
void TableCellTextEditor::resize(Rect r) { if (input_) input_->resize(r.x, r.y, r.w, r.h); }
void TableCellTextEditor::finish(bool accepted)
{
    if (!input_) return;
    std::optional<CellValue> result;
    if (accepted) result = std::string(input_->value());
    auto callback = std::move(finished_);
    auto* parent = input_->parent();
    if (parent) parent->remove(*input_);
    // FLTK may still be dispatching this widget's key event: defer its destruction.
    Fl::delete_widget(input_.release());
    if (parent) parent->redraw();
    if (callback) callback(std::move(result));
}
void TableCellTextEditor::cancel() { finish(false); }
void TableCellCheckBoxEditor::begin(Fl_Group&, Rect, const CellValue& value, CellEditFinished finished)
{
    auto checked = std::get_if<bool>(&value);
    if (finished) finished(CellValue(checked ? !*checked : true));
}
}
