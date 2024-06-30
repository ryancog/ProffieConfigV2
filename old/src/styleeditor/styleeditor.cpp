#include "styleeditor.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * styleeditor/styleeditor.cpp
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

#include <wx/clipbrd.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/tglbtn.h>
#include <wx/time.h>
#include <wx/timer.h>
#include <wx/window.h>
#include <wx/wrapsizer.h>

#include "appcore/interfaces.h"
#include "log/logger.h"
#include "styleeditor/blocks/styleblock.h"
#include "styles/bladestyle.h"
#include "styles/elements/colorstyles.h"
#include "styles/elements/functions.h"
#include "styles/elements/transitions.h"
#include "styles/parse.h"
#include "ui/frame.h"
#include "ui/movable.h"
#include "wx/dataobj.h"

namespace StyleEditor {

PCUI::Frame* frame{nullptr};

void createUI();
void createToolbox();
void bindEvents();
void updateVisibleBlocks();
void updateToolboxSize();
uint32_t filterType{BladeStyles::COLOR};

wxToggleButton* styleFilter;
wxToggleButton* layerFilter;
wxToggleButton* funcFilter;
wxToggleButton* transitionFilter;
wxToggleButton* effectFilter;
std::vector<PCUI::StyleBlock*>* toolboxBlocks;
wxScrolledCanvas* toolboxScroller{nullptr};
wxPanel* toolbox{nullptr};
PCUI::MoveArea* blockMoveArea{nullptr};
PCUI::ScrolledMovePanel* blockWorkspace{nullptr};
auto scale{1.F};

constexpr auto JOG_DISTANCE{20};
constexpr auto SCROLL_UNITS{JOG_DISTANCE / 2};

void launch(wxWindow* parent) {
    if (frame) {
        frame->Raise();
        return;
    }

    frame = new PCUI::Frame(parent, AppCore::Interface::STYLEMAN, "ProffieConfig Style Editor");
    frame->setReference(&frame);
    createUI();
    updateVisibleBlocks();
    bindEvents();

    frame->Show();
    frame->SetSize(600, 300); // NOLINT(readability-magic-numbers)
}

void createUI() {
    constexpr auto TOOLBOX_MIN_WIDTH{220};
    constexpr auto WORKSPACE_MIN_WIDTH{100};

    auto *frameSizer{new wxBoxSizer(wxVERTICAL)};
    blockMoveArea = new PCUI::MoveArea(frame, wxID_ANY);
    frameSizer->Add(blockMoveArea, wxSizerFlags(1).Expand());
    auto *moveAreaSizer{new wxBoxSizer(wxVERTICAL)};
    auto *splitter{new wxSplitterWindow(blockMoveArea, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_PERMIT_UNSPLIT)};
    moveAreaSizer->Add(splitter, wxSizerFlags(1).Expand());
    splitter->SetMinimumPaneSize(TOOLBOX_MIN_WIDTH);

    toolbox = new wxPanel(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER, "Toolbox");
    createToolbox();

    auto *workspaceSplitter{new wxSplitterWindow(splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE)};
    workspaceSplitter->SetMinimumPaneSize(WORKSPACE_MIN_WIDTH);
    auto preview = new wxPanel(workspaceSplitter, wxID_ANY);

    blockWorkspace = new PCUI::ScrolledMovePanel(workspaceSplitter, blockMoveArea, wxID_ANY);
    workspaceSplitter->SplitHorizontally(preview, blockWorkspace->getCanvas());
    splitter->SplitVertically(toolbox, workspaceSplitter);

    blockMoveArea->SetSizerAndFit(moveAreaSizer);
    frame->SetSizerAndFit(frameSizer);
}

void createToolbox() {
    constexpr auto PADDING{5};

    auto *padSizer{new wxBoxSizer(wxHORIZONTAL)};
    auto *sizer{new wxBoxSizer(wxVERTICAL)};
    padSizer->AddSpacer(PADDING);
    padSizer->Add(sizer, wxSizerFlags(1).Expand());
    padSizer->AddSpacer(PADDING);

    auto *blockSearch{new wxSearchCtrl(toolbox, wxID_ANY)};
    sizer->Add(blockSearch, wxSizerFlags(0).Expand().Border(wxTOP | wxBOTTOM, PADDING));

    auto *categorySizer{new wxWrapSizer(wxHORIZONTAL)};
    styleFilter = new wxToggleButton(toolbox, BladeStyles::COLOR, "Styles");
    styleFilter->SetValue(true);
    layerFilter = new wxToggleButton(toolbox, BladeStyles::LAYER, "Layers");
    funcFilter = new wxToggleButton(toolbox, BladeStyles::FUNCTION, "Functions");
    transitionFilter = new wxToggleButton(toolbox, BladeStyles::TRANSITION, "Transitions");
    effectFilter = new wxToggleButton(toolbox, BladeStyles::EFFECT, "Effects");
    categorySizer->Add(styleFilter, wxSizerFlags(0).Border(wxALL, 2));
    categorySizer->Add(layerFilter, wxSizerFlags(0).Border(wxALL, 2));
    categorySizer->Add(funcFilter, wxSizerFlags(0).Border(wxALL, 2));
    categorySizer->Add(transitionFilter, wxSizerFlags(0).Border(wxALL, 2));
    categorySizer->Add(effectFilter, wxSizerFlags(0).Border(wxALL, 2));

    sizer->Add(categorySizer);
    sizer->AddSpacer(PADDING * 2);

    toolboxScroller = new wxScrolledCanvas(toolbox, wxID_ANY);
    auto *scrollSizer{new wxBoxSizer(wxVERTICAL)};
    toolboxBlocks = new std::vector<PCUI::StyleBlock*>;

    auto addStyleBlock{[&scrollSizer](const BladeStyles::StyleGenerator& generator) {
        auto *style{generator({})};
        auto *block{new PCUI::StyleBlock(nullptr, toolboxScroller, style)};

        toolboxBlocks->push_back(block);
        block->collapse();
        block->Bind(PCUI::SB_COLLAPSED, [](wxCommandEvent&){ updateToolboxSize(); });
        block->Bind(wxEVT_LEFT_DOWN, [block](wxMouseEvent& evt){
            evt.Skip(false);
            if (!block->hitTest(evt.GetPosition())) return;

            auto *newBlock{new PCUI::StyleBlock(blockMoveArea, toolboxScroller, new BladeStyles::BladeStyle(*block->getStyle()))};

            newBlock->collapse(block->isCollapsed());
            newBlock->setScale(scale);
            newBlock->SetPosition(block->GetPosition());
            newBlock->doGrab(evt);
        });
        for (const auto& child : block->GetChildren()) {
            // Prevent moving children blocks
            child->Bind(wxEVT_LEFT_DOWN, [](wxMouseEvent& evt) { evt.Skip(false); });
            child->Disable();
        }
        scrollSizer->Add(block, wxSizerFlags(0).Border(wxALL, PADDING));
    }};

    for (const auto& [ _, generator ] : BladeStyles::ColorStyle::getMap()) addStyleBlock(generator);
    for (const auto& [ _, generator ] : BladeStyles::FunctionStyle::getMap()) addStyleBlock(generator);
    for (const auto& [ _, generator ] : BladeStyles::TransitionStyle::getMap()) addStyleBlock(generator);

    toolboxScroller->SetSizerAndFit(scrollSizer);
    toolboxScroller->SetMinSize({200, 200}); // NOLINT(readability-magic-numbers)
    sizer->Add(toolboxScroller, wxSizerFlags(1).Expand());

    toolbox->SetSizerAndFit(padSizer);
}

void updateToolboxSize() {
    toolboxScroller->GetSizer()->Layout();
    toolboxScroller->SetVirtualSize(toolboxScroller->GetBestVirtualSize());
    auto virtSize{toolboxScroller->GetVirtualSize()};
    static constexpr auto SCROLL_UNITS{15};
    toolboxScroller->SetScrollbars(SCROLL_UNITS, SCROLL_UNITS, virtSize.x / SCROLL_UNITS, virtSize.y / SCROLL_UNITS);
}

void updateVisibleBlocks() {
    for (auto *const block : *toolboxBlocks) {
        auto shown{(block->type & BladeStyles::FLAGMASK) & filterType};
        block->Show(shown);
    }
    updateToolboxSize();
}

void bindEvents() {
    frame->Bind(wxEVT_CLOSE_WINDOW, [](wxCloseEvent& evt) {
        evt.Skip();
        if (toolboxBlocks) {
            delete toolboxBlocks;
            toolboxBlocks = nullptr;
        }
    });
    toolbox->Bind(wxEVT_TOGGLEBUTTON, [](wxCommandEvent& evt) {
        auto *button{static_cast<wxToggleButton*>(evt.GetEventObject())};
        if (!button->GetValue()) {
            button->SetValue(true);
            return;
        }

#       define FILTER(filter, type) if (button != (filter)) (filter)->SetValue(false); else filterType = type
        FILTER(styleFilter, BladeStyles::COLOR);
        FILTER(layerFilter, BladeStyles::LAYER);
        FILTER(funcFilter, BladeStyles::FUNCTION);
        FILTER(transitionFilter, BladeStyles::TRANSITION);
        FILTER(effectFilter, BladeStyles::EFFECT);
#       undef FITLER

        updateVisibleBlocks();
    });
    frame->Bind(wxEVT_TEXT_PASTE, [&](wxClipboardTextEvent&) {
        wxTextDataObject textData;
        if (!wxClipboard::Get()->Open()) return;
        wxClipboard::Get()->GetData(textData);
        wxClipboard::Get()->Close();

        auto styleStr{textData.GetText().ToStdString()};
        auto *style{BladeStyles::parseString(styleStr)};
        if (!style) return;

        auto *block{new PCUI::StyleBlock(blockMoveArea, blockWorkspace, style)};
        block->SetPosition(blockWorkspace->ScreenToClient(wxGetMousePosition()));
    });

    // This should be turned into a menu option w/ accelerator
    frame->Bind(wxEVT_CHAR_HOOK, [](wxKeyEvent& evt){
        evt.Skip();
#       ifdef __WXOSX__
        if (!evt.CmdDown()) return;
#       else
        if (!evt.ControlDown()) return;
#       endif

        constexpr auto SCALE_DIFF{0.1F};
        switch (evt.GetUnicodeKey()) {
            case '=':
                if (!evt.ShiftDown()) return;
            case '+':
                if (scale < 1.F) scale += SCALE_DIFF; // NOLINT(readability-magic-numbers)
                break;
            case '-':
                if (scale > 0.7F) scale -= SCALE_DIFF; // NOLINT(readability-magic-numbers)
                break;
            default:
                return;
        }
        for (auto *const child : blockWorkspace->GetChildren()) {
            auto *styleBlock{dynamic_cast<PCUI::StyleBlock*>(child)};
            if (!styleBlock) return;

            styleBlock->setScale(scale);
            styleBlock->update(false);
        }
    });
}

} // namespace StyleEditor
