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

    template<typename ImplInterface>
    ImplInterface &find_implementation(const database &d, map<fc::time_point_sec, std::unique_ptr<ImplInterface>> &impls)
    {
        assert(!impls.empty());

        //find point at implementations X-axis that satisfy condition
        // I(X): d.head_block_time() >= X = HARDFORK_*
        auto &&current_time_point = d.head_block_time();
        auto it = impls.begin();
        while (++it != impls.end() && it->first <= current_time_point);

        return *((--it)->second);
    }

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
        if (allow_voting)
        {
            scheduled_voting_for_results(d, table_voting, table);
        }else
        {
            d.remove(table_voting);

            d.push_applied_operation(
                        game_event_operation{ table.id, table.room(d).owner, fail_expire_game_result{} } );

            rollback_game(d, table);
        }
    }

    void scheduled_voting_for_playing(database& d, const table_voting_object &table_voting, const table_object &table)
    {
        const auto& parameters = get_playchain_parameters(d);

        voting_data_type valid_vote;
        account_votes_type accounts_with_invalid_vote;
        game_witnesses_type required_witnesses;

        if (d.head_block_time() >= HARDFORK_PLAYCHAIN_9_TIME)
        {
            bool any_invalid = false;
            decltype(table_voting.votes) valid_votes;
            for ( auto &&vote: table_voting.votes )
            {
                game_initial_data initial_data = vote.second.get<game_initial_data>();

                if (!validate_ivariants(d, table, initial_data))
                {
                    any_invalid |= true;
                }else
                {
                    valid_votes.emplace(vote.first, vote.second);
                }
            }

            if (any_invalid)
            {
                d.modify(table_voting, [&valid_votes](table_voting_object &obj)
                {
                    obj.votes = std::move(valid_votes);
                });
            }
        }

        if (table_voting.votes.size() >= parameters.min_votes_for_playing &&
                voting(d,
                   table_voting,
                   table,
                   parameters.voting_for_playing_requied_percent,
                   valid_vote,
                   required_witnesses,
                   accounts_with_invalid_vote))
        {
            apply_start_playing_with_consensus(d, table, valid_vote, required_witnesses, accounts_with_invalid_vote);
        }else
        {
            wlog("There is no consensus to start game at table '${t}'", ("t", table_voting.table));

            d.push_applied_operation(
                        game_event_operation{ table.id, table.room(d).owner, fail_consensus_game_start_playing{} } );
        }

        cleanup_pending_votes(d, table);
    }

    void scheduled_voting_for_results(database& d, const table_voting_object &table_voting, const table_object &table)
    {
        const auto& parameters = get_playchain_parameters(d);

        voting_data_type valid_vote;
        account_votes_type accounts_with_invalid_vote;
        game_witnesses_type required_witnesses = table.voted_witnesses;

        if (voting(d,
                   table_voting,
                   table,
                   parameters.voting_for_results_requied_percent,
                   valid_vote,
                   required_witnesses,
                   accounts_with_invalid_vote))
        {
            apply_game_result_with_consensus(d, table, valid_vote, required_witnesses, accounts_with_invalid_vote);
        }else
        {
            wlog("There is no consensus to apply game results at table '${t}'", ("t", table_voting.table));

            d.push_applied_operation(
                        game_event_operation{ table.id, table.room(d).owner, fail_consensus_game_result{} } );

            rollback_game(d, table);
        }

        cleanup_pending_votes(d, table);
    }

    game_start_playing_check_evaluator::game_start_playing_check_evaluator()
    {
        _impls.emplace(fc::time_point_sec{}, std::make_unique<game_start_playing_check_evaluator_impl_v1>(*this));
        _impls.emplace(HARDFORK_PLAYCHAIN_8_TIME, std::make_unique<game_start_playing_check_evaluator_impl_v2>(*this));
    }
    game_start_playing_check_evaluator::~game_start_playing_check_evaluator(){}

    void_result game_start_playing_check_evaluator::do_evaluate( const operation_type& op )
    {
        return find_implementation<impl_interface>(db(), _impls).do_evaluate(op);
    }

    operation_result game_start_playing_check_evaluator::do_apply( const operation_type& op )
    {
        return find_implementation<impl_interface>(db(), _impls).do_apply(op);
    }

    game_result_check_evaluator::game_result_check_evaluator()
    {
        _impls.emplace(fc::time_point_sec{}, std::make_unique<game_result_check_evaluator_impl_v1>(*this));
        _impls.emplace(HARDFORK_PLAYCHAIN_8_TIME, std::make_unique<game_result_check_evaluator_impl_v2>(*this));
    }
    game_result_check_evaluator::~game_result_check_evaluator(){}

    void_result game_result_check_evaluator::do_evaluate( const operation_type& op )
    {
        return find_implementation<impl_interface>(db(), _impls).do_evaluate(op);
    }

    operation_result game_result_check_evaluator::do_apply( const operation_type& op )
    {
        return find_implementation<impl_interface>(db(), _impls).do_apply(op);
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
