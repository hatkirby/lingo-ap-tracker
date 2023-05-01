#ifndef TRACKER_PANEL_H_D675A54D
#define TRACKER_PANEL_H_D675A54D

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class TrackerPanel : public wxPanel {
public:
  TrackerPanel(wxWindow *parent);

private:
  void OnPaint(wxPaintEvent &event);

  void Redraw();

  wxImage map_image_;
  wxBitmap rendered_;
};

#endif /* end of include guard: TRACKER_PANEL_H_D675A54D */
