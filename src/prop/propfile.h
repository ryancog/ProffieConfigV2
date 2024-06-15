#pragma once
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * prop/propfile.h
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

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "config/settings.h"

namespace PropFile {

struct Data;

std::vector<Data*> getPropData(const std::vector<std::string>& pconfs);

struct Data {
    Data() = default;
    Data(const Data &) = default;
    Data(Data &&) = delete;
    Data &operator=(const Data &) = default;
    Data &operator=(Data &&) = delete;
    ~Data();

    struct LayoutItem;
    struct LayoutLevel;

    struct ButtonMap;
    struct ButtonState;
    struct Button;

    std::string name;
    std::string filename;

    std::string info;

    Config::Setting::DefineMap *settings{nullptr};

    using LayoutVec = std::vector<LayoutItem *>;
    LayoutVec *layout{nullptr};

    using Buttons = std::unordered_map<uint8_t, ButtonMap *>;
    Buttons buttonControls;
};

enum class LayoutType {
    ITEM,
    LEVEL
};

struct Data::LayoutItem {
    LayoutItem() = default;
    LayoutItem(const LayoutItem &) = default;
    LayoutItem(LayoutItem &&) = delete;
    LayoutItem &operator=(const LayoutItem &) = default;
    LayoutItem &operator=(LayoutItem &&) = delete;
    virtual ~LayoutItem() {
        delete setting;
    }

    Config::Setting::DefineBase *setting{nullptr};

    [[nodiscard]] virtual LayoutType getType() const { return LayoutType::ITEM; }
};

struct Data::LayoutLevel : LayoutItem {
    LayoutLevel() = default;
    LayoutLevel(const LayoutLevel &) = default;
    LayoutLevel(LayoutLevel &&) = delete;
    LayoutLevel &operator=(const LayoutLevel &) = default;
    LayoutLevel &operator=(LayoutLevel &&) = delete;
     ~LayoutLevel() override {
        if (items) {
            for (auto &item : *items) delete item;
            delete items;
        }
    }

    std::string label;
    std::vector<LayoutItem *> *items{nullptr};
    enum class Direction { HORIZONTAL, VERTICAL } direction;

     [[nodiscard]] LayoutType getType() const override { return LayoutType::LEVEL; }
};


struct Data::Button {
    std::string label;
    std::unordered_map<Config::Setting::DefineBase*, std::string> descriptions;
};

struct Data::ButtonState {
    ButtonState() = default;
    ButtonState(const ButtonState &) = default;
    ButtonState(ButtonState &&) = delete;
    ButtonState &operator=(const ButtonState &) = default;
    ButtonState &operator=(ButtonState &&) = delete;
    virtual ~ButtonState() {
        for (auto &button : buttons) delete button;
    }

    std::string label;
    std::vector<Button *> buttons;
};

struct Data::ButtonMap {
    ButtonMap() = default;
    ButtonMap(const ButtonMap &) = default;
    ButtonMap(ButtonMap &&) = delete;
    ButtonMap &operator=(const ButtonMap &) = default;
    ButtonMap &operator=(ButtonMap &&) = delete;
    virtual ~ButtonMap() {
        for (const auto &state : states) delete state;
    }

    std::unordered_set<ButtonState *> states;
    int8_t numButton;
};

} // namespace PropFile

