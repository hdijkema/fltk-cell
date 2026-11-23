#pragma once
#include "fltk_cell.h"
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <functional>
#include <memory>
#include <optional>

namespace fltk_cell {
// A null optional means cancellation. The application validates/commits the proposed value.
using CellEditFinished = std::function<void(std::optional<CellValue>)>;
// Editors are separate from drawing. Destroy/cancel the editor before destroying its parent group.
// Only one active cell is edited by an instance; data ownership remains with the application.
class TableCellEditor {
public:
    virtual ~TableCellEditor() = default;
    virtual void begin(Fl_Group& parent, Rect rect, const CellValue&, CellEditFinished) = 0;
    virtual void cancel() = 0;
};
class TableCellTextEditor : public TableCellEditor {
public:
    TableCellTextEditor();
    ~TableCellTextEditor() override;
    void begin(Fl_Group&, Rect, const CellValue&, CellEditFinished) override;
    void cancel() override;
    // Reposition during table resize/scroll, or cancel if the edited cell leaves the viewport.
    void resize(Rect);
    bool active() const { return input_ != nullptr; }
private:
    struct Input;
    std::unique_ptr<Input> input_;
    CellEditFinished finished_;
    void finish(bool accepted);
};
class TableCellCheckBoxEditor : public TableCellEditor {
public:
    void begin(Fl_Group&, Rect, const CellValue&, CellEditFinished) override;
    void cancel() override {}
};
}
