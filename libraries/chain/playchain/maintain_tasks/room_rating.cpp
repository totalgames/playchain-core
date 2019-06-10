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
#include <playchain/chain/playchain_utils.hpp>
#include <playchain/chain/evaluators/validators.hpp>

namespace playchain { namespace chain {

namespace
{
    class randomizing_context
    {
    public:
        explicit randomizing_context(const database &d);

        uint16_t next() const;

    private:

        const size_t _MIN = 1;
        const size_t _MAX = 0xfff;

        int64_t _entropy = 0;
        mutable size_t _pos = 0;
    };

    randomizing_context::randomizing_context(const database &d)
    {
        _entropy = utils::get_database_entropy(d);
    }

    uint16_t randomizing_context::next() const
    {
        return (uint16_t)utils::get_pseudo_random(_entropy, _pos++, _MIN, _MAX);
    }

    void recalculate_rating(database &d, const randomizing_context &rnd, const room_object &room)
    {
        const auto& dprops = d.get_dynamic_global_properties();
        auto old_rating = room.rating;

        assert(sizeof(old_rating) == 4);

        auto new_rating = old_rating;

        //(1) randomize to make chance for different rooms with same rating
        new_rating  &= (~0xffff);
        new_rating |= rnd.next();

        // TODO: (2) calculate fraud statistic to decrease rating
        // TODO: (3) use votes from players to increase rating

        d.modify(room, [&](room_object &obj){
            obj.prev_rating = old_rating;
            obj.rating = new_rating;
            obj.last_rating_update = dprops.time;
        });
    }
}

void update_room_rating(database &d)
{
    auto& rooms_by_last_update = d.get_index_type<room_index>().indices().get<by_last_rating_update>();

    if (rooms_by_last_update.size() == 1) //only special room
        return;

    randomizing_context rnd{d};

    const auto& parameters = get_playchain_parameters(d);
    auto itr = rooms_by_last_update.begin();
    size_t ci = 0;
    while( itr != rooms_by_last_update.end() &&
           ci++ < parameters.rooms_rating_recalculations_per_maintenance)
    {
        const room_object &room = *itr++;
        if (room_object::is_special_room(room.id))
            continue;

        if (room.last_updated_table != table_id_type{})
        {
            --ci;
            continue;
        }
        recalculate_rating(d, rnd, room);
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
            continue;

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
                if (table_object::is_special_table(table.id))
                    continue;

                table.set_weight(d);
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
