#include "tracker_frame.h"

#include <wx/choicebk.h>
#include <wx/webrequest.h>

#include <nlohmann/json.hpp>
#include <sstream>

#include "achievements_pane.h"
#include "ap_state.h"
#include "connection_dialog.h"
#include "tracker_config.h"
#include "tracker_panel.h"
#include "version.h"

enum TrackerFrameIds { ID_CONNECT = 1, ID_CHECK_FOR_UPDATES = 2 };

wxDEFINE_EVENT(STATE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(STATUS_CHANGED, wxCommandEvent);

TrackerFrame::TrackerFrame()
    : wxFrame(nullptr, wxID_ANY, "Lingo Archipelago Tracker", wxDefaultPosition,
              wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE) {
  ::wxInitAllImageHandlers();

  AP_SetTrackerFrame(this);

  SetSize(1280, 728);

  wxMenu *menuFile = new wxMenu();
  menuFile->Append(ID_CONNECT, "&Connect");
  menuFile->Append(wxID_EXIT);

  wxMenu *menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);
  menuHelp->Append(ID_CHECK_FOR_UPDATES, "Check for Updates");

  wxMenuBar *menuBar = new wxMenuBar();
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  SetMenuBar(menuBar);

  CreateStatusBar();
  SetStatusText("Not connected to Archipelago.");

  Bind(wxEVT_MENU, &TrackerFrame::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &TrackerFrame::OnExit, this, wxID_EXIT);
  Bind(wxEVT_MENU, &TrackerFrame::OnConnect, this, ID_CONNECT);
  Bind(wxEVT_MENU, &TrackerFrame::OnCheckForUpdates, this,
       ID_CHECK_FOR_UPDATES);
  Bind(STATE_CHANGED, &TrackerFrame::OnStateChanged, this);
  Bind(STATUS_CHANGED, &TrackerFrame::OnStatusChanged, this);

  wxChoicebook *choicebook = new wxChoicebook(this, wxID_ANY);
  achievements_pane_ = new AchievementsPane(this);
  choicebook->AddPage(achievements_pane_, "Achievements");

  tracker_panel_ = new TrackerPanel(this);

  wxBoxSizer *top_sizer = new wxBoxSizer(wxHORIZONTAL);
  top_sizer->Add(choicebook, wxSizerFlags().Expand().Proportion(1));
  top_sizer->Add(tracker_panel_, wxSizerFlags().Expand().Proportion(3));

  SetSizerAndFit(top_sizer);

  if (!GetTrackerConfig().asked_to_check_for_updates) {
    GetTrackerConfig().asked_to_check_for_updates = true;

    if (wxMessageBox(
            "Check for updates automatically when the tracker is opened?",
            "Lingo AP Tracker", wxYES_NO) == wxYES) {
      GetTrackerConfig().should_check_for_updates = true;
    } else {
      GetTrackerConfig().should_check_for_updates = false;
    }

    GetTrackerConfig().Save();
  }

  if (GetTrackerConfig().should_check_for_updates) {
    CheckForUpdates(/*manual=*/false);
  }
}

void TrackerFrame::SetStatusMessage(std::string message) {
  wxCommandEvent *event = new wxCommandEvent(STATUS_CHANGED);
  event->SetString(message.c_str());

  QueueEvent(event);
}

void TrackerFrame::UpdateIndicators() {
  QueueEvent(new wxCommandEvent(STATE_CHANGED));
}

void TrackerFrame::OnAbout(wxCommandEvent &event) {
  std::ostringstream message_text;
  message_text << "Lingo Archipelago Tracker " << kTrackerVersion
               << " by hatkirby";

  wxMessageBox(message_text.str(), "About lingo-ap-tracker",
               wxOK | wxICON_INFORMATION);
}

void TrackerFrame::OnExit(wxCommandEvent &event) { Close(true); }

void TrackerFrame::OnConnect(wxCommandEvent &event) {
  ConnectionDialog dlg;

  if (dlg.ShowModal() == wxID_OK) {
    GetTrackerConfig().ap_server = dlg.GetServerValue();
    GetTrackerConfig().ap_player = dlg.GetPlayerValue();
    GetTrackerConfig().ap_password = dlg.GetPasswordValue();
    GetTrackerConfig().Save();

    AP_Connect(dlg.GetServerValue(), dlg.GetPlayerValue(),
               dlg.GetPasswordValue());
  }
}

void TrackerFrame::OnCheckForUpdates(wxCommandEvent &event) {
  CheckForUpdates(/*manual=*/true);
}

void TrackerFrame::OnStateChanged(wxCommandEvent &event) {
  tracker_panel_->UpdateIndicators();
  achievements_pane_->UpdateIndicators();
  Refresh();
}

void TrackerFrame::OnStatusChanged(wxCommandEvent &event) {
  SetStatusText(event.GetString());
}

void TrackerFrame::CheckForUpdates(bool manual) {
  wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
      this,
      "https://api.github.com/repos/hatkirby/lingo-ap-tracker/"
      "releases?per_page=8");

  if (!request.IsOk()) {
    if (manual) {
      wxMessageBox("Could not check for updates.", "Error",
                   wxOK | wxICON_ERROR);
    } else {
      SetStatusText("Could not check for updates.");
    }

    return;
  }

  Bind(wxEVT_WEBREQUEST_STATE, [this, manual](wxWebRequestEvent &evt) {
    if (evt.GetState() == wxWebRequest::State_Completed) {
      std::string response = evt.GetResponse().AsString().ToStdString();
      nlohmann::json parsed_response = nlohmann::json::parse(response);

      if (parsed_response.is_array() && !parsed_response.empty()) {
        // This will parse to 0.0.0 if it's invalid, which will always be older
        // than our current version.
        Version latest_version(
            parsed_response[0]["tag_name"].get<std::string>());
        if (kTrackerVersion < latest_version) {
          std::ostringstream message_text;
          message_text << "There is a newer version of Lingo AP Tracker "
                          "available. You have "
                       << kTrackerVersion << ", and the latest version is "
                       << latest_version << ". Would you like to update?";

          if (wxMessageBox(message_text.str(), "Update available", wxYES_NO) ==
              wxYES) {
            wxLaunchDefaultBrowser(
                parsed_response[0]["html_url"].get<std::string>());
          }
        } else if (manual) {
          wxMessageBox("Lingo AP Tracker is up to date!", "Lingo AP Tracker",
                       wxOK);
        }
      } else if (manual) {
        wxMessageBox("Lingo AP Tracker is up to date!", "Lingo AP Tracker",
                     wxOK);
      }
    } else if (evt.GetState() == wxWebRequest::State_Failed) {
      if (manual) {
        wxMessageBox("Could not check for updates.", "Error",
                     wxOK | wxICON_ERROR);
      } else {
        SetStatusText("Could not check for updates.");
      }
    }
  });

  request.Start();
}
