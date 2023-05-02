#ifndef TRACKER_PANEL_H_D675A54D
#define TRACKER_PANEL_H_D675A54D

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "area_window.h"

class TrackerPanel : public wxPanel {
 public:
  TrackerPanel(wxWindow *parent);

  void UpdateIndicators();

 private:
  void OnPaint(wxPaintEvent &event);

  void Redraw();

  wxImage map_image_;
  wxBitmap rendered_;

  std::vector<AreaWindow *> area_windows_;
  std::vector<AreaPopup *> area_popups_;
};

#endif /* end of include guard: TRACKER_PANEL_H_D675A54D */
