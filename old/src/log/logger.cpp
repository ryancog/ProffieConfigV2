#include "logger.h"
/*
 * ProffieConfig, All-In-One Proffieboard Management Utility
 * Copyright (C) 2024 Ryan Ogurek
 *
 * logger.cpp
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
#include <ostream>
#include <unordered_map>

namespace Logger {

std::unordered_map<std::ostream*, LogLevel>* logOutputs;

void init() {
    if (!logOutputs) logOutputs = new std::unordered_map<std::ostream*, LogLevel>{ {&std::cout, LogLevel::ALL} };
}

void log(LogLevel level, const std::string& message, const bool) {
    std::string prefix;
    switch (level) {
    case ERR:
        prefix += "\e[31m (ERR) ";
        break;
    case WARN:
        prefix += "\e[31m(WARN) ";
        break;
    case INFO:
        prefix += "\e[32m(INFO) ";
        break;
    case DEBUG:
        prefix += "\e[36m (DBG) ";
        break;
    case VERBOSE:
        prefix += "\e[37m(VERB) ";
        break;
    default:
        std::cerr << "Invalid Log Level\n";
        return;
    }

    for (auto logOut : *logOutputs) {
        if (!(level & logOut.second)) continue;
        (*logOut.first) << prefix << message << "\e[0m\n";
        (*logOut.first).flush();
    }
}

void addLogOut(std::ostream& out, LogLevel level) {
    logOutputs->insert({ &out, level});
}

void removeLogOut(std::ostream& out) {
    auto toRemove{logOutputs->find(&out)};
    if (toRemove != logOutputs->end()) logOutputs->erase(toRemove);
}


void error(const std::string& message, bool notify) {
    log(ERR, message, notify);
}
void warn (const std::string& message, bool notify) {
    log(WARN, message, notify);
}
void info (const std::string& message, bool notify) {
    log(INFO, message, notify);
}
void debug(const std::string& message, bool notify) {
    log(DEBUG, message, notify);
}
void verbose(const std::string& message, bool notify) {
    log(VERBOSE, message, notify);
}

} // namespace Logger

