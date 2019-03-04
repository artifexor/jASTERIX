/*
 * This file is part of jASTERIX.
 *
 * jASTERIX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * jASTERIX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with jASTERIX.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fixedbytesitemparser.h"
#include "logger.h"
#include "string_conv.h"

#include <iostream>

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{

FixedBytesItemParser::FixedBytesItemParser (const nlohmann::json& item_definition)
 : ItemParserBase (item_definition)
{
    assert (type_ == "fixed_bytes");

    if (item_definition.find("length") == item_definition.end())
        throw runtime_error ("fixed bytes item '"+name_+"' parsing without length");

    length_ = item_definition.at("length");

    if (item_definition.find("data_type") == item_definition.end())
        throw runtime_error ("fixed bytes item '"+name_+"' parsing without data type");

    data_type_ = item_definition.at("data_type");

    reverse_bits_ = (item_definition.find("reverse_bits") != item_definition.end()
            && item_definition.at("reverse_bits") == true);

    reverse_bytes_ = (item_definition.find("reverse_bytes") != item_definition.end()
            && item_definition.at("reverse_bytes") == true);

    negative_bit_pos_ = length_*8-1;
}

size_t FixedBytesItemParser::parseItem (const char* data, size_t index, size_t size, size_t current_parsed_bytes,
                              nlohmann::json& target, bool debug)
{
    if (debug)
    {
        assert (type_ == "fixed_bytes");
        loginf << "parsing fixed bytes item '" << name_ << "'" << logendl;
    }

    unsigned char tmp{0};
    size_t data_uint{0};
    int data_int{0};

    const char* current_data = &data[index];

    if (data_type_ == "string")
    {
        std::string data_str (reinterpret_cast<char const*>(current_data), length_-1); // -1 to account for end 0

        if (debug)
            loginf << "fixed bytes item '"+name_+"' parsing index " << index << " length " << length_
                   << " data type " << data_type_ << " value '" << data_str << "'" << logendl;

        assert (target.find(name_) == target.end());
        target.emplace(name_, std::move(data_str));

        return length_;
    }
    else if (data_type_ == "uint")
    {
        if (length_ > sizeof(size_t))
            throw runtime_error ("fixed bytes item '"+name_+"' length larger than "+to_string(sizeof(size_t)));

        if (reverse_bytes_)
        {
            for (int cnt = length_-1; cnt >= 0; --cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*> (&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '"+name_+"' cnt " << cnt << " byte "
                           << std::hex << static_cast<unsigned int> (tmp) << " reverse bytes false bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }
        else
        {
            for (size_t cnt = 0; cnt < length_; ++cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*> (&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '"+name_+"' cnt " << cnt << " byte "
                           << std::hex << static_cast<unsigned int> (tmp) << " reverse bytes true bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }

        if (debug)
            loginf << "parsing fixed bytes item '"+name_+"' index " << index << " length " << length_
                   << " data type " << data_type_ << " value '" << data_uint << "'" << logendl;

        assert (target.find(name_) == target.end());
        //target[name_] = data_uint;
        target.emplace(name_, data_uint);

        return length_;
    }
    else if (data_type_ == "int")
    {
        if (length_ > sizeof(size_t))
            throw runtime_error ("fixed bytes item '"+name_+"' length larger than "+to_string(sizeof(size_t)));

        if (reverse_bytes_)
        {
            for (int cnt = length_-1; cnt >= 0; --cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*> (&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '"+name_+"' cnt " << cnt << " byte "
                           << std::hex << static_cast<unsigned int> (tmp) << " reverse bytes false bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }
        else
        {
            for (size_t cnt = 0; cnt < length_; ++cnt)
            {
                tmp = *reinterpret_cast<const unsigned char*> (&current_data[cnt]);

                if (debug)
                    loginf << "fixed bytes item '"+name_+"' cnt " << cnt << " byte "
                           << std::hex << static_cast<unsigned int> (tmp) << " reverse bytes true bits "
                           << reverse_bits_ << " data " << data_uint << logendl;

                if (reverse_bits_)
                    reverseBits(tmp);

                data_uint = (data_uint << 8) + tmp;
            }
        }

        if ( (data_uint & (1 << negative_bit_pos_)) != 0)
            data_int = data_uint | ~((1 << negative_bit_pos_) - 1);
        else
            data_int = data_uint;

        if (debug)
            loginf << "parsing fixed bytes item '"+name_+"' index " << index << " length " << length_
                   << " data type " << data_type_ << " value '" << data_int << "'" << logendl;

        assert (target.find(name_) == target.end());
        //target[name_] = data_int;
        target.emplace(name_, data_int);

        return length_;
    }
    else if (data_type_ == "bin")
    {
        std::string data_str = binary2hex(reinterpret_cast<const unsigned char*>(current_data), length_);

        if (debug)
        {
            loginf << "fixed bytes item '"+name_+"' parsing index " << index << " length " << length_
                   << " data type " << data_type_ << " value '" << data_str << "'" << logendl;
        }

        assert (target.find(name_) == target.end());
        //target[name_] = data_str;
        target.emplace(name_, std::move(data_str));

        return length_;
    }
    else
        throw runtime_error ("fixed bytes item '"+name_+"' parsing with unknown data type '"+data_type_+"'");
}

}
