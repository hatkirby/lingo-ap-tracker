#include "tracker_panel.h"

TrackerPanel::TrackerPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY) {
  map_image_ = wxImage("assets/lingo_map.png", wxBITMAP_TYPE_PNG);
  if (!map_image_.IsOk()) {
    return;
  }

  Redraw();

  Bind(wxEVT_PAINT, &TrackerPanel::OnPaint, this);
}

void TrackerPanel::OnPaint(wxPaintEvent &event) {
  if (GetSize() != rendered_.GetSize()) {
    Redraw();
  }

  wxPaintDC dc(this);
  dc.DrawBitmap(rendered_, 0, 0);
}

void TrackerPanel::Redraw() {
  wxSize sz = GetSize();
  rendered_ = wxBitmap(
      map_image_.Scale(sz.GetWidth(), sz.GetHeight(), wxIMAGE_QUALITY_NORMAL));
}
