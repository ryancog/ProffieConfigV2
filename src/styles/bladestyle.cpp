#include "bladestyle.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * styles/bladestyle.cpp
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

#include "log/logger.h"
#include "stylepreview/blade.h"
#include "styles/elements/args.h"
#include "styles/elements/colorstyles.h"
#include "styles/elements/effects.h"
#include "styles/elements/functions.h"
#include "styles/elements/layers.h"
#include "styles/elements/lockuptype.h"
#include "styles/elements/timefunctions.h"
#include "styles/elements/colors.h"
#include "styles/elements/builtin.h"
#include "styles/elements/wrappers.h"
#include "styles/elements/transitions.h"
#include "ui/bool.h"
#include <algorithm>
#include <cstdint>
#include <variant>

using namespace BladeStyles;

BladeStyle::BladeStyle(
        const char* osName, 
        const char* humanName, 
        const StyleType type,
        const std::vector<Param*>& params) :
    osName(osName),
    humanName(humanName),
    type(type),
    params(params) {}

BladeStyle::BladeStyle(const BladeStyle& other) :
    osName(other.osName),
    humanName(other.osName),
    type(other.type) {
    params.clear();

    for (const auto param : other.params) {
        Param* newParam;
        switch (param->getType() & FLAGMASK) {
            case NUMBER:
                newParam = new NumberParam(param->name, static_cast<const NumberParam*>(param)->getNum(), param->getType());
                break;
            case BITS:
                newParam = new BitsParam(param->name, static_cast<const BitsParam*>(param)->getBits(), param->getType());
                break;
            case BOOL:
                newParam = new BoolParam(param->name, static_cast<const BoolParam*>(param)->getBool(), param->getType());
                break;
            default:
                newParam = new StyleParam(param->name, param->getType(), new BladeStyle(*static_cast<const StyleParam*>(param)->getStyle()));
                break;
        }
        params.push_back(newParam);
    }
}

BladeStyle::~BladeStyle() {
    for (auto param : params) {
        if (param) delete param;
    }
}

StyleType BladeStyle::getType() const { return type; }

StyleGenerator BladeStyles::get(const std::string& styleName) {
    StyleGenerator gen;

    gen = FunctionStyle::get(styleName);
    if (gen) return gen;
    gen = TimeFunctionStyle::get(styleName);
    if (gen) return gen;
    gen = ColorStyle::get(styleName);
    if (gen) return gen;
    gen = FixedColorStyle::get(styleName);
    if (gen) return gen;
    gen = BuiltIn::get(styleName);
    if (gen) return gen;
    gen = ArgumentStyle::get(styleName);
    if (gen) return gen;
    gen = LockupTypeStyle::get(styleName);
    if (gen) return gen;
    gen = EffectStyle::get(styleName);
    if (gen) return gen;
    gen = TransitionStyle::get(styleName);
    if (gen) return gen;
    gen = LayerStyle::get(styleName);
    if (gen) return gen;
    gen = WrapperStyle::get(styleName);
    if (gen) return gen;

    return nullptr;
}

bool BladeStyle::validateParams(std::string* err) const {
    for (const auto param : params) {
        if (!(param->getType() & STYLETYPE)) continue;

        if (!static_cast<const StyleParam*>(param)->getStyle()) {
            if (err) {
                (*err) = "Empty parameter: \"";
                (*err) += param->name;
                (*err) += '"';
            }
            return false;
        }
    }
    return true;
}

bool BladeStyle::setParams(const std::vector<ParamValue>& inParams) {
    if (inParams.size() > params.size() && !(params.back()->getType() & VARIADIC)) return false;

    for (size_t i{0}; i < inParams.size(); i++) {
        if (i < params.size()) setParam(i, inParams.at(i));
        else addParam(inParams.at(i));
    }

    return true;
}

bool BladeStyle::setParam(size_t idx, const ParamValue& inParam) {
    if (idx >= params.size()) return false;
    auto& param{params.at(idx)};

    if (std::holds_alternative<BladeStyle*>(inParam)) {
        auto inStyle{std::get<BladeStyle*>(inParam)};
        if (!(inStyle->type & param->getType() & FLAGMASK)) return false;
        static_cast<StyleParam*>(param)->setStyle(inStyle);
        return true;
    }

    switch (param->getType()) {
        case FUNCTION:
        case BOOL:
        case BITS:
            if (!std::holds_alternative<int32_t>(inParam)) return false;
            break;
        default:
            Logger::error("Invalid Parameter");
            return false;
    }

    switch (param->getType()) {
        case FUNCTION:
            static_cast<NumberParam*>(param)->setNum(std::get<int32_t>(inParam));
            break;
        case BOOL:
            static_cast<BoolParam*>(param)->setBool(std::get<int32_t>(inParam));
            break;
        case BITS:
            static_cast<BitsParam*>(param)->setBits(std::get<int32_t>(inParam));
            break;
    }

    return true;
}

bool BladeStyle::addParam(const ParamValue& newParam) {
    auto& lastParam{params.back()};
    if (~lastParam->getType() & VARIADIC) return false;

    switch (lastParam->getType() & FLAGMASK) {
        case NUMBER:
        case BITS:
        case BOOL:
            if (!std::holds_alternative<int32_t>(newParam)) return false;
            break;
        case FUNCTION:
        case FUNCTION3D:
        case COLOR:
        case LAYER:
        case TRANSITION:
        case TIMEFUNC: 
        case EFFECT:
        case LOCKUPTYPE:
        case ARGUMENT:
            if (!std::holds_alternative<BladeStyle*>(newParam)) return false;
            if (!(std::get<BladeStyle*>(newParam)->getType() & lastParam->getType() & FLAGMASK)) return false;
            break;
    }

    Param* param;
    switch (lastParam->getType() & FLAGMASK) {
        case NUMBER:
            param = new NumberParam(lastParam->name, std::get<int32_t>(newParam), lastParam->getType());
            break;
        case BITS:
            param = new BitsParam(lastParam->name, std::get<int32_t>(newParam), lastParam->getType());
            break;
        case BOOL:
            param = new BoolParam(lastParam->name, std::get<int32_t>(newParam), lastParam->getType());
            break;
        case FUNCTION:
        case FUNCTION3D:
        case COLOR:
        case LAYER:
        case TRANSITION:
        case TIMEFUNC: 
        case EFFECT:
        case LOCKUPTYPE:
        case ARGUMENT:
            param = new StyleParam(lastParam->name, lastParam->getType(), std::get<BladeStyle*>(newParam));
            break;
    }

    params.push_back(param);
    return true;
}

bool BladeStyle::removeParam(size_t idx) {
    if (idx >= params.size()) return false;
    auto it{std::next(params.begin(), idx)};
    if (~(*it)->getType() & VARIADIC) return false;

    if (*it) delete (*it);
    params.erase(it);
    return true;
}

const std::vector<Param*>& BladeStyle::getParams() const {
    return params;
}

const Param* BladeStyle::getParam(size_t idx) const {
    return idx >= params.size() ? nullptr : *std::next(params.begin(), idx);
}

Param::Param(const char* name, const StyleType type) :
    name(name), type(type) {}

Param::~Param() {}

StyleType Param::getType() const { return type; }

StyleParam::StyleParam(const char* name, StyleType type, BladeStyle* style) :
    Param(name, type), style(style) {}

StyleParam::~StyleParam() { if (style) delete style; }

void StyleParam::setStyle(BladeStyle* newStyle) {
    if (style) delete style;
    style = newStyle;
}

const BladeStyle* StyleParam::getStyle() const { return style; }

BladeStyle* StyleParam::detachStyle() {
    auto ret{style};
    style = nullptr;
    return ret;
}

NumberParam::NumberParam(const char* name, const StyleType initialValue, const StyleType additionalFlags) :
    Param(name, NUMBER | additionalFlags), value(initialValue) {}

void NumberParam::setNum(int32_t newValue) { value = std::clamp(newValue, -32768, 32768); }
int32_t NumberParam::getNum() const { return value; }

BitsParam::BitsParam(const char* name, const StyleType initialValue, const StyleType additionalFlags) :
    Param(name, BITS | additionalFlags), value(initialValue & 0xFFFF) {}

void BitsParam::setBits(int32_t newValue) { value = newValue & 0xFFFF; }
int32_t BitsParam::getBits() const { return value; }

BoolParam::BoolParam(const char* name, const bool initialValue, const StyleType additionalFlags) :
    Param(name, BOOL | additionalFlags), value(initialValue) {}

void BoolParam::setBool(bool newValue) { value = newValue; }
bool BoolParam::getBool() const { return value; }

