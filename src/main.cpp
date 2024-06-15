/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * main.cpp
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

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/settings.h>

#include "appcore/state.h"
#include "styleeditor/styleeditor.h"

#include "test/uitest.h"

class ProffieConfig : public wxApp {
public:
    void setupTheme() {
#	ifdef __WXMSW__
#	elif defined(__WXGTK__) && defined(__WINDOWS__)
        putenv(std::string("GTK_CSD=0").data());
        putenv(std::string(R"(GTK_THEME=../../resources/Orchis-Red-Dark-Compact)").data());
#	endif
    }

    bool OnInit() override {
#   	ifdef __WINDOWS__
        if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()){
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            freopen("CONIN$", "r", stdin);
            std::ios::sync_with_stdio();
        }
#		endif
#       ifdef __WXGTK__
        GTKSuppressDiagnostics();
#       endif
        setupTheme();

        chdir(argv[0].BeforeLast('/'));

        AppCore::init();

        // // auto config{Config::readConfig("problemConfig.h")};
        // // Config::writeConfig("test/problemConfig.h", *config);

        // std::unique_ptr<BladeStyles::BladeStyle> style{BladeStyles::parseString(
        //         "// This is a line comment, lol!\n"
        //         "/* This is a block comment\n"
        //         "that spans multiple lines\n"
        //         "and is kinda scary\n"
        //         "*/\n"
        //         "StylePtr< /* This comment is for the Cylon */ Cylon < /* And for the AFlicker*/AudioFlicker<Blue, Black>, 20, 20, Green, 10, 200, 1000, DarkOrange>>()"
        //         )};
        // auto styleStr{BladeStyles::asString(*style)};
        // Logger::verbose(styleStr.value());

        Test::run();
        StyleEditor::launch(nullptr);

        return true;
    }
};

wxIMPLEMENT_APP(ProffieConfig); // NOLINT

