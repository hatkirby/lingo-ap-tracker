#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "tracker_frame.h"

class TrackerApp : public wxApp {
public:
  virtual bool OnInit() {
    TrackerFrame *frame = new TrackerFrame();
    frame->Show(true);
    return true;
  }
};

wxIMPLEMENT_APP(TrackerApp);
