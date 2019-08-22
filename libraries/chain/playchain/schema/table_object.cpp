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

#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/room_object.hpp>

#include <playchain/chain/evaluators/validators.hpp>
#include <playchain/chain/evaluators/db_helpers.hpp>

#include <graphene/chain/database.hpp>

#include <playchain/chain/schema/pending_buy_in_object.hpp>
#include <playchain/chain/schema/player_object.hpp>

#include <graphene/chain/hardfork.hpp>

#include <algorithm>
#include <limits>

namespace playchain { namespace chain {

bool table_object::is_valid_voter(const database &d, const game_start_playing_check_operation &op) const
{
    FC_ASSERT(is_player_exists(d, op.voter), "Invalid player");

    const auto &player = get_player(d, op.voter);

    return this->is_waiting_at_table(player.id);
}

bool table_object::is_valid_voter(const database &d, const game_result_check_operation &op) const
{
    FC_ASSERT(is_player_exists(d, op.voter), "Invalid player");

    const auto &player = get_player(d, op.voter);

    return this->is_playing(player.id);
}

void table_object::set_weight(database &d) const
{
    if (d.head_block_time() >= HARDFORK_PLAYCHAIN_2_TIME)
    {
        auto new_weight = room(d).rating;

        if (is_table_alive(d, id))
        {
            d.modify(*this, [&new_weight](table_object &obj){
                obj.weight = new_weight;
            });
        }else
        {
            if (new_weight <= 0)
                new_weight = std::numeric_limits<decltype(new_weight)>::min();

            d.modify(*this, [&new_weight](table_object &obj){
                obj.weight = -new_weight;
            });
        }
    }else
    {
        d.modify(*this, [&](table_object &obj){
            obj.weight = 0;
        });
    }
}

void table_object::adjust_cash(const player_id_type &player, asset delta)
{
    auto it = cash.find(player);
    if (cash.end() == it && delta.amount > 0)
    {
        cash[player] += delta;
        if (!playing_cash.count(player) && !pending_proposals.count(player))
        {
            ++occupied_places;
        }
    } else if (cash.end() != it)
    {
        it->second += delta;
        if (it->second.amount < 1)
        {
            cash.erase(it);
            if (!playing_cash.count(player) && !pending_proposals.count(player))
            {
                assert(occupied_places > 0);
                --occupied_places;
            }
        }
    }
}

void table_object::adjust_playing_cash(const player_id_type &player, asset delta)
{
    auto it = playing_cash.find(player);
    if (playing_cash.end() == it && delta.amount > 0)
    {
        playing_cash[player] += delta;
        if (!cash.count(player) && !pending_proposals.count(player))
        {
            ++occupied_places;
        }
    } else if (playing_cash.end() != it)
    {
        it->second += delta;
        if (it->second.amount < 1)
        {
            playing_cash.erase(it);
            if (!cash.count(player) && !pending_proposals.count(player))
            {
                assert(occupied_places > 0);
                --occupied_places;
            }
        }
    }
}

void table_object::clear_cash()
{
    for (const auto &p: cash)
    {
        if (!playing_cash.count(p.first) && !pending_proposals.count(p.first))
        {
            assert(occupied_places > 0);
            --occupied_places;
        }
    }
    cash.clear();
}

void table_object::clear_playing_cash()
{
    for (const auto &p: playing_cash)
    {
        if (!cash.count(p.first) && !pending_proposals.count(p.first))
        {
            assert(occupied_places > 0);
            --occupied_places;
        }
    }
    playing_cash.clear();
}

pending_buy_in_id_type table_object::add_pending_proposal(const player_id_type &player, const pending_buy_in_id_type &pending)
{
    pending_buy_in_id_type prev_proposal = PLAYCHAIN_NULL_PENDING_BUYIN;

    auto it = pending_proposals.find(player);
    if (pending_proposals.end() == it)
    {
        if (!cash.count(player) && !playing_cash.count(player))
        {
            ++occupied_places;
        }
    }
    else
    {
        if (it->second != pending)
            prev_proposal = it->second;
        pending_proposals.erase(it);
    }
    pending_proposals.emplace(std::make_pair(player, pending));
    return prev_proposal;
}

void table_object::remove_pending_proposal(const player_id_type &player)
{
    auto it = pending_proposals.find(player);
    if (it != pending_proposals.end())
    {
        pending_proposals.erase(it);
        if (!cash.count(player) && !playing_cash.count(player))
        {
            assert(occupied_places > 0);
            --occupied_places;
        }
    }
}

asset table_object::get_pending_balance(const database &db, const player_id_type &id) const
{
    auto it = pending_proposals.find(id);
    if (it == pending_proposals.end())
        return {};

    const pending_buy_in_object &obj = it->second(db);
    return obj.amount;
}

asset table_object::get_cash_balance(const player_id_type &id) const
{
    auto it = cash.find(id);
    if (it == cash.end())
        return {};

    return it->second;
}

asset table_object::get_playing_cash_balance(const player_id_type &id) const
{
    auto it = playing_cash.find(id);
    if (it == playing_cash.end())
        return {};

    return it->second;
}

table_owner_index::table_owner_index(const database& db): _db(db)
{}

table_owner_index::~table_owner_index()
{}

void table_owner_index::object_inserted( const object& obj )
{
    assert( dynamic_cast<const table_object*>(&obj) ); // for debug only
    const table_object& table = static_cast<const table_object&>(obj);

    if (PLAYCHAIN_NULL_ROOM == table.room)
        return;

    auto owner = table.room(_db).owner;

    tables_by_owner[owner].insert(table.id);
    rooms_owners[table.room] = owner;
}
void table_owner_index::object_removed( const object& obj )
{
    assert( dynamic_cast<const table_object*>(&obj) ); // for debug only
    const table_object& table = static_cast<const table_object&>(obj);

    if (PLAYCHAIN_NULL_ROOM == table.room)
        return;

    account_id_type owner = GRAPHENE_NULL_ACCOUNT;

    if (is_room_exists(_db, table.room))
    {
        owner = table.room(_db).owner;
    }else if (rooms_owners.count(table.room))
    {
        auto it = rooms_owners.find(table.room);
        owner = it->second;
    }

    //rooms_owners is never clean because rooms obects are not deleted

    if (owner != GRAPHENE_NULL_ACCOUNT)
    {
        auto it = tables_by_owner.find(owner);
        if (tables_by_owner.end() != it)
        {
            tables_type &tables = it->second;
            tables.erase(table.id);
            if (tables.empty())
            {
                tables_by_owner.erase(it);
            }
        }
    }
}

table_players_index::table_players_index(const database& db): _db(db)
{}

table_players_index::~table_players_index()
{}

table_players_index::player_set_type  table_players_index::get_pending_proposals( const table_object& table )const
{
    player_set_type result;
    for( auto item : table.pending_proposals )
       result.emplace(item.first);
    return result;
}

table_players_index::player_set_type table_players_index::get_cash( const table_object& table )const
{
    player_set_type result;
    for( auto item : table.cash )
       result.emplace(item.first);
    return result;
}

table_players_index::player_set_type table_players_index::get_playing_cash( const table_object& table )const
{
    player_set_type result;
    for( auto item : table.playing_cash )
       result.emplace(item.first);
    return result;
}

void table_players_index::about_to_modify( const object& before )
{
    assert( dynamic_cast<const table_object*>(&before) );
    const table_object& table = static_cast<const table_object&>(before);

    before_pending_proposals = get_pending_proposals(table);
    before_cash = get_cash(table);
    before_playing_cash = get_playing_cash(table);
}

void table_players_index::object_modified( const object& after )
{
    assert( dynamic_cast<const table_object*>(&after) );
    const table_object& table = static_cast<const table_object&>(after);

    {
       player_set_type after_players = get_pending_proposals(table);
       vector<player_id_type> removed;
       removed.reserve(before_pending_proposals.size());
       std::set_difference(before_pending_proposals.begin(), before_pending_proposals.end(),
                           after_players.begin(), after_players.end(),
                           std::inserter(removed, removed.end()));

       for( auto itr = removed.begin(); itr != removed.end(); ++itr )
          tables_with_pending_proposals_by_player[*itr].erase(after.id);

       vector<object_id_type> added; added.reserve(after_players.size());
       std::set_difference(after_players.begin(), after_players.end(),
                           before_pending_proposals.begin(), before_pending_proposals.end(),
                           std::inserter(added, added.end()));

       for( auto itr = added.begin(); itr != added.end(); ++itr )
          tables_with_pending_proposals_by_player[*itr].emplace(after.id);
    }

    {
       player_set_type after_players = get_cash(table);
       vector<player_id_type> removed;
       removed.reserve(before_cash.size());
       std::set_difference(before_cash.begin(), before_cash.end(),
                           after_players.begin(), after_players.end(),
                           std::inserter(removed, removed.end()));

       for( auto itr = removed.begin(); itr != removed.end(); ++itr )
          tables_with_cash_by_player[*itr].erase(after.id);

       vector<object_id_type> added; added.reserve(after_players.size());
       std::set_difference(after_players.begin(), after_players.end(),
                           before_cash.begin(), before_cash.end(),
                           std::inserter(added, added.end()));

       for( auto itr = added.begin(); itr != added.end(); ++itr )
          tables_with_cash_by_player[*itr].emplace(after.id);
    }

    {
       player_set_type after_players = get_playing_cash(table);
       vector<player_id_type> removed;
       removed.reserve(before_playing_cash.size());
       std::set_difference(before_playing_cash.begin(), before_playing_cash.end(),
                           after_players.begin(), after_players.end(),
                           std::inserter(removed, removed.end()));

       for( auto itr = removed.begin(); itr != removed.end(); ++itr )
          tables_with_playing_cash_by_player[*itr].erase(after.id);

       vector<object_id_type> added; added.reserve(after_players.size());
       std::set_difference(after_players.begin(), after_players.end(),
                           before_playing_cash.begin(), before_playing_cash.end(),
                           std::inserter(added, added.end()));

       for( auto itr = added.begin(); itr != added.end(); ++itr )
          tables_with_playing_cash_by_player[*itr].emplace(after.id);
    }
}

table_voting_statistics_index::table_voting_statistics_index(const database& db): _db(db)
{}

table_voting_statistics_index::~table_voting_statistics_index()
{}

void table_voting_statistics_index::object_inserted( const object& obj )
{
    assert( dynamic_cast<const table_voting_object*>(&obj) );
    const table_voting_object& table_voting = static_cast<const table_voting_object&>(obj);

    accounts_type votes;
    votes.reserve(table_voting.votes.size());

    std::transform(table_voting.votes.begin(), table_voting.votes.end(),
        std::back_inserter(votes), [](const decltype(table_voting.votes)::value_type &pair){return pair.first;});

    not_voted_last_time_players_by_table[table_voting.table].clear();
    voted_last_time_players_by_table[table_voting.table] = std::move(votes);
}

void table_voting_statistics_index::object_removed( const object& obj )
{
    assert( dynamic_cast<const table_voting_object*>(&obj) );
    const table_voting_object& table_voting = static_cast<const table_voting_object&>(obj);

    accounts_type votes;
    votes.reserve(table_voting.votes.size());

    std::transform(table_voting.votes.begin(), table_voting.votes.end(),
        std::back_inserter(votes), [](const decltype(table_voting.votes)::value_type &pair){return pair.first;});

    accounts_type missed;
    missed.reserve(table_voting.required_player_voters.size());
    std::set_difference(table_voting.required_player_voters.begin(), table_voting.required_player_voters.end(),
                        votes.begin(), votes.end(),
                        std::back_inserter(missed));

    not_voted_last_time_players_by_table[table_voting.table] = std::move(missed);
    voted_last_time_players_by_table[table_voting.table] = std::move(votes);
}
}}
