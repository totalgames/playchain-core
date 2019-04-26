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

#include "room_rating.hpp"

#include <graphene/chain/database.hpp>

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

namespace playchain { namespace chain {

namespace
{
    void recalculate_rating(database &d, const room_object &room)
    {
        const auto& dprops = d.get_dynamic_global_properties();
        auto old_rating = room.rating;

        //TODO: calculate fraud statistic

        auto new_rating = old_rating;

        d.modify(room, [&](room_object &obj){
            obj.prev_rating = obj.rating;
            obj.rating = new_rating;
            obj.last_rating_update = dprops.time;
        });
    }

    void recalculate_weight(database &d, const table_object &table)
    {
        //invert 'rating' because table 'weight' sorted by ascending (look allocation_of_vacancies)
        auto new_weight = -table.room(d).rating;
        d.modify(table, [&](table_object &obj){
            obj.weight = new_weight;
        });
    }
}

void update_room_rating(database &d)
{
    const auto& parameters = get_playchain_parameters(d);
    auto& rooms_by_last_update = d.get_index_type<room_index>().indices().get<by_last_rating_update>();
    auto itr = rooms_by_last_update.begin();
    size_t ci = 0;
    while( itr != rooms_by_last_update.end() &&
           ci++ < parameters.rooms_rating_recalculations_per_maintenance)
    {
        const room_object &room = *itr++;
        if (room.last_updated_table != table_id_type{})
        {
            --ci;
            continue;
        }
        recalculate_rating(d, room);
    }
}
void update_table_weight(database &d)
{
    const auto& parameters = get_playchain_parameters(d);
    const auto& idx = d.get_index_type<room_index>().indices().get<by_room_rating>();
    using cref_type = std::reference_wrapper<const room_object>;
    std::vector<cref_type> rooms;
    for( const room_object& room : idx )
    {
        if (room.is_not_changed_rating())
            break;

        rooms.emplace_back(std::cref(room));
    }

    if (!rooms.empty())
    {
        auto& tables_by_last_update = d.get_index_type<table_index>().indices().get<by_room>();
        for( const room_object& room : rooms )
        {
            auto last_updated_table = room.last_updated_table;
            auto itr = tables_by_last_update.lower_bound(std::make_tuple(room.id, last_updated_table));
            size_t ci = 0;
            while( itr != tables_by_last_update.end() &&
                   ci++ < parameters.tables_weight_recalculations_per_maintenance)
            {
                const table_object &table = *itr++;
                recalculate_weight(d, table);
                last_updated_table = table.id;
            }
            d.modify(room, [&](room_object &obj){
                if (ci >= parameters.tables_weight_recalculations_per_maintenance)
                {
                    obj.last_updated_table = last_updated_table;
                }
                else
                {
                    obj.last_updated_table = table_id_type{};
                    obj.prev_rating = obj.rating;
                }
            });
        }
    }
}

}}
