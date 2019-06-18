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

#include "pending_buy_in.hpp"

#include <graphene/chain/database.hpp>

#include <playchain/chain/schema/pending_buy_in_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/room_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

#include <playchain/chain/evaluators/buy_in_buy_out_evaluators.hpp>

#include <graphene/chain/hardfork.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

#include <limits>
#include <sstream>
#include <iostream>

namespace playchain { namespace chain {

namespace
{
    pending_buy_in_id_type allocate_table(database &d, const pending_buy_in_object &buy_in, const table_object &table)
    {
        const player_object &player = get_player(d, buy_in.player);

        pending_buy_in_id_type prev_proposal = PLAYCHAIN_NULL_PENDING_BUYIN;

        d.modify(table, [&](table_object &obj)
        {
            prev_proposal = obj.add_pending_proposal(player.id, buy_in.id);
        });

        const auto &room = table.room(d);

        d.push_applied_operation( buy_in_reserving_allocated_table_operation{ buy_in.player, buy_in.uid, buy_in.amount,
                                                           buy_in.metadata, table.id, room.owner, table.weight, room.rating} );
        d.modify(buy_in, [&](pending_buy_in_object &obj)
        {
            obj.table = table.id;
        });

        return prev_proposal;
    }

    void push_pending_buy_expire_operation(database &d, const pending_buy_in_object &buy_in,
                               bool expire_by_replacement)
    {
        if (PLAYCHAIN_NULL_TABLE != buy_in.table)
        {
            d.push_applied_operation( buy_in_reserving_expire_operation{ buy_in.player,
                                                      buy_in.uid, buy_in.amount,
                                                      buy_in.metadata,
                                                      expire_by_replacement,
                                                      buy_in.table,
                                                      buy_in.table(d).room(d).owner} );
        }else
        {
            d.push_applied_operation( buy_in_reserving_expire_operation{ buy_in.player,
                                                      buy_in.uid, buy_in.amount,
                                                      buy_in.metadata,
                                                      expire_by_replacement} );
        }
    }

    template<typename Iterator>
    void print_tables_range(const Iterator &range_first, const Iterator &range_second, const string &preffix = "")
#ifdef NDEBUG
    {
        //SLOW OP. SKIPPED
    }
#else
    {
#if defined(PRINT_PLAYCHAIN_TABLES_RANGE)
        auto itr = range_first;
        std::vector<string> table_list;

        size_t ci = 0;
        while(itr != range_second)
        {
            const auto &table = *itr++;

            std::stringstream ss;

            if (ci > 0)
                ss << ", ";
            ss << '"' << ci++ << '"' << ": ";
            ss << fc::json::to_string( table.to_variant() );

            table_list.emplace_back(ss.str());
        }

        if (!table_list.empty())
        {
            std::stringstream ss;

            ss << preffix << " >> ";

            for(auto &&r: table_list)
            {
                ss << r;
            }

            std::cerr << ss.str() << std::endl;
        }
#endif //PRINT_PLAYCHAIN_TABLES_RANGE
    }
#endif //DEBUG
}

void update_expired_pending_buy_in(database &d)
{
    auto& by_expiration= d.get_index_type<pending_buy_in_index>().indices().get<by_playchain_obj_expiration>();
    while( !by_expiration.empty() && by_expiration.begin()->expiration <= d.head_block_time() )
    {
        const pending_buy_in_object &buy_in = *by_expiration.begin();

        if (buy_in.is_allocated())
        {
            const table_object& table = buy_in.table(d);
            const player_object &player = get_player(d, buy_in.player);

            assert(table.pending_proposals.find(player.id) != table.pending_proposals.end());

            d.modify(table, [&](table_object &obj)
            {
                obj.remove_pending_proposal(player.id);
            });
        }

        d.adjust_balance(buy_in.player, buy_in.amount);

        push_pending_buy_expire_operation(d, buy_in, false);
        d.remove(buy_in);
    }
}

template <typename Index>
auto create_range_v1(const Index& index,
              const string& search_meta,
              const uint32_t min_occupied_places,
              const uint32_t max_occupied_places,
              const int32_t min_allowed_table_weight_to_be_allocated)
{
    static const auto min_table_id = object_id_type{table_object::space_id, table_object::type_id, 0};
    static const auto max_table_id = object_id_type{table_object::space_id, table_object::type_id, std::numeric_limits<uint32_t>::max()};

    return index.range(::boost::lambda::_1 >= std::make_tuple(search_meta, min_occupied_places,
                                                             min_allowed_table_weight_to_be_allocated, min_table_id),
                       ::boost::lambda::_1 <= std::make_tuple(search_meta, max_occupied_places,
                                                             std::numeric_limits<int32_t>::max(), max_table_id));
}

template <typename Index>
auto create_range_v2(const Index& index,
              const string& search_meta,
              const uint32_t min_occupied_places,
              const uint32_t max_occupied_places,
              const int32_t /*min_allowed_table_weight_to_be_allocated*/)
{
    return index.range(::boost::lambda::_1 >= std::make_tuple(search_meta, min_occupied_places),
                       ::boost::lambda::_1 <= std::make_tuple(search_meta, max_occupied_places));
}

template <typename Index>
auto create_range(const database &d,
                  const Index& index,
                  const string& search_meta,
                  const uint32_t min_occupied_places,
                  const uint32_t max_occupied_places,
                  const int32_t min_allowed_table_weight_to_be_allocated)
{
    if (d.head_block_time() >= HARDFORK_PLAYCHAIN_4_TIME)
    {
        return create_range_v2(index, search_meta, min_occupied_places, max_occupied_places, min_allowed_table_weight_to_be_allocated);
    }else
    {
        return create_range_v1(index, search_meta, min_occupied_places, max_occupied_places, min_allowed_table_weight_to_be_allocated);
    }
}

template<typename Range>
bool find_in_range(const Range &range, database &d, const pending_buy_in_object &buy_in,
                   const int32_t min_allowed_table_weight_to_be_allocated,
                   flat_set<pending_buy_in_id_type> &prev_proposals)
{
    auto itr = range.first;

    bool found = false;
    while(itr != range.second)
    {
        const auto &table = *itr++;

        if (table.weight < min_allowed_table_weight_to_be_allocated)
        {
            continue;
        }

        // if versions do not match by algorithm (maj.min.XXXX)
        if (table.room(d).protocol_version != buy_in.protocol_version)
        {
            continue;
        }

        // if player is table owner
        if (table.room(d).owner == buy_in.player)
        {
            continue;
        }

        // if min_accepted_proposal_asset is not satisfying
        if (table.min_accepted_proposal_asset.asset_id != buy_in.amount.asset_id ||
            table.min_accepted_proposal_asset > buy_in.amount)
        {
            continue;
        }

        // do not allocate more then once
        if (table.is_waiting_at_table(buy_in.player_iternal) ||
            table.is_playing(buy_in.player_iternal))
        {
            continue;
        }

        pending_buy_in_id_type prev_proposal = allocate_table(d, buy_in, table);
        if (prev_proposal != PLAYCHAIN_NULL_PENDING_BUYIN)
            prev_proposals.emplace(prev_proposal);
        found = true;
        break;
    }

    return found;
}

void allocation_of_vacancies(database &d)
{
    const auto& parameters = get_playchain_parameters(d);
    auto& by_expiration= d.get_index_type<pending_buy_in_index>().indices().get<by_pending_buy_in_allocation_status_and_expiration>();
    auto itr_buy_in = by_expiration.begin();
    size_t ci = 0;
    flat_set<pending_buy_in_id_type> prev_proposals;
    while( itr_buy_in != by_expiration.end() &&
           !itr_buy_in->is_allocated() &&
           itr_buy_in->expiration > d.head_block_time() &&
           ci++ < parameters.pending_buy_in_allocate_per_block)
    {
        const pending_buy_in_object &buy_in = *itr_buy_in++;

        if (buy_in.id == PLAYCHAIN_NULL_PENDING_BUYIN)
            continue;

        const auto& tables_by_free_places = d.get_index_type<table_index>().indices().get<by_table_choose_algorithm>();

        bool lookup_out_range = false;
        bool last_loop = false;
        do
        {
            auto reachable_minimum = parameters.minimum_desired_number_of_players_for_tables_allocation;
            auto reachable_maximum = parameters.maximum_desired_number_of_players_for_tables_allocation;
            --reachable_maximum;

            if (!lookup_out_range)
            {
                auto range = create_range(d, tables_by_free_places, buy_in.metadata,
                                          reachable_minimum,
                                          reachable_maximum, parameters.min_allowed_table_weight_to_be_allocated);
                print_tables_range(range.first, range.second, "first");
                if (range.first != range.second)
                {
                    lookup_out_range = !find_in_range(range, d, buy_in, parameters.min_allowed_table_weight_to_be_allocated, prev_proposals);

                    continue;
                }else
                {
                    reachable_minimum = 0;
                }
            }else
            {
                reachable_minimum = 0;
                last_loop = true;
            }

            auto range = create_range(d, tables_by_free_places, buy_in.metadata,
                                 reachable_minimum,
                                 reachable_maximum, parameters.min_allowed_table_weight_to_be_allocated);
            print_tables_range(range.first, range.second);

            lookup_out_range = !find_in_range(range, d, buy_in, parameters.min_allowed_table_weight_to_be_allocated, prev_proposals);

        } while(lookup_out_range && !last_loop);
    }

    for(const auto &id: prev_proposals)
    {
        const pending_buy_in_object &buy_in = id(d);

        d.adjust_balance(buy_in.player, buy_in.amount);

        push_pending_buy_expire_operation(d, buy_in, true);
        d.remove(buy_in);
    }
}

void update_expired_buy_in(database &d)
{
    auto& by_expiration= d.get_index_type<buy_in_index>().indices().get<by_playchain_obj_expiration>();
    while( !by_expiration.empty() && by_expiration.begin()->expiration <= d.head_block_time() )
    {
        const buy_in_object &buy_in = *by_expiration.begin();
        const table_object &table = buy_in.table(d);
        if (table.is_playing(buy_in.player))
        {
            prolong_life_for_by_in(d, buy_in);

            continue;
        }else
        {
            expire_buy_in(d, buy_in, table);
        }
    }
}
}}
