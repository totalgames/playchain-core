#include "game_evaluators_impl.hpp"

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

#include <algorithm>

#include "game_evaluators_obsolete.hpp"

#include <boost/range/algorithm/merge.hpp>

namespace playchain { namespace chain {

namespace {

template<typename Operation>
void check_incoming_vote(const database &d, const table_object &table, const Operation &op)
{
    const auto &index = d.get_index_type<table_voting_index>().indices().get<by_table>();
    auto it = index.find(table.id);
    if (it!= index.end())
    {
        const table_voting_object &table_voting  = (*it);
        FC_ASSERT(!table_voting.votes.count(op.voter), "Voter has already voted");
    } else
    {
        const auto &index_pending = d.get_index_type<pending_table_vote_index>().indices().get<by_table_voter>();
        FC_ASSERT(index_pending.find(boost::make_tuple(table.id, op.voter)) == index_pending.end(), "Voter has already voted");
    }

    FC_ASSERT(is_game_witness(d, table, op.voter) || is_table_owner(d, table, op.voter) || table.is_valid_voter(d, op), "Invalid voter");

    FC_ASSERT(validate_ivariants(d, table, op.data()), "Invalid vote data");
}

template<typename Operation>
void check_pending_vote(const database &d, const table_object &table, const table_voting_object &table_voting, const Operation &op)
{
    FC_ASSERT(table_voting.required_player_voters.count(op.voter) || is_game_witness(d, table, op.voter) ||
              is_table_owner(d, table, op.voter), "Invalid voter");

    FC_ASSERT(validate_ivariants(d, table, op.data()), "Invalid vote data");
}

const game_witness_object *get_witness_if_exist(const database& d,
                                                const table_object &table,
                                                const account_id_type &voter)
{
    const auto& witness_by_account = d.get_index_type<game_witness_index>().indices().get<by_playchain_account>();
    auto &&it = witness_by_account.find(voter);
    if (witness_by_account.end() == it || is_table_owner(d, table, voter))
        return nullptr;
    //return pointer to fixed memory address
    return &(*it);
}

using game_digest_type = fc::ripemd160;

struct get_game_digest_visitor
{
    get_game_digest_visitor() = default;

    using result_type = game_digest_type;

    result_type operator()(const game_initial_data &data)
    {
        game_digest_type::encoder enc;
        fc::raw::pack( enc, data.cash );
        fc::raw::pack( enc, data.info );
        return enc.result();
    }

    result_type operator()(const game_result &data)
    {
        game_digest_type::encoder enc;
        for(const auto &sub_data: data.cash)
        {
            fc::raw::pack( enc, sub_data.first );
            const gamer_cash_result &player_result = sub_data.second;
            fc::raw::pack( enc, player_result.cash );
            fc::raw::pack( enc, player_result.rake );
        }
        fc::raw::pack( enc, data.log );
        return enc.result();
    }
};

bool voting_impl(const table_voting_object &table_voting,
            const percent_type requied,
            voting_data_type &valid_vote,
            account_votes_type &accounts_with_invalid_vote)
{
    flat_map<game_digest_type, account_votes_type> votes_by_content;

    std::for_each(begin(table_voting.votes), end(table_voting.votes),
                   [&](const decltype(table_voting.votes)::value_type &data)
    {
        get_game_digest_visitor get_digest;
        auto digest = data.second.visit(get_digest);
        votes_by_content[digest].insert(std::make_pair(data.first, data.second));
    });

    if (votes_by_content.empty())
        return false;

    auto miss_votes_percent = (votes_by_content.size() - 1) * GRAPHENE_100_PERCENT;
    miss_votes_percent /= table_voting.votes.size();

    if ((percent_type)miss_votes_percent <= GRAPHENE_100_PERCENT - requied)
    {
        auto min_max = std::minmax_element(votes_by_content.begin(), votes_by_content.end(),
                            [](const decltype(votes_by_content)::value_type &ld,
                               const decltype(votes_by_content)::value_type &rd)
        {
            if (ld.second.size() == rd.second.size())
                return ld.first < rd.first;

            return ld.second.size() < rd.second.size();
        });

        const decltype(votes_by_content)::value_type& max_vote = (*min_max.second);
        if (max_vote.first != game_digest_type{})
        {
            valid_vote = max_vote.second.begin()->second;
            votes_by_content.erase(min_max.second);

            for(const auto &data: votes_by_content)
            {
                const account_votes_type &voters = data.second;

                boost::merge(voters, accounts_with_invalid_vote,
                             std::inserter(accounts_with_invalid_vote, accounts_with_invalid_vote.end()));
            }

            return true;
        }
    }

    return false;
}

struct set_required_voters
{
    set_required_voters(
            const database& d,
            const table_object &table,
            table_voting_object &table_voting,
            const account_id_type &voter,
            const percent_type pv_witness_substitution):
        d(d),
        table(table),
        table_voting(table_voting),
        voter(voter),
        pv_witness_substitution(pv_witness_substitution)
    {}

    //start playing voting
    void operator()(const game_initial_data &vote_data)
    {
        if (voter == table.room(d).owner)
        {
            table_voting.required_player_voters.clear();
            for(const auto &pr: vote_data.cash)
            {
                table_voting.required_player_voters.emplace(pr.first);
            }

            table_voting.etalon_vote = vote_data;
            table_voting.witnesses_allowed_for_substitution =
                    calculate_witnesses_allowed_for_substitution(table_voting.required_player_voters.size(),
                                                         pv_witness_substitution);
        }
    }

    //result voting
    void operator()(const game_result &)
    {
        table_voting.required_player_voters.clear();
        for(const auto &pr: table.playing_cash)
        {
            const player_id_type &id = pr.first;
            table_voting.required_player_voters.emplace(id(d).account);
        }

        table_voting.witnesses_allowed_for_substitution =
                calculate_witnesses_allowed_for_substitution(table_voting.required_player_voters.size(),
                                                     pv_witness_substitution);
    }

private:

    static size_t calculate_witnesses_allowed_for_substitution(const size_t required_voters,
                                                      const percent_type percent)
    {
        size_t r = required_voters * percent;
        r /= GRAPHENE_100_PERCENT;
        return r;
    }

    const database& d;
    const table_object &table;
    table_voting_object &table_voting;
    const account_id_type &voter;
    const percent_type pv_witness_substitution;
};

struct set_voter
{
    set_voter(
            const database& d,
            const table_object &table,
            table_voting_object &table_voting,
            const account_id_type &voter):
        d(d),
        table(table),
        table_voting(table_voting),
        voter(voter)
    {}

    //voting for start playing
    void operator()(const game_initial_data &vote_data)
    {
        if (table_voting.required_player_voters.count(voter))
        {
            table_voting.required_player_voters.erase(voter);
        }else
        {
            const game_witness_object *pwitness = get_witness_if_exist(d, table, voter);
            if (pwitness)
            {
                table_voting.voted_witnesses.insert(pwitness->id);
                table_voting.required_witness_voters.insert(pwitness->id);
            }
        }
        table_voting.votes[voter] = vote_data;
    }

    //voting for result
    void operator()(const game_result &vote_data)
    {
        if (table_voting.required_player_voters.count(voter))
        {
            table_voting.required_player_voters.erase(voter);
        }else
        {
            const game_witness_object *pwitness = get_witness_if_exist(d, table, voter);
            if (pwitness)
            {
                table_voting.voted_witnesses.insert(pwitness->id);
                table_voting.required_witness_voters.erase(pwitness->id);
            }
        }
        table_voting.votes[voter] = vote_data;
    }

private:

    const database& d;
    const table_object &table;
    table_voting_object &table_voting;
    const account_id_type &voter;
};

struct is_the_required_number_of_voters
{
    is_the_required_number_of_voters(
                           const table_object &table,
                           const table_voting_object &table_voting):
        table(table),
        table_voting(table_voting)
    {
    }

    //voting for start playing
    bool operator()(const game_initial_data &)
    {
        assert(table_voting.voted_witnesses.size() == table_voting.required_witness_voters.size());

        if (table_voting.voted_witnesses.size() < table.required_witnesses)
            return false;

        if (table_voting.required_player_voters.empty())
            return true;

        return is_required_players_with_substitution_of_witnesses();
    }

    //voting for result
    bool operator()(const game_result &)
    {
        if (!table_voting.required_witness_voters.empty())
            return false;

        if (table_voting.required_player_voters.empty())
            return true;

        return is_required_players_with_substitution_of_witnesses();
    }

 private:

    bool is_required_players_with_substitution_of_witnesses()
    {
        if (!table_voting.voted_witnesses.empty())
        {
            if (table_voting.voted_witnesses.size() >= table_voting.witnesses_allowed_for_substitution &&
                table_voting.required_player_voters.size() <= table_voting.witnesses_allowed_for_substitution)
                return true;
        }

        return false;
    }

    const table_object &table;
    const table_voting_object &table_voting;
};

void pending_buyouts_resolve(database& d, const table_object &table, const account_id_type &table_owner, game_result &result)
{
    const auto& bout_by_table = d.get_index_type<pending_buy_out_index>().indices().get<by_table>();
    auto range = bout_by_table.equal_range(table.id);
    for (auto itr = range.first; itr != range.second;) {
        const pending_buy_out_object& buyout = *itr++;
        const auto &player = buyout.player;
        const auto &account_id = player(d).account;
        auto rest = buyout.amount;

#if defined(LOG_VOTING)
        if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
        {
            ilog("${t} >> pending_buyouts_resolve: ${b}", ("t", d.head_block_time())("b", buyout));
        }
#endif

        if (!result.cash.empty())
        {
            auto it_result = result.cash.find(account_id);
            if (it_result != result.cash.end())
            {
                auto cash = std::min(it_result->second.cash, rest);
                rest -= cash;

                it_result->second.cash -= cash;
            }
        }

        if (rest.amount > 0 && table.is_waiting_at_table(player))
        {
            assert(table.cash.end() != table.cash.find(player));

            auto cash = std::min(table.cash.at(player), rest);
            rest -= cash;

            d.modify(table, [&](table_object &obj)
            {
                obj.adjust_cash(player, -cash);
            });
        }

        if (rest.amount > 0 && table.is_playing(player) && !result.cash.count(account_id))
        {
            assert(table.playing_cash.end() != table.playing_cash.find(player));

            auto cash = std::min(table.playing_cash.at(player), rest);
            rest -= cash;

            d.modify(table, [&](table_object &obj)
            {
                obj.adjust_playing_cash(player, -cash);
            });
        }

        auto allowed = buyout.amount - rest;
        if (allowed.amount > 0)
        {
            d.adjust_balance(account_id, allowed);

            d.push_applied_operation(game_event_operation{table.id, table_owner, buy_out_allowed{account_id, allowed}});
        }

        if (rest.amount > 0)
        {
            d.push_applied_operation(game_event_operation{table.id, table_owner, fraud_buy_out{account_id, rest, allowed}});
        }

#if defined(LOG_VOTING)
        if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
        {
            idump((table));
        }
#endif

        d.remove(buyout);
    }
}

template< typename Body >
void with_registration_buy_in(database& d, const table_object& table, Body &&callback )
{
    //must be sorted
    std::set<player_id_type> players;

    for(const auto &cash: table.playing_cash)
    {
        players.emplace(cash.first);
    }

    callback();

    decltype(players) with_cash;

    for(const auto &cash: table.cash)
    {
        with_cash.emplace(cash.first);
    }

    std::vector<player_id_type> players_with_cash;
    players_with_cash.reserve(players.size());

    std::set_intersection(players.begin(), players.end(),
                          with_cash.begin(), with_cash.end(),
                          std::back_inserter(players_with_cash));
    for(const auto &player_id: players_with_cash)
    {
        register_buy_in(d, player_id, table);
    }
}

template<typename VoteData>
const table_voting_object &collect_votes_impl(database& d,
                   const table_object &table,
                   const account_id_type &voter,
                   const VoteData &vote_data,
                   const uint32_t voting_seconds,
                   const percent_type pv_witness_substitution,
                   const game_witnesses_type &required_witnesses,
                   bool &voters_collected)
{
    const auto& dyn_props = d.get_dynamic_global_properties();

    if (!is_table_voting(d, table.id))
    {
        return d.create<table_voting_object>([&](table_voting_object& table_voting) {
           table_voting.table = table.id;
           table_voting.created = dyn_props.time;
           table_voting.expiration = table_voting.created + fc::seconds(voting_seconds);
           table_voting.scheduled_voting = fc::time_point_sec::maximum();
           table_voting.required_witness_voters = required_witnesses;
           set_required_voters{d, table, table_voting, voter, pv_witness_substitution}(vote_data);
           set_voter{d, table, table_voting, voter}(vote_data);
        });
    }

    const table_voting_object &table_voting = (*d.get_index_type<table_voting_index>().indices().get<by_table>().find(table.id));

    d.modify(table_voting, [&](table_voting_object &obj){
        set_voter{d, table, obj, voter}(vote_data);
    });

    voters_collected = is_the_required_number_of_voters{table, table_voting}(vote_data);

    return table_voting;
}

template<typename Operation>
operation_result try_voting(database &d,
                            const table_object &table,
                            const game_witnesses_type &required_witnesses,
                            const uint32_t voting_seconds,
                            const percent_type pv_witness_substitution,
                            const Operation &op)
{
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

    const auto& parameters = get_playchain_parameters(d);

    bool voters_collected = false;

    const table_voting_object &table_voting = collect_votes(d, table, op.voter, op.data(),
                                                            voting_seconds,
                                                            pv_witness_substitution,
                                                            required_witnesses,
                                                            voters_collected);
    const auto &index_pending = d.get_index_type<pending_table_vote_index>().indices().get<by_table>();
    auto range = index_pending.equal_range(table.id);
    if (d.head_block_time() < HARDFORK_PLAYCHAIN_11_TIME)
    {
        //invalid range
        range = std::make_pair(index_pending.begin(), index_pending.end());
    }
    auto pending = get_objects_from_index<pending_table_vote_object>(range.first,
                                                                     range.second,
                                                                     parameters.maximum_desired_number_of_players_for_tables_allocation * 10);
    for (const pending_table_vote_object& vote_obj: pending) {
        if (vote_obj.vote.op.which() != operation::tag<Operation>::value)
        {
            wlog("At ${t} fail vote: ${v}", ("t", d.head_block_time())("v", vote_obj));
            push_fail_vote_operation(d, vote_obj);
        }else
        {
            auto pending_op = vote_obj.vote.op.get<Operation>();
            try
            {
                try {
                    check_pending_vote(d, table, table_voting, pending_op);

                    collect_votes(d, table, pending_op.voter, pending_op.data(),
                                  voting_seconds,
                                  pv_witness_substitution,
                                  required_witnesses,
                                  voters_collected);
                } FC_CAPTURE_LOG_AND_RETHROW( (table_voting)(pending_op) )
            }catch (const fc::assert_exception& e)
            {
                wlog("At ${t} fail vote", ("t", d.head_block_time()));
                push_fail_vote_operation(d, pending_op);
            }
        }

        d.remove(vote_obj);
    }
    if (voters_collected && table_voting.scheduled_voting == fc::time_point_sec::maximum())
    {
        const auto& dyn_props = d.get_dynamic_global_properties();
        auto block_interval = d.block_interval();
        d.modify<table_voting_object>(table_voting, [&](table_voting_object &obj)
        {
            obj.scheduled_voting = dyn_props.time + block_interval;
            if (obj.expiration <= obj.scheduled_voting)
            {
                //scheduled voting should not be expired
                obj.expiration = obj.scheduled_voting + block_interval;
            }
        });
    }

    alive_for_table(d, table.id);

    return table_voting.id;
}
}

bool validate_ivariants(const database& d,
                        const table_object &table,
                        const game_initial_data &initial_data)
{
    if (initial_data.cash.empty())
        return d.head_block_time() < HARDFORK_PLAYCHAIN_6_TIME;

    decltype(table.cash) cash_initial;
    for (const decltype(initial_data.cash)::value_type &data: initial_data.cash)
    {
        const auto &player_cash = data.second;

        if (player_cash.amount <= 0)
            return false;

        if (!is_player_exists(d, data.first))
            return false;

        const player_object &player = get_player(d, data.first);

        if (!table.cash.count(player.id))
            return false;

        cash_initial[player.id] = player_cash;
    }

    decltype(table.cash) table_cash_matched;
    std::set_intersection(begin(table.cash), end(table.cash), begin(cash_initial), end(cash_initial),
                        std::inserter(table_cash_matched, table_cash_matched.end()),
                        [](const decltype(table.cash)::value_type &ld,
                           const decltype(table.cash)::value_type &rd)
    {
        return ld.first < rd.first;
    });

    for(const decltype(table.cash)::value_type &data: table_cash_matched)
    {
        if (data.second < cash_initial[data.first])
            return false; //not enough cash on the table
    }

    if (d.head_block_time() >= HARDFORK_PLAYCHAIN_1_TIME)
    {
        /*
         * Check that the players list corresponds to the proposed by table owner
        */

        const auto& idx = d.get_index_type<table_voting_index>().indices().get<by_table>();
        auto it = idx.find(table.id);
        if (idx.end() != it && it->etalon_vote.valid())
        {
            auto &&etalon_vote = (*it->etalon_vote);
            if (etalon_vote.which() == voting_data_type::tag<game_initial_data>::value)
            {
                auto &&etalon_cash = etalon_vote.get<game_initial_data>().cash;
                if (cash_initial.size() != etalon_cash.size())
                    return false;

                decltype(table.cash) etalon_cash_initial;
                for (const decltype(initial_data.cash)::value_type &data: etalon_cash)
                {
                    assert(is_player_exists(d, data.first));

                    const player_object &player = get_player(d, data.first);

                    assert(table.cash.count(player.id));

                    etalon_cash_initial[player.id] = data.second;
                }

                decltype(table.cash) table_cash_diff;
                std::set_difference(begin(etalon_cash_initial), end(etalon_cash_initial), begin(cash_initial), end(cash_initial),
                                    std::inserter(table_cash_diff, table_cash_diff.end()),
                                    [](const decltype(table.cash)::value_type &ld,
                                       const decltype(table.cash)::value_type &rd)
                {
                    return ld.first < rd.first;
                });

                if (!table_cash_diff.empty())
                    return false;
            }
        }
    }

    return true;
}

bool validate_ivariants(const database& d,
                        const table_object &table,
                        const game_result &result)
{
    if (result.cash.empty())
        return true; //special voting

    decltype(table.playing_cash) cash_result;
    for (const decltype(result.cash)::value_type &data: result.cash)
    {
        const gamer_cash_result &player_result = data.second;

        if (player_result.cash.amount < 0 ||
            player_result.rake.amount < 0)
            return false;

        if (!is_player_exists(d, data.first))
            return false;

        const player_object &player = get_player(d, data.first);
        cash_result[player.id] = player_result.cash;
    }

    decltype(table.playing_cash) table_cash_matched;
    std::set_intersection(begin(table.playing_cash), end(table.playing_cash),
                          begin(cash_result), end(cash_result),
                        std::inserter(table_cash_matched, table_cash_matched.end()),
                        [](const decltype(table.playing_cash)::value_type &ld,
                           const decltype(table.playing_cash)::value_type &rd)
    {
        return ld.first < rd.first;
    });

    //check if all players in initial_data exist in game
    if (table_cash_matched.size() != cash_result.size())
        return false;

    asset in_balance;
    asset out_balance;
    std::for_each(begin(table_cash_matched), end(table_cash_matched),
                  [&](const decltype(table.playing_cash)::value_type &data)
    {
        in_balance += data.second;
        const auto &player_id = data.first;
        const gamer_cash_result &player_result = result.cash.at(player_id(d).account);
        out_balance += player_result.cash;
        out_balance += player_result.rake;
    });

    return in_balance == out_balance;
}

bool voting(database& d,
                           const table_voting_object &table_voting,
                           const table_object &table,
                           const percent_type voting_requied_percent,
                           voting_data_type &valid_vote,
                           game_witnesses_type &required_witnesses,
                           account_votes_type &accounts_with_invalid_vote)
{
    bool voting_consensus = voting_impl(table_voting, voting_requied_percent, valid_vote, accounts_with_invalid_vote);
    if (voting_consensus)
    {
        required_witnesses = table_voting.voted_witnesses;
        if (!required_witnesses.empty())
        {
            for(const auto &data: accounts_with_invalid_vote)
            {
                const game_witness_object *pwitness = get_witness_if_exist(d, table, data.first);
                if (!pwitness)
                    continue;

                required_witnesses.erase(pwitness->id);
            }
        }
    }

#if defined(LOG_VOTING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
    {
        ilog("${t} >> voting: ${v}", ("t", d.head_block_time())("v", table_voting));
        idump((table));
    }
#endif

    d.remove(table_voting);

    return voting_consensus;
}

const table_voting_object &collect_votes(database& d,
                   const table_object &table,
                   const account_id_type &voter,
                   const game_initial_data &vote_data,
                   const uint32_t voting_seconds,
                   const percent_type pv_witness_substitution,
                   const game_witnesses_type &required_witnesses,
                   bool &voters_collected)
{
    return collect_votes_impl(d, table, voter, vote_data, voting_seconds, pv_witness_substitution, required_witnesses, voters_collected);
}

const table_voting_object &collect_votes(database& d,
                   const table_object &table,
                   const account_id_type &voter,
                   const game_result &vote_data,
                   const uint32_t voting_seconds,
                   const percent_type pv_witness_substitution,
                   const game_witnesses_type &required_witnesses,
                   bool &voters_collected)
{
    return collect_votes_impl(d, table, voter, vote_data, voting_seconds, pv_witness_substitution, required_witnesses, voters_collected);
}

void apply_start_playing_with_consensus(database& d, const table_object &table,
                                       const voting_data_type &valid_vote,
                                       const game_witnesses_type &required_witnesses,
                                       const account_votes_type &accounts_with_invalid_vote)
{
    game_initial_data initial_data = valid_vote.get<game_initial_data>();

    account_id_type table_owner = table.room(d).owner;

    const auto& dyn_props = d.get_dynamic_global_properties();

    const auto& parameters = get_playchain_parameters(d);

#if defined(LOG_VOTING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
    {
        ilog("${t} >> apply_start_playing_with_consensus:", ("t", d.head_block_time()));
        idump((initial_data));
        idump((table));
    }
#endif

    d.modify(table, [&](table_object &obj)
    {
        decltype(obj.cash) cash_playing;
        std::for_each(begin(initial_data.cash), end(initial_data.cash),
                       [&](const decltype(initial_data.cash)::value_type &data)
        {
            const player_object &player = get_player(d, data.first);

            cash_playing[player.id] = data.second;
        });

        std::for_each(begin(cash_playing), end(cash_playing),
                      [&](const decltype(obj.cash)::value_type &data)
        {
            const auto &player_id = data.first;
            const auto &player_cash = data.second;

            obj.adjust_playing_cash(player_id, player_cash);

            if (d.head_block_time() >= HARDFORK_PLAYCHAIN_1_TIME && d.head_block_time() < HARDFORK_PLAYCHAIN_8_TIME)
            {
                assert(obj.cash.find(player_id) != obj.cash.end());
            }

            obj.adjust_cash(player_id, -player_cash);
        });

        obj.game_created = dyn_props.time;
        obj.game_expiration = obj.game_created + fc::seconds(parameters.game_lifetime_limit_in_seconds);

        obj.voted_witnesses = required_witnesses;
    });

    d.push_applied_operation(
                game_event_operation{ table.id, table_owner, game_start_playing_validated{ initial_data } } );

    for(const auto &data: accounts_with_invalid_vote)
    {
        const auto &fail_info = data.second.get<game_initial_data>().info;
        d.push_applied_operation(
                    game_event_operation{ table.id, table_owner,
                                          fraud_game_start_playing_check{data.first, fail_info, initial_data.info} } );
    }

#if defined(LOG_VOTING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
    {
        idump((table));
    }
#endif
}

void apply_game_result_with_consensus(database& d, const table_object &table,
                                       const voting_data_type &valid_vote,
                                       const game_witnesses_type &required_witnesses,
                                       const account_votes_type &accounts_with_invalid_vote)
{
    game_result result = valid_vote.get<game_result>();

    account_id_type table_owner = table.room(d).owner;

#if defined(LOG_VOTING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
    {
        ilog("${t} >> apply_game_result_with_consensus:", ("t", d.head_block_time()));
        idump((result));
        idump((table));
    }
#endif

    if (result.cash.empty())
    {
        //there was a vote to cancel the game
        roolback(d, table, false);
    }else
    {
        auto apply_impl = [&]()
        {
            pending_buyouts_resolve(d, table, table_owner, result);

            d.modify(table, [&](table_object &table_obj)
            {
                auto room = table_obj.room;
                asset room_rake;

                decltype(table_obj.playing_cash) cash_result;
                std::for_each(begin(result.cash), end(result.cash),
                               [&](const decltype(result.cash)::value_type &data)
                {
                    const account_id_type &player_account_id = data.first;
                    const player_object &player = get_player(d, player_account_id);
                    const gamer_cash_result &gamer_result = data.second;

                    if (gamer_result.rake.amount > 0)
                    {
                        d.modify(player, [&table_obj, &player_account_id, &room, &gamer_result, &required_witnesses](player_object &obj)
                        {
                            obj.pending_fees.emplace_back( player_account_id, table_obj.metadata, gamer_result.rake, room, required_witnesses);
                        });

                        room_rake += gamer_result.rake;
                    }

                    cash_result[player.id] = gamer_result.cash;
                });

                d.modify(room(d), [&room_rake](room_object &obj)
                {
                    obj.pending_rake += room_rake;
                });

                std::for_each(begin(cash_result), end(cash_result),
                              [&](const decltype(table_obj.cash)::value_type &data)
                {
                    assert(table_obj.playing_cash.end() != table_obj.playing_cash.find(data.first));

                    auto &&new_amount = data.second;
                    if (new_amount.amount > 0)
                    {
                        table_obj.adjust_cash(data.first, new_amount);
                    }
                    auto &&old_amount = table_obj.playing_cash[data.first];
                    table_obj.adjust_playing_cash(data.first, -old_amount);
                });

                auto rest_playing_cash = table_obj.playing_cash;
                std::for_each(begin(rest_playing_cash), end(rest_playing_cash),
                              [&](const decltype(table_obj.playing_cash)::value_type &data)
                {
                    auto &&amount = data.second;
                    table_obj.adjust_playing_cash(data.first, -amount);
                    table_obj.adjust_cash(data.first, amount);
                });

                table_obj.game_created = fc::time_point_sec::min();
                table_obj.game_expiration = fc::time_point_sec::maximum();

                table_obj.voted_witnesses.clear();
            });

            d.push_applied_operation(
                        game_event_operation{ table.id, table_owner, game_result_validated{ result } } );
        };

        if (d.head_block_time() >= HARDFORK_PLAYCHAIN_10_TIME)
        {
            with_registration_buy_in(d, table, apply_impl);
        }else
        {
            apply_impl();
            obsolete_buyins_resolve(d, table, false);
        }
    }

    for(const auto &data: accounts_with_invalid_vote)
    {
        const auto &fail_log = data.second.get<game_result>().log;
        d.push_applied_operation(
                    game_event_operation{ table.id, table_owner,
                                          fraud_game_result_check{data.first, fail_log, result.log} } );
    }

#if defined(LOG_VOTING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
    {
        idump((table));
    }
#endif
}

void cleanup_pending_votes(database& d, const table_object &table, const char *from)
{
    auto &&current_time_point = d.head_block_time();
    if (current_time_point >= HARDFORK_PLAYCHAIN_1_TIME)
    {
        const auto& parameters = get_playchain_parameters(d);
        const auto &index_pending = d.get_index_type<pending_table_vote_index>().indices().get<by_table>();
        auto range = index_pending.equal_range(table.id);
        if (range.first != range.second && from)
        {
            dlog("from '${f}' prepare cleanup pending votes at table = ${t}", ("f", from)("t", table.id));
        }
        auto pending = get_objects_from_index<pending_table_vote_object>(range.first,
                                                                         range.second,
                                                                         parameters.maximum_desired_number_of_players_for_tables_allocation * 10);
        for (const pending_table_vote_object& vote_obj: pending) {
            wlog("At ${t} fail vote: ${v}", ("t", d.head_block_time())("v", vote_obj));
            push_fail_vote_operation(d, vote_obj);
            d.remove(vote_obj);
        }
    }
}

void cleanup_voting(database& d, const table_id_type &table_id)
{
    auto& voting_by_table= d.get_index_type<table_voting_index>().indices().get<by_table>();
    auto it = voting_by_table.find(table_id);
    if (voting_by_table.end() != it)
    {
        d.remove(*it);
    }
}

void roolback(database& d, const table_object &table, bool full_clear)
{
    auto roolback_impl = [&]()
    {
        const auto &table_owner = table.room(d).owner;

        d.push_applied_operation(
                    game_event_operation{ table.id, table_owner, game_rollback{} } );

        game_result empty_result;
        pending_buyouts_resolve(d, table, table_owner, empty_result);

        if (full_clear)
        {
            decltype(table.cash) rooling_cash;

            std::copy(begin(table.playing_cash), end(table.playing_cash), std::inserter(rooling_cash, rooling_cash.end()));

            for(const auto &data: table.cash)
            {
                rooling_cash[data.first] += data.second;
            }

            for(const auto &data: rooling_cash)
            {
                const player_object &player = data.first(d);

                d.push_applied_operation(game_event_operation{table.id, table_owner, buy_in_return{player.account, data.second}});

                d.adjust_balance(player.account, data.second);
            }
        }else
        {
            for(const auto &data: table.playing_cash)
            {
                const player_object &player = data.first(d);

                assert(data.second.amount > 0);

                d.push_applied_operation(game_event_operation{table.id, table_owner, game_cash_return{player.account, data.second}});
            }
        }

        d.modify(table, [&](table_object &obj)
        {
            obj.game_created = fc::time_point_sec::min();
            obj.game_expiration = fc::time_point_sec::maximum();

            if (full_clear)
            {
                obj.clear_playing_cash();
                obj.clear_cash();
            }else
            {
                for(const auto &data: obj.playing_cash)
                {
                    assert(data.second.amount > 0);

                    obj.adjust_cash(data.first, data.second);
                }
                obj.clear_playing_cash();
            }
        });

        cleanup_pending_votes(d, table, "roolback");
    };

    if (d.head_block_time() >= HARDFORK_PLAYCHAIN_10_TIME)
    {
        if (!full_clear)
        {
            with_registration_buy_in(d, table, roolback_impl);
        }else
        {
            roolback_impl();
            cleanup_buy_ins(d, table);
        }

        cleanup_voting(d, table.id);
    }else
    {
        roolback_impl();
        obsolete_buyins_resolve(d, table, full_clear);
    }
}

void push_fail_vote_operation(database &d, const game_start_playing_check_operation &op)
{
    d.push_applied_operation(
                game_event_operation{ op.table, op.table_owner, fail_vote{ op.voter, op.initial_data} });
}

void push_fail_vote_operation(database &d, const game_result_check_operation &op)
{
    d.push_applied_operation(
                game_event_operation{ op.table, op.table_owner, fail_vote{ op.voter, op.result} });
}

void push_fail_vote_operation(database &d, const pending_table_vote_object &obj)
{
    auto which = obj.vote.op.which();
    if (which == operation::tag<game_start_playing_check_operation>::value)
    {
        push_fail_vote_operation(d, obj.vote.op.get<game_start_playing_check_operation>());
    }else if (which == operation::tag<game_result_check_operation>::value)
    {
        push_fail_vote_operation(d, obj.vote.op.get<game_result_check_operation>());
    }
}

void push_fail_vote_operation(database &d, const table_object &table,
                              const account_id_type &voter, const game_initial_data &vote)
{
    game_start_playing_check_operation op_reverted;

    op_reverted.table = table.id;
    op_reverted.table_owner = table.room(d).owner;
    op_reverted.voter = voter;
    op_reverted.initial_data = vote;

    push_fail_vote_operation(d, op_reverted);
}

game_start_playing_check_evaluator_impl::game_start_playing_check_evaluator_impl(generic_evaluator &ev): _ev(ev)
{}
database& game_start_playing_check_evaluator_impl::db() const
{
    return _ev.db();
}

game_result_check_evaluator_impl::game_result_check_evaluator_impl(generic_evaluator &ev): _ev(ev)
{}
database& game_result_check_evaluator_impl::db() const
{
    return _ev.db();
}

/////////////////////////////////////////////////////////////////////////////
// Last version of voting algorithm here. All old versions should be at
// game_evaluators_obsolete (for history chain syncronization)

void_result game_start_playing_check_evaluator_impl_v2::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();

        FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");

        const table_object &table = op.table(d);

        FC_ASSERT(is_table_owner(d, table, op.table_owner), "Wrong table owner");

        FC_ASSERT(op.initial_data.cash.size() >= 2, "Invalid data to vote. At least two players required");

        FC_ASSERT(table.is_free(), "Wrong type of voting. There is game on table");

        check_incoming_vote(d, table, op);

        return void_result{};
    }FC_CAPTURE_AND_RETHROW((op))
}

operation_result game_start_playing_check_evaluator_impl_v2::do_apply( const operation_type& op )
{
    try {
        database& d = db();

        const table_object &table = op.table(d);

        const auto& parameters = get_playchain_parameters(d);
        game_witnesses_type null;

#if defined(LOG_VOTING)
        if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
        {
            ilog("${t} >> game_start_playing_check_evaluator_impl_v2:", ("t", d.head_block_time()));
            idump((table));
            idump((op));
        }
#endif

        return try_voting(d,
                          table,
                          null,
                          parameters.voting_for_playing_expiration_seconds,
                          parameters.percentage_of_voter_witness_substitution_while_voting_for_playing,
                          op);
    }FC_CAPTURE_AND_RETHROW((op))
}

void_result game_result_check_evaluator_impl_v2::do_evaluate( const operation_type& op )
{
    try {
        const database& d = db();

        FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");

        const table_object &table = op.table(d);

        FC_ASSERT(is_table_owner(d, table, op.table_owner), "Wrong table owner");

        FC_ASSERT(table.is_playing(), "Wrong type of voting. There is no game on table");

        check_incoming_vote(d, table, op);

        return void_result{};
    }FC_CAPTURE_AND_RETHROW((op))
}

operation_result game_result_check_evaluator_impl_v2::do_apply( const operation_type& op )
{
    try {
        database& d = db();

        const table_object &table = op.table(d);

        const auto& parameters = get_playchain_parameters(d);

#if defined(LOG_VOTING)
        if (d.head_block_time() >= fc::time_point_sec( LOG_VOTING_BLOCK_TIMESTUMP_FROM ))
        {
            ilog("${t} >> game_result_check_evaluator_impl_v2:", ("t", d.head_block_time()));
            idump((table));
            idump((op));
        }
#endif

        return try_voting(d,
                          table,
                          table.voted_witnesses,
                          parameters.voting_for_results_expiration_seconds,
                          parameters.percentage_of_voter_witness_substitution_while_voting_for_results,
                          op);
    }FC_CAPTURE_AND_RETHROW((op))
}
}}
