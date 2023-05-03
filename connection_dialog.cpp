#include "connection_dialog.h"

#include "tracker_config.h"

ConnectionDialog::ConnectionDialog()
    : wxDialog(nullptr, wxID_ANY, "Connect to Archipelago") {
  server_box_ = new wxTextCtrl(this, -1, GetTrackerConfig().ap_server, wxDefaultPosition,
                               {300, -1});
  player_box_ = new wxTextCtrl(this, -1, GetTrackerConfig().ap_player, wxDefaultPosition,
                               {300, -1});
  password_box_ = new wxTextCtrl(this, -1, GetTrackerConfig().ap_password,
                                 wxDefaultPosition, {300, -1});

  wxFlexGridSizer* form_sizer = new wxFlexGridSizer(2, 10, 10);

  form_sizer->Add(
      new wxStaticText(this, -1, "Server:"),
      wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
  form_sizer->Add(server_box_, wxSizerFlags().Expand());
  form_sizer->Add(
      new wxStaticText(this, -1, "Player:"),
      wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
  form_sizer->Add(player_box_, wxSizerFlags().Expand());
  form_sizer->Add(
      new wxStaticText(this, -1, "Password:"),
      wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT));
  form_sizer->Add(password_box_, wxSizerFlags().Expand());

  wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);
  top_sizer->Add(new wxStaticText(
                     this, -1, "Enter the details to connect to Archipelago."),
                 wxSizerFlags().Align(wxALIGN_LEFT).DoubleBorder());
  top_sizer->Add(form_sizer, wxSizerFlags().DoubleBorder().Expand());
  top_sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Center());

  SetSizerAndFit(top_sizer);

  Center();
  server_box_->SetFocus();
}
