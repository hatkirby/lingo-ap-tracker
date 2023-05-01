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
  wxSize panel_size = GetSize();
  wxSize image_size = map_image_.GetSize();

  int final_x = 0;
  int final_y = 0;
  int final_width = panel_size.GetWidth();
  int final_height = panel_size.GetHeight();

  if (image_size.GetWidth() * panel_size.GetHeight() >
      panel_size.GetWidth() * image_size.GetHeight()) {
    final_height = (panel_size.GetWidth() * image_size.GetHeight()) /
                   image_size.GetWidth();
    final_y = (panel_size.GetHeight() - final_height) / 2;
  } else {
    final_width = (image_size.GetWidth() * panel_size.GetHeight()) /
                  image_size.GetHeight();
    final_x = (panel_size.GetWidth() - final_width) / 2;
  }

  rendered_ = wxBitmap(
      map_image_.Scale(final_width, final_height, wxIMAGE_QUALITY_NORMAL)
          .Size(panel_size, {final_x, final_y}, 0, 0, 0));
}
