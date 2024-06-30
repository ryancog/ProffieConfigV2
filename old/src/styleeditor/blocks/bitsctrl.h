#pragma once
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * ui/blocks/bitsctrl.h
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
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace PCUI {

class BitsCtrl : public wxPanel {
public:
    BitsCtrl(
        wxWindow* parent,
        wxWindowID winID = wxID_ANY,
        int8_t numBits = sizeof(int16_t) * CHAR_BIT,
        int32_t initialValue = 0x0,
        const wxString& label = wxEmptyString,
        const wxSize& size = wxDefaultSize,
        int64_t style = 0,
        const wxOrientation& orient = wxVERTICAL
        );

    void setToolTip(wxToolTip* tip);
    void setValue(int32_t);
    [[nodiscard]] int32_t getValue() const;

    [[nodiscard]] const wxTextCtrl* entry() const;
    [[nodiscard]] const wxStaticText* text() const;

private:
    void bindEvents();

    const int8_t mNumBits;
    wxTextCtrl* mEntry{nullptr};
    wxStaticText* mText{nullptr};
};

} // namespace PCUI
