#ifndef TRACKER_FRAME_H_86BD8DFB
#define TRACKER_FRAME_H_86BD8DFB

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class TrackerFrame : public wxFrame {
public:
  TrackerFrame();

private:
  void OnExit(wxCommandEvent &event);
  void OnAbout(wxCommandEvent &event);
};

#endif /* end of include guard: TRACKER_FRAME_H_86BD8DFB */
