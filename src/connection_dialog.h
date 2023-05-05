#ifndef CONNECTION_DIALOG_H_E526D0E7
#define CONNECTION_DIALOG_H_E526D0E7

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <string>

class ConnectionDialog : public wxDialog {
 public:
  ConnectionDialog();

  std::string GetServerValue() { return server_box_->GetValue().ToStdString(); }

  std::string GetPlayerValue() { return player_box_->GetValue().ToStdString(); }

  std::string GetPasswordValue() {
    return password_box_->GetValue().ToStdString();
  }

 private:
  wxTextCtrl* server_box_;
  wxTextCtrl* player_box_;
  wxTextCtrl* password_box_;
};

#endif /* end of include guard: CONNECTION_DIALOG_H_E526D0E7 */
