#include "tracker_frame.h"

#include "tracker_panel.h"

TrackerFrame::TrackerFrame()
    : wxFrame(nullptr, wxID_ANY, "Lingo Archipelago Tracker") {
  ::wxInitAllImageHandlers();

  SetSize(1280, 728);

  wxMenu *menuFile = new wxMenu();
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

  new TrackerPanel(this);
}

void TrackerFrame::OnAbout(wxCommandEvent &event) {
  wxMessageBox("Lingo Archipelago Tracker by hatkirby",
               "About lingo-ap-tracker", wxOK | wxICON_INFORMATION);
}

void TrackerFrame::OnExit(wxCommandEvent &event) { Close(true); }
