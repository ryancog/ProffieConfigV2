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

#include <wx/button.h>
#include <wx/event.h>
#include <wx/graphics.h>
#include <wx/window.h>

#include "styles/bladestyle.h"
#include "ui/movable.h"

namespace PCUI {

wxDECLARE_EVENT(SB_COLLAPSED, wxCommandEvent);

class StyleBlock : public Movable {
public:
    /**
     * Creating a StyleBlock w/ a style will bind the style to
     * the block, and the style will be destroyed when the block
     * is.
     *
     * Additionally, if the style has children parameters, they will be
     * recursively bound to child blocks of the newly-created block.
     */
    StyleBlock(MoveArea *, wxWindow *, BladeStyles::BladeStyle *);
    StyleBlock(const StyleBlock &) = delete;
    StyleBlock(StyleBlock &&) = delete;
    StyleBlock &operator=(const StyleBlock &) = delete;
    StyleBlock &operator=(StyleBlock &&) = delete;
    ~StyleBlock() override;

    [[nodiscard]] const BladeStyles::BladeStyle *getStyle() const;
    [[nodiscard]] std::optional<std::string> getString() const;

    void update(bool repaint = true);
    void recurseUpdate(bool repaint = true);

    void collapse(bool = true);
    [[nodiscard]] bool isCollapsed() const;
    void setScale(float);

    /**
     * Takes in a point relative to the block.
     *
     * Returns true if the point is on the block and isn't covered
     * by a parameter block, false otherwise.
     */
    [[nodiscard]] bool hitTest(wxPoint) const override;

    const uint32_t type;
    [[nodiscard]] wxSize DoGetBestClientSize() const override { return mSize; }

    static const wxColour* textColor;
    static const wxColour* faceColor;
    static const wxColour* bgColor;
    static const wxColour* dimColor;
    static const wxColour* hlColor;

    static const wxColour* builtinColor;
    static const wxColour* function3DColor;
    static const wxColour* timeFuncColor;
    static const wxColour* wrapperColor;
    static const wxColour* transitionColor;
    static const wxColour* functionColor;
    static const wxColour* layerColor;
    static const wxColour* colorColor;
    static const wxColour* colorLayerColor;
    static const wxColour* effectColor;
    static const wxColour* lockupTypeColor;
    static const wxColour* argumentColor;


protected:
    void doOnGrab() override;

    /**
     * Takes in a Movable and a point which is the 
     * position (likely of the cursor) relative to the
     * screen.
     */
    bool tryAdopt(Movable*, wxPoint);
    BladeStyles::BladeStyle* const pStyle;
    static std::unordered_map<const BladeStyles::BladeStyle*, StyleBlock*> blockMap;

private:
    enum : int8_t {
        HELP,
        COLLAPSE,
        COPY,
        CUT,
    };

    static void initStatic();
    void initHelp();
    void bindEvents();
    void bindChildren();

    void paintEvent(wxPaintEvent&);
    void render(wxDC&);
    void calc();
    void updateElementPos();

    void showPopMenu(wxMouseEvent&);

    wxSize mSize{0, 0};

    struct ParamData {
        std::vector<const wxColour*> colors;
        wxPoint textPos;
        wxPoint rectPos;
        wxSize rectSize;
        
        wxWindow* control{nullptr};
    };
public: std::vector<ParamData> mParamsData;

    double mScale{1.F};
    template<typename T>
    void scaleValue(T& input) const { input = input * mScale; }
    template<typename T>
    void inverseScale(T& input) const { input = input / mScale; }

    template<typename T>
    [[nodiscard]] T getScaled(T input) const { return input * mScale; }
    template<typename T>
    [[nodiscard]] T getInverseScale(T input) const { return input / mScale; }

    // NOLINTBEGIN(readability-magic-numbers)
    [[nodiscard]] float rectangeRadius() const { return getScaled(5.F); }
    [[nodiscard]] int32_t edgePadding() const { return getScaled(10); }
    [[nodiscard]] int32_t borderThickness() const { return getScaled(4); }
    [[nodiscard]] int32_t internalPadding() const { return getScaled(5); }
    // NOLINTEND(readability-magic-numbers)


    wxString mName;

    const wxColour* mColor;
    wxPoint mHeaderTextPos;
    wxPoint mHeaderBarPos;
    wxSize mRectSize;
    wxPoint mHelpPos;
    wxButton* mHelpButton;

    bool mCollapsed{false};
};

} // namespace PCUI
