#ifndef EYE_INDICATOR_H_778150F2
#define EYE_INDICATOR_H_778150F2

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class EyeIndicator : public wxWindow {
 public:
  EyeIndicator(wxWindow* parent);

  void SetChecked(bool checked);

 private:
  static const wxImage& GetUncheckedImage();
  static const wxImage& GetCheckedImage();

  void OnPaint(wxPaintEvent& event);

  void Redraw();

  bool intended_checked_ = false;

  wxBitmap rendered_;
  bool rendered_checked_ = false;
};

#endif /* end of include guard: EYE_INDICATOR_H_778150F2 */
