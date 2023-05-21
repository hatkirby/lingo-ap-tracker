#include "achievements_pane.h"

#include "ap_state.h"
#include "game_data.h"

AchievementsPane::AchievementsPane(wxWindow* parent)
    : wxListView(parent, wxID_ANY) {
  AppendColumn("Achievement");

  for (int panel_id : GD_GetAchievementPanels()) {
    achievement_names_.push_back(GD_GetPanel(panel_id).achievement_name);
  }

  std::sort(std::begin(achievement_names_), std::end(achievement_names_));

  for (int i = 0; i < achievement_names_.size(); i++) {
    InsertItem(i, achievement_names_.at(i));
  }

  SetColumnWidth(0, wxLIST_AUTOSIZE);

  UpdateIndicators();
}

void AchievementsPane::UpdateIndicators() {
  for (int i = 0; i < achievement_names_.size(); i++) {
    if (AP_HasAchievement(achievement_names_.at(i))) {
      SetItemTextColour(i, *wxBLACK);
    } else {
      SetItemTextColour(i, *wxRED);
    }
  }
}
