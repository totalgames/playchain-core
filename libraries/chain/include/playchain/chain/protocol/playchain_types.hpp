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

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <playchain/chain/playchain_config.hpp>

#include <boost/container/flat_set.hpp>

#include <playchain/chain/version.hpp>

#include <functional>

namespace playchain { namespace chain {

struct by_playchain_account;
struct by_playchain_obj_expiration;

using game_witnesses_type = flat_set<game_witness_id_type>;
using game_accounts_type = flat_set<account_id_type>;

template<typename T>
struct greater_for_size :
        public std::binary_function<T, T, bool>
{
    bool operator()(const T& l, const T& r) const
    {
        return l.size() > r.size();
    }
};

template<typename T>
struct less_for_size :
        public std::binary_function<T, T, bool>
{
    bool operator()(const T& l, const T& r) const
    {
        return l.size() < r.size();
    }
};

using namespace graphene::chain;

struct game_initial_data
{
    flat_map<account_id_type, asset>  cash;

    ///who is the diller and etc.
    ///    info must include all game information (include cash)
    ///    or you should fix fraud_game_start_playing_check
    string                            info;
};

struct gamer_cash_result
{
    ///gamer funds at the end of the game
    asset          cash;
    ///payment for the game
    asset          rake;
};

struct game_result
{
    flat_map<account_id_type, gamer_cash_result>  cash;

    ///hands, showdown and etc.
    ///    log must include all game information (include cash)
    ///    or you should fix fraud_game_result_check
    string                                        log;
};

using voting_data_type = fc::static_variant<game_initial_data,
                                            game_result>;

}}

FC_REFLECT( playchain::chain::game_initial_data, (cash)(info))
FC_REFLECT( playchain::chain::gamer_cash_result, (cash)(rake))
FC_REFLECT( playchain::chain::game_result, (cash)(log))

FC_REFLECT_TYPENAME( playchain::chain::voting_data_type )
