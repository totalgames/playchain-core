#include "game_evaluators_obsolete.hpp"

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

#include <graphene/chain/hardfork.hpp>

namespace playchain { namespace chain {

namespace {

void check_voter(const database& d,
                      const table_object &table,
                      const account_id_type &voter)
{
    const auto &index = d.get_index_type<table_voting_index>().indices().get<by_table>();
    auto it = index.find(table.id);
    if (it!= index.end())
    {
        const table_voting_object &table_voting  = (*it);
        FC_ASSERT(!table_voting.votes.count(voter), "Voter has already voted");
        FC_ASSERT (table_voting.required_player_voters.count(voter) || is_game_witness(d, table, voter) ||
                   is_table_owner(d, table, voter), "Invalid voter");
    } else
    {
        const auto &index_pending = d.get_index_type<pending_table_vote_index>().indices().get<by_table_voter>();
        FC_ASSERT (index_pending.find(boost::make_tuple(table.id, voter)) == index_pending.end(), "Voter has already voted");
    }
}

bool apply_start_playing_check_voting(database& d, const table_object &table, const game_start_playing_check_operation &op)
{
    const auto& parameters = get_playchain_parameters(d);

    voting_data_type valid_vote;
    account_votes_type accounts_with_invalid_vote;
    game_witnesses_type required_witnesses;
    const percent_type voting_requied_percent = parameters.voting_for_playing_requied_percent;
    bool voters_collected = false;

    const table_voting_object &table_voting = collect_votes(d, table, op.voter, op.initial_data,
                                                            parameters.voting_for_playing_expiration_seconds,
                                                            parameters.percentage_of_voter_witness_substitution_while_voting_for_playing,
                                                            required_witnesses,
                                                            voters_collected);
    if (voters_collected)
    {
        if (voting(d, table_voting, table, voting_requied_percent, valid_vote, required_witnesses, accounts_with_invalid_vote))
        {
            apply_start_playing_with_consensus(d, table, valid_vote, required_witnesses, accounts_with_invalid_vote);
        }else
        {
            wlog("There is no consensus to start game at table '${t}'", ("t", table_voting.table));

            d.push_applied_operation(
                        game_event_operation{ table.id, table.room(d).owner, fail_consensus_game_start_playing{} } );
        }

        return false;
    }

    return true;
}

bool apply_game_result_check_voting(database& d, const table_object &table, const game_result_check_operation &op)
{
    const auto& parameters = get_playchain_parameters(d);

    voting_data_type valid_vote;
    account_votes_type accounts_with_invalid_vote;
    game_witnesses_type required_witnesses = table.voted_witnesses;
    const percent_type voting_requied_percent = parameters.voting_for_results_requied_percent;
    bool voters_collected = false;

    const table_voting_object &table_voting = collect_votes(d, table, op.voter, op.result,
                                     parameters.voting_for_results_expiration_seconds,
                                     parameters.percentage_of_voter_witness_substitution_while_voting_for_results,
                                     required_witnesses,
                                     voters_collected);
    if (voters_collected)
    {
        if (voting(d, table_voting, table, voting_requied_percent, valid_vote, required_witnesses, accounts_with_invalid_vote))
        {
            apply_game_result_with_consensus(d, table, valid_vote, required_witnesses, accounts_with_invalid_vote);
        }else
        {
            wlog("There is no consensus to apply game results at table '${t}'", ("t", table_voting.table));

            d.push_applied_operation(
                        game_event_operation{ table.id, table.room(d).owner, fail_consensus_game_result{} } );

            roolback(d, table, false);
        }

        return false;
    }

    return true;
}

template<typename Operation>
bool ignore_op_in_broken_block(const database &d, const Operation &op)
{
#if defined(PLAYCHAIN_TESTNET)
    auto b_num = d.get_dynamic_global_properties().head_block_number;
    switch(++b_num) //next block would failed
    {
    case 3578351:
        wlog("Historically BRoKEN block !!! ${f}, b=${b}, op=${op}", ("f", __FUNCTION__)("b", b_num)("op", op));

        return true;
    default:;
    }
#endif
    return false;
}

}

void obsolete_buyins_resolve(database& d, const table_object &table, bool clear)
{
    const auto& bin_by_table = d.get_index_type<buy_in_index>().indices().get<by_buy_in_table>();
    auto range = bin_by_table.equal_range(table.id);

    auto buy_ins = get_objects_from_index<buy_in_object>(range.first, range.second,
                                                         get_playchain_parameters(d).maximum_desired_number_of_players_for_tables_allocation);
    for (const buy_in_object& buy_in: buy_ins) {

        if (clear || !table.cash.count(buy_in.player))
        {
#if defined(LOG_VOTING)
        if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
        {
            ilog("${t} >> obsolete_buyins_resolve: ${b} - remove!!!", ("t", d.head_block_time())("b", buy_in));
        }
#endif
            d.remove(buy_in);
        }else if (d.head_block_time() < HARDFORK_PLAYCHAIN_7_TIME)
        {
            prolong_life_for_by_in(d, buy_in);
        }
    }
}

void_result game_start_playing_check_evaluator_impl_v1::do_evaluate(const operation_type& op)
{
    try { try {
        const database& d = db();

        FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");

        const table_object &table = op.table(d);

        FC_ASSERT(is_table_owner(d, table, op.table_owner), "Wrong table owner");
        FC_ASSERT(table.is_free(), "Wrong type of voting. There is game on table");
        FC_ASSERT(op.initial_data.cash.size() >= 2, "Invalid data to vote. At least two players required");

        FC_ASSERT( !is_table_voting_for_results(d, table.id), "Wrong type of voting" );

        check_voter(d, table, op.voter);

        FC_ASSERT(validate_ivariants(d, table, op.initial_data), "Invalid initial data");

        return void_result();
    } catch ( const fc::assert_exception& ) {
        _ignore = ignore_op_in_broken_block(db(), op);
        if (_ignore)
            return void_result();
         throw;
    }}FC_CAPTURE_AND_RETHROW((op))
}

operation_result game_start_playing_check_evaluator_impl_v1::do_apply( const operation_type& op )
{
    try {
        if (_ignore)
            return void_result();

        database& d = db();

        const table_object &table = op.table(d);

        if (!is_table_voting(d, table.id) &&
            !is_table_owner(d, table, op.voter))
        {
            return d.create<pending_table_vote_object>([&](pending_table_vote_object &obj)
            {
                 obj.table = table.id;
                 obj.voter = op.voter;
                 obj.vote = op_wrapper{op};
            }).id;
        }

        bool wait_next_vote = apply_start_playing_check_voting(d, table, op);
        const auto &index_pending = d.get_index_type<pending_table_vote_index>().indices().get<by_table>();
        auto range = index_pending.equal_range(table.id);
        for (auto itr = range.first; itr != range.second;) {
            const pending_table_vote_object & vote_obj = *itr++;

            if (wait_next_vote)
            {
                auto pending_op = vote_obj.vote.op.get<operation_type>();

                if (d.head_block_time() < HARDFORK_PLAYCHAIN_1_TIME)
                {
                    wait_next_vote = apply_start_playing_check_voting(d, table, pending_op);
                }else
                {
                    if (validate_ivariants(d, table, pending_op.initial_data))
                    {
                        wait_next_vote = apply_start_playing_check_voting(d, table, pending_op);
                    }else
                    {
                        wlog("Pending vote for start playing voting ignored: ${v}",  ("v", vote_obj));
                        push_fail_vote_operation(d, pending_op);
                    }
                }
            }

            d.remove(vote_obj);
        }

        return alive_for_table(d, table.id);
    }FC_CAPTURE_AND_RETHROW((op))
}

void_result game_result_check_evaluator_impl_v1::do_evaluate( const operation_type& op )
{
    try { try {
        const database& d = db();

        FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");

        const table_object &table = op.table(d);

        FC_ASSERT(is_table_owner(d, table, op.table_owner), "Wrong table owner");

        FC_ASSERT(table.is_playing(), "Wrong type of voting. There is no game on table");

        FC_ASSERT( !is_table_voting_for_playing(d, table.id), "Wrong type of voting" );

        check_voter(d, table, op.voter);

        FC_ASSERT(validate_ivariants(d, table, op.result), "Invalid result");

        return void_result();
    } catch ( const fc::assert_exception& ) {
        _ignore = ignore_op_in_broken_block(db(), op);
        if (_ignore)
            return void_result();
         throw;
    }}FC_CAPTURE_AND_RETHROW((op))
}

operation_result game_result_check_evaluator_impl_v1::do_apply( const operation_type& op )
{
    try {
        if (_ignore)
            return void_result();

        database& d = db();

        const table_object &table = op.table(d);

        if (!is_table_voting(d, table.id) &&
            !is_game_witness(d, table, op.voter) &&
            !is_table_owner(d, table, op.voter))
        {
            return d.create<pending_table_vote_object>([&](pending_table_vote_object &obj)
            {
                 obj.table = table.id;
                 obj.voter = op.voter;
                 obj.vote = op_wrapper{op};
            }).id;
        }

        bool wait_next_vote = apply_game_result_check_voting(d, table, op);
        const auto &index_pending = d.get_index_type<pending_table_vote_index>().indices().get<by_table>();
        auto range = index_pending.equal_range(table.id);
        for (auto itr = range.first; itr != range.second;) {
            const pending_table_vote_object & vote_obj = *itr++;

            if (wait_next_vote)
            {
                auto pending_op = vote_obj.vote.op.get<operation_type>();

                if (d.head_block_time() < HARDFORK_PLAYCHAIN_1_TIME)
                {
                    wait_next_vote = apply_game_result_check_voting(d, table, pending_op);
                }else
                {
                    if (validate_ivariants(d, table, pending_op.result))
                    {
                        wait_next_vote = apply_game_result_check_voting(d, table, pending_op);
                    }else
                    {
                        wlog("Pending vote for result voting ignored: ${v}", ("v", vote_obj));
                        push_fail_vote_operation(d, pending_op);
                    }
                }
            }

            d.remove(vote_obj);
        }

        return alive_for_table(d, table.id);
    }FC_CAPTURE_AND_RETHROW((op))
}
}}
