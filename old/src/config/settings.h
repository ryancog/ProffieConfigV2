#pragma once
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * config/settings.h
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

#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "ui/combobox.h"
#include "ui/numeric.h"
#include "ui/numericdec.h"
#include "ui/selection.h"
#include "ui/toggle.h"

namespace Config::Setting {

struct DefineBase;
struct SettingBase;

using DefineMap = std::map<std::string, DefineBase *>;
using SettingMap = std::unordered_map<std::string, SettingBase *>;

enum class SettingType {
    TOGGLE,
    SELECTION,
    NUMERIC,
    DECIMAL,
    COMBO
};

template<class> struct Toggle;
template<class> struct Selection;
template<class> struct Numeric;
template<class> struct Decimal;
template<class> struct Combo;

struct SettingBase {
    SettingBase() = default;
    SettingBase(const SettingBase &) = default;
    SettingBase(SettingBase &&) = delete;
    SettingBase &operator=(const SettingBase &) = default;
    SettingBase &operator=(SettingBase &&) = delete;
    virtual ~SettingBase() = default;

    std::string name;
    std::string description;

    [[nodiscard]] virtual SettingType getType() const = 0;
};

struct DefineBase : SettingBase {
    std::string define;
    std::string postfix;
    bool pureDef{true};

    DefineMap group;
    std::unordered_set<std::string> require;
    bool requireAny{false};

    SettingType getType() const override = 0;
    virtual bool isDisabled();
};


template<class BASE>
struct Toggle : BASE {
    std::unordered_set<std::string> disable;
    bool value{false};

    PCUI::Toggle* control{nullptr};

    [[nodiscard]] virtual SettingType getType() const { return SettingType::TOGGLE; }
};

template<class BASE>
struct Selection : Toggle<BASE> {
    bool output{true};
    std::unordered_set<Selection*> peers;

    PCUI::Selection* control{nullptr};

    [[nodiscard]] virtual SettingType getType() const { return SettingType::SELECTION; }
};

template<class BASE>
struct Numeric : BASE {
    int32_t min{0};
    int32_t max{100}; // NOLINT(readability-magic-numbers)
    int32_t value{min};
    int32_t increment{1};

    PCUI::Numeric* control{nullptr};

    [[nodiscard]] virtual SettingType getType() const { return SettingType::NUMERIC; }
};

template<class BASE>
struct Decimal : BASE {
    double min{0};
    double max{0};
    double value{min};
    double increment{0.1}; // NOLINT(readability-magic-numbers)

    PCUI::NumericDec* control{nullptr};

    [[nodiscard]] virtual SettingType getType() const { return SettingType::DECIMAL; }
};

template<class BASE>
struct Combo : BASE {
    using OptionMap = std::unordered_map<std::string, std::string>;
    OptionMap options;
    std::string value;

    PCUI::ComboBox* control{nullptr};

    [[nodiscard]] virtual SettingType getType() const { return SettingType::COMBO; }
};

} // namespace Config::Setting
