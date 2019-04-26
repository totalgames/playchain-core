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

#include <graphene/chain/protocol/asset.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>
#include <playchain/chain/protocol/game_operations.hpp>

namespace playchain { namespace chain {

    using namespace graphene::chain;

    struct game_start_playing_validated
    {
        game_start_playing_validated() = default;
        game_start_playing_validated(const game_initial_data &initial_data)
           : initial_data(initial_data){}

        game_initial_data initial_data;
    };

    struct game_result_validated
    {
        game_result_validated() = default;
        game_result_validated(const game_result &result)
           : result(result){}

        game_result result;
    };

    struct game_rollback
    {
    };

    struct fail_consensus_game_start_playing
    {
    };

    struct fail_consensus_game_result
    {
    };

    struct fail_expire_game_start_playing
    {
    };

    struct fail_expire_game_result
    {
    };

    struct fail_expire_game_lifetime
    {
    };

    struct fraud_game_start_playing_check
    {
        fraud_game_start_playing_check() = default;
        fraud_game_start_playing_check(
                                    const account_id_type &witness,
                                    const std::string &fail_info,
                                    const std::string &actual_info)
           : witness(witness),
             fail_info(fail_info),
             actual_info(actual_info) {}

        account_id_type           witness;
        std::string               fail_info;
        std::string               actual_info;
    };

    struct fraud_game_result_check
    {
        fraud_game_result_check() = default;
        fraud_game_result_check(
                                    const account_id_type &witness,
                                    const std::string &fail_log,
                                    const std::string &actual_log)
           : witness(witness),
             fail_log(fail_log),
             actual_log(actual_log) {}

        account_id_type           witness;
        std::string               fail_log;
        std::string               actual_log;
    };

    struct buy_out_allowed
    {
        buy_out_allowed() = default;
        buy_out_allowed(
                                const account_id_type &player,
                                const asset &amount )
           : player(player),
             amount(amount) {}

        account_id_type           player;
        asset                     amount;
    };

    struct buy_in_return
    {
        buy_in_return() = default;
        buy_in_return(
                                 const account_id_type &player,
                                 const asset &amount )
           : player(player),
             amount(amount) {}

        account_id_type           player;
        asset                     amount;
    };

    struct game_cash_return
    {
        game_cash_return() = default;
        game_cash_return(
                                const account_id_type &player,
                                const asset &amount )
           : player(player),
             amount(amount) {}

        account_id_type           player;
        asset                     amount;
    };

    struct fraud_buy_out
    {
        fraud_buy_out() = default;
        fraud_buy_out(
                                 const account_id_type &player,
                                 const asset &fail_amount,
                                 const asset &allowed_amount)
           : player(player),
             fail_amount(fail_amount),
             allowed_amount(allowed_amount) {}

        account_id_type           player;
        asset                     fail_amount;
        asset                     allowed_amount;
    };

    struct fail_vote
    {
        fail_vote() = default;
        fail_vote(const account_id_type &voter,
                  const voting_data_type &vote):
            voter(voter),
            vote(vote)
        {
        }

        account_id_type           voter;
        voting_data_type          vote;
    };

    using game_event_type = static_variant<
                                           game_start_playing_validated,
                                           game_result_validated,
                                           game_rollback,
                                           fail_consensus_game_start_playing,
                                           fail_consensus_game_result,
                                           fail_expire_game_start_playing,
                                           fail_expire_game_result,
                                           fail_expire_game_lifetime,
                                           fraud_game_start_playing_check,
                                           fraud_game_result_check,
                                           buy_out_allowed,
                                           buy_in_return,
                                           game_cash_return,
                                           fraud_buy_out,
                                           fail_vote>;

    struct game_event_operation : public base_operation
    {
        struct fee_parameters_type {};

        game_event_operation(){}
        game_event_operation(const table_id_type &table,
                             const account_id_type &table_owner,
                             const game_event_type &event)
           : table(table),
             table_owner(table_owner),
             event(event){}

        asset                     fee; // always zero
        table_id_type             table;
        account_id_type           table_owner;
        game_event_type           event;

        account_id_type fee_payer()const { return GRAPHENE_TEMP_ACCOUNT; }
        void            validate()const { FC_ASSERT( !"virtual operation" ); }
        share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };
}}

FC_REFLECT( playchain::chain::game_start_playing_validated, (initial_data))
FC_REFLECT( playchain::chain::game_result_validated, (result))
FC_REFLECT( playchain::chain::game_rollback, )
FC_REFLECT( playchain::chain::fail_consensus_game_start_playing, )
FC_REFLECT( playchain::chain::fail_consensus_game_result, )
FC_REFLECT( playchain::chain::fail_expire_game_start_playing, )
FC_REFLECT( playchain::chain::fail_expire_game_result, )
FC_REFLECT( playchain::chain::fail_expire_game_lifetime, )
FC_REFLECT( playchain::chain::fraud_game_start_playing_check, (witness)(fail_info)(actual_info))
FC_REFLECT( playchain::chain::fraud_game_result_check, (witness)(fail_log)(actual_log))
FC_REFLECT( playchain::chain::buy_out_allowed, (player)(amount))
FC_REFLECT( playchain::chain::buy_in_return, (player)(amount))
FC_REFLECT( playchain::chain::game_cash_return, (player)(amount))
FC_REFLECT( playchain::chain::fraud_buy_out, (player)(fail_amount)(allowed_amount))
FC_REFLECT( playchain::chain::fail_vote, (voter)(vote))

FC_REFLECT_TYPENAME( playchain::chain::game_event_type )

FC_REFLECT( playchain::chain::game_event_operation::fee_parameters_type, )

FC_REFLECT( playchain::chain::game_event_operation, (fee)(table)(table_owner)(event))
