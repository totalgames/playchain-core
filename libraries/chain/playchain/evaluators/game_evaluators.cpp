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

#include <playchain/chain/evaluators/game_evaluators.hpp>
#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <playchain/chain/evaluators/validators.hpp>
#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/buy_in_buy_out_evaluators.hpp>
#include <playchain/chain/evaluators/table_evaluators.hpp>

#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/pending_buy_in_object.hpp>
#include <playchain/chain/schema/pending_buy_out_object.hpp>

#include "game_evaluators_impl.hpp"
#include "game_evaluators_obsolete.hpp"

#include <graphene/chain/hardfork.hpp>

namespace playchain{ namespace chain{

    void rollback_table(database& d, const table_object &table)
    {
        roolback(d, table, true);
    }

    void rollback_game(database& d, const table_object &table)
    {
        roolback(d, table, false);
    }

    void expire_voting_for_playing(database& d, const table_voting_object &table_voting, const table_object &table)
    {
        d.push_applied_operation(
                    game_event_operation{ table.id, table.room(d).owner, fail_expire_game_start_playing{} } );

        d.remove(table_voting);

        cleanup_pending_votes(d, table);
    }

    void expire_voting_for_results(database& d, const table_voting_object &table_voting, const table_object &table)
    {
        assert(table.is_playing());

        const auto& parameters = get_playchain_parameters(d);

        voting_data_type valid_vote;
        account_votes_type accounts_with_invalid_vote;
        game_witnesses_type required_witnesses = table.voted_witnesses;
        bool allow_voting = false;
        if (table.playing_cash.size() >= table_voting.required_player_voters.size())
        {
            //allow voting without voters collection
            auto votes =  table.playing_cash.size() - table_voting.required_player_voters.size();
            allow_voting = votes >= parameters.min_votes_for_results;
        }
        if (allow_voting &&
                voting(d,
                       table_voting,
                       table,
                       parameters.voting_for_results_requied_percent,
                       valid_vote,
                       required_witnesses,
                       accounts_with_invalid_vote))
        {
            apply_game_result_with_consensus(d, table, valid_vote, required_witnesses, accounts_with_invalid_vote);

            cleanup_pending_votes(d, table);
        }else
        {
            if (!allow_voting)
            {
                d.remove(table_voting);
            }

            d.push_applied_operation(
                        game_event_operation{ table.id, table.room(d).owner, fail_expire_game_result{} } );

            rollback_game(d, table);
        }
    }

    game_start_playing_check_evaluator::game_start_playing_check_evaluator()
    {
        _impls.emplace(fc::time_point_sec{}, new game_start_playing_check_evaluator_impl_v1{*this});
    }
    game_start_playing_check_evaluator::~game_start_playing_check_evaluator(){}

    void_result game_start_playing_check_evaluator::do_evaluate( const operation_type& op )
    {
        return _impls[fc::time_point_sec{}]->do_evaluate(op);
    }

    operation_result game_start_playing_check_evaluator::do_apply( const operation_type& op )
    {
         return _impls[fc::time_point_sec{}]->do_apply(op);
    }

    game_result_check_evaluator::game_result_check_evaluator()
    {
        _impls.emplace(fc::time_point_sec{}, new game_result_check_evaluator_impl_v1{*this});
    }
    game_result_check_evaluator::~game_result_check_evaluator(){}

    void_result game_result_check_evaluator::do_evaluate( const operation_type& op )
    {
        return _impls[fc::time_point_sec{}]->do_evaluate(op);
    }

    operation_result game_result_check_evaluator::do_apply( const operation_type& op )
    {
        return _impls[fc::time_point_sec{}]->do_apply(op);
    }

    void_result game_reset_evaluator::do_evaluate( const operation_type& op )
    {
        try {
            const database& d = db();
            FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");
            FC_ASSERT(is_table_owner(d, op.table_owner, op.table), "Wrong table owner");

            return void_result();
        }FC_CAPTURE_AND_RETHROW((op))
    }

    operation_result game_reset_evaluator::do_apply( const operation_type& op )
    {
        try {
            database& d = db();

            const table_object &table = op.table(d);

            if (!op.rollback_table)
                rollback_game(d, table);
            else
                rollback_table(d, table);

            auto& voting_by_table= d.get_index_type<table_voting_index>().indices().get<by_table>();
            auto it = voting_by_table.find(table.id);
            if (voting_by_table.end() != it)
            {
                d.remove(*it);
            }

            return alive_for_table(d, table.id);
        }FC_CAPTURE_AND_RETHROW((op))
    }
}}
