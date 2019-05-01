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

#include "asterixparser.h"
#include "logger.h"
#include "itemparserbase.h"
#include "record.h"
#include "jasterix.h"

#include <iostream>
#include <iomanip>

#include <tbb/tbb.h>

using namespace std;
using namespace nlohmann;

namespace jASTERIX {

ASTERIXParser::ASTERIXParser(const nlohmann::json& data_block_definition,
                             const std::map<unsigned int, std::shared_ptr<Edition>>& asterix_category_definitions,
                             const std::map<unsigned int, std::shared_ptr<Mapping>>& mappings,
                             bool debug)
    : mappings_(mappings)
{
    // data block

    if (data_block_definition.find("name") == data_block_definition.end())
        throw runtime_error ("data block construction without JSON name definition");

    data_block_name_ = data_block_definition.at("name");

    if (data_block_definition.find("items") == data_block_definition.end())
        throw runtime_error ("data block construction without header items");

    if (!data_block_definition.at("items").is_array())
        throw runtime_error ("data block construction with items non-array");

    std::string item_name;
    ItemParserBase* item {nullptr};

    for (const json& data_item_it : data_block_definition.at("items"))
    {
        item_name = data_item_it.at("name");

        if (debug)
            loginf << "asterix parser constructing data block item '" << item_name << "'" << logendl;

        item = ItemParserBase::createItemParser(data_item_it);
        assert (item);
        data_block_items_.push_back(std::unique_ptr<ItemParserBase>{item});
    }

    // asterix recird definitions

    for (auto& ast_cat_def_it : asterix_category_definitions)
    {
        if (debug)
            loginf << "asterix parser constructing cat '" << setfill('0') << setw(3) << ast_cat_def_it.first << "'"
                   << logendl;

        records_.insert(
                    std::pair<unsigned int, std::shared_ptr<Record>>
                    (ast_cat_def_it.first, std::shared_ptr<Record>{ast_cat_def_it.second->record()}));

        //        if (ast_cat_def_it.second->hasCurrentMapping())
        //        {
        //            mappings_.insert(
        //                        std::pair<unsigned int, std::shared_ptr<Mapping>>
        //                        (ast_cat_def_it.first, std::shared_ptr<Mapping>{ast_cat_def_it.second->record()}));
        //        }
    }
}

std::tuple<size_t, size_t, bool> ASTERIXParser::findDataBlocks (const char* data, size_t index, size_t length,
                                                         nlohmann::json& target, bool debug)
{
    size_t parsed_bytes {0};
    size_t parsed_data_block_bytes {0};
    size_t parsed_bytes_sum {0};
    size_t num_blocks {0};

    //loginf << "UGA target '" << target.dump(4) << "' parsed " << parsed_bytes_sum << " size " << length << logendl;

    bool record_limit_hit {false};
    size_t current_index {index};

    while (current_index < length)
    {
        if (record_chunk_size > 0 && num_blocks >= static_cast<size_t> (record_chunk_size))
        {
            record_limit_hit = true;
            break;
        }

        parsed_data_block_bytes = 0;

        for (auto& r_item : data_block_items_)
        {
            //loginf << "UGA1 index " << current_index << " pb " << parsed_data_block_bytes << logendl;
            parsed_bytes = r_item->parseItem(data, current_index, length, parsed_data_block_bytes,
                                              target[data_block_name_][num_blocks], debug);
            parsed_data_block_bytes += parsed_bytes;
            parsed_bytes_sum += parsed_bytes;
            current_index += parsed_bytes;
        }

        //loginf << "UGA2 target block '" << target[data_block_name_][num_blocks].dump(4) << "'" << logendl;

        ++num_blocks;
     }


    //loginf << "UGA3 target '" << target.dump(4) << "' parsed " << parsed_bytes_sum << " size " << length << logendl;

    //    {
    //        "cnt": 0,
    //        "content": {
    //            "index": 134,
    //            "length": 56,
    //            "record": {
    //                "category": 1,
    //                "content": {
    //                    "index": 137,
    //                    "length": 42
    //                },
    //                "length": 45
    //            }
    //        },
    //        "frame_length": 56,
    //        "frame_relative_time_ms": 2117
    //    }

    return std::make_tuple (parsed_bytes_sum, num_blocks, !record_limit_hit);
}

size_t ASTERIXParser::decodeDataBlocks (const char* data, nlohmann::json& data_blocks, bool debug)
{
    size_t num_records_sum {0};

    if (!data_blocks.is_array())
        throw runtime_error("asterix parser data blocks is not an array");

    size_t num_data_blocks = data_blocks.size();

    std::vector<size_t> num_records;
    num_records.resize(num_data_blocks, 0);

    tbb::parallel_for( size_t(0), num_data_blocks, [&]( size_t cnt )
    {
        num_records.at(cnt) = decodeDataBlock (data, data_blocks[cnt], debug);
    });

    for (auto num_record_it : num_records)
        num_records_sum += num_record_it;

    return num_records_sum;
}

size_t ASTERIXParser::decodeDataBlock (const char* data, nlohmann::json& data_block, bool debug)
{
    size_t parsed_bytes {0}; // TODO unsure if used
    size_t num_records {0};

    // check record information
    //json& record = target.at(data_block_name_);

    //        {
    //            "category": 1,
    //            "content": {
    //                "index": 3265,
    //                "length": 52
    //            },
    //            "length": 55
    //        }

    if (debug && data_block.find ("category") == data_block.end())
        throw runtime_error("asterix parser data block does not contain category information");

    unsigned int cat = data_block.at("category");

    if (debug && data_block.find ("content") == data_block.end())
        throw runtime_error("asterix parser data block does not contain content information");

    json& data_block_content = data_block.at("content");

    if (debug && data_block_content.find ("index") == data_block_content.end())
        throw runtime_error("asterix parser record content does not contain index information");

    size_t record_index = data_block_content.at("index");

    if (debug && data_block_content.find ("length") == data_block_content.end())
        throw runtime_error("asterix parser record content does not contain length information");

    size_t record_length = data_block_content.at("length");

    // try to decode
    if (records_.count(cat) != 0)
    {
        // decode
        if (debug)
            loginf << "asterix parser decoding record with cat " << cat << " index " << record_index
                   << " length " << record_length << logendl;

        size_t parsed_bytes_record {0};

        data_block_content["records"] = json::array();

        // create records until end of content
        while (parsed_bytes_record < record_length)
        {
            //loginf << "asterix parser decoding record " << cnt << " parsed bytes " << parsed_bytes_record << " length " << record_length;

            parsed_bytes_record += records_.at(cat)->parseItem(
                        data, record_index+parsed_bytes_record, record_length-parsed_bytes_record,
                        parsed_bytes+parsed_bytes_record,
                        data_block_content.at("records")[num_records], debug);

            if (debug)
                loginf << "asterix parser decoding record with cat " << cat << " index " << record_index
                       << ": " << data_block_content.at("records")[num_records].dump(4) << "'" << logendl;
            ++num_records;
        }
    }
    else if (debug)
        loginf << "asterix parser decoding record with cat " << cat << " index " << record_index
               << " length " << record_length << " skipped since cat definition is missing " << logendl;

    if (num_records && mappings_.count(cat))
    {
        if (debug)
            loginf << "asterix parser decoding mapping cat " << cat << ", num records " << num_records << logendl;

        std::shared_ptr<Mapping> current_mapping = mappings_.at(cat);
        json& mapping_src = data_block_content.at("records");
        json mapping_dest = json::array();

        for (size_t cnt=0; cnt < num_records; ++cnt)
            current_mapping->map(mapping_src[cnt], mapping_dest[cnt]);

        data_block_content["records"] = std::move(mapping_dest);

    }

    //loginf << "UGA " << num_records << logendl;

    return num_records;
}

//size_t ASTERIXParser::decodeDataBlock (const char* data, unsigned int cat, size_t index, size_t size,
//                                       nlohmann::json& target, bool debug)
//{

//}

}
