#include "frame.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * ui/frame.cpp
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

#include <wx/sizer.h>

#include "../resources/icons/icon.xpm"

using namespace PCUI;

#include <iostream>

Frame::Frame(wxWindow* parent,
             wxWindowID winID,
             const wxString& title,
             const wxPoint& pos,
             const wxSize& size,
             int64_t style,
             const wxString& name) {
#	ifdef __WINDOWS__
    // gtk_widget_get_window(GetHandle());
    // auto var{this->GetGetHandle()};
    // DwmSetWindowAttribute(this->GTKGetDrawingWindow()->);
    SetIcon(wxICON(IDI_ICON1));
#	endif
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    Create(parent, winID, title, pos, size, style, name);
    SetDoubleBuffered(true);
}

Frame::~Frame() {
    if (mReference && *mReference) (*mReference) = nullptr;
}

void Frame::setReference(Frame** ref) {
    mReference = ref;
}
