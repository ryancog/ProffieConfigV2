#pragma once
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * ui/numericdec.cpp
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
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace PCUI {

class NumericDec : wxPanel {
public:
    NumericDec(
        wxWindow* parent,
        wxWindowID winID = wxID_ANY,
        const wxString& label = wxEmptyString,
        const wxSize& size = wxDefaultSize,
        int64_t style = wxSP_ARROW_KEYS,
        double min       = 0,
        double max       = 0,
        double initial   = 0,
        double increment = 0,
        const wxOrientation& orient = wxVERTICAL
        );

    void setToolTip(wxToolTip* tip);

    [[nodiscard]] const wxSpinCtrlDouble* entry() const;
    [[nodiscard]] const wxStaticText* text() const;

private:
    wxSpinCtrlDouble* mEntry{nullptr};
    wxStaticText* mText{nullptr};
};

} // namespace PCUI
