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

#include <iostream>
#include <optional>

#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/settings.h>
#include <wx/colour.h>
#include <wx/button.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/graphics.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>

#include "styleeditor/blocks/bitsctrl.h"
#include "styles/documentation/styledocs.h"
#include "styles/bladestyle.h"
#include "styles/elements/args.h"
#include "styles/elements/colors.h"
#include "styles/elements/effects.h"
#include "styles/elements/lockuptype.h"
#include "styles/parse.h"
#include "ui/bool.h"
#include "ui/numeric.h"

using namespace PCUI;

wxDEFINE_EVENT(PCUI::SB_COLLAPSED, wxCommandEvent);
std::unordered_map<const BladeStyles::BladeStyle*, StyleBlock*> StyleBlock::blockMap;

StyleBlock::StyleBlock(MoveArea* moveParent, wxWindow* parent, BladeStyles::BladeStyle* style) :
    Block(parent, style->getType()),
    Movable(moveParent),
    style(style),
    name(style->humanName) {

    SetName("StyleBlock");
    blockMap.emplace(style, this);

    routine = std::bind(&StyleBlock::tryAdopt, this, std::placeholders::_1, std::placeholders::_2);
    bindEvents();

    bindChildren();
    initHelp();
    update();
}

StyleBlock::~StyleBlock() {
    if (moveArea) moveArea->removeAdoptRoutine(this);
    blockMap.erase(blockMap.find(style));
}

void StyleBlock::bindEvents() {
    Block::Bind(wxEVT_PAINT, [&](wxPaintEvent& evt) { paintEvent(evt); });
    Block::Bind(wxEVT_MIDDLE_DOWN, [&](wxMouseEvent&) { collapse(!collapsed); });
    Block::Bind(wxEVT_RIGHT_DOWN, [&](wxMouseEvent& evt) { showPopMenu(evt); });
    Block::Bind(wxEVT_MENU, [&](wxCommandEvent&) { collapse(!collapsed); }, COLLAPSE);
    Block::Bind(wxEVT_MENU, [&](wxCommandEvent&) { 
        if (!wxClipboard::Get()->Open()) return;
        wxClipboard::Get()->SetData(new wxTextDataObject(getString().value_or("INVALID STYLE")));
        wxClipboard::Get()->Close();
    }, COPY);
    Block::Bind(wxEVT_MENU, [&](wxCommandEvent&) { 
        if (!wxClipboard::Get()->Open()) return;
        wxClipboard::Get()->SetData(new wxTextDataObject(getString().value_or("INVALID STYLE")));
        wxClipboard::Get()->Close();
        Destroy();
    }, CUT);
    Block::Bind(wxEVT_MENU, [&](wxCommandEvent&) { BladeStyles::Documentation::open(name.ToStdString()); }, HELP);
}

void StyleBlock::bindChildren() {
    for (const auto& param : style->getParams()) {
        if (!(param->getType() & BladeStyles::STYLETYPE)) continue;
        auto styleParam{static_cast<const BladeStyles::StyleParam*>(param)};
        auto style{styleParam->getStyle()};
        if (!style) continue;

        new StyleBlock(moveArea, this, const_cast<BladeStyles::BladeStyle*>(style));
    }
}

std::optional<std::string> StyleBlock::getString() const { 
    auto style{getStyle()};
    auto ret{BladeStyles::asString(*style)};
    delete style;
    return ret;
}

const BladeStyles::BladeStyle* StyleBlock::getStyle() const { return style; }
bool StyleBlock::isCollapsed() const { return collapsed; }

void StyleBlock::initHelp() {
    helpButton = new wxButton(this, HELP, " ? ", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    Bind(wxEVT_BUTTON, [&](wxCommandEvent&){
        BladeStyles::Documentation::open(name.ToStdString());
    });
}

void StyleBlock::doOnGrab() {
    auto parentBlock{dynamic_cast<StyleBlock*>(GetParent())};
    if (!parentBlock) return;

    for (auto param : parentBlock->style->getParams()) {
        if (!(param->getType() & BladeStyles::STYLETYPE)) continue;
        auto styleParam{static_cast<BladeStyles::StyleParam*>(param)};

        if (styleParam->getStyle() != style) continue;
        // We *are* the style, so we can ignore the return.
        (void)styleParam->detachStyle();
    }
}

bool StyleBlock::tryAdopt(Movable* window, wxPoint pos) {
        auto block{dynamic_cast<StyleBlock*>(window)};
        if (!block) return false;

        const BladeStyles::Param* paramToFill{nullptr};

        for (size_t i{0}; i < paramsData.size(); i++) {
            auto& paramData{paramsData.at(i)};
            auto param{style->getParam(i)};
            if (paramData.control) continue;

            auto typesEqual{param->getType() & block->type & BladeStyles::FLAGMASK};
            if (!typesEqual) continue;

            auto paramScreenLoc{GetScreenPosition() + paramData.rectPos};
            auto blockScreenEdge{GetScreenPosition() + GetSize()};

            if (
                    pos.x > paramScreenLoc.x &&
                    pos.x < blockScreenEdge.x &&
                    pos.y > paramScreenLoc.y &&
                    pos.y < paramScreenLoc.y + paramData.rectSize.y + (borderThickness * 2)
               ) {
                paramToFill = param;
                break;
            }
        }

        if (!paramToFill) return false;
        auto styleParam{static_cast<const BladeStyles::StyleParam*>(paramToFill)};

        auto paramStyle{styleParam->getStyle()};
        if (paramStyle) return blockMap.find(paramStyle)->second->tryAdopt(window, pos);    

        const_cast<BladeStyles::StyleParam*>(styleParam)->setStyle(const_cast<BladeStyles::BladeStyle*>(block->getStyle()));
        block->Reparent(this);

        update();
        
        return true;
}

bool StyleBlock::hitTest(wxPoint point) const {
    if (point.x < 0 || point.y < 0) return false;
    if (point.x > size.x || point.y > size.y) return false;

    for (const auto& param : paramsData) {
        if (
                point.x > param.rectPos.x && 
                point.x < param.rectPos.x + param.rectSize.x &&
                point.y > param.rectPos.y &&
                point.y < param.rectPos.y + param.rectSize.y
           ) {
            return false;
        }
    }

    return true;
}

void StyleBlock::update(bool repaint) {
    calc();
    if (repaint) paintNow();
    updateElementPos();
    auto sizeToSet{collapsed ? rectSize : size};
    Block::SetSize(sizeToSet);
    Block::SetMinSize(sizeToSet);
}

void StyleBlock::recurseUpdate(bool repaint) {
    update(repaint);

    auto styleBlockParent{dynamic_cast<StyleBlock*>(GetParent())};
    if (!styleBlockParent) {
        GetParent()->Layout();
        return;
    }

    styleBlockParent->recurseUpdate(repaint);
}

void StyleBlock::collapse(bool collapse) {
    if (paramsData.size() == 0) return;
    if (collapse == collapsed) return;

    collapsed = collapse;
    recurseUpdate();

    wxPostEvent(GetEventHandler(), wxCommandEvent(SB_COLLAPSED));
}

void StyleBlock::showPopMenu(wxMouseEvent& evt) {
    wxMenu menu;

    menu.Append(COPY, "Copy");
    menu.Append(CUT, "Cut");
    menu.AppendSeparator();
    menu.Append(COLLAPSE, collapsed ? "Expand" : "Collapse");
    menu.AppendSeparator();
    menu.Append(HELP, "View StyleDoc");

    PopupMenu(&menu, evt.GetPosition());
}

void StyleBlock::calc() {
    wxPoint drawLocation{edgePadding, edgePadding};
    size.Set(drawLocation.x, drawLocation.y);
    rectSize.Set(-1, -1);

    switch (type & BladeStyles::FLAGMASK) {
        case BladeStyles::BUILTIN:
        case BladeStyles::WRAPPER:
            color = wrapperColor;
            break;
        case BladeStyles::COLOR:
            color = colorColor;
            break;
        case BladeStyles::LAYER:
            color = layerColor;
            break;
        case BladeStyles::FUNCTION:
            color = functionColor;
            break;
        case BladeStyles::TRANSITION:
            color = transitionColor;
            break;
        case BladeStyles::EFFECT:
            color = effectColor;
            break;
        default:
            color = nullptr;
    }

    auto dc{wxClientDC(this)};
    auto headerTextSize{dc.GetTextExtent(name)};
    headerTextPos = drawLocation;
    drawLocation.y += headerTextSize.y;
    if (drawLocation.y > size.y) size.y = drawLocation.y;
    if (drawLocation.x + headerTextSize.x > size.x) size.x = drawLocation.x + headerTextSize.x;
    if (drawLocation.x + headerTextSize.x > rectSize.x) rectSize.x = drawLocation.x + headerTextSize.x;

    auto& params{style->getParams()};
    if (params.size() > 0) {
        drawLocation.y += internalPadding;
        headerBarPos = drawLocation;
        drawLocation.y += internalPadding;
        if (drawLocation.y > size.y) size.y = drawLocation.y;

        drawLocation.x += edgePadding * 2;
        if (drawLocation.x > size.x) size.x = drawLocation.x;
    }

    paramsData.resize(params.size());
    for (size_t i{0}; i < params.size(); i++) {
        using namespace BladeStyles;

        auto param{params.at(i)};
        auto& data{paramsData.at(i)};
        data.colors.clear();

        auto paramType{param->getType()};
        auto paramStyle{static_cast<StyleParam*>(param)->getStyle()};
        if ((paramType & STYLETYPE) && paramStyle) {
            data.rectSize = blockMap.find(paramStyle)->second->GetSize();
        } else if (data.control) {
            data.rectSize = data.control->GetBestSize();
        } else {
            data.rectSize.x = -1;
            data.rectSize.y = 30;
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
            auto numberParam{static_cast<NumberParam*>(param)};
            if (!data.control) data.control = new PCUI::Numeric(this, wxID_ANY, wxEmptyString, wxDefaultSize, 0, -32768, 32768, numberParam->getNum());
		} if (paramType & BITS) {
            auto bitsParam{static_cast<BitsParam*>(param)};
            if (!data.control) data.control = new PCUI::BitsCtrl(this, wxID_ANY, 16, bitsParam->getBits(), wxEmptyString, wxDefaultSize);
		} if (paramType & BOOL) {
            auto boolParam{static_cast<BoolParam*>(param)};
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
        drawLocation.y += internalPadding;
        auto argTextSize{dc.GetTextExtent(param->name)};
        drawLocation.y += argTextSize.y;
        if (drawLocation.x + argTextSize.x > size.x) size.x = drawLocation.x + argTextSize.x;
        if (drawLocation.x + argTextSize.x > rectSize.x) rectSize.x = drawLocation.x + argTextSize.x;

        data.rectPos = drawLocation;
        drawLocation.y += data.rectSize.y;
        if (drawLocation.x + data.rectSize.x > size.x){
            if (paramStyle && rectSize.x == -1) {
                // Freeze smaller size
                rectSize.x = size.x;
            }

            size.x = drawLocation.x + data.rectSize.x;
        }
        if (data.control && drawLocation.x + data.rectSize.x > rectSize.x) {
            rectSize.x = drawLocation.x + data.rectSize.x;
        }
        if (paramType & BladeStyles::FUNCTION) {
            drawLocation.y += borderThickness;
        }
        drawLocation.y += internalPadding * 2;

        if (paramType & BladeStyles::REFMASK) {
            const auto refNum{(paramType & BladeStyles::REFMASK) >> 16};
            auto refTextSize{dc.GetTextExtent(params.at(refNum - 1)->name)};
            if (drawLocation.x + refTextSize.x + (internalPadding * 2) > size.x) size.x = drawLocation.x + refTextSize.x + (internalPadding * 2);
        }
    }
    if (drawLocation.y > size.y) size.y = drawLocation.y;

    if (rectSize.x == -1) rectSize.x = size.x;
    if (rectSize.y == -1) rectSize.y = size.y;

    auto helpButtonSize{helpButton->GetSize()};
    auto hbNeededWidth{headerTextPos.x + headerTextSize.x + internalPadding + helpButtonSize.x};
    if (hbNeededWidth > size.x) size.x = hbNeededWidth;
    if (hbNeededWidth > rectSize.x) rectSize.x = hbNeededWidth;

    helpPos.x = rectSize.x - helpButtonSize.x;

    if (collapsed) {
        size.y = rectSize.y = headerTextPos.y + headerTextSize.y + internalPadding; 
    }

    rectSize.x += edgePadding;
    rectSize.y += edgePadding;

    if (paramsData.size() > 0) helpPos.y = (headerBarPos.y - helpButtonSize.y) / 2;
    else helpPos.y = (rectSize.y - helpButtonSize.y) / 2;

    if (size.x < rectSize.x) size.x = rectSize.x;
    if (size.y < rectSize.y) size.y = rectSize.y;
    size.x += borderThickness; // For block borders
}

void StyleBlock::updateElementPos() {
    helpButton->SetPosition(helpPos);
    for (size_t i{0}; i < paramsData.size(); i++) {
        auto& paramData{paramsData.at(i)};
        auto param{style->getParam(i)};

        switch (param->getType() & BladeStyles::FLAGMASK) {
            case BladeStyles::BITS:
            case BladeStyles::NUMBER:
            case BladeStyles::BOOL: {
                paramData.control->SetPosition(paramData.rectPos);
                paramData.control->SetSize(paramData.rectSize);
                break; }
            default:
                auto style{static_cast<const BladeStyles::StyleParam*>(param)->getStyle()};
                auto block{blockMap.find(style)};
                if (block == blockMap.end()) continue;

                block->second->SetPosition({
                        paramData.rectPos.x + borderThickness,
                        paramData.rectPos.y + borderThickness,
                        });
                break;
        };
    }
}

void StyleBlock::render(wxDC& dc) {
    dc.SetBrush(wxBrush(*faceColor));

    if (color) {
        dc.SetPen(wxPen(color->ChangeLightness(130), 1));
        dc.SetBrush(wxBrush(*color));
    }

    dc.SetClippingRegion(0, 0, rectSize.x, rectSize.y);
    dc.DrawRoundedRectangle(0, 0, rectSize.x, rectSize.y, 5);

    dc.SetTextForeground(*textColor);
    dc.DrawText(name, headerTextPos.x, headerTextPos.y);

    if (paramsData.size() > 0) {
        dc.SetPen(*textColor);
        dc.DrawLine(headerBarPos.x, headerBarPos.y, rectSize.x - edgePadding, headerBarPos.y);
    }

    if (collapsed) return;

    auto& params{style->getParams()};
    for (size_t i{0}; i < paramsData.size(); i++) {
        auto param{params.at(i)};
        auto& data{paramsData.at(i)};
        dc.SetTextForeground(*textColor);
        dc.DrawText(param->name, data.textPos.x, data.textPos.y);

        if (!(param->getType() & BladeStyles::STYLETYPE)) continue;

        auto blockBoxSize{data.rectSize};
        blockBoxSize.x = size.x;

        const wxColour* paramColor{data.colors.size() ? data.colors.at(0) : nullptr};
        if (color && paramColor) {
            dc.SetBrush(*paramColor);
            dc.SetPen(color->ChangeLightness(80));
        } else {
            dc.SetBrush(*dimColor);
            dc.SetPen(*dimColor);
        }
        dc.DrawRoundedRectangle(
                                data.rectPos.x,
                                data.rectPos.y,
                                blockBoxSize.x + (2 * borderThickness),
                                blockBoxSize.y + (2 * borderThickness),
                                6);

        if (paramColor) {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(paramColor->ChangeLightness(130));
        } else {
            dc.SetPen(*dimColor);
            dc.SetBrush(*dimColor);
        }
        dc.DrawRoundedRectangle(data.rectPos.x + borderThickness, data.rectPos.y + borderThickness, blockBoxSize.x, blockBoxSize.y, 5);


        if (param->getType() & BladeStyles::REFMASK) {
            const auto refNum{(param->getType() & BladeStyles::REFMASK) >> 16};
            const auto yOffset{(data.rectSize.y - dc.GetCharHeight()) / 2};
            dc.SetTextForeground(*dimColor);
            dc.DrawText(params.at(refNum - 1)->name, data.rectPos.x + borderThickness + internalPadding, data.rectPos.y + borderThickness + yOffset);
        }
    }
}

void StyleBlock::paintEvent(wxPaintEvent& evt) {
    Block::paintEvent(evt);
    update(false);
}
