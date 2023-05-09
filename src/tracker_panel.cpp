#include "tracker_panel.h"

#include "ap_state.h"
#include "area_popup.h"
#include "game_data.h"
#include "tracker_state.h"

constexpr int AREA_ACTUAL_SIZE = 64;
constexpr int AREA_BORDER_SIZE = 5;
constexpr int AREA_EFFECTIVE_SIZE = AREA_ACTUAL_SIZE + AREA_BORDER_SIZE * 2;

TrackerPanel::TrackerPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY) {
  map_image_ = wxImage("assets/lingo_map.png", wxBITMAP_TYPE_PNG);
  if (!map_image_.IsOk()) {
    return;
  }

  for (const MapArea &map_area : GD_GetMapAreas()) {
    AreaIndicator area;
    area.area_id = map_area.id;

    area.popup = new AreaPopup(this, map_area.id);
    area.popup->SetPosition({0, 0});

    areas_.push_back(area);
  }

  Redraw();

  Bind(wxEVT_PAINT, &TrackerPanel::OnPaint, this);
  Bind(wxEVT_MOTION, &TrackerPanel::OnMouseMove, this);
}

void TrackerPanel::UpdateIndicators() {
  Redraw();

  for (AreaIndicator &area : areas_) {
    area.popup->UpdateIndicators();
  }
}

void TrackerPanel::OnPaint(wxPaintEvent &event) {
  if (GetSize() != rendered_.GetSize()) {
    Redraw();
  }

  wxPaintDC dc(this);
  dc.DrawBitmap(rendered_, 0, 0);

  event.Skip();
}

void TrackerPanel::OnMouseMove(wxMouseEvent &event) {
  for (AreaIndicator &area : areas_) {
    if (area.active &&
        area.real_x1 <= event.GetX() && event.GetX() < area.real_x2 &&
        area.real_y1 <= event.GetY() && event.GetY() < area.real_y2) {
      area.popup->Show();
    } else {
      area.popup->Hide();
    }
  }

  event.Skip();
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

  wxMemoryDC dc;
  dc.SelectObject(rendered_);

  for (AreaIndicator &area : areas_) {
    const wxBrush *brush_color = wxGREY_BRUSH;

    const MapArea &map_area = GD_GetMapArea(area.area_id);
    if (map_area.exclude_reduce && AP_IsReduceChecks()) {
      area.active = false;
      continue;
    } else {
      area.active = true;
    }

    bool has_reachable_unchecked = false;
    bool has_unreachable_unchecked = false;
    for (int section_id = 0; section_id < map_area.locations.size();
         section_id++) {
      if (!AP_HasCheckedGameLocation(area.area_id, section_id)) {
        if (IsLocationReachable(area.area_id, section_id)) {
          has_reachable_unchecked = true;
        } else {
          has_unreachable_unchecked = true;
        }
      }
    }

    if (has_reachable_unchecked && has_unreachable_unchecked) {
      brush_color = wxYELLOW_BRUSH;
    } else if (has_reachable_unchecked) {
      brush_color = wxGREEN_BRUSH;
    } else if (has_unreachable_unchecked) {
      brush_color = wxRED_BRUSH;
    }

    int real_area_size =
        final_width * AREA_EFFECTIVE_SIZE / image_size.GetWidth();
    int actual_border_size =
        real_area_size * AREA_BORDER_SIZE / AREA_EFFECTIVE_SIZE;
    int real_area_x = final_x + (map_area.map_x - (AREA_EFFECTIVE_SIZE / 2)) *
                                    final_width / image_size.GetWidth();
    int real_area_y = final_y + (map_area.map_y - (AREA_EFFECTIVE_SIZE / 2)) *
                                    final_width / image_size.GetWidth();

    dc.SetPen(*wxThePenList->FindOrCreatePen(*wxBLACK, actual_border_size));
    dc.SetBrush(*brush_color);
    dc.DrawRectangle({real_area_x, real_area_y},
                     {real_area_size, real_area_size});

    area.real_x1 = real_area_x;
    area.real_x2 = real_area_x + real_area_size;
    area.real_y1 = real_area_y;
    area.real_y2 = real_area_y + real_area_size;

    int popup_x =
        final_x + map_area.map_x * final_width / image_size.GetWidth();
    int popup_y =
        final_y + map_area.map_y * final_width / image_size.GetWidth();
    if (popup_x + area.popup->GetSize().GetWidth() > panel_size.GetWidth()) {
      popup_x = panel_size.GetWidth() - area.popup->GetSize().GetWidth();
    }
    if (popup_y + area.popup->GetSize().GetHeight() > panel_size.GetHeight()) {
      popup_y = panel_size.GetHeight() - area.popup->GetSize().GetHeight();
    }
    area.popup->SetPosition({popup_x, popup_y});
  }
}
