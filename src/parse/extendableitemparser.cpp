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

#include "extendableitemparser.h"

using namespace std;
using namespace nlohmann;

namespace jASTERIX
{

ExtendableItemParser::ExtendableItemParser (const nlohmann::json& item_definition)
 : ItemParser (item_definition)
{
    assert (type_ == "extendable");

    if (item_definition.find("items") == item_definition.end())
        throw runtime_error ("parsing extendable item '"+name_+"' without items");

    const json& items = item_definition.at("items");

    if (!items.is_array())
        throw runtime_error ("parsing extendable item '"+name_+"' items specification is not an array");

    std::string item_name;
    ItemParser* item {nullptr};

    for (const json& data_item_it : items)
    {
        item_name = data_item_it.at("name");
        item = ItemParser::createItemParser(data_item_it);
        assert (item);
        items_.push_back(std::unique_ptr<ItemParser>{item});
    }
}

size_t ExtendableItemParser::parseItem (const char* data, size_t index, size_t size, size_t current_parsed_bytes,
                              nlohmann::json& target, bool debug)
{
    if (debug)
        loginf << "parsing extendable item '" << name_ << "' with " << items_.size() << " items";

    size_t parsed_bytes {0};

    if (debug)
        loginf << "parsing extendable item '"+name_+"' items";

    unsigned int extend = 1;
    unsigned int cnt = 0;

    assert (target.find(name_) == target.end());
    json j_data = json::array();

    while (extend)
    {


        for (auto& data_item_it : items_)
        {
            if (debug)
                loginf << "parsing extendable item '" << name_ << "' data item '" << data_item_it->name() << "' index "
                       << index+parsed_bytes << " cnt " << cnt;

            parsed_bytes += data_item_it->parseItem(data, index+parsed_bytes, size, parsed_bytes,
                                                    j_data[cnt], debug);

            if (debug && j_data.at(cnt).find("extend") == j_data.at(cnt).end())
                throw runtime_error ("parsing extendable item '"+name_+"' without extend information");

            extend = j_data.at(cnt).at("extend");

            ++cnt;
        }
    }

    target.emplace(name_, std::move(j_data));

    return parsed_bytes;
}

}
