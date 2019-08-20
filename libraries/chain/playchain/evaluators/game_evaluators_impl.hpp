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

#include <graphene/chain/evaluator.hpp>

#include <playchain/chain/protocol/game_event_operations.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;

    using account_votes_type = flat_map<account_id_type, voting_data_type>;

    bool is_table_owner(const database& d,
                        const table_object &table,
                        const account_id_type &voter);

    bool is_witness(const database& d,
                    const table_object &table,
                    const account_id_type &voter);

    void check_voter(const database& d,
                          const table_object &table,
                          const account_id_type &voter);

    bool validate_game_start_ivariants(const database& d,
                                       const table_object &table,
                                       const game_initial_data &initial_data);

    bool validate_game_result_ivariants(const database& d,
                                        const table_object &table,
                                        const game_result &result);

    bool voting(database& d,
                const table_voting_object &table_voting,
                const table_object &table,
                const percent_type voting_requied_percent,
                voting_data_type &valid_vote,
                game_witnesses_type &required_witnesses,
                account_votes_type &accounts_with_invalid_vote);

    const table_voting_object &collect_votes(database& d,
                       const table_object &table,
                       const account_id_type &voter,
                       const game_initial_data &vote_data,
                       const uint32_t voting_seconds,
                       const percent_type pv_witness_substitution,
                       game_witnesses_type &required_witnesses,
                       bool &voters_collected);

    const table_voting_object &collect_votes(database& d,
                       const table_object &table,
                       const account_id_type &voter,
                       const game_result &vote_data,
                       const uint32_t voting_seconds,
                       const percent_type pv_witness_substitution,
                       game_witnesses_type &required_witnesses,
                       bool &voters_collected);

    void apply_start_playing_with_consensus(database& d, const table_object &table,
                                           const voting_data_type &valid_vote,
                                           const game_witnesses_type &required_witnesses,
                                           const account_votes_type &accounts_with_invalid_vote);

    void apply_game_result_with_consensus(database& d, const table_object &table,
                                           const voting_data_type &valid_vote,
                                           const game_witnesses_type &required_witnesses,
                                           const account_votes_type &accounts_with_invalid_vote);

    void cleanup_pending_votes(database& d, const table_object &table);

    void roolback(database& d, const table_object &table, bool full_clear);

    void push_fail_vote_operation(database &d, const game_start_playing_check_operation &op);

    void push_fail_vote_operation(database &d, const game_result_check_operation &op);

    void push_fail_vote_operation(database &d, const pending_table_vote_object &obj);

    class game_start_playing_check_evaluator_impl
    {
          friend class game_start_playing_check_evaluator;

       public:

          virtual ~game_start_playing_check_evaluator_impl() = default;

       protected:
          game_start_playing_check_evaluator_impl(generic_evaluator &ev);

          database& db() const;

          using operation_type = game_start_playing_check_operation;

          virtual void_result do_evaluate( const operation_type& ) = 0;
          virtual operation_result do_apply( const operation_type& ) = 0;

        private:
          generic_evaluator &_ev;
    };

    class game_result_check_evaluator_impl
    {
         friend class game_result_check_evaluator;

      public:

         virtual ~game_result_check_evaluator_impl() = default;

      protected:
          game_result_check_evaluator_impl(generic_evaluator &db);

          database& db() const;

          using operation_type = game_result_check_operation;

          virtual void_result do_evaluate( const operation_type& ) = 0;
          virtual operation_result do_apply( const operation_type& ) = 0;

      private:
          generic_evaluator &_ev;
    };

    class game_start_playing_check_evaluator_impl_v2: public game_start_playing_check_evaluator_impl
    {
          friend class game_start_playing_check_evaluator;

       protected:
          using game_start_playing_check_evaluator_impl::game_start_playing_check_evaluator_impl;

          void_result do_evaluate( const operation_type& ) override;
          operation_result do_apply( const operation_type& ) override;
    };

    class game_result_check_evaluator_impl_v2: public game_result_check_evaluator_impl
    {
         friend class game_result_check_evaluator;

      protected:
          using game_result_check_evaluator_impl::game_result_check_evaluator_impl;

          void_result do_evaluate( const operation_type& ) override;
          operation_result do_apply( const operation_type& ) override;
    };
}}
