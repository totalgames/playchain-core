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

#include <playchain/chain/maintain_tasks.hpp>
#include <graphene/chain/database.hpp>

#include "deposit_pending_fees.hpp"
#include "room_rating.hpp"
#include "committee_applying.hpp"

#include <graphene/chain/asset_object.hpp>

#if defined(LOG_RATING)
#include <playchain/chain/evaluators/db_helpers.hpp>

#include <playchain/chain/schema/room_rating_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#endif

namespace playchain { namespace chain {

void process_maintain_tasks(database &d)
{
#if defined(LOG_RATING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_RATING_BLOCK_TIMESTUMP_FROM ))
    {
        {
            const auto& measurements = d.get_index_type<room_rating_kpi_measurement_index>().indices().get<by_room>();
            auto room_id = room_id_type{1};
            auto range = measurements.equal_range(room_id);
            if (range.first != range.second)
            {
                ilog("${t} >> room_rating_kpi_measurements: room = ${id}:", ("t", d.head_block_time())("id", room_id));
                print_objects_in_range(range.first, range.second, "_measurement(s)");
            }

            room_id = room_id_type{16};
            range = measurements.equal_range(room_id);
            if (range.first != range.second)
            {
                ilog("${t} >> room_rating_kpi_measurements: room = ${id}:", ("t", d.head_block_time())("id", room_id));
                print_objects_in_range(range.first, range.second, "_measurement(s)");
            }
        }

        {
            const auto& measurements = d.get_index_type<room_rating_standby_measurement_index>().indices().get<by_room>();
            auto room_id = room_id_type{1};
            auto range = measurements.equal_range(room_id);
            if (range.first != range.second)
            {
                ilog("${t} >> room_rating_standby_measurements: room = ${id}:", ("t", d.head_block_time())("id", room_id));
                print_objects_in_range(range.first, range.second, "_measurement(s)");
            }

            room_id = room_id_type{16};
            range = measurements.equal_range(room_id);
            if (range.first != range.second)
            {
                ilog("${t} >> room_rating_standby_measurements: room = ${id}:", ("t", d.head_block_time())("id", room_id));
                print_objects_in_range(range.first, range.second, "_measurement(s)");
            }
        }

        if (d.head_block_time() >= fc::time_point_sec( LOG_RATING_BLOCK_TIMESTUMP_TO ))
        {
            wlog("STOP at ${t}", ("t", d.head_block_time().to_iso_string()));
            assert(false);
        }
    }
#endif

    const auto &core_asset_dd = asset_dynamic_data_id_type(0)(d);
    if (core_asset_dd.accumulated_fees.value > 0)
    {
        ilog("playchain ($): ${s} = ${cs} + ${accumulated_fees}",
             ("s",  core_asset_dd.current_supply + core_asset_dd.accumulated_fees)
             ("cs",  core_asset_dd.current_supply)
             ("accumulated_fees", core_asset_dd.accumulated_fees));
    }

    deposit_pending_fees(d);

    update_room_rating(d);
    update_table_weight(d);
    clenup_room_rating(d);

    playchain_committee_applying_database_impl ca(d);
    ca.update_active_members();
    ca.update_playchain_parameters();
}

}}
