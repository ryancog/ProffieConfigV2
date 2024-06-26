#include "selection.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * ui/text.cpp
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/tooltip.h>

using namespace PCUI;

Selection::Selection(
    wxWindow* parent,
    wxWindowID winID,
    const wxString& label,
    const wxSize& size,
    int64_t style,
    wxOrientation orient) :
    wxPanel(parent, winID, wxDefaultPosition, size)
{
    auto *sizer{new wxBoxSizer(orient)};

    // if (!label.empty()) {
    //     mText = new wxStaticText(this, wxID_ANY, label);
    //     auto sizerFlags{wxSizerFlags(0).Border(wxLEFT | wxRIGHT, 5)};
    //     sizer->Add(mText, orient == wxHORIZONTAL ? sizerFlags.Center() : sizerFlags);
    // }

    mEntry = new wxRadioButton(this,
                               wxID_ANY,
                               label,
                               wxDefaultPosition,
                               wxDefaultSize,
                               style);
    sizer->Add(mEntry, wxSizerFlags(1).Expand());

    SetSizer(sizer);
}

void Selection::setToolTip(wxToolTip* tip) {
    SetToolTip(tip->GetTip());
    mEntry->SetToolTip(new wxToolTip(tip->GetTip()));
    if (mText) mText->SetToolTip(new wxToolTip(tip->GetTip()));
}
