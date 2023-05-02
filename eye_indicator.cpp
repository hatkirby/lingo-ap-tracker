#include "eye_indicator.h"

EyeIndicator::EyeIndicator(wxWindow* parent) : wxWindow(parent, wxID_ANY) {
  SetMinSize({32, 32});

  Redraw();

  Bind(wxEVT_PAINT, &EyeIndicator::OnPaint, this);
}

void EyeIndicator::SetChecked(bool checked) {
  if (intended_checked_ != checked) {
    intended_checked_ = checked;

    Redraw();
  }
}

const wxImage& EyeIndicator::GetUncheckedImage() {
  static wxImage* unchecked_image =
      new wxImage("assets/unchecked.png", wxBITMAP_TYPE_PNG);
  return *unchecked_image;
}

const wxImage& EyeIndicator::GetCheckedImage() {
  static wxImage* checked_image =
      new wxImage("assets/checked.png", wxBITMAP_TYPE_PNG);
  return *checked_image;
}

void EyeIndicator::OnPaint(wxPaintEvent& event) {
  if (GetSize() != rendered_.GetSize() ||
      intended_checked_ != rendered_checked_) {
    Redraw();
  }

  wxPaintDC dc(this);
  dc.DrawBitmap(rendered_, 0, 0);
}

void EyeIndicator::Redraw() {
  rendered_ =
      wxBitmap((intended_checked_ ? GetCheckedImage() : GetUncheckedImage())
                   .Scale(GetSize().GetWidth(), GetSize().GetHeight(),
                          wxIMAGE_QUALITY_NORMAL));
  rendered_checked_ = intended_checked_;
}
