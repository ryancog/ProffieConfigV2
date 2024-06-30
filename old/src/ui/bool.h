#pragma once
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * ui/bool.h
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

#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/tglbtn.h>

namespace PCUI {

class Bool : public wxPanel {
public:
    Bool(
            wxWindow* parent, 
            wxWindowID winID, 
            bool initialValue = false,
            const wxString& label = wxEmptyString,
            wxString onText = "True",
            wxString offText = "False",
            const wxPoint& pos = wxDefaultPosition, 
            const wxSize& size = wxDefaultSize, 
            int64_t style = 0,
            wxOrientation orient = wxVERTICAL);

    void setToolTip(wxToolTip* tip);

    wxToggleButton* entry();
    wxStaticText* text();
    [[nodiscard]] const wxToggleButton* entry() const;
    [[nodiscard]] const wxStaticText* text() const;

private:
    wxToggleButton* mEntry{nullptr};
    wxStaticText* mText{nullptr};

    void bindEvents();

    const wxString mOnText;
    const wxString mOffText;
};

} // namespace PCUI
