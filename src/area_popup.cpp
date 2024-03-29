#include "area_popup.h"

#include "ap_state.h"
#include "game_data.h"
#include "tracker_state.h"

AreaPopup::AreaPopup(wxWindow* parent, int area_id)
    : wxScrolledWindow(parent, wxID_ANY), area_id_(area_id) {
  const MapArea& map_area = GD_GetMapArea(area_id);

  wxFlexGridSizer* section_sizer = new wxFlexGridSizer(2, 10, 10);

  for (const Location& location : map_area.locations) {
    EyeIndicator* eye_indicator = new EyeIndicator(this);
    section_sizer->Add(eye_indicator, wxSizerFlags().Expand());
    eye_indicators_.push_back(eye_indicator);

    wxStaticText* section_label = new wxStaticText(this, -1, location.name);
    section_label->SetForegroundColour(*wxWHITE);
    section_sizer->Add(
        section_label,
        wxSizerFlags().Align(wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL));
    section_labels_.push_back(section_label);
  }

  wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);

  wxStaticText* top_label = new wxStaticText(this, -1, map_area.name);
  top_label->SetForegroundColour(*wxWHITE);
  top_label->SetFont(top_label->GetFont().Bold());
  top_sizer->Add(top_label,
                 wxSizerFlags().Center().DoubleBorder(wxUP | wxLEFT | wxRIGHT));

  top_sizer->Add(section_sizer, wxSizerFlags().DoubleBorder(wxALL).Expand());

  SetSizerAndFit(top_sizer);
  SetScrollRate(5, 5);

  SetBackgroundColour(*wxBLACK);
  Hide();
}

void AreaPopup::UpdateIndicators() {
  const MapArea& map_area = GD_GetMapArea(area_id_);
  for (int section_id = 0; section_id < map_area.locations.size();
       section_id++) {
    const Location& location = map_area.locations.at(section_id);
    wxSizer* container_sizer =
        section_labels_[section_id]->GetContainingSizer();

    if (!AP_IsLocationVisible(location.classification)) {
      container_sizer->Hide(section_labels_[section_id]);
      container_sizer->Hide(eye_indicators_[section_id]);
      continue;
    } else {
      container_sizer->Show(section_labels_[section_id]);
      container_sizer->Show(eye_indicators_[section_id]);
    }

    bool checked = AP_HasCheckedGameLocation(location.ap_location_id);
    bool reachable = IsLocationReachable(location.ap_location_id);
    const wxColour* text_color = reachable ? wxWHITE : wxRED;

    section_labels_[section_id]->SetForegroundColour(*text_color);
    eye_indicators_[section_id]->SetChecked(checked);
  }

  section_labels_[0]->GetContainingSizer()->Layout();
  GetSizer()->Fit(this);
}
