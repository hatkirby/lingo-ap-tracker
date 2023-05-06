#ifndef TRACKER_FRAME_H_86BD8DFB
#define TRACKER_FRAME_H_86BD8DFB

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class TrackerPanel;

wxDECLARE_EVENT(STATE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(STATUS_CHANGED, wxCommandEvent);

class TrackerFrame : public wxFrame {
 public:
  TrackerFrame();

  void SetStatusMessage(std::string message);

  void UpdateIndicators();

 private:
  void OnExit(wxCommandEvent &event);
  void OnAbout(wxCommandEvent &event);
  void OnConnect(wxCommandEvent &event);
  void OnCheckForUpdates(wxCommandEvent &event);

  void OnStateChanged(wxCommandEvent &event);
  void OnStatusChanged(wxCommandEvent &event);

  void CheckForUpdates(bool manual);

  TrackerPanel *tracker_panel_;
};

#endif /* end of include guard: TRACKER_FRAME_H_86BD8DFB */
