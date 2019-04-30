#include "playchain_common.hpp"

#include <playchain/chain/schema/playchain_committee_member_object.hpp>
#include <playchain/chain/schema/playchain_property_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/validators.hpp>

#include <graphene/chain/committee_member_object.hpp>

namespace committee_tests
{
struct committee_fixture: public playchain_common::playchain_fixture
{
    const int64_t rich_registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(member1)
    DECLARE_ACTOR(member2)
    DECLARE_ACTOR(member3)
    DECLARE_ACTOR(player0)
    DECLARE_ACTOR(player1)
    DECLARE_ACTOR(player2)
    DECLARE_ACTOR(player3)

    committee_fixture()
    {
        try
        {
            actor(member1).supply(asset(rich_registrator_init_balance));
        }FC_LOG_AND_RETHROW()
        actor(member2).supply(asset(rich_registrator_init_balance));
        actor(member3).supply(asset(rich_registrator_init_balance));

        init_fees();

        CREATE_PLAYER(member1, player1);
        CREATE_PLAYER(member1, player2);
        CREATE_PLAYER(member1, player3);
    }

    struct proposal_info
    {
        proposal_info() = default;

        object_id_type id;
        fc::time_point_sec expiration_time;
    };

    void shrink_committee(uint32_t new_capacity = 1)
    {
        idump((db.get_global_properties().active_committee_members));

        Actor member{"init0", init_account_priv_key, init_account_priv_key.get_public_key()};

        actor(member).supply(asset(rich_registrator_init_balance));

        const chain_parameters& current_params = db.get_global_properties().parameters;
        chain_parameters new_params = current_params;

        new_params.maximum_committee_count = new_capacity;

        committee_member_update_global_parameters_operation update_op;
        update_op.new_parameters = new_params;

        proposal_create_operation prop_op;

        prop_op.review_period_seconds = current_params.committee_proposal_review_period;
        prop_op.expiration_time = db.head_block_time() + current_params.committee_proposal_review_period + (uint32_t)fc::minutes(1).to_seconds();
        prop_op.fee_paying_account = get_account(member).id;

        prop_op.proposed_ops.emplace_back( update_op );

        object_id_type proposal_id;

        try
        {
            proposal_id = actor(member).push_operation(prop_op).operation_results.front().get<object_id_type>();
        }FC_LOG_AND_RETHROW()

        proposal_update_operation prop_update_op;

        prop_update_op.fee_paying_account = get_account(member).id;
        prop_update_op.proposal = proposal_id;
        prop_update_op.active_approvals_to_add.insert( get_account( member ).id );

        try
        {
            actor(member).push_operation(prop_update_op);
        }FC_LOG_AND_RETHROW()

        generate_blocks(prop_op.expiration_time);

        next_maintenance();

        BOOST_REQUIRE_EQUAL(db.get_global_properties().parameters.maximum_committee_count, new_capacity);
    }

    const playchain_committee_member_object &get_playchain_committee_member(const Actor &member) const
    {
        auto &&member_account_object = get_account(member);

        using namespace playchain::chain;

        BOOST_REQUIRE(is_game_witness(db, member_account_object.id));

        auto &&witness = get_game_witness(db, member_account_object.id);

        const auto& committee_members_by_witness = db.get_index_type<playchain_committee_member_index>().indices().get<by_game_witness>();
        auto it = committee_members_by_witness.find(witness.id);
        BOOST_REQUIRE(committee_members_by_witness.end() != it);

        return (*it);
    }

    bool is_active_playchain_committee_member(const Actor &member) const
    {
        auto account_id = get_account(member).id;
        auto &&active = get_playchain_properties(db).active_games_committee_members;
        auto lmb = [this, &account_id](const game_witness_id_type &w_id)
        {
            return w_id(db).account == account_id;
        };
        return active.end() != std::find_if(begin(active), end(active), lmb);
    }

    void vote_for_playchain_committee_member(const Actor &member, const Actor &voter, bool approve = true)
    {
        auto vote_id = get_playchain_committee_member(member).vote_id;
        vote_for(vote_id, voter, approve);
    }

    void approve_proposal(const object_id_type &proposal_id, const Actor &member, bool approve = true)
    {
        BOOST_REQUIRE(db.find_object(proposal_id));

        proposal_update_operation prop_update_op;

        prop_update_op.fee_paying_account = get_account(member).id;
        prop_update_op.proposal = proposal_id;
        if (approve)
        {
            prop_update_op.active_approvals_to_add.insert( get_account( member ).id );
        }
        else
        {
            prop_update_op.active_approvals_to_remove.insert( get_account( member ).id );
        }

        try
        {
            actor(member).push_operation(prop_update_op);
        }FC_LOG_AND_RETHROW()
    }

    void elect_member(const Actor &member)
    {
        vote_for_playchain_committee_member(member, member);

        next_maintenance();

        BOOST_REQUIRE(is_active_playchain_committee_member(member));

        ilog("Playchain Commitee: ${l}", ("l", get_playchain_properties(db).active_games_committee_members));
    }

    template <typename... Args> void elect_members(Args... member)
    {
        std::array<Actor, sizeof...(member)> list = { member... };
        for (Actor& a : list)
        {
            for (Actor& a_for : list)
            {
                vote_for_playchain_committee_member(a_for, a);
            }
        }

        next_maintenance();

        for (Actor& a : list)
        {
            BOOST_REQUIRE(is_active_playchain_committee_member(a));
        }

        ilog("Playchain Commitee: ${l}", ("l", get_playchain_properties(db).active_games_committee_members));
    }

    proposal_info propose_playchain_params_change(const Actor &member, const playchain_parameters &new_params,
                                                  bool review = true,
                                                  const fc::microseconds &ext_review = fc::minutes(1))
    {
        auto time = db.head_block_time();

        playchain_committee_member_update_parameters_operation update_op;
        update_op.new_parameters = new_params;

        proposal_create_operation prop_op;

        const chain_parameters& current_params = db.get_global_properties().parameters;

        if (review)
        {
            prop_op.review_period_seconds = current_params.committee_proposal_review_period;
            prop_op.expiration_time = time + current_params.committee_proposal_review_period + (uint32_t)ext_review.to_seconds();
        }else
        {
            prop_op.expiration_time = time + (uint32_t)ext_review.to_seconds();
        }
        prop_op.fee_paying_account = get_account(member).id;

        prop_op.proposed_ops.emplace_back( update_op );

        object_id_type proposal_id;

        try
        {
            proposal_id = actor(member).push_operation(prop_op).operation_results.front().get<object_id_type>();
        }FC_LOG_AND_RETHROW()

        proposal_update_operation prop_update_op;

        prop_update_op.fee_paying_account = get_account(member).id;
        prop_update_op.proposal = proposal_id;
        prop_update_op.active_approvals_to_add.insert( get_account( member ).id );

        try
        {
            actor(member).push_operation(prop_update_op);
        }FC_LOG_AND_RETHROW()

        return {proposal_id, prop_op.expiration_time};
    }
};

BOOST_FIXTURE_TEST_SUITE( committee_tests, committee_fixture)

PLAYCHAIN_TEST_CASE(check_create_committee_operation)
{    
    create_witness(member1);
    create_witness(member2);

    auto initial_url_for_member1 = "https://hub.docker.com/r/totalpoker/jenkins.backend";

    try
    {
        playchain_committee_member_create(member1, initial_url_for_member1);
    }FC_LOG_AND_RETHROW()
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2, ""));

    //dublicate
    BOOST_CHECK_THROW(playchain_committee_member_create(member1, initial_url_for_member1), fc::exception);
    BOOST_CHECK_THROW(playchain_committee_member_create(member1, ""), fc::exception);
    BOOST_CHECK_THROW(playchain_committee_member_create(member2, ""), fc::exception);

    //member3 is not witness
    BOOST_CHECK_THROW(playchain_committee_member_create(member3, ""), fc::exception);

    //there is no player0
    BOOST_CHECK_THROW(playchain_committee_member_create(player0, ""), fc::exception);

    //player1 is not witness
    BOOST_CHECK_THROW(playchain_committee_member_create(player1, ""), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_update_committee_operation)
{
    create_witness(member1);
    create_witness(member2);
    create_witness(member3);

    auto initial_url_for_member1 = "";
    auto initial_url_for_member2 = "https://www.google.com";
    auto initial_url_for_member3 = "https://en.wikipedia.org/wiki/Eos";

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1, initial_url_for_member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2, initial_url_for_member2));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member3, initial_url_for_member3));

    generate_block();

    BOOST_REQUIRE_EQUAL(playchain_committee_member_id_type{0}(db).url, initial_url_for_member1);
    BOOST_REQUIRE_EQUAL(playchain_committee_member_id_type{1}(db).url, initial_url_for_member2);
    BOOST_REQUIRE_EQUAL(playchain_committee_member_id_type{2}(db).url, initial_url_for_member3);

    auto new_url_for_member1 = "https://github.com";
    auto new_url_for_member2 = "https://duckduckgo.com";

    BOOST_CHECK_NO_THROW(playchain_committee_member_update(member1, playchain_committee_member_id_type{0}, new_url_for_member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_update(member2, playchain_committee_member_id_type{1}, new_url_for_member2));
    BOOST_CHECK_NO_THROW(playchain_committee_member_update(member3, playchain_committee_member_id_type{2}));

    BOOST_CHECK_EQUAL(playchain_committee_member_id_type{0}(db).url, new_url_for_member1);
    BOOST_CHECK_EQUAL(playchain_committee_member_id_type{1}(db).url, new_url_for_member2);
    BOOST_CHECK_EQUAL(playchain_committee_member_id_type{2}(db).url, initial_url_for_member3);
}

PLAYCHAIN_TEST_CASE(check_proposal_validation_with_single_member)
{
    create_witness(member1);

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));

    generate_block();

    elect_member(member1);

    playchain_parameters new_params = get_playchain_properties(db).parameters;

    BOOST_REQUIRE_NE(new_params.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds/ 2);

    new_params.game_lifetime_limit_in_seconds /= 2;

    auto proposal_info = propose_playchain_params_change(member1, new_params);

    generate_blocks(proposal_info.expiration_time);

    next_maintenance();

    BOOST_CHECK_EQUAL(get_playchain_properties(db).parameters.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds);
}

PLAYCHAIN_TEST_CASE(check_proposal_validation_with_multiple_members)
{
    create_witness(member1);
    create_witness(member2);
    create_witness(member3);

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member3));

    elect_members(member1, member2, member3);

    playchain_parameters new_params = get_playchain_properties(db).parameters;

    BOOST_REQUIRE_NE(new_params.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds/ 2);

    new_params.game_lifetime_limit_in_seconds /= 2;

    auto proposal_info = propose_playchain_params_change(member2, new_params);

    approve_proposal(proposal_info.id, member2, true);
    //one vote is not enough. W = sum(w)/2 + 1 is required
    approve_proposal(proposal_info.id, member3, true);

    generate_blocks(proposal_info.expiration_time);

    next_maintenance();

    BOOST_CHECK_EQUAL(get_playchain_properties(db).parameters.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds);
}

PLAYCHAIN_TEST_CASE(check_player_proposal_deny)
{
    create_witness(member1);
    create_witness(member2);

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2));

    elect_members(member1, member2);

    playchain_parameters new_params = get_playchain_properties(db).parameters;

    BOOST_REQUIRE_NE(new_params.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds/ 3);

    new_params.game_lifetime_limit_in_seconds /= 3;

    auto proposal_info = propose_playchain_params_change(player1, new_params);

    approve_proposal(proposal_info.id, player1, true);

    generate_blocks(proposal_info.expiration_time);

    next_maintenance();

    BOOST_CHECK_NE(get_playchain_properties(db).parameters.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds);
}

PLAYCHAIN_TEST_CASE(check_player_proposal_approve)
{    
    create_witness(member1);
    create_witness(member2);

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2));

    elect_members(member1, member2);

    playchain_parameters new_params = get_playchain_properties(db).parameters;

    BOOST_REQUIRE_NE(new_params.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds/ 3);

    new_params.game_lifetime_limit_in_seconds /= 3;

    auto proposal_info = propose_playchain_params_change(player2, new_params);

    approve_proposal(proposal_info.id, member1, true);
    //one vote is not enough. W = sum(w)/2 + 1 is required
    approve_proposal(proposal_info.id, member2, true);

    generate_blocks(proposal_info.expiration_time);

    next_maintenance();

    BOOST_CHECK_EQUAL(get_playchain_properties(db).parameters.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds);
}

PLAYCHAIN_TEST_CASE(check_change_committee_composition)
{
    shrink_committee(2);

    ilog("Playchain Commitee (0): ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

    create_witness(member1);

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));

    vote_for_playchain_committee_member(member1, member1);

    next_maintenance();

    ilog("Playchain Commitee (1): ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

    create_witness(player1);
    try
    {
        playchain_committee_member_create(player1);
    }FC_LOG_AND_RETHROW();

    next_maintenance();

    ilog("Playchain Commitee (2): ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

    for(const auto &wid: get_playchain_properties(db).active_games_committee_members)
    {
        const auto &cm = playchain_property_object::get_committee_member(db, wid);
        idump((cm));
    }

    BOOST_CHECK(is_active_playchain_committee_member(member1));
    BOOST_CHECK(!is_active_playchain_committee_member(player1));

    //change commitee by votes
    vote_for_playchain_committee_member(player1, player1);
    vote_for_playchain_committee_member(player1, player2);
    vote_for_playchain_committee_member(player1, member2);

    next_maintenance();

    ilog("Playchain Commitee (3): ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

    for(const auto &wid: get_playchain_properties(db).active_games_committee_members)
    {
        const auto &cm = playchain_property_object::get_committee_member(db, wid);
        idump((cm));
    }

    BOOST_CHECK(!is_active_playchain_committee_member(member1));
    BOOST_CHECK(is_active_playchain_committee_member(player1));

    //enlarge for two members
    vote_for_playchain_committee_member(member1, member2);
    vote_for_playchain_committee_member(member1, member3);
    vote_for_playchain_committee_member(player1, member3);

    next_maintenance();

    ilog("Playchain Commitee (4): ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

    for(const auto &wid: get_playchain_properties(db).active_games_committee_members)
    {
        const auto &cm = playchain_property_object::get_committee_member(db, wid);
        idump((cm));
    }

    BOOST_CHECK(is_active_playchain_committee_member(member1));
    BOOST_CHECK(is_active_playchain_committee_member(player1));

    //reduce to one
    vote_for_playchain_committee_member(player1, member2, false);
    vote_for_playchain_committee_member(player1, member3, false);

    next_maintenance();

    ilog("Playchain Commitee (5): ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

    for(const auto &wid: get_playchain_properties(db).active_games_committee_members)
    {
        const auto &cm = playchain_property_object::get_committee_member(db, wid);
        idump((cm));
    }

    BOOST_CHECK(is_active_playchain_committee_member(member1));
    BOOST_CHECK(!is_active_playchain_committee_member(player1));
}

PLAYCHAIN_TEST_CASE(check_committee_proposal_without_review)
{
    create_witness(member1);
    create_witness(member2);

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2));

    elect_members(member1, member2);

    playchain_parameters new_params = get_playchain_properties(db).parameters;

    BOOST_REQUIRE_NE(new_params.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds/ 3);

    new_params.game_lifetime_limit_in_seconds /= 3;

    BOOST_CHECK_THROW(propose_playchain_params_change(member1, new_params, false), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_monopolistic_member_proposal_approve)
{
    create_witness(member1);
    create_witness(player1);
    create_witness(player2);

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(player1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(player2));

    vote_for_playchain_committee_member(member1, member1); //1
    vote_for_playchain_committee_member(member1, member2);
    vote_for_playchain_committee_member(member1, member3);
    vote_for_playchain_committee_member(member1, player1); //3
    vote_for_playchain_committee_member(member1, player2);
    vote_for_playchain_committee_member(member1, player3);
    vote_for_playchain_committee_member(player1, player1); //3
    vote_for_playchain_committee_member(player1, player2);
    vote_for_playchain_committee_member(player1, player3);
    vote_for_playchain_committee_member(player2, player1); //3
    vote_for_playchain_committee_member(player2, player2);
    vote_for_playchain_committee_member(player2, player3);

    next_maintenance();

    ilog("Playchain Commitee: ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

    BOOST_REQUIRE(is_active_playchain_committee_member(member1));
    BOOST_REQUIRE(is_active_playchain_committee_member(player1));
    BOOST_REQUIRE(is_active_playchain_committee_member(player2));

    {
        playchain_parameters new_params = get_playchain_properties(db).parameters;

        BOOST_REQUIRE_NE(new_params.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds/ 3);

        new_params.game_lifetime_limit_in_seconds /= 3;

        auto proposal_info = propose_playchain_params_change(member1, new_params);

        //one approve is enough for monopolist
        approve_proposal(proposal_info.id, member1, true);

        generate_blocks(proposal_info.expiration_time);

        next_maintenance();

        BOOST_CHECK_EQUAL(get_playchain_properties(db).parameters.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds);
    }

    {
        playchain_parameters new_params = get_playchain_properties(db).parameters;

        BOOST_REQUIRE_NE(new_params.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds/ 3);

        new_params.game_lifetime_limit_in_seconds /= 4;

        auto proposal_info = propose_playchain_params_change(player1, new_params);

        //all approves is not enough for others memebers
        approve_proposal(proposal_info.id, player1, true);
        approve_proposal(proposal_info.id, player2, true);

        generate_blocks(proposal_info.expiration_time);

        next_maintenance();

        BOOST_CHECK_NE(get_playchain_properties(db).parameters.game_lifetime_limit_in_seconds, new_params.game_lifetime_limit_in_seconds);
    }
}

PLAYCHAIN_TEST_CASE(check_get_committee_members_api)
{
    auto witness1 = create_witness(member1).id;
    auto witness2 = create_witness(member2).id;

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2));

    vector<playchain_committee_member_id_type> members_ids = {playchain_committee_member_id_type(0), playchain_committee_member_id_type(1)};
    auto members_objs = pplaychain_api->get_committee_members(members_ids);

    BOOST_CHECK_EQUAL(members_objs.size(), 2u);

    idump((members_objs));

    BOOST_CHECK(members_objs[0]->committee_member_game_witness == witness1);
    BOOST_CHECK(members_objs[1]->committee_member_game_witness == witness2);
}

PLAYCHAIN_TEST_CASE(check_get_committee_member_by_account_api)
{
    auto witness1 = create_witness(member1).id;

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));

    BOOST_REQUIRE(pplaychain_api->get_committee_member_by_account(member1.name).valid());
    BOOST_REQUIRE(pplaychain_api->get_committee_member_by_account(member1.name)->committee_member_game_witness == witness1);

    BOOST_CHECK(!pplaychain_api->get_committee_member_by_account(member2.name).valid());
}

PLAYCHAIN_TEST_CASE(check_list_all_committee_members_api)
{
    BOOST_CHECK_THROW(pplaychain_api->list_all_committee_members(
                          "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                      fc::exception);

    BOOST_CHECK(pplaychain_api->list_all_committee_members("", 1).empty());

    create_witness(member1).id;
    create_witness(member2).id;

    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member1));
    BOOST_CHECK_NO_THROW(playchain_committee_member_create(member2));

    BOOST_CHECK_EQUAL(pplaychain_api->list_all_committee_members("", 1).size(), 1u);
    BOOST_CHECK_EQUAL(pplaychain_api->list_all_committee_members("", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST).size(), 2u);
}

BOOST_AUTO_TEST_SUITE_END()
}
