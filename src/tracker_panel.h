#ifndef TRACKER_PANEL_H_D675A54D
#define TRACKER_PANEL_H_D675A54D

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class AreaPopup;

class TrackerPanel : public wxPanel {
 public:
  TrackerPanel(wxWindow *parent);

  void UpdateIndicators();

 private:
  struct AreaIndicator {
    int area_id = -1;
    AreaPopup *popup = nullptr;
    int real_x1 = 0;
    int real_y1 = 0;
    int real_x2 = 0;
    int real_y2 = 0;
  };
 
  void OnPaint(wxPaintEvent &event);
  void OnMouseMove(wxMouseEvent &event);

  void Redraw();

  wxImage map_image_;
  wxBitmap rendered_;

  std::vector<AreaIndicator> areas_;
};

#endif /* end of include guard: TRACKER_PANEL_H_D675A54D */
