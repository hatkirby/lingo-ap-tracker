#ifndef AREA_POPUP_H_03FAC988
#define AREA_POPUP_H_03FAC988

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "eye_indicator.h"

class AreaPopup : public wxPanel {
 public:
  AreaPopup(wxWindow* parent, int area_id);

  void UpdateIndicators();

 private:
  int area_id_;

  std::vector<wxStaticText*> section_labels_;
  std::vector<EyeIndicator*> eye_indicators_;
};

#endif /* end of include guard: AREA_POPUP_H_03FAC988 */
