#include "area_popup.h"

#include "game_data.h"

AreaPopup::AreaPopup(wxWindow* parent, int area_id)
    : wxPanel(parent, wxID_ANY), area_id_(area_id) {
  const MapArea& map_area = GetGameData().GetMapArea(area_id);

  wxBoxSizer* list_sizer = new wxBoxSizer(wxVERTICAL);

  wxStaticText* top_label = new wxStaticText(this, -1, map_area.name);
  top_label->SetForegroundColour(*wxBLACK);
  top_label->SetFont(top_label->GetFont().Bold());
  list_sizer->Add(top_label, wxSizerFlags().Center().DoubleBorder(wxDOWN));

  bool is_first = true;
  for (const Location& location : map_area.locations) {
    wxSizerFlags sizer_flags = wxSizerFlags().Left();
    if (!is_first) {
      sizer_flags = sizer_flags.Border(wxUP);
    }

    wxStaticText* section_label = new wxStaticText(this, -1, location.name);
    section_label->SetForegroundColour(*wxBLACK);
    list_sizer->Add(section_label, sizer_flags);

    is_first = false;
  }

  wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);
  top_sizer->Add(list_sizer, wxSizerFlags().DoubleBorder(wxALL));

  SetSizerAndFit(top_sizer);

  SetBackgroundColour(*wxLIGHT_GREY);
  Hide();
}
