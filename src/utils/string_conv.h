/*
 * This file is part of ATSDB.
 *
 * ATSDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ATSDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with ATSDB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STRING_CONV_H
#define STRING_CONV_H

#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

#include "json.hpp"

extern int char2int(char input);
extern size_t hex2bin(const char* src, char* target);
extern char getIcaoChar(unsigned char c);
extern std::string binary2hex(const unsigned char* src, unsigned int length);

extern std::vector<std::string>& split(const std::string& s, char delim,
                                       std::vector<std::string>& elems);
// extern std::vector<std::string> split(const std::string &s, char delim);

extern std::string toString(const nlohmann::json& j);

extern bool isASCII(const std::string& s);

inline std::string timeStringFromDouble(double seconds, bool milliseconds = true)
{
    int hours, minutes;
    std::ostringstream out;

    if (seconds < 0)
    {
        out << "-";
        seconds *= -1;
    }

    hours = static_cast<int>(seconds / 3600.0);
    minutes = static_cast<int>(static_cast<double>(static_cast<int>(seconds) % 3600) / 60.0);
    seconds = seconds - hours * 3600.0 - minutes * 60.0;

    out << std::fixed << std::setw(2) << std::setfill('0') << hours << ":" << std::setw(2)
        << std::setfill('0') << minutes << ":";

    if (milliseconds)
        out << std::setw(6) << std::setfill('0') << std::setprecision(3) << seconds;
    else
        out << std::setw(2) << std::setfill('0') << std::setprecision(0)
            << static_cast<int>(seconds);

    return out.str();
}

#endif  // STRING_CONV_H
