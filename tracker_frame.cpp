#include "tracker_frame.h"

#include "ap_state.h"
#include "connection_dialog.h"
#include "tracker_panel.h"

enum TrackerFrameIds { ID_CONNECT = 1 };

TrackerFrame::TrackerFrame()
    : wxFrame(nullptr, wxID_ANY, "Lingo Archipelago Tracker") {
  ::wxInitAllImageHandlers();

  GetAPState().SetTrackerFrame(this);

  SetSize(1280, 728);

  wxMenu *menuFile = new wxMenu();
  menuFile->Append(ID_CONNECT, "&Connect");
  menuFile->Append(wxID_EXIT);

  wxMenu *menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar *menuBar = new wxMenuBar();
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText("Not connected to Archipelago.");

  Bind(wxEVT_MENU, &TrackerFrame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &TrackerFrame::OnExit, this, wxID_EXIT);
  Bind(wxEVT_MENU, &TrackerFrame::OnConnect, this, ID_CONNECT);

  tracker_panel_ = new TrackerPanel(this);
}

void TrackerFrame::SetStatusMessage(std::string message) {
  SetStatusText(message);
}

void TrackerFrame::UpdateIndicators() {
  tracker_panel_->UpdateIndicators();
  Refresh();
}

void TrackerFrame::OnAbout(wxCommandEvent &event) {
  wxMessageBox("Lingo Archipelago Tracker by hatkirby",
               "About lingo-ap-tracker", wxOK | wxICON_INFORMATION);
}

void TrackerFrame::OnExit(wxCommandEvent &event) { Close(true); }

void TrackerFrame::OnConnect(wxCommandEvent &event) {
  ConnectionDialog dlg;

  if (dlg.ShowModal() == wxID_OK) {
    GetAPState().Connect(dlg.GetServerValue(), dlg.GetPlayerValue(),
                         dlg.GetPasswordValue());
  }
}
