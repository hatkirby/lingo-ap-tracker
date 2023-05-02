#ifndef AREA_WINDOW_H_C2653ACF
#define AREA_WINDOW_H_C2653ACF

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class AreaWindow : public wxWindow {
 public:
  static constexpr int ACTUAL_SIZE = 64;
  static constexpr int BORDER_SIZE = 5;
  static constexpr int EFFECTIVE_SIZE = ACTUAL_SIZE + BORDER_SIZE * 2;

  AreaWindow(wxWindow* parent, int area_id);

  int GetAreaId() const { return area_id_; }

 private:
  void OnPaint(wxPaintEvent& event);
  void OnEnterWindow(wxMouseEvent& event);

  void Redraw();

  int area_id_;
  wxBitmap rendered_;
};

#endif /* end of include guard: AREA_WINDOW_H_C2653ACF */
