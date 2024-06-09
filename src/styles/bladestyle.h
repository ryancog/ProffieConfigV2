#pragma once
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * styles/bladestyle.h
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

#include <cstdint>
#include <variant>
#include <vector>
#include <string>
#include <map>

namespace BladeStyles {

typedef uint32_t StyleType;
enum : StyleType {
    // Flags
    VARIADIC	= 0b1000000000000000U << 16,
    FIXEDCOLOR	= 0b0100000000000000U << 16,

    // Types
    NUMBER      = 0b0000000000000001,
    BITS		= 0b0000000000000010,
    BOOL		= 0b0000000000000100,

    WRAPPER		= 0b0000000000001000,
    BUILTIN		= 0b0000000000010000,
    FUNCTION	= 0b0000000000100000,
    FUNCTION3D  = 0b0000000001000000,
    COLOR 		= 0b0000000010000000,
    LAYER       = 0b0000000100000000,
    TRANSITION	= 0b0000001000000000,
    TIMEFUNC    = 0b0000010000000000,
    EFFECT		= 0b0000100000000000,
    LOCKUPTYPE	= 0b0001000000000000,
    ARGUMENT    = 0b0010000000000000,

    // Refs
    REFARG_1	= 1U << 28,
    REFARG_2	= 2U << 28,
    REFARG_3	= 3U << 28,
    REFARG_4	= 4U << 28,
    REFARG_5	= 5U << 28,
    REFARG_6	= 6U << 28,
    REFARG_7	= 7U << 28,
    REFARG_8	= 8U << 28,
    // Could go up to 15, but I don't feel like putting all those here right now
    REFMASK		= 0b1111U << 28,

    // Masks & Stuff
    FLAGMASK	= ~(VARIADIC | FIXEDCOLOR | REFMASK),
    // If this type uses a BladeStyle base
    STYLETYPE   = (
            WRAPPER | 
            BUILTIN | 
            FUNCTION | 
            FUNCTION3D | 
            COLOR | 
            LAYER | 
            TRANSITION | 
            TIMEFUNC | 
            EFFECT | 
            LOCKUPTYPE | 
            ARGUMENT),

    // OFFTYPE?
    // CHANGETYPE?
    // CCTYPE?
};

class Param;
class BladeStyle;

typedef std::variant<int32_t, BladeStyle*> ParamValue;

class BladeStyle {
public:
    BladeStyle(const BladeStyle&);
    virtual ~BladeStyle();

    virtual StyleType getType() const;

    bool setParams(const std::vector<ParamValue>&);
    bool setParam(size_t idx, const ParamValue&);
    bool addParam(const ParamValue&);
    /**
     * Used to remove variadic args
     */
    bool removeParam(size_t idx);

    const std::vector<Param*>& getParams() const;
    const Param* getParam(size_t idx) const;
    virtual bool validateParams(std::string* err = nullptr) const;

    /** 
     * Get param as StyleParam, then get style from param.
     * Use  with caution, it's just static casts! 
     * */
    inline const BladeStyle* getParamStyle(size_t idx) const;
    /** 
     * Get param as NumberParam, then get number from param.
     * Use  with caution, it's just static casts! 
     * */
    inline int32_t getParamNumber(size_t idx) const;
    /** 
     * Get param as bitsparam, then get bits from param.
     * Use  with caution, it's just static casts! 
     * */
    inline int32_t getParamBits(size_t idx) const;
    /** 
     * Get param as BoolParam, then get bool from param.
     * Use  with caution, it's just static casts! 
     * */
    inline bool getParamBool(size_t idx) const;

    const char* osName;
    const char* humanName;
    std::string comment;

protected:
    BladeStyle(
            const char* osName, 
            const char* humanName, 
            const StyleType type,
            const std::vector<Param*>& params
            );

    const StyleType type;

private:
    std::vector<Param*> params{};
};

class Param {
public:
    Param(const Param&) = delete;
    virtual ~Param();

    virtual StyleType getType() const;

    const char* name;

protected:
    Param(const char* name, StyleType type);

    const StyleType type;
};

class StyleParam : public Param {
public:
    StyleParam(const StyleParam&) = delete;
    StyleParam(const char* name, StyleType type, BladeStyle* style);
    virtual ~StyleParam() override;

    void setStyle(BladeStyle*);
    const BladeStyle* getStyle() const;
    /**
     * Clear pointer, return style
     *
     * Must acquire otherwise memory leak.
     */
    [[nodiscard]] BladeStyle* detachStyle();

private:
    BladeStyle* style;
};

class NumberParam : public Param {
public:
    NumberParam(const char* name, const StyleType initialValue = 0, const StyleType additionalFlags = 0);

    void setNum(int32_t);
    int32_t getNum() const;

private:
    int32_t value;
};

class BitsParam : public Param {
public:
    BitsParam(const char* name, const StyleType initialValue = 0, const StyleType additionalFlags = 0);

    void setBits(int32_t);
    int32_t getBits() const;

private:
    int32_t value;
};

class BoolParam : public Param {
public:
    BoolParam(const char* name, const bool initialValue = false, const StyleType additionalFlags = 0);

    // These really don't need to exist for this one...
    void setBool(bool);
    bool getBool() const;

private:
    bool value;
};

inline const BladeStyle* BladeStyle::getParamStyle(size_t idx) const {
    return static_cast<const BladeStyle*>(static_cast<const StyleParam*>(getParam(idx))->getStyle());
}

inline int32_t BladeStyle::getParamNumber(size_t idx) const {
    return static_cast<const NumberParam*>(getParam(idx))->getNum();
}

inline int32_t BladeStyle::getParamBits(size_t idx) const {
    return static_cast<const BitsParam*>(getParam(idx))->getBits();
}

inline bool BladeStyle::getParamBool(size_t idx) const {
    return static_cast<const BoolParam*>(getParam(idx))->getBool();
}

typedef BladeStyle* (*StyleGenerator)(const std::vector<ParamValue>&);
typedef std::map<std::string, StyleGenerator> StyleMap;

StyleGenerator get(const std::string& styleName);

#define STYLECAST(resultType, input) const_cast<resultType*>(static_cast<const resultType*>(input))

#define PARAMS(...) std::vector<Param*>{ __VA_ARGS__ }
#define PARAMVEC(...) std::vector<ParamValue>{ __VA_ARGS__ }
#define STYLEPAIR(name) { \
    #name, \
    [](const std::vector<ParamValue>& paramArgs) -> BladeStyle* { \
        auto ret{new name()}; \
        if (!ret->setParams(paramArgs)) { \
            delete ret; \
            return nullptr; \
        } \
        return ret; \
    } \
}

}
