#include "area_window.h"

#include <iostream>

#include "ap_state.h"
#include "game_data.h"

AreaWindow::AreaWindow(wxWindow* parent, int area_id, AreaPopup* popup)
    : wxWindow(parent, wxID_ANY), area_id_(area_id), popup_(popup) {
  SetSize(EFFECTIVE_SIZE, EFFECTIVE_SIZE);

  Redraw();

  Bind(wxEVT_PAINT, &AreaWindow::OnPaint, this);
  Bind(wxEVT_ENTER_WINDOW, &AreaWindow::OnEnterWindow, this);
  Bind(wxEVT_LEAVE_WINDOW, &AreaWindow::OnLeaveWindow, this);
}

void AreaWindow::UpdateIndicators() { Redraw(); }

void AreaWindow::OnPaint(wxPaintEvent& event) {
  if (GetSize() != rendered_.GetSize()) {
    Redraw();
  }

  wxPaintDC dc(this);
  dc.DrawBitmap(rendered_, 0, 0);
}

void AreaWindow::OnEnterWindow(wxMouseEvent& event) { popup_->Show(); }

void AreaWindow::OnLeaveWindow(wxMouseEvent& event) { popup_->Hide(); }

void AreaWindow::Redraw() {
  const wxBrush* brush_color = wxGREY_BRUSH;

  const MapArea& map_area = GetGameData().GetMapArea(area_id_);
  int unchecked_sections = 0;
  for (int section_id = 0; section_id < map_area.locations.size();
       section_id++) {
    if (!GetAPState().HasCheckedGameLocation(area_id_, section_id)) {
      unchecked_sections++;
    }
  }

  if (unchecked_sections > 0) {
    brush_color = wxGREEN_BRUSH;
  }

  int actual_border_size = GetSize().GetWidth() * BORDER_SIZE / EFFECTIVE_SIZE;

  rendered_ = wxBitmap(GetSize());
  wxMemoryDC dc;
  dc.SelectObject(rendered_);
  dc.SetPen(*wxThePenList->FindOrCreatePen(*wxBLACK, actual_border_size));
  dc.SetBrush(*brush_color);
  dc.DrawRectangle({0, 0}, GetSize());
}
