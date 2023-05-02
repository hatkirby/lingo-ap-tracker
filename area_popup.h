#ifndef AREA_POPUP_H_03FAC988
#define AREA_POPUP_H_03FAC988

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class AreaPopup : public wxPanel {
 public:
  AreaPopup(wxWindow* parent, int area_id);

 private:
  int area_id_;
};

#endif /* end of include guard: AREA_POPUP_H_03FAC988 */
