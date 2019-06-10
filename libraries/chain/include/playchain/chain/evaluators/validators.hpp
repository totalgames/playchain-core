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

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;

    bool is_account_exists(const database& d, const account_id_type &account);

    bool is_player_exists(const database& d, const account_id_type &account);

    bool is_player_exists(const database& d, const player_id_type &player);

    bool is_room_exists(const database& d, const room_id_type &room);

    bool is_game_witness_exists(const database& d, const game_witness_id_type &witness);

    bool is_table_exists(const database& d, const table_id_type &table);

    bool is_table_voting(const database& d, const table_id_type &table);

    bool is_table_alive(const database& d, const table_id_type &table);

    bool is_table_voting_for_playing(const database& d, const table_id_type &table);

    bool is_table_voting_for_results(const database& d, const table_id_type &table);

    bool is_room_owner(const database& d, const account_id_type &table_owner);

    bool is_room_owner(const database& d, const account_id_type &account, const room_id_type &room);

    bool is_table_owner(const database& d, const account_id_type &account, const table_id_type &table);

    bool is_game_witness(const database& d, const account_id_type &account);

    void check_authority(const database& d, const account_id_type &account, const digest_type &digest, const signature_type &mandat);

    bool is_pending_buy_in_exists(const database& d, const pending_buy_in_id_type &buy_in);
}}
