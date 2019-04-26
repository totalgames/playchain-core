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

#include <playchain/chain/evaluators/db_helpers.hpp>

#include <graphene/chain/database.hpp>

#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/playchain_property_object.hpp>

namespace playchain { namespace chain {

const player_object &get_player(const database& d, const account_id_type &account)
{
    return (*d.get_index_type<player_index>().indices().get<by_playchain_account>().find(account));
}

const game_witness_object &get_game_witness(const database& d, const account_id_type &account)
{
    return (*d.get_index_type<game_witness_index>().indices().get<by_playchain_account>().find(account));
}

const playchain_property_object &get_playchain_properties(const database& d)
{
    return d.get( playchain_property_id_type() );
}

const playchain_parameters  &get_playchain_parameters(const database& d)
{
    return get_playchain_properties( d ).parameters;
}

}}
