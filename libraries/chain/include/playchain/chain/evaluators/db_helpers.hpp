/*
* Copyright (c) 2018 Total Games LLC and contributors.
*
* The MIT License
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#pragma once

#include <playchain/chain/protocol/playchain_types.hpp>
#include <playchain/chain/schema/playchain_property_object.hpp>

#include <functional>
#include <vector>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;

    const player_object &get_player(const database& d, const account_id_type &account);

    const game_witness_object  &get_game_witness(const database& d, const account_id_type &account);

    const playchain_property_object &get_playchain_properties(const database& d);

    const playchain_parameters  &get_playchain_parameters(const database& d);

    //Use this helper to iterate probably removing objects (will call d.remove(o) or break index in some branch of complex logic)
    //Note: This helper protect index but not protect object (double remove)
    template<typename ObjectType, typename IteratorType>
    auto get_objects_from_index(IteratorType &&from, IteratorType &&to, const size_t max_items,
                                std::function<bool (const ObjectType &)> until_f = nullptr)
    {
        using object_type = ObjectType;
        using object_reff_type = std::reference_wrapper<const object_type>;
        std::vector<object_reff_type> result;

        if (max_items > 0)
        {
            result.reserve(max_items);
        }

        for (auto itr = from; itr != to;) {
            const object_type& obj = *itr++;

            if (until_f && !until_f(obj))
                break;

            result.emplace_back(std::cref(obj));
        }

        return result;
    }

    //For DEBUG:
    template<typename Iterator>
    void print_objects_in_range(const Iterator &range_first, const Iterator &range_second, const string &preffix = "")
#ifdef NDEBUG
    {
        //SLOW OP. SKIPPED
    }
#else
    {
        auto itr = range_first;
        std::vector<string> obj_list;

        size_t ci = 0;
        while(itr != range_second)
        {
            const auto &obj = *itr++;

            std::stringstream ss;

            if (ci > 0)
                ss << ", ";
            ss << '"' << ci++ << '"' << ": ";
            ss << fc::json::to_string( obj.to_variant() );

            obj_list.emplace_back(ss.str());
        }

        if (!obj_list.empty())
        {
            std::stringstream ss;

            ss << preffix << " >> ";

            for(auto &&r: obj_list)
            {
                ss << r;
            }

            std::cerr << ss.str() << std::endl;
        }
    }
#endif //DEBUG
}}
