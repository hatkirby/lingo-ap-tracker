#include "area_window.h"

#include <iostream>

#include "game_data.h"

AreaWindow::AreaWindow(wxWindow* parent, int area_id)
    : wxWindow(parent, wxID_ANY), area_id_(area_id) {
  SetSize(EFFECTIVE_SIZE, EFFECTIVE_SIZE);

  Redraw();

  Bind(wxEVT_PAINT, &AreaWindow::OnPaint, this);
  Bind(wxEVT_ENTER_WINDOW, &AreaWindow::OnEnterWindow, this);
}

void AreaWindow::OnPaint(wxPaintEvent& event) {
  if (GetSize() != rendered_.GetSize()) {
    Redraw();
  }

  wxPaintDC dc(this);
  dc.DrawBitmap(rendered_, 0, 0);
}

void AreaWindow::OnEnterWindow(wxMouseEvent& event) {
  std::cout << GetGameData().GetMapArea(area_id_).name << std::endl;
  std::cout << "---" << std::endl;
  for (const Location& loc : GetGameData().GetMapArea(area_id_).locations) {
    std::cout << loc.name << std::endl;
  }
  std::cout << "---" << std::endl;
}

void AreaWindow::Redraw() {
  int actual_border_size = GetSize().GetWidth() * BORDER_SIZE / EFFECTIVE_SIZE;

  rendered_ = wxBitmap(GetSize());
  wxMemoryDC dc;
  dc.SelectObject(rendered_);
  dc.SetPen(*wxThePenList->FindOrCreatePen(*wxBLACK, actual_border_size));
  dc.SetBrush(*wxGREEN_BRUSH);
  dc.DrawRectangle({0, 0}, GetSize());
}
