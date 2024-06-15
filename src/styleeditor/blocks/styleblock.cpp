#include "styleblock.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * styleeditor/blocks/styleblock.cpp
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
#include <wx/clipbrd.h>
#include <wx/colour.h>
#include <wx/dataobj.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/graphics.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/string.h>

#include "log/logger.h"
#include "styleeditor/blocks/bitsctrl.h"
#include "styles/bladestyle.h"
#include "styles/documentation/styledocs.h"
#include "styles/parse.h"
#include "ui/bool.h"
#include "ui/numeric.h"

namespace PCUI {

wxDEFINE_EVENT(SB_COLLAPSED, wxCommandEvent);

std::unordered_map<const BladeStyles::BladeStyle*, StyleBlock*> StyleBlock::blockMap;

const wxColour* StyleBlock::textColor;
const wxColour* StyleBlock::faceColor;
const wxColour* StyleBlock::bgColor;
const wxColour* StyleBlock::dimColor;
const wxColour* StyleBlock::hlColor;

const wxColour* StyleBlock::builtinColor;
const wxColour* StyleBlock::function3DColor;
const wxColour* StyleBlock::timeFuncColor;
const wxColour* StyleBlock::wrapperColor;
const wxColour* StyleBlock::transitionColor;
const wxColour* StyleBlock::functionColor;
const wxColour* StyleBlock::colorColor;
const wxColour* StyleBlock::layerColor;
const wxColour* StyleBlock::colorLayerColor;
const wxColour* StyleBlock::effectColor;
const wxColour* StyleBlock::lockupTypeColor;
const wxColour* StyleBlock::argumentColor;

void StyleBlock::initStatic() {
    static bool initialized{false};
    if (initialized) return;

    // NOLINTBEGIN(readability-magic-numbers)
    // textColor = new wxColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    textColor = new wxColour(255, 255, 255);
    faceColor = new wxColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    bgColor   = new wxColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    dimColor  = new wxColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    hlColor   = new wxColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

    builtinColor    = new wxColour(144, 129, 114); // Grey 
    function3DColor = new wxColour( 50, 116,  63); // Green
    timeFuncColor   = new wxColour( 50, 116,  63); // Green
    wrapperColor    = new wxColour(144, 129, 114); // Grey
    transitionColor = new wxColour( 47, 101, 179); // Blue
    functionColor   = new wxColour( 50, 116,  63); // Green
    colorColor      = new wxColour(217,  64,  64); // Red
    layerColor      = new wxColour(214, 102,   0); // Orange
    colorLayerColor = new wxColour(170,  56,   0);
    effectColor     = new wxColour(209, 132, 153); // Pink
    lockupTypeColor = new wxColour(214,  53,  97); // Different Pink
    argumentColor   = new wxColour(101,  45, 144); // Deep Purple
    // NOLINTEND(readability-magic-numbers)
    initialized = true;
}

StyleBlock::StyleBlock(MoveArea* moveParent, wxWindow* parent, BladeStyles::BladeStyle* style) :
    Movable(moveParent),
    type(style->getType()),
    pStyle(style),
    mName(style->humanName) {
    initStatic();

#	ifndef __WXGTK__
    // current wx build complains about GDK compositing with this.
    // Required on Win32. I think it's responsible for eliminating
    // the flickers, since what it's really doing is enabling compositing.
    SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
# 	endif

    Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, "StyleBlock");

    SetName("StyleBlock");
    blockMap.emplace(style, this);

    pRoutine = [this](auto&& PH1, auto&& PH2) { return tryAdopt(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); };
    bindEvents();

    bindChildren();
    initHelp();
    update();
}

StyleBlock::~StyleBlock() {
    if (pMoveArea) pMoveArea->removeAdoptRoutine(this);
    blockMap.erase(blockMap.find(pStyle));
}

void StyleBlock::bindEvents() {
    Bind(wxEVT_PAINT, [&](wxPaintEvent& evt) { paintEvent(evt); });
    Bind(wxEVT_MIDDLE_DOWN, [&](wxMouseEvent&) { collapse(!mCollapsed); });
    Bind(wxEVT_RIGHT_DOWN, [&](wxMouseEvent& evt) { showPopMenu(evt); });
    Bind(wxEVT_MENU, [&](wxCommandEvent&) { collapse(!mCollapsed); }, COLLAPSE);
    Bind(wxEVT_MENU, [&](wxCommandEvent&) { 
        if (!wxClipboard::Get()->Open()) return;
        wxClipboard::Get()->SetData(new wxTextDataObject(getString().value_or("INVALID STYLE")));
        wxClipboard::Get()->Close();
    }, COPY);
    Bind(wxEVT_MENU, [&](wxCommandEvent&) { 
        if (!wxClipboard::Get()->Open()) return;
        wxClipboard::Get()->SetData(new wxTextDataObject(getString().value_or("INVALID STYLE")));
        wxClipboard::Get()->Close();
        Destroy();
    }, CUT);
    Bind(wxEVT_MENU, [&](wxCommandEvent&) { BladeStyles::Documentation::open(mName.ToStdString()); }, HELP);
}

void StyleBlock::bindChildren() {
    for (const auto& param : pStyle->getParams()) {
        if (!(param->getType() & BladeStyles::STYLETYPE)) continue;
        const auto *styleParam{static_cast<const BladeStyles::StyleParam*>(param)};
        const auto *style{styleParam->getStyle()};
        if (!style) continue;

        new StyleBlock(pMoveArea, this, const_cast<BladeStyles::BladeStyle*>(style));
    }
}

std::optional<std::string> StyleBlock::getString() const { 
    const auto *style{getStyle()};
    auto ret{BladeStyles::asString(*style)};
    delete style;
    return ret;
}

const BladeStyles::BladeStyle* StyleBlock::getStyle() const { 
    return pStyle; 
}
bool StyleBlock::isCollapsed() const { return mCollapsed; }

void StyleBlock::initHelp() {
    mHelpButton = new wxButton(this, HELP, " ? ", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    Bind(wxEVT_BUTTON, [&](wxCommandEvent&){
        BladeStyles::Documentation::open(mName.ToStdString());
    });
}

void StyleBlock::doOnGrab() {
    auto *parentBlock{dynamic_cast<StyleBlock*>(GetParent())};
    if (!parentBlock) return;

    for (auto *param : parentBlock->pStyle->getParams()) {
        if (!(param->getType() & BladeStyles::STYLETYPE)) continue;
        auto *styleParam{static_cast<BladeStyles::StyleParam*>(param)};

        if (styleParam->getStyle() != pStyle) continue;
        // We *are* the style, so we can ignore the return.
        (void)styleParam->detachStyle();
    }
}

bool StyleBlock::tryAdopt(Movable* window, wxPoint pos) {
        auto *block{dynamic_cast<StyleBlock*>(window)};
        if (!block) return false;

        const BladeStyles::Param* paramToFill{nullptr};

        for (size_t i{0}; i < mParamsData.size(); i++) {
            auto& paramData{mParamsData.at(i)};
            const auto *param{pStyle->getParam(i)};
            if (paramData.control) continue;

            auto typesEqual{param->getType() & block->type & BladeStyles::FLAGMASK};
            if (!typesEqual) continue;

            auto paramScreenLoc{GetScreenPosition() + paramData.rectPos};
            auto blockScreenEdge{GetScreenPosition() + GetSize()};

            if (
                    pos.x > paramScreenLoc.x &&
                    pos.x < blockScreenEdge.x &&
                    pos.y > paramScreenLoc.y &&
                    pos.y < paramScreenLoc.y + paramData.rectSize.y + (borderThickness() * 2)
               ) {
                paramToFill = param;
                break;
            }
        }

        if (!paramToFill) return false;
        const auto *styleParam{static_cast<const BladeStyles::StyleParam*>(paramToFill)};

        const auto *paramStyle{styleParam->getStyle()};
        if (paramStyle) return blockMap.find(paramStyle)->second->tryAdopt(window, pos);    

        const_cast<BladeStyles::StyleParam*>(styleParam)->setStyle(const_cast<BladeStyles::BladeStyle*>(block->getStyle()));
        block->Reparent(this);

        update();
        
        return true;
}

bool StyleBlock::hitTest(wxPoint point) const {
    if (point.x < 0 || point.y < 0) return false;
    if (point.x > mRectSize.x || point.y > mRectSize.y) return false;

    for (const auto& param : mParamsData) {
        if (
                point.x > param.rectPos.x && 
                point.x < param.rectPos.x + param.rectSize.x &&
                point.y > param.rectPos.y &&
                point.y < param.rectPos.y + param.rectSize.y
           ) return false;
    }

    return true;
}

void StyleBlock::update(bool repaint) {
    calc();
    if (repaint) Update();
    updateElementPos();
}

void StyleBlock::recurseUpdate(bool repaint) {
    update(repaint);

    auto *styleBlockParent{dynamic_cast<StyleBlock*>(GetParent())};
    if (!styleBlockParent) {
        GetParent()->Layout();
        return;
    }

    styleBlockParent->recurseUpdate(repaint);
}

void StyleBlock::collapse(bool collapse) {
    if (mParamsData.empty()) return;
    if (collapse == mCollapsed) return;

    mCollapsed = collapse;
    recurseUpdate();

    wxPostEvent(GetEventHandler(), wxCommandEvent(SB_COLLAPSED));
}

void StyleBlock::setScale(float val) { 
    mScale = val; 
    for (auto *const param : pStyle->getParams()) {
        if (!(param->getType() & BladeStyles::STYLETYPE)) continue;

        auto blockIt{blockMap.find(static_cast<BladeStyles::StyleParam*>(param)->getStyle())};
        if (blockIt == blockMap.end()) continue;
        blockIt->second->setScale(val);
    }
}

void StyleBlock::showPopMenu(wxMouseEvent& evt) {
    wxMenu menu;

    menu.Append(COPY, "Copy");
    menu.Append(CUT, "Cut");
    menu.AppendSeparator();
    menu.Append(COLLAPSE, mCollapsed ? "Expand" : "Collapse");
    menu.AppendSeparator();
    menu.Append(HELP, "View StyleDoc");

    PopupMenu(&menu, evt.GetPosition());
}

void StyleBlock::calc() {
    constexpr auto EMPTY_RECT_HEIGHT{30};
    wxPoint drawLocation{edgePadding(), edgePadding()};
    mSize.Set(drawLocation.x, drawLocation.y);
    mRectSize.Set(-1, -1);

    switch (type & BladeStyles::FLAGMASK) {
        case BladeStyles::BUILTIN:
        case BladeStyles::WRAPPER:
            mColor = wrapperColor;
            break;
        case BladeStyles::COLOR:
            mColor = colorColor;
            break;
        case BladeStyles::LAYER:
            mColor = layerColor;
            break;
        case BladeStyles::FUNCTION:
            mColor = functionColor;
            break;
        case BladeStyles::TRANSITION:
            mColor = transitionColor;
            break;
        case BladeStyles::EFFECT:
            mColor = effectColor;
            break;
        default:
            mColor = nullptr;
    }

    auto clientDC{wxClientDC(this)};
    auto headerTextSize{clientDC.GetTextExtent(mName)};
    mHeaderTextPos = drawLocation;
    drawLocation.y += headerTextSize.y;
    if (drawLocation.y > mSize.y) mSize.y = drawLocation.y;
    if (drawLocation.x + headerTextSize.x > mSize.x) mSize.x = drawLocation.x + headerTextSize.x;
    if (drawLocation.x + headerTextSize.x > mRectSize.x) mRectSize.x = drawLocation.x + headerTextSize.x;

    const auto& params{pStyle->getParams()};
    if (!params.empty()) {
        drawLocation.y += internalPadding();
        mHeaderBarPos = drawLocation;
        drawLocation.y += internalPadding();
        if (drawLocation.y > mSize.y) mSize.y = drawLocation.y;

        drawLocation.x += edgePadding() * 2;
        if (drawLocation.x > mSize.x) mSize.x = drawLocation.x;
    }

    mParamsData.resize(params.size());
    for (size_t i{0}; i < params.size(); i++) {
        using namespace BladeStyles;

        auto *param{params.at(i)};
        auto& data{mParamsData.at(i)};
        data.colors.clear();

        auto paramType{param->getType()};
        const auto *paramStyle{static_cast<StyleParam*>(param)->getStyle()};
        if ((paramType & STYLETYPE) && paramStyle) {
            data.rectSize = getInverseScale(blockMap.find(paramStyle)->second->GetSize());
        } else if (data.control) {
            data.rectSize = data.control->GetBestSize();
        } else {
            data.rectSize.x = -1;
            data.rectSize.y = EMPTY_RECT_HEIGHT;
        }

        if (paramType & WRAPPER) {
            data.colors.push_back(wrapperColor);
        } if (paramType & BUILTIN) {
            data.colors.push_back(builtinColor);
        } if (paramType & FUNCTION) {
            data.colors.push_back(functionColor);
        } if (paramType & FUNCTION3D) {
            data.colors.push_back(function3DColor);
		} if (paramType & NUMBER) {
            auto *numberParam{static_cast<NumberParam*>(param)};
            if (!data.control) data.control = new PCUI::Numeric(this, wxID_ANY, wxEmptyString, wxDefaultSize, 0, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max() + 1, numberParam->getNum());
		} if (paramType & BITS) {
            auto *bitsParam{static_cast<BitsParam*>(param)};
            if (!data.control) data.control = new PCUI::BitsCtrl(this, wxID_ANY, sizeof(int16_t) * CHAR_BIT, bitsParam->getBits(), wxEmptyString, wxDefaultSize);
		} if (paramType & BOOL) {
            auto *boolParam{static_cast<BoolParam*>(param)};
            if (!data.control) data.control = new PCUI::Bool(this, wxID_ANY, boolParam->getBool());
		} if (paramType & COLOR) {
            data.colors.push_back(colorColor);
		} if (paramType & LAYER) {
            data.colors.push_back(layerColor);
		} if (paramType & TRANSITION) {
            data.colors.push_back(transitionColor);
        } if (paramType & TIMEFUNC) {
            data.colors.push_back(timeFuncColor);
		} if (paramType & EFFECT) {
            data.colors.push_back(effectColor);
		} if (paramType & LOCKUPTYPE) {
            data.colors.push_back(lockupTypeColor);
		} if (paramType & ARGUMENT) {
            data.colors.push_back(argumentColor);
        }

        data.textPos = drawLocation;
        drawLocation.y += internalPadding();
        auto argTextSize{clientDC.GetTextExtent(param->name)};
        drawLocation.y += argTextSize.y;
        if (drawLocation.x + argTextSize.x > mSize.x) mSize.x = drawLocation.x + argTextSize.x;
        if (drawLocation.x + argTextSize.x > mRectSize.x) mRectSize.x = drawLocation.x + argTextSize.x;

        data.rectPos = drawLocation;
        drawLocation.y += data.rectSize.y;
        if (drawLocation.x + data.rectSize.x > mSize.x){
            if (paramStyle && mRectSize.x == -1) {
                // Freeze smaller mSize
                mRectSize.x = mSize.x;
            }

            mSize.x = drawLocation.x + data.rectSize.x;
        }
        if (data.control && drawLocation.x + data.rectSize.x > mRectSize.x) {
            mRectSize.x = drawLocation.x + data.rectSize.x;
        }
        if (paramType & BladeStyles::STYLETYPE) {
            drawLocation.y += borderThickness() * 2;
        }
        drawLocation.y += internalPadding() * 2;

        if (paramType & BladeStyles::REFMASK) {
            const auto refNum{getRefFromType(paramType)};
            auto refTextSize{clientDC.GetTextExtent(params.at(refNum - 1)->name)};
            if (drawLocation.x + refTextSize.x + (internalPadding() * 2) > mSize.x) mSize.x = drawLocation.x + refTextSize.x + (internalPadding() * 2);
        }
    }
    if (drawLocation.y > mSize.y) mSize.y = drawLocation.y;

    if (mRectSize.x == -1) mRectSize.x = mSize.x;
    if (mRectSize.y == -1) mRectSize.y = mSize.y;

    auto helpButtonSize{mHelpButton->GetBestSize()};
    auto hbNeededWidth{mHeaderTextPos.x + headerTextSize.x + internalPadding() + helpButtonSize.x};
    if (hbNeededWidth > mSize.x) mSize.x = hbNeededWidth;
    if (hbNeededWidth > mRectSize.x) mRectSize.x = hbNeededWidth;

    mHelpPos.x = mRectSize.x - helpButtonSize.x;

    if (mCollapsed) {
        mSize.y = mRectSize.y = mHeaderTextPos.y + headerTextSize.y + internalPadding(); 
    }

    mRectSize.x += edgePadding();
    mRectSize.y += edgePadding();

    if (!mParamsData.empty()) mHelpPos.y = (mHeaderBarPos.y - helpButtonSize.y) / 2;
    else mHelpPos.y = (mRectSize.y - helpButtonSize.y) / 2;

    if (mSize.x < mRectSize.x) mSize.x = mRectSize.x;
    if (mSize.y < mRectSize.y) mSize.y = mRectSize.y;
    mSize.x += borderThickness(); // For block borders
    
    scaleValue(mHelpPos);
    mHelpButton->SetSize(getScaled(helpButtonSize));
    scaleValue(mSize);
    scaleValue(mRectSize);
    scaleValue(mHeaderBarPos);
    scaleValue(mHeaderTextPos);
    for (auto &param : mParamsData) {
        scaleValue(param.textPos);
        scaleValue(param.rectPos);

        scaleValue(param.rectSize.y);
        if (param.rectSize.x != -1) scaleValue(param.rectSize.x);
    }

    auto mSizeToSet{mCollapsed ? mRectSize : mSize};
    SetSize(mSizeToSet);
    SetMinSize(mSizeToSet);
}

void StyleBlock::updateElementPos() {
    auto font{wxSystemSettings::GetFont(wxSystemFont::wxSYS_DEFAULT_GUI_FONT)};
    font.SetPixelSize(getScaled(font.GetPixelSize()));
    mHelpButton->SetPosition(mHelpPos);
    mHelpButton->SetFont(font);
    for (size_t i{0}; i < mParamsData.size(); i++) {
        auto& paramData{mParamsData.at(i)};
        const auto *param{pStyle->getParam(i)};

        switch (param->getType() & BladeStyles::FLAGMASK) {
            case BladeStyles::BITS:
            case BladeStyles::NUMBER:
            case BladeStyles::BOOL: {
                paramData.control->SetPosition(paramData.rectPos);
                paramData.control->SetSize(paramData.rectSize);
                paramData.control->SetFont(font);
                break; }
            default:
                const auto *style{static_cast<const BladeStyles::StyleParam*>(param)->getStyle()};
                auto block{blockMap.find(style)};
                if (block == blockMap.end()) continue;

                block->second->SetPosition({
                        paramData.rectPos.x + borderThickness(),
                        paramData.rectPos.y + borderThickness(),
                        });
                break;
        };
    }
}

void StyleBlock::render(wxDC& devContext) {
    constexpr auto BORDER_LIGHT_DEC{80};
    constexpr auto BG_LIGHT_INCREASE{130};

    devContext.SetBrush(wxBrush(*faceColor));
    auto font{devContext.GetFont()};
    font.SetPixelSize(getScaled(font.GetPixelSize()));
    devContext.SetFont(font);

    if (mColor) {
        devContext.SetPen(wxPen(mColor->ChangeLightness(BG_LIGHT_INCREASE), 1));
        devContext.SetBrush(wxBrush(*mColor));
    }

    devContext.SetClippingRegion(0, 0, mRectSize.x, mRectSize.y);
    devContext.DrawRoundedRectangle(0, 0, mRectSize.x, mRectSize.y, rectangeRadius());

    devContext.SetTextForeground(*textColor);
    devContext.DrawText(mName, mHeaderTextPos.x, mHeaderTextPos.y);

    if (!mParamsData.empty()) {
        devContext.SetPen(*textColor);
        devContext.DrawLine(mHeaderBarPos.x, mHeaderBarPos.y, mRectSize.x - edgePadding(), mHeaderBarPos.y);
    }

    if (mCollapsed) return;

    const auto& params{pStyle->getParams()};
    for (size_t i{0}; i < mParamsData.size(); i++) {
        auto *param{params.at(i)};
        auto& data{mParamsData.at(i)};
        devContext.SetTextForeground(*textColor);
        devContext.DrawText(param->name, data.textPos.x, data.textPos.y);

        if (!(param->getType() & BladeStyles::STYLETYPE)) continue;

        auto blockBoxSize{data.rectSize};
        blockBoxSize.x = mSize.x;

        const wxColour* paramColor{!data.colors.empty() ? data.colors.at(0) : nullptr};
        if (mColor && paramColor) {
            devContext.SetBrush(*paramColor);
            devContext.SetPen(mColor->ChangeLightness(BORDER_LIGHT_DEC));
        } else {
            devContext.SetBrush(*dimColor);
            devContext.SetPen(*dimColor);
        }
        devContext.DrawRoundedRectangle(
                                data.rectPos.x,
                                data.rectPos.y,
                                blockBoxSize.x + (2 * borderThickness()),
                                blockBoxSize.y + (2 * borderThickness()),
                                rectangeRadius() + 1);

        if (paramColor) {
            devContext.SetPen(*wxTRANSPARENT_PEN);
            devContext.SetBrush(paramColor->ChangeLightness(BG_LIGHT_INCREASE));
        } else {
            devContext.SetPen(*dimColor);
            devContext.SetBrush(*dimColor);
        }
        devContext.DrawRoundedRectangle(data.rectPos.x + borderThickness(), data.rectPos.y + borderThickness(), blockBoxSize.x, blockBoxSize.y, rectangeRadius());

        if (param->getType() & BladeStyles::REFMASK) {
            const auto refNum{BladeStyles::getRefFromType(param->getType())};
            const auto yOffset{(data.rectSize.y - devContext.GetCharHeight()) / 2};
            devContext.SetTextForeground(*dimColor);
            devContext.DrawText(params.at(refNum - 1)->name, data.rectPos.x + borderThickness() + internalPadding(), data.rectPos.y + borderThickness() + yOffset);
        }
    }
}

void StyleBlock::paintEvent(wxPaintEvent&) {
    wxPaintDC paintDC(this);
    render(paintDC);
}

} // namespace PCUI
