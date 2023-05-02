#include "tracker_panel.h"

#include "area_popup.h"
#include "game_data.h"

TrackerPanel::TrackerPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY) {
  map_image_ = wxImage("assets/lingo_map.png", wxBITMAP_TYPE_PNG);
  if (!map_image_.IsOk()) {
    return;
  }

  for (const MapArea &map_area : GetGameData().GetMapAreas()) {
    AreaPopup *area_popup = new AreaPopup(this, map_area.id);
    area_popup->SetPosition({0, 0});
    area_popup->Raise();
    area_popups_.push_back(area_popup);

    AreaWindow *area_window = new AreaWindow(this, map_area.id, area_popup);
    area_window->Lower();
    area_windows_.push_back(area_window);
  }

  Redraw();

  Bind(wxEVT_PAINT, &TrackerPanel::OnPaint, this);
}

void TrackerPanel::UpdateIndicators() {
  Redraw();

  for (AreaWindow *area_window : area_windows_) {
    area_window->UpdateIndicators();
  }

  for (AreaPopup *area_popup : area_popups_) {
    area_popup->UpdateIndicators();
  }
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

  for (AreaWindow *area_window : area_windows_) {
    const MapArea &map_area =
        GetGameData().GetMapArea(area_window->GetAreaId());
    int real_area_size =
        final_width * AreaWindow::EFFECTIVE_SIZE / image_size.GetWidth();
    area_window->SetSize({real_area_size, real_area_size});
    area_window->SetPosition({
        final_x + (map_area.map_x - (AreaWindow::EFFECTIVE_SIZE / 2)) *
                      final_width / image_size.GetWidth(),
        final_y + (map_area.map_y - (AreaWindow::EFFECTIVE_SIZE / 2)) *
                      final_width / image_size.GetWidth(),
    });

    AreaPopup *area_popup = area_window->GetPopup();
    int popup_x =
        final_x + map_area.map_x * final_width / image_size.GetWidth();
    int popup_y =
        final_y + map_area.map_y * final_width / image_size.GetWidth();
    if (popup_x + area_popup->GetSize().GetWidth() > panel_size.GetWidth()) {
      popup_x = panel_size.GetWidth() - area_popup->GetSize().GetWidth();
    }
    if (popup_y + area_popup->GetSize().GetHeight() > panel_size.GetHeight()) {
      popup_y = panel_size.GetHeight() - area_popup->GetSize().GetHeight();
    }
    area_popup->SetPosition({popup_x, popup_y});
  }
}
