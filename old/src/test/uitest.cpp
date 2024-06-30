#include "uitest.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * test/uitest.cpp
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

#include <wx/button.h>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/wrapsizer.h>

#include "ui/frame.h"
#include "ui/movable.h"

namespace Test {

class MoveButton : public PCUI::Movable {
public:
    MoveButton(PCUI::MovePanel *parent, PCUI::MoveArea *moveArea, const wxString& title) :
        PCUI::Movable(moveArea) {
        Create(parent, wxID_ANY);

        auto sizer{new wxBoxSizer(wxVERTICAL)};
        auto text{new wxStaticText(this, wxID_ANY, title)};
        sizer->AddSpacer(20);
        sizer->Add(text);
        sizer->AddSpacer(20);
        SetSizer(sizer);
    }
};

void run() {
    auto *frame{new PCUI::Frame(nullptr, wxID_ANY, "TestFrame")};
    auto *frameSizer{new wxBoxSizer(wxVERTICAL)};
    auto *moveArea{new PCUI::MoveArea(frame, wxID_ANY)};
    auto *areaSizer{new wxBoxSizer(wxVERTICAL)};
    auto *movePanel{new PCUI::MovePanel(moveArea, moveArea, wxID_ANY)};
    auto *panelSizer{new wxWrapSizer(wxHORIZONTAL)};

    for (auto i{0}; i < 250; i++) {
        panelSizer->Add(new MoveButton(movePanel, moveArea, "B"));
    }

    movePanel->SetSizer(panelSizer);
    areaSizer->Add(movePanel, wxSizerFlags(1).Expand());
    moveArea->SetSizer(areaSizer);
    frameSizer->Add(moveArea, wxSizerFlags(1).Expand());
    frame->SetSizer(frameSizer);
    frame->Show();
}

} // namespace Test

