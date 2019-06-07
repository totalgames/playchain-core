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

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <fc/smart_ref_fwd.hpp>

#include <playchain/chain/playchain_config.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

   struct playchain_parameters
   {
      percent_type            player_referrer_percent_of_fee        = PLAYCHAIN_DEFAULT_REFERRER_PERCENT_OF_FEE;
      percent_type            player_referrer_parent_percent_of_fee = PLAYCHAIN_DEFAULT_REFERRER_PARENT_PERCENT_OF_FEE;
      share_type              player_referrer_balance_min_threshold = PLAYCHAIN_DEFAULT_REFERRER_BALANCE_MIN_THRESHOLD;
      share_type              player_referrer_balance_max_threshold = PLAYCHAIN_DEFAULT_REFERRER_BALANCE_MAX_THRESHOLD;
      percent_type            game_witness_percent_of_fee           = PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE;
      bool                    take_into_account_graphene_balances   = PLAYCHAIN_DEFAULT_TAKE_INTO_ACCOUNT_GRAPHENE_BALANCES;
      uint32_t                voting_for_playing_expiration_seconds = PLAYCHAIN_VOTING_FOR_PLAYING_EXPIRATION_SECONDS;
      uint32_t                voting_for_results_expiration_seconds = PLAYCHAIN_VOTING_FOR_RESULTS_EXPIRATION_SECONDS;
      percent_type            voting_for_playing_requied_percent = PLAYCHAIN_DEFAULT_VOTING_FOR_PLAYING_REQUIED_PERCENT;
      percent_type            voting_for_results_requied_percent = PLAYCHAIN_DEFAULT_VOTING_FOR_RESULTS_REQUIED_PERCENT;
      uint32_t                game_lifetime_limit_in_seconds = PLAYCHAIN_DEFAULT_GAME_LIFETIME_LIMIT_IN_SECONDS;
      uint32_t                pending_buyin_proposal_lifetime_limit_in_seconds = PLAYCHAIN_DEFAULT_PENDING_BUYIN_PROPOSAL_LIFETIME_LIMIT_IN_SECONDS;
      uint32_t                amount_reserving_places_per_user = PLAYCHAIN_DEFAULT_AMOUNT_RESERVING_PLACES_PER_USER;
      uint32_t                pending_buy_in_allocate_per_block = PLAYCHAIN_DEFAULT_PENDING_BUY_IN_ALLOCATE_PER_BLOCK;
      uint32_t                rooms_rating_recalculations_per_maintenance = PLAYCHAIN_DEFAULT_ROOMS_RATING_RECALCULATIONS_PER_MAINTENANCE;
      uint32_t                tables_weight_recalculations_per_maintenance = PLAYCHAIN_DEFAULT_TABLES_WEIGHT_RECALCULATIONS_PER_MAINTENANCE;
      int32_t                 min_allowed_table_weight_to_be_allocated = PLAYCHAIN_DEFAULT_MIN_ALLOWED_TABLE_WEIGHT_TO_BE_ALLOCATED;
      uint32_t                minimum_desired_number_of_players_for_tables_allocation = PLAYCHAIN_DEFAULT_MINIMUM_DESIRED_NUMBER_OF_PLAYERS_FOR_TABLES_ALLOCATION;
      uint32_t                maximum_desired_number_of_players_for_tables_allocation = PLAYCHAIN_DEFAULT_MAXIMUM_DESIRED_NUMBER_OF_PLAYERS_FOR_TABLES_ALLOCATION;
      uint32_t                buy_in_expiration_seconds = PLAYCHAIN_DEFAULT_BUY_IN_EXPIRATION_SECONDS;

      percent_type            percentage_of_voter_witness_substitution_while_voting_for_playing = PLAYCHAIN_DEFAULT_PERCENTAGE_OF_VOTER_WITNESS_SUBSTITUTION_WHILE_VOTING_FOR_PLAYING;
      percent_type            percentage_of_voter_witness_substitution_while_voting_for_results = PLAYCHAIN_DEFAULT_PERCENTAGE_OF_VOTER_WITNESS_SUBSTITUTION_WHILE_VOTING_FOR_RESULTS;
      uint16_t                min_votes_for_results = PLAYCHAIN_DEFAULT_MIN_VOTES_FOR_RESULTS;
      extensions_type         extensions;

      /** defined in fee_schedule.cpp */
      void validate()const;
   };

} }  // graphene::chain

FC_REFLECT( playchain::chain::playchain_parameters,
            (player_referrer_percent_of_fee)
            (player_referrer_parent_percent_of_fee)
            (player_referrer_balance_min_threshold)
            (player_referrer_balance_max_threshold)
            (game_witness_percent_of_fee)
            (take_into_account_graphene_balances)
            (voting_for_playing_expiration_seconds)
            (voting_for_results_expiration_seconds)
            (voting_for_playing_requied_percent)
            (voting_for_results_requied_percent)
            (game_lifetime_limit_in_seconds)
            (pending_buyin_proposal_lifetime_limit_in_seconds)
            (amount_reserving_places_per_user)
            (pending_buy_in_allocate_per_block)
            (rooms_rating_recalculations_per_maintenance)
            (tables_weight_recalculations_per_maintenance)
            (min_allowed_table_weight_to_be_allocated)
            (minimum_desired_number_of_players_for_tables_allocation)
            (maximum_desired_number_of_players_for_tables_allocation)
            (buy_in_expiration_seconds)
            (percentage_of_voter_witness_substitution_while_voting_for_playing)
            (percentage_of_voter_witness_substitution_while_voting_for_results)
            (min_votes_for_results)
            (extensions)
          )
