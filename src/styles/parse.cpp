#include "parse.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * styles/parse.cpp
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
#include "styles/bladestyle.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <optional>
#include <string>

using namespace BladeStyles;

struct TokenizedStyle {
    std::string name;
    std::string comment;

    struct Param {
        std::string rawStr;
        std::string paramStr;
        std::string comments;
    };
    std::vector<Param> params;
};
static std::optional<TokenizedStyle> tokenizeStyle(const std::string &);

static std::string typeToString(uint32_t);

BladeStyle *BladeStyles::parseString(const std::string &styleStr, bool *foundStyle) {
    auto styleTokens{tokenizeStyle(styleStr)};
    if (!styleTokens) return nullptr;

    if (foundStyle) *foundStyle = false;

    auto styleGen{get(styleTokens->name)};
    if (!styleGen) {
        Logger::error("Style not recognized: " + styleTokens->name);
        return nullptr;
    } 
    if (foundStyle) *foundStyle = true;

    auto *style{styleGen({})};
    auto numParams{style->getParams().size()};
    auto numTokenParams{styleTokens->params.size()};
    if (
            numTokenParams < numParams || 
            (
             numParams > 0 &&
             (!(style->getParams().back()->getType() & VARIADIC)) && 
             numTokenParams > numParams
            )
       ) {
        Logger::error("Incorrect number of parameters for style: " + 
                styleTokens->name + 
                " (Expected " + 
                std::to_string(numParams) +
                " got " + 
                std::to_string(numTokenParams) +
                ")");
        delete style;
        return nullptr;
    }
    style->comment = std::move(styleTokens->comment);

    for (size_t i{0}; i < numTokenParams; i++) {
        const auto *param{style->getParam(i < numParams ? i : numParams - 1)};
        auto& tokenParam{styleTokens->params.at(i)};

        int32_t val{0};
        if (param->getType() & (NUMBER | BITS | BOOL)) {
            if (tokenParam.paramStr.find("true") != std::string::npos) val = true;
            else if (tokenParam.paramStr.find("false") != std::string::npos) val = false;
            else {
                char* stoiEnd{nullptr};
                val = static_cast<int32_t>(std::strtol(tokenParam.paramStr.c_str(), &stoiEnd, 0));
                if (val == 0 && *stoiEnd == 'b' && *(stoiEnd - 1) == '0') {
                    val = static_cast<int32_t>(std::strtol(stoiEnd + 1, nullptr, 2));
                }
            }
        }
        switch (param->getType() & FLAGMASK) {
            case NUMBER:
                STYLECAST(NumberParam, param)->setNum(val);
                break;
            case BITS:
                STYLECAST(BitsParam, param)->setBits(val);
                break;
            case BOOL:
                STYLECAST(BoolParam, param)->setBool(val);
                break;
            default:
                auto *paramStyle{parseString(tokenParam.rawStr, foundStyle)};
                if (!paramStyle) {
                    Logger::error("Failure while parsing parameter " + std::to_string(i + 1) + " in style " + styleTokens->name);
                    delete style;
                    return nullptr;
                }
                STYLECAST(StyleParam, param)->setStyle(paramStyle);
                break;
        }
    }

    return style;
}

std::optional<std::string> BladeStyles::asString(const BladeStyle& style) {
    std::string ret;

    if (!style.comment.empty()) {
        ret += "/*\n";
        ret += style.comment;
        ret += "\n*/\n";
    }
    auto type{style.getType()};
    if (type & BUILTIN) {
        ret += '&';
        ret += style.osName;
        return ret;
    }

    auto shouldIndentParams{false};
    for (auto *const param : style.getParams()) {
        if (param->getType() & STYLETYPE) {
            if (static_cast<StyleParam*>(param)->getStyle()->getType() & FIXEDCOLOR) continue;
            shouldIndentParams = true;
            break;
        }
    }

    ret += style.osName;
    if (!(style.getType() & FIXEDCOLOR)) ret += '<';
    for (auto *const param : style.getParams()) {
        if (shouldIndentParams) ret += "\n\t";
        switch (param->getType() & FLAGMASK) {
            case NUMBER:
                ret += std::to_string(static_cast<NumberParam*>(param)->getNum());
                break;
            case BITS: {                
                auto bits{static_cast<BitsParam*>(param)->getBits()};
                ret += "0b";
                for (uint32_t i{0}; i < sizeof(uint16_t) * CHAR_BIT; i++) {
                    ret += ((bits >> i) & 0x1) ? '1' : '0';
                }
                break; }
            case BOOL:
                ret += static_cast<BoolParam*>(param)->getBool() ? "true" : "false";
                break;
            default:
                const auto *paramStyle{static_cast<StyleParam*>(param)->getStyle()};
                if (!paramStyle) return std::nullopt;
                auto styleStr{asString(*paramStyle)};
                if (!styleStr) return std::nullopt;
                if (shouldIndentParams) {
                    size_t newlinePos{0};
                    while ((newlinePos = styleStr->find('\n', newlinePos + 1)) != std::string::npos) {
                        styleStr->replace(newlinePos, 1, "\n\t");
                    }
                }
                ret += styleStr.value();
                break;
        }
        if (param != style.getParams().back()) ret += ", ";
    }
    if (shouldIndentParams) ret += '\n'; 
    if (!(style.getType() & FIXEDCOLOR)) ret += '>';
    if (type & WRAPPER) ret += "()";

    return ret;
}

static std::optional<TokenizedStyle> tokenizeStyle(const std::string& styleStr) {
    // Find all comments
    std::vector<std::pair<size_t, size_t>> commentRanges;
    size_t commentIndex{0};
    while (!false) {
        auto commentBegin{styleStr.find("/*", commentIndex)};
        auto commentEnd{styleStr.find("*/", commentBegin)};

        auto commentFound{commentBegin != std::string::npos || commentEnd != std::string::npos};
        auto localCommentIndex{commentIndex};

        if (commentFound) {
            if (commentBegin == std::string::npos || commentEnd == std::string::npos) {
                Logger::error("Mismatched block comment, aborting!");
                return std::nullopt;
            }
            commentRanges.emplace_back(commentBegin, commentEnd + 2);
            localCommentIndex = commentEnd + 2;
        }

        commentBegin = styleStr.find("//", commentIndex);
        commentFound |= commentBegin != std::string::npos;
        if (commentBegin != std::string::npos) {
            commentEnd = styleStr.find('\n', commentBegin);
            commentRanges.emplace_back(commentBegin, commentEnd + 1);

            if (commentEnd > localCommentIndex) localCommentIndex = commentEnd;
        }

        commentIndex = localCommentIndex;
        if (!commentFound) break;
    }
    std::sort(commentRanges.begin(), commentRanges.end());

    TokenizedStyle styleTokens;

    // Find name
    size_t nameEnd{0};
    for (size_t i{0}; i < styleStr.length(); i++) {
        // Jump over commentRanges
        bool jumped{false};
        for (const auto& [ cBegin, cEnd ] : commentRanges) {
            if (i == cBegin) {
                i = cEnd - 1; // compensate for i++ in loop
                jumped = true;
                break;
            }
        }
        if (jumped) continue;

        char characer{styleStr.at(i)};

        if (std::isalpha(characer) != 0) {
            auto nameBegin{i};
            while (i < styleStr.length() && std::isalnum(styleStr.at(i))) { i++; }
            styleTokens.name = styleStr.substr(nameBegin, i - nameBegin);
            nameEnd = i;
            break;
        }
    }

    if (styleTokens.name.empty()) {
        Logger::error("Could not find style name/begin of style!");
        return std::nullopt;
    }
    auto getCommentsInRange{[&commentRanges, &styleStr](size_t begin, size_t end) -> std::string {
        std::string ret;
        for (const auto& [ cmntBegin, cmntEnd ] : commentRanges) {
            if (cmntEnd < begin) continue;
            if (cmntBegin > end) break;

            if (!ret.empty()) ret += '\n';
            if (styleStr.at(cmntEnd - 1) == '/') { // is a block comment
                auto contentBegin{std::find_if_not(std::next(styleStr.cbegin(), static_cast<int64_t>(cmntBegin + 2)), std::next(styleStr.cbegin(), static_cast<int64_t>(cmntEnd - 3)), isspace)};
                // ***backwards*** lol
                auto contentEnd{std::find_if_not(std::prev(styleStr.crend(), static_cast<int64_t>(cmntEnd - 2)), std::prev(styleStr.crend(), static_cast<int64_t>(cmntBegin + 2)), isspace)};
                const char *subCStr{contentBegin.base()};
                auto length{contentEnd.base() - contentBegin};
                ret += std::string(subCStr, length);
            } else { // is a line comment
                ret += styleStr.substr(cmntBegin + 2, cmntEnd - cmntBegin - 3);
            }
        }
        return ret;
    }};
    styleTokens.comment = getCommentsInRange(0, nameEnd);

    if (nameEnd > styleStr.length()) return styleTokens;

    auto paramSubStr{styleStr.substr(nameEnd)};
    size_t paramBegin{1}; // Skip over the < at nameEnd
    size_t paramNameEnd{std::string::npos};
    std::string buf;
    int32_t depth{0};
    char character{'\0'};
    for (size_t i = 0; i < paramSubStr.size(); i++) {
        // Jump over commentRanges
        for (const auto& [ cBegin, cEnd ] : commentRanges) {
            if (i + nameEnd == cBegin) i = cEnd - nameEnd;
        }

        character = paramSubStr.at(i);
        if (character == ' ') continue;
        if (character == '\n') continue;
        if (character == '\t') continue;
        if (character == '\r') continue;

        if (character == '<') {
            if (depth == 1 && paramNameEnd == std::string::npos) {
                paramNameEnd = i;
            } else if (depth == 0) {
                buf.clear();
                depth++;
                continue;
            }
            depth++;
        } else if (character == '>') {
            depth--;
            if (depth < 0) {
                Logger::warn("Error parsing arguments for style: " + styleTokens.name);
                return std::nullopt;
            }
            if (depth == 0) {
                styleTokens.params.push_back({
                        .rawStr = paramSubStr.substr(paramBegin, i - paramBegin),
                        .paramStr = buf,
                        .comments = getCommentsInRange(paramBegin + nameEnd, paramNameEnd + nameEnd),
                        });
                break;
            }
        } else if (character == ',' && depth == 1) {
            styleTokens.params.push_back({
                    .rawStr = paramSubStr.substr(paramBegin, i - paramBegin),
                    .paramStr = buf,
                    .comments = getCommentsInRange(paramBegin + nameEnd, paramNameEnd + nameEnd),
                    });
            buf.clear();
            paramBegin = i + 1;
            continue;
        }

        buf += character;
    }
    if (depth != 0) {
        Logger::warn("Mismatched <> in style: " + styleTokens.name);
        return std::nullopt;
    }

    return styleTokens;
}

static std::string typeToString(uint32_t type) {
    switch (type & FLAGMASK) {
    case WRAPPER:
        return "Wrapper";
    case BUILTIN:
        return "BuiltIn";
    case FUNCTION:
        return "Int";
    case BITS:
        return "Bits";
    case BOOL:
        return "Boolean";
    case COLOR:
        return "Color";
    case LAYER:
        return "Layer (Transparent Color)";
    case EFFECT:
        return "Effect";
    case LOCKUPTYPE:
        return "LockupType";
    }

    return "INVALID";
}
