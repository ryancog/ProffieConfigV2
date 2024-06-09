#pragma once
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * styleeditor/blocks/styleblock.h
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

#include <optional>

#include <wx/window.h>
#include <wx/graphics.h>
#include <wx/event.h>
#include <wx/button.h>

#include "styles/bladestyle.h"
#include "styleeditor/blocks/block.h"
#include "ui/movable.h"

namespace PCUI {

wxDECLARE_EVENT(SB_COLLAPSED, wxCommandEvent);

class StyleBlock : public Block, public Movable {
public:
    /**
     * Creating a StyleBlock w/ a style will bind the style to
     * the block, and the style will be destroyed when the block
     * is.
     *
     * Additionally, if the style has children parameters, they will be
     * recursively bound to child blocks of the newly-created block.
     */
    StyleBlock(MoveArea*, wxWindow*, BladeStyles::BladeStyle*);
    ~StyleBlock();

    const BladeStyles::BladeStyle* getStyle() const;
    std::optional<std::string> getString() const;

    void update(bool repaint = true);
    void recurseUpdate(bool repaint = true);

    void collapse(bool = true);
    bool isCollapsed() const;

    /**
     * Takes in a point relative to the block.
     *
     * Returns true if the point is on the block and isn't covered
     * by a parameter block, false otherwise.
     */
    virtual bool hitTest(wxPoint) const override;

protected:
    virtual void doOnGrab() override;

    /**
     * Takes in a Movable and a point which is the 
     * position (likely of the cursor) relative to the
     * screen.
     */
    bool tryAdopt(Movable*, wxPoint);
    BladeStyles::BladeStyle* const style;
    static std::unordered_map<const BladeStyles::BladeStyle*, StyleBlock*> blockMap;

private:
    enum {
        HELP,
        COLLAPSE,
        COPY,
        CUT,
    };

    void initHelp();
    void bindEvents();
    void bindChildren();

    virtual void paintEvent(wxPaintEvent&) override;
    virtual void render(wxDC&) override;
    void calc();
    void updateElementPos();

    void showPopMenu(wxMouseEvent&);


    struct ParamData {
        std::vector<const wxColour*> colors;
        wxPoint textPos;
        wxPoint rectPos;
        wxSize rectSize;
        
        wxWindow* control{nullptr};
    };
    std::vector<ParamData> paramsData;


    wxString name;

    const wxColour* color;
    wxPoint headerTextPos;
    wxPoint headerBarPos;
    wxSize rectSize;
    wxPoint helpPos;
    wxButton* helpButton;

    bool collapsed{false};
};

}
