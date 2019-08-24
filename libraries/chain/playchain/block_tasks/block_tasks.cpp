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

#include <playchain/chain/block_tasks.hpp>
#include <graphene/chain/database.hpp>

#include "player_invitation.hpp"
#include "table_check.hpp"
#include "pending_buy_in.hpp"

#include <graphene/chain/hardfork.hpp>

#if defined(LOG_VOTING)
#include <playchain/chain/evaluators/db_helpers.hpp>

#include <playchain/chain/schema/pending_buy_in_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#endif

namespace playchain { namespace chain {

void process_block_tasks(database &d, const bool maintenance)
{
#if defined(LOG_VOTING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
    {
        const auto& bin_by_table = d.get_index_type<buy_in_index>().indices().get<by_buy_in_table>();
        auto table_id = table_id_type{1};
        auto range = bin_by_table.equal_range(table_id);
        if (range.first != range.second)
        {
            ilog("${t} >> process_block_tasks: table = ${id}:", ("t", d.head_block_time())("id", table_id));
            print_objects_in_range(range.first, range.second, "buy_in_object(s)");
        }

        table_id = table_id_type{6};
        range = bin_by_table.equal_range(table_id);
        if (range.first != range.second)
        {
            ilog("${t} >> process_block_tasks: table = ${id}:", ("t", d.head_block_time())("id", table_id));
            print_objects_in_range(range.first, range.second, "buy_in_object(s)");
        }

        table_id = table_id_type{26};
        range = bin_by_table.equal_range(table_id);
        if (range.first != range.second)
        {
            ilog("${t} >> process_block_tasks: table = ${id}:", ("t", d.head_block_time())("id", table_id));
            print_objects_in_range(range.first, range.second, "buy_in_object(s)");
        }

        if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_TO ))
        {
            wlog("STOP at ${t}", ("t", d.head_block_time().to_iso_string()));
            assert(false);
        }
    }
#endif

    update_expired_invitations(d);
    if (d.head_block_time() < HARDFORK_PLAYCHAIN_9_TIME)
    {
        update_expired_table_voting(d);
        update_expired_table_game(d, maintenance);
        update_expired_pending_buy_in(d);
        update_expired_buy_in(d);
        update_expired_table_alive(d);
        update_scheduled_voting(d);
    }else
    {
        update_scheduled_voting(d);
        update_expired_table_voting(d);
        update_expired_table_game(d, maintenance);
        update_expired_pending_buy_in(d);
        update_expired_buy_in(d);
        update_expired_table_alive(d);
    }

    allocation_of_vacancies(d);
}

}}
