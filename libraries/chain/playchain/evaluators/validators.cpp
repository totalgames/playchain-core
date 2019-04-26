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

#include <playchain/chain/evaluators/validators.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>

#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/pending_buy_in_object.hpp>

#include <playchain/chain/sign_utils.hpp>
#include <graphene/chain/protocol/sign_state.hpp>

namespace playchain { namespace chain {

    bool is_account_exists(const database& d, const account_id_type &account)
    {
        return nullptr != d.find(account);
    }

    bool is_player_exists(const database& d, const account_id_type &account)
    {
        const auto& idx = d.get_index_type<player_index>().indices().get<by_playchain_account>();
        return idx.end() != idx.find(account);
    }

    bool is_player_exists(const database& d, const player_id_type &player)
    {
        return nullptr != d.find(player);
    }

    bool is_room_exists(const database& d, const room_id_type &room)
    {
        return nullptr != d.find(room);
    }

    bool is_game_witness_exists(const database& d, const game_witness_id_type &witness)
    {
        return nullptr != d.find(witness);
    }

    bool is_table_exists(const database& d, const table_id_type &table)
    {
        return nullptr != d.find(table);
    }

    bool is_table_voting(const database& d, const table_id_type &table)
    {
        const auto& idx = d.get_index_type<table_voting_index>().indices().get<by_table>();
        return idx.end() != idx.find(table);
    }

    bool is_table_voting_for_playing(const database& d, const table_id_type &table)
    {
        const auto& idx = d.get_index_type<table_voting_index>().indices().get<by_table>();
        auto pobj = idx.find(table);
        if (idx.end() == pobj)
            return false;

        return pobj->is_voting_for_playing();
    }

    bool is_table_voting_for_results(const database& d, const table_id_type &table)
    {
        const auto& idx = d.get_index_type<table_voting_index>().indices().get<by_table>();
        auto pobj = idx.find(table);
        if (idx.end() == pobj)
            return false;

        return pobj->is_voting_for_results();
    }

    void check_authority(const database& d, const account_id_type &account, const digest_type &digest, const signature_type &mandat)
    {
        public_key_type pk = utils::get_signature_key(mandat, digest);

        auto get_active = [&](
           const account_id_type &id
           ) -> const authority*
        {
           return &(id(d).active);
        } ;

        auto get_owner = [&](
           const account_id_type &id
           ) -> const authority*
        {
           return &(id(d).owner);
        } ;

        sign_state s({pk}, get_active);
        s.max_recursion = 0; //not allow guarantors

        FC_ASSERT( s.check_authority(account) || s.check_authority(get_owner(account)),
                   "Missing inviter authority for '${name}'. Client Code = 20180007", ("name",account(d).name) );
    }

    bool is_room_owner(const database& d, const account_id_type &table_owner)
    {
        const auto& rooms_by_owner = d.get_index_type<room_index>().indices().get<by_room_owner>();
        auto rooms_range = rooms_by_owner.equal_range(table_owner);
        return rooms_range.first != rooms_range.second;
    }

    bool is_room_owner(const database& d, const account_id_type &account, const room_id_type &room)
    {
        return room(d).owner == account;
    }

    bool is_table_owner(const database& d, const account_id_type &account, const table_id_type &table)
    {
        return table(d).room(d).owner == account;
    }

    bool is_game_witness(const database& d, const account_id_type &account)
    {
        const auto& witness_by_account = d.get_index_type<game_witness_index>().indices().get<by_playchain_account>();
        return witness_by_account.end() != witness_by_account.find(account);
    }

    bool is_pending_buy_in_exists(const database& d, const pending_buy_in_id_type &buy_in)
    {
        return nullptr != d.find(buy_in);
    }
}}
