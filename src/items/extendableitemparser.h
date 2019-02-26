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

#ifndef EXTENDABLEITEMPARSER_H
#define EXTENDABLEITEMPARSER_H

#include "itemparserbase.h"

#include <vector>
#include <memory>

namespace jASTERIX
{

// parses item into array, extend into next has to signified by bool extend
class ExtendableItemParser : public ItemParserBase
{
public:
    ExtendableItemParser (const nlohmann::json& item_definition);
    virtual ~ExtendableItemParser() {}

    virtual size_t parseItem (const char* data, size_t index, size_t size, size_t current_parsed_bytes,
                              nlohmann::json& target, bool debug) override;
protected:
    std::vector<std::unique_ptr<ItemParserBase>> items_;
};

}


#endif // EXTENDABLEITEMPARSER_H
