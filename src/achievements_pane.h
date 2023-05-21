#ifndef ACHIEVEMENTS_PANE_H_C320D0B8
#define ACHIEVEMENTS_PANE_H_C320D0B8

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/listctrl.h>

class AchievementsPane : public wxListView {
 public:
  explicit AchievementsPane(wxWindow* parent);

  void UpdateIndicators();

 private:
  std::vector<std::string> achievement_names_;
};

#endif /* end of include guard: ACHIEVEMENTS_PANE_H_C320D0B8 */