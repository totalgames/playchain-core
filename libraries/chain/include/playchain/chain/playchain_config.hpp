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

#include <graphene/chain/config.hpp>

#include <fc/time.hpp>
#include <memory>
#include <playchain/chain/version.hpp>

//use 0.0.1 after genesis restart
//and 0.x.x after harforks (when hard forks selection mechanism will be created ...)
#define PLAYCHAIN_VERSION       ( version(1, 0, 0) )

#define PLAYCHAIN_SYMBOL         GRAPHENE_SYMBOL
#define PLAYCHAIN_ADDRESS_PREFIX GRAPHENE_ADDRESS_PREFIX

#define PLAYCHAIN_DEFAULT_TX_EXPIRATION_PERIOD fc::seconds(30)

#define PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD fc::minutes(5)
#define PLAYCHAIN_MAXIMUM_INVITATION_EXPIRATION_PERIOD fc::days(30)

#define PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST    100

#define PLAYCHAIN_MAXIMUM_BUYOUT_REASON_SIZE      255

#define PLAYCHAIN_NULL_PLAYER (playchain::chain::player_id_type(0))
#define PLAYCHAIN_NULL_GAME_WITNESS (playchain::chain::game_witness_id_type(0))
#define PLAYCHAIN_NULL_ROOM (playchain::chain::room_id_type(0))
#define PLAYCHAIN_NULL_TABLE (playchain::chain::table_id_type(0))
#define PLAYCHAIN_NULL_PENDING_BUYIN (playchain::chain::pending_buy_in_id_type(0))

#define PLAYCHAIN_COMMITTEE_ACCOUNT GRAPHENE_RELAXED_COMMITTEE_ACCOUNT

#define PLAYCHAIN_DEFAULT_REFERRER_PERCENT_OF_FEE   (75*GRAPHENE_1_PERCENT)
#define PLAYCHAIN_DEFAULT_REFERRER_PARENT_PERCENT_OF_FEE (667) //6.(6) % for GRAPHENE_100_PERCENT == 10000
#define PLAYCHAIN_DEFAULT_REFERRER_BALANCE_MIN_THRESHOLD    10*GRAPHENE_BLOCKCHAIN_PRECISION
#define PLAYCHAIN_DEFAULT_REFERRER_BALANCE_MAX_THRESHOLD    100*GRAPHENE_BLOCKCHAIN_PRECISION
#define PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE   (10*GRAPHENE_1_PERCENT)

#define PLAYCHAIN_DEFAULT_TAKE_INTO_ACCOUNT_GRAPHENE_BALANCES  (false)

//if 0 then one player can be substituted by witness
// (total required N = {players at the table amount} + {this one})
#define PLAYCHAIN_DEFAULT_VOTING_FOR_PLAYING_VOTERS_AMOUNT_DELTA   (0)
#define PLAYCHAIN_DEFAULT_VOTING_FOR_RESULTS_VOTERS_AMOUNT_DELTA   (0)

//at least not more then 1 cheater from 3 is allowed for consensus
#define PLAYCHAIN_DEFAULT_VOTING_FOR_PLAYING_REQUIED_PERCENT   (60*GRAPHENE_1_PERCENT)
#define PLAYCHAIN_DEFAULT_VOTING_FOR_RESULTS_REQUIED_PERCENT   (60*GRAPHENE_1_PERCENT)

#define PLAYCHAIN_DEFAULT_PENDING_BUY_IN_ALLOCATE_PER_BLOCK (300)
#define PLAYCHAIN_DEFAULT_ROOMS_RATING_RECALCULATIONS_PER_MAINTENANCE (1000)
#define PLAYCHAIN_DEFAULT_TABLES_WEIGHT_RECALCULATIONS_PER_MAINTENANCE (1000)
#define PLAYCHAIN_DEFAULT_MIN_ALLOWED_TABLE_WEIGHT_TO_BE_ALLOCATED (0)

#define PLAYCHAIN_DEFAULT_PERCENTAGE_OF_VOTER_WITNESS_SUBSTITUTION_WHILE_VOTING_FOR_PLAYING (0)
#define PLAYCHAIN_DEFAULT_PERCENTAGE_OF_VOTER_WITNESS_SUBSTITUTION_WHILE_VOTING_FOR_RESULTS (50*GRAPHENE_1_PERCENT)

namespace playchain { namespace protocol { namespace detail {

struct config
{
    static std::unique_ptr<config> instance;

    const uint32_t blockid_pool_size;

    const int maximum_inviters_depth;

    const uint32_t voting_for_playing_expiration_seconds;
    const uint32_t voting_for_results_expiration_seconds;
    const uint32_t game_lifetime_limit_in_seconds;
    const uint32_t pending_buyin_proposal_lifetime_limit_in_seconds;
    const uint32_t amount_reserving_places_per_user;
    const uint32_t min_votes_for_results;
    const uint32_t minimum_desired_number_of_players_for_tables_allocation;
    const uint32_t maximum_desired_number_of_players_for_tables_allocation;
    const uint32_t buy_in_expiration_seconds;

    enum test_mode { test };

    explicit config(test_mode);
    config();
};

const config& get_config();

void override_config(std::unique_ptr<config> new_config);

}}}

#define PLAYCHAIN_BLOCKID_POOL_SIZE (playchain::protocol::detail::get_config().blockid_pool_size)
#define PLAYCHAIN_MAXIMUM_INVITERS_DEPTH (playchain::protocol::detail::get_config().maximum_inviters_depth)
#define PLAYCHAIN_VOTING_FOR_PLAYING_EXPIRATION_SECONDS (playchain::protocol::detail::get_config().voting_for_playing_expiration_seconds)
#define PLAYCHAIN_VOTING_FOR_RESULTS_EXPIRATION_SECONDS (playchain::protocol::detail::get_config().voting_for_results_expiration_seconds)
#define PLAYCHAIN_DEFAULT_GAME_LIFETIME_LIMIT_IN_SECONDS (playchain::protocol::detail::get_config().game_lifetime_limit_in_seconds)
#define PLAYCHAIN_DEFAULT_PENDING_BUYIN_PROPOSAL_LIFETIME_LIMIT_IN_SECONDS (playchain::protocol::detail::get_config().pending_buyin_proposal_lifetime_limit_in_seconds)
#define PLAYCHAIN_DEFAULT_AMOUNT_RESERVING_PLACES_PER_USER (playchain::protocol::detail::get_config().amount_reserving_places_per_user)
#define PLAYCHAIN_DEFAULT_MIN_VOTES_FOR_RESULTS (playchain::protocol::detail::get_config().min_votes_for_results)
#define PLAYCHAIN_DEFAULT_MINIMUM_DESIRED_NUMBER_OF_PLAYERS_FOR_TABLES_ALLOCATION (playchain::protocol::detail::get_config().minimum_desired_number_of_players_for_tables_allocation)
#define PLAYCHAIN_DEFAULT_MAXIMUM_DESIRED_NUMBER_OF_PLAYERS_FOR_TABLES_ALLOCATION (playchain::protocol::detail::get_config().maximum_desired_number_of_players_for_tables_allocation)
#define PLAYCHAIN_DEFAULT_BUY_IN_EXPIRATION_SECONDS (playchain::protocol::detail::get_config().buy_in_expiration_seconds)

