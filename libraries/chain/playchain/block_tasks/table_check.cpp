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

#include "table_check.hpp"

#include <graphene/chain/database.hpp>

#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/room_object.hpp>

#include <playchain/chain/evaluators/game_evaluators.hpp>
#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/validators.hpp>

namespace playchain { namespace chain {

void update_expired_table_voting(database &d)
{
   auto& voting_by_expiration= d.get_index_type<table_voting_index>().indices().get<by_table_expiration>();
   while( !voting_by_expiration.empty() && voting_by_expiration.begin()->expiration <= d.head_block_time() )
   {
       const table_voting_object &voting = *voting_by_expiration.begin();
       const table_object &table = voting.table(d);

       if (voting.is_voting_for_playing())
       {
           expire_voting_for_playing(d, voting, table);
       }else if (voting.is_voting_for_results())
       {
           expire_voting_for_results(d, voting, table);
       }
   }
}

void update_expired_table_game(database &d, const bool maintenance)
{
    const auto& props = d.get_global_properties();
    const auto& playchain_parameters = get_playchain_parameters(d);
    //if GLT period is large enough to do accuracy less then 10%
    //move posible hard rollback_table(s) to maintenance loop
    if (playchain_parameters.game_lifetime_limit_in_seconds > props.parameters.maintenance_interval * 10 &&
        !maintenance)
        return;

    auto& game_by_expiration= d.get_index_type<table_index>().indices().get<by_playchain_obj_expiration>();
    auto itr = game_by_expiration.begin();
    while( itr != game_by_expiration.end() && itr->game_expiration <= d.head_block_time() )
    {
        const table_object &table = *itr++;

        d.push_applied_operation(
                    game_event_operation{ table.id, table.room(d).owner, fail_expire_game_lifetime{} } );

        rollback_table(d, table);
    }
}

void update_expired_table_alive(database &d)
{
    auto& alive_by_expiration= d.get_index_type<table_alive_index>().indices().get<by_table_expiration>();
    while( !alive_by_expiration.empty() && alive_by_expiration.begin()->expiration <= d.head_block_time() )
    {
        const table_alive_object &alive = *alive_by_expiration.begin();

        assert(is_table_exists(d, alive.table));

        const table_object &table = alive.table(d);

        d.remove(alive);

        table.set_weight(d);
    }
}

} }
