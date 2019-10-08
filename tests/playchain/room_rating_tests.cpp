#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/playchain_property_object.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <graphene/chain/hardfork.hpp>

#include <algorithm>

namespace room_rating_tests
{
struct room_rating_fixture : public playchain_common::playchain_fixture
{
    const int64_t registrator_init_balance = 3000 * GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator)
    DECLARE_ACTOR(richregistrator2)
    DECLARE_ACTOR(temp)

    room_rating_fixture()
    {
        actor(richregistrator).supply(asset(registrator_init_balance));
        actor(richregistrator2).supply(asset(registrator_init_balance));
        actor(temp).supply(asset(player_init_balance));

        init_fees();

        generate_blocks(HARDFORK_PLAYCHAIN_11_TIME);
    }

    table_id_type create_new_table(const Actor& owner, const room_id_type &room, const amount_type required_witnesses = 0u, const std::string &metadata = {})
    {
        table_id_type table = playchain_common::playchain_fixture::create_new_table(owner, room, required_witnesses, metadata);
        table_alive(owner, table);
        return table;
    }

    void shrink_maintenance()
    {
        idump((db.get_global_properties().active_committee_members));

        Actor member{"init0", init_account_priv_key, init_account_priv_key.get_public_key()};

        actor(member).supply(asset(registrator_init_balance));

        const chain_parameters& current_params = db.get_global_properties().parameters;
        chain_parameters new_params = current_params;

        auto expected_maintenance_interval = playchain::protocol::detail::get_config().table_alive_expiration_seconds / 2;

        new_params.maintenance_interval = expected_maintenance_interval;

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

        BOOST_REQUIRE_EQUAL(db.get_global_properties().parameters.maintenance_interval, expected_maintenance_interval);
    }
};

BOOST_FIXTURE_TEST_SUITE(room_rating_tests, room_rating_fixture)

PLAYCHAIN_TEST_CASE(check_rating_changes_only_in_maintance)
{
    const std::string protocol_version = "1.0.0+20190223";
    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    auto initial_room_rating = room(db).rating;

    generate_block();

    auto stake = asset(player_init_balance / 3);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));

    generate_block();

    BOOST_REQUIRE(!table(db).pending_proposals.empty());
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));

    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - GRAPHENE_DEFAULT_BLOCK_INTERVAL);

    BOOST_CHECK_EQUAL(initial_room_rating, room(db).rating);

    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

    BOOST_CHECK_NE(initial_room_rating, room(db).rating);
}

PLAYCHAIN_TEST_CASE(check_failed_resolve_results_in_rating_degrodation)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "player3", asset(player_init_balance));
    Actor player4 = create_new_player(richregistrator, "player4", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    generate_block();

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESERVE");

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 2;
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 2u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta, protocol_version));

    generate_block();

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESOLVE");

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));

    BOOST_TEST_MESSAGE("\nGENERATE_UNTIL_EXPIRATION");
    generate_blocks(db.get_dynamic_global_properties().time + fc::seconds(get_playchain_parameters(db).buy_in_expiration_seconds));

    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 0u);

    auto room_exelent_rating = room(db).rating;
    auto room_1fail_rating = room2(db).rating;

    BOOST_CHECK_LT(room_1fail_rating, room_exelent_rating);
}

PLAYCHAIN_TEST_CASE(check_more_buyins_give_higher_rating)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "player3", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    generate_block();

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESERVE");

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 2;
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 2u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta, protocol_version));

    generate_block();

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESOLVE");

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 1u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));

    generate_block();

    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 0u);

    BOOST_CHECK_LT(room2(db).rating, room(db).rating);
}

PLAYCHAIN_TEST_CASE(check_canceled_buyin_does_not_influence_rating)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "player3", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    generate_block();

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESERVE");

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 2;
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 2u);

    string player2_uid = get_next_uid(actor(player2));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, player2_uid, stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta, protocol_version));


    generate_block();

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESOLVE");

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 1u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel(player2, player2_uid));

    generate_block();

    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 0u);

    auto room_exelent_rating = room(db).rating;
    auto room_1fail_rating = room2(db).rating;

    BOOST_TEST_MESSAGE("\nroom_50percent_rating == " << room_1fail_rating << ", room_100percent_rating == " << room_exelent_rating);

    BOOST_CHECK_EQUAL(room(db).rating, room2(db).rating);
}

PLAYCHAIN_TEST_CASE(check_before_fade_start_buyins_give_the_same_contribution_to_rating)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - fc::minutes(TIME_FACTOR_FADE_START));

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 1;
        p.parameters.buy_in_expiration_seconds = (uint32_t)fc::minutes(TIME_FACTOR_FADE_START).to_seconds();
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 1u);


    BOOST_TEST_MESSAGE("\nSTART BUYIN ON TABLE1");
    table_alive(richregistrator, table);
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 1u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    generate_block();


    //move to begin of no-fade period
    generate_blocks(db.get_dynamic_global_properties().time + fc::minutes(TIME_FACTOR_FADE_START-1));


    BOOST_TEST_MESSAGE("\nSTART BUYIN ON TABLE2");
    table_alive(richregistrator, table2);
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
    generate_block();
    BOOST_TEST_MESSAGE("\n" << table(db).get_pending_proposals() << "  " << table2(db).get_pending_proposals());
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 1u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));
    generate_block();


    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 0u);

    auto room1_exelent_rating = room(db).rating;
    auto room2_exelent_rating = room2(db).rating;

    BOOST_TEST_MESSAGE("\nroom1_exelent_rating == " << room1_exelent_rating << ", room2_exelent_rating == " << room2_exelent_rating);

    BOOST_CHECK_EQUAL(room(db).rating, room2(db).rating);
}

PLAYCHAIN_TEST_CASE(check_notresolved_buyins_are_not_used_in_rating_count)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "player3", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    const auto delta_until_maintance = 2;

    generate_block();
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - fc::minutes(delta_until_maintance));

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESERVE");

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 2;
        p.parameters.pending_buyin_proposal_lifetime_limit_in_seconds = fc::minutes(delta_until_maintance + 1).to_seconds();
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 2u);

    table_alive(richregistrator, table);
    table_alive(richregistrator, table2);


    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta, protocol_version));

    generate_block();

    BOOST_TEST_MESSAGE("\nSTART BUYIN_RESOLVE");

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 1u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));

    generate_block();

    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 1u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 0u);

    BOOST_CHECK_EQUAL(room2(db).rating, room(db).rating);
}


namespace
{
    template <class T>
    T adjust_precision(const T& val)
    {
        return ROOM_RATING_PRECISION * val;
    }

    uint64_t K_factor(uint64_t x)
    {
        if (x <= TIME_FACTOR_FADE_START)
        {
            return adjust_precision(x);
        }
        else
        {
            return adjust_precision(TIME_FACTOR_C1 * std::exp(TIME_FACTOR_C2 * x) + TIME_FACTOR_C3);
        }
    }

    uint64_t f_N(uint64_t N)
    {
        if (N <= QUANTITY_FACTOR_FADE_START)
        {
            return adjust_precision(QUANTITY_FACTOR_C1 + N * QUANTITY_FACTOR_C2);
        }
        else
        {
            return adjust_precision(std::log(N) / QUANTITY_FACTOR_C3 + QUANTITY_FACTOR_C4);
        }
    }

    uint64_t stat_correction()
    {
        return adjust_precision(STAT_CORRECTION_M);
    }
}

PLAYCHAIN_TEST_CASE(check_calculation_formula)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "player3", asset(player_init_balance));
    Actor player4 = create_new_player(richregistrator, "player4", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    generate_block();

    auto diff_time1 = db.get_dynamic_global_properties().next_maintenance_time.sec_since_epoch() - db.get_dynamic_global_properties().time.sec_since_epoch();
    auto diff_time2 = diff_time1/2;


    BOOST_TEST_MESSAGE("\nSTART BUYIN ON TABLE1 at time T1");

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    generate_block();

    generate_blocks(db.get_dynamic_global_properties().time + diff_time2);

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 2;
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 2u);

    table_alive(richregistrator, table);
    table_alive(richregistrator, table2);

    BOOST_TEST_MESSAGE("\nSTART BUYIN ON TABLE1 at time T2");
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));
    generate_block();


    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - GRAPHENE_DEFAULT_BLOCK_INTERVAL);
    generate_block();

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);

    auto room1_rating_in_blockchain = room(db).rating;
    auto room2_rating_in_blockchain = room2(db).rating;

    //////////////////////////////////////////////////////////////////////////
    //      calculate room rating
    //////////////////////////////////////////////////////////////////////////

    auto room1_time_coef = K_factor(diff_time1/60) + K_factor(diff_time1 / 60) + K_factor(diff_time2 / 60) + K_factor(diff_time2 / 60);
    auto room1_weight_with_time_coef = 1*K_factor(diff_time1 / 60) + 0*K_factor(diff_time1 / 60) + 1*K_factor(diff_time2 / 60) + 0*K_factor(diff_time2 / 60);

    auto room2_time_coef = K_factor(diff_time2 / 60) + K_factor(diff_time2 / 60);
    auto room2_weight_with_time_coef = 1 * K_factor(diff_time2 / 60) + 0 * K_factor(diff_time2 / 60);

    const auto M = stat_correction();

    auto time_coef = room1_time_coef + room2_time_coef;
    auto weight_with_time_coef = room1_weight_with_time_coef + room2_weight_with_time_coef;

    BOOST_TEST_MESSAGE("\n"<< weight_with_time_coef<<" "<< time_coef<< " " << M << " ");

    const auto room1_calculated_rating = (room1_weight_with_time_coef + weight_with_time_coef * M/ time_coef) * 25 * f_N(4) / (room1_time_coef + M);
    const auto room2_calculated_rating = (room2_weight_with_time_coef + weight_with_time_coef * M / time_coef) * 25 * f_N(2) / (room2_time_coef + M);

    BOOST_CHECK_EQUAL(room1_rating_in_blockchain, room1_calculated_rating);
    BOOST_CHECK_EQUAL(room2_rating_in_blockchain, room2_calculated_rating);
}


PLAYCHAIN_TEST_CASE(check_initial_room_rating_equals_0_before_playchain_5hf)
{
    const std::string protocol_version = "1.0.0+20190223";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);

    BOOST_CHECK_EQUAL(0, room(db).rating);
}

PLAYCHAIN_TEST_CASE(check_initial_room_rating_equals_avarage_rating_after_playchain_5hf)
{
    const std::string protocol_version = "1.0.0+20190223";
    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    BOOST_CHECK_EQUAL(0, room(db).rating);

    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE(!table(db).pending_proposals.empty());
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    generate_block();


    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

    BOOST_REQUIRE_NE(0, room(db).rating);
    BOOST_REQUIRE_NE(0, get_dynamic_playchain_properties(db).average_room_rating);
    BOOST_REQUIRE_EQUAL(room(db).rating, get_dynamic_playchain_properties(db).average_room_rating);


    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    BOOST_REQUIRE_EQUAL(room2(db).rating, get_dynamic_playchain_properties(db).average_room_rating);
}

PLAYCHAIN_TEST_CASE(check_room_rake_changing)
{
    const std::string meta = "Game";

    room_id_type room1 = create_new_room(richregistrator, "room1");
    table_id_type table1_1 = create_new_table(richregistrator, room1, 0u, meta);
    table_id_type table1_2 = create_new_table(richregistrator, room1, 0u, meta);

    room_id_type room2 = create_new_room(richregistrator, "room2");
    table_id_type table2_1 = create_new_table(richregistrator, room2, 0u, meta);
    table_id_type table2_2 = create_new_table(richregistrator, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "p3", asset(player_init_balance));

    auto stake = asset(player_init_balance / 2);

    // update parameter for test needs
    db.modify(room1(db), [&](room_object& r) {
        r.rating = 10;
    });
    BOOST_REQUIRE_EQUAL(room1(db).rating, 10);
    BOOST_REQUIRE_EQUAL(room2(db).rating, 0);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table2_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table2_2));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table1_1(db).get_pending_proposals(), 3u);
    BOOST_CHECK_EQUAL(table1_2(db).get_pending_proposals(), 0u);
    BOOST_CHECK_EQUAL(table2_1(db).get_pending_proposals(), 0u);
    BOOST_CHECK_EQUAL(table2_2(db).get_pending_proposals(), 0u);

    generate_blocks(db.get_dynamic_global_properties().time + get_playchain_properties(db).parameters.pending_buyin_proposal_lifetime_limit_in_seconds);

    BOOST_REQUIRE_EQUAL(table1_1(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table1_2(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2_1(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2_2(db).get_pending_proposals(), 0u);

    // update parameter for test needs
    db.modify(room1(db), [&](room_object& r) {
        r.rating = 10;
    });
    db.modify(room2(db), [&](room_object& r) {
        r.rating = 20;
    });
    BOOST_REQUIRE_EQUAL(room1(db).rating, 10);
    BOOST_REQUIRE_EQUAL(room2(db).rating, 20);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table2_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table2_2));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table1_1(db).get_pending_proposals(), 0u);
    BOOST_CHECK_EQUAL(table1_2(db).get_pending_proposals(), 0u);
    BOOST_CHECK_EQUAL(table2_1(db).get_pending_proposals(), 3u);
    BOOST_CHECK_EQUAL(table2_2(db).get_pending_proposals(), 0u);
}

PLAYCHAIN_TEST_CASE(check_room_rake_hf12_broke_monopoly)
{
    shrink_maintenance();

    const std::string meta = "Game";
    const std::string protocol_version = "1.0.0+20190223";

    room_id_type room1 = create_new_room(richregistrator, "room1", protocol_version);

    table_id_type table1_1 = create_new_table(richregistrator, room1, 0u, meta);
    table_id_type table1_2 = create_new_table(richregistrator, room1, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));

    next_maintenance();

    room_id_type room2 = create_new_room(richregistrator2, "room2", protocol_version);

    table_id_type table2_1 = create_new_table(richregistrator2, room2, 0u, meta);
    table_id_type table2_2 = create_new_table(richregistrator2, room2, 0u, meta);
    table_id_type table2_3 = create_new_table(richregistrator2, room2, 0u, meta);
    table_id_type table2_4 = create_new_table(richregistrator2, room2, 0u, meta);
    table_id_type table2_5 = create_new_table(richregistrator2, room2, 0u, meta);

    next_maintenance();

    idump((room1(db)));
    idump((room2(db)));

    BOOST_REQUIRE_GT(room1(db).rating, room2(db).rating);

    generate_blocks(HARDFORK_PLAYCHAIN_12_TIME);

    BOOST_REQUIRE_EQUAL(get_playchain_properties(db).parameters.kpi_weight_per_measurement, 1u);
    BOOST_REQUIRE_EQUAL(get_playchain_properties(db).parameters.standby_weight_per_measurement, 1u);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));

    stake = asset(player_init_balance / 3);
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));

    next_maintenance();

    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));
    stake = asset(player_init_balance / 3);
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player2)));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_out_table(player2, table1_1, stake));

    table_id_type table1_3 = create_new_table(richregistrator, room1, 0u, meta);
    table_id_type table1_4 = create_new_table(richregistrator, room1, 0u, meta);
    table_id_type table1_5 = create_new_table(richregistrator, room1, 0u, meta);

    room_id_type room3 = create_new_room(richregistrator2, "room3", protocol_version);

    table_id_type table3_1 = create_new_table(richregistrator2, room3, 0u, meta);
    table_id_type table3_2 = create_new_table(richregistrator2, room3, 0u, meta);
    table_id_type table3_3 = create_new_table(richregistrator2, room3, 0u, meta);
    table_id_type table3_4 = create_new_table(richregistrator2, room3, 0u, meta);
    table_id_type table3_5 = create_new_table(richregistrator2, room3, 0u, meta);

    int ci = 3; //depend on standby_weight_per_measurement, (room2-3) tables amount, buy_in resolve per maintenance
    while(ci-- > 0)
    {
        //room1 keeps service all the time
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_3));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_4));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_5));
        BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
        BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
        generate_block();
        BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
        BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player2)));
        BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));
        BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));
        BOOST_REQUIRE_NO_THROW(buy_out_table(player1, table1_1, stake));
        BOOST_REQUIRE_NO_THROW(buy_out_table(player2, table1_1, stake));

        //room2 and room3 wait self rating grow up

        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2_1));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2_2));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2_3));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2_4));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2_5));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table3_1));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table3_2));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table3_3));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table3_4));
        BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table3_5));

        next_maintenance();

        idump((room1(db)));
        idump((room2(db)));
        idump((room3(db)));
    }

    BOOST_REQUIRE_LT(room1(db).rating, room2(db).rating);
    BOOST_REQUIRE_LT(room1(db).rating, room3(db).rating);
}

BOOST_AUTO_TEST_SUITE_END()

struct room_rating_v2_fixture : public room_rating_fixture
{
    room_rating_v2_fixture()
    {
        shrink_maintenance();

        generate_blocks(HARDFORK_PLAYCHAIN_12_TIME);

        set_playchain_parameters(1u, 2u);
    }

    void set_playchain_parameters( const uint16_t kpi_weight_per_measurement,
                                   const uint16_t standby_weight_per_measurement )
    {
        playchain_parameters new_params = get_playchain_properties(db).parameters;
        if (new_params.kpi_weight_per_measurement == kpi_weight_per_measurement &&
           new_params.standby_weight_per_measurement == standby_weight_per_measurement)
           return;

        create_witness(richregistrator);

        BOOST_CHECK_NO_THROW(playchain_committee_member_create(richregistrator));

        generate_block();

        elect_member(richregistrator);

        new_params.kpi_weight_per_measurement = kpi_weight_per_measurement;
        new_params.standby_weight_per_measurement = standby_weight_per_measurement;

        auto proposal_info = propose_playchain_params_change(richregistrator, new_params);

        approve_proposal(proposal_info.id, richregistrator, true);

        generate_blocks(proposal_info.expiration_time);

        next_maintenance();

        BOOST_REQUIRE_EQUAL(get_playchain_properties(db).parameters.kpi_weight_per_measurement, kpi_weight_per_measurement);
        BOOST_REQUIRE_EQUAL(get_playchain_properties(db).parameters.standby_weight_per_measurement, standby_weight_per_measurement);
    }
};

BOOST_FIXTURE_TEST_SUITE(room_rating_v2_tests, room_rating_v2_fixture)

PLAYCHAIN_TEST_CASE(check_room_rake_moving_to_free_rooms)
{
    const std::string meta = "Game";
    const std::string protocol_version = "1.0.0+20190223";

    room_id_type room1 = create_new_room(richregistrator, "room1", protocol_version);

    table_id_type table1 = create_new_table(richregistrator, room1, 0u, meta);

    room_id_type room2 = create_new_room(richregistrator2, "room2", protocol_version);

    table_id_type table2 = create_new_table(richregistrator2, room2, 0u, meta);

    room_id_type room3 = create_new_room(richregistrator2, "room3", protocol_version);

    table_id_type table3_1 = create_new_table(richregistrator2, room3, 0u, meta);
    table_id_type table3_2 = create_new_table(richregistrator2, room3, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE(table1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1, table1(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, table1, stake));

    next_maintenance();

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table3_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table3_2));

    next_maintenance();

    idump((room1(db)));
    idump((room2(db)));
    idump((room3(db)));

    BOOST_REQUIRE_LT(room1(db).rating, room2(db).rating);
    //room3 has more tables
    BOOST_REQUIRE_LT(room2(db).rating, room3(db).rating);
}

PLAYCHAIN_TEST_CASE(check_select_different_rooms_with_swap)
{
    const std::string meta = "Game";
    const std::string protocol_version = "1.0.0+20190223";

    BOOST_REQUIRE_GT(get_playchain_properties(db).parameters.amount_reserving_places_per_user, 1u);

    room_id_type room1 = create_new_room(richregistrator, "room1", protocol_version);

    table_id_type table1_1 = create_new_table(richregistrator, room1, 0u, meta);
    table_id_type table1_2 = create_new_table(richregistrator, room1, 0u, meta);

    room_id_type room2 = create_new_room(richregistrator2, "room2", protocol_version);

    table_id_type table2 = create_new_table(richregistrator2, room2, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2));
    std::string proposal2_uid;
    try {
        auto proposal1 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        auto proposal2 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        proposal2_uid = proposal2.uid;
    } FC_LOG_AND_RETHROW()

    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE(table2(db).is_pending_at_table(get_player(player1)));

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel(player1, proposal2_uid));

    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, table1_1, stake));

    next_maintenance();

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2));

    idump((room1(db)));
    idump((room2(db)));

    try {
        auto proposal1 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        auto proposal2 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        proposal2_uid = proposal2.uid;
    } FC_LOG_AND_RETHROW()

    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE(table2(db).is_pending_at_table(get_player(player1)));

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator2, table2, table2(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel(player1, proposal2_uid));

    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, table2, stake));

    next_maintenance();

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2));

    idump((room1(db)));
    idump((room2(db)));

    try {
        auto proposal1 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        auto proposal2 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        proposal2_uid = proposal2.uid;
    } FC_LOG_AND_RETHROW()

    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE(table2(db).is_pending_at_table(get_player(player1)));
}

PLAYCHAIN_TEST_CASE(check_select_different_rooms_with_multyplaying)
{
    const std::string meta = "Game";
    const std::string protocol_version = "1.0.0+20190223";

    BOOST_REQUIRE_GT(get_playchain_properties(db).parameters.amount_reserving_places_per_user, 1u);

    room_id_type room1 = create_new_room(richregistrator, "room1", protocol_version);

    table_id_type table1_1 = create_new_table(richregistrator, room1, 0u, meta);
    table_id_type table1_2 = create_new_table(richregistrator, room1, 0u, meta);

    room_id_type room2 = create_new_room(richregistrator2, "room2", protocol_version);

    table_id_type table2 = create_new_table(richregistrator2, room2, 0u, meta);

    next_maintenance();

    idump((room1(db)));
    idump((room2(db)));

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));

    auto stake = asset(player_init_balance / 3);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2));
    std::string proposal_p1_uid;
    try {
        auto proposal1 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        auto proposal2 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        proposal_p1_uid = proposal2.uid;
    } FC_LOG_AND_RETHROW()

    std::string proposal_p2_uid;
    try {
        auto proposal1 = buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version);
        auto proposal2 = buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version);
        proposal_p2_uid = proposal2.uid;
    } FC_LOG_AND_RETHROW()

    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player2)));
    BOOST_REQUIRE(table2(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE(table2(db).is_pending_at_table(get_player(player2)));

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table1_1, table1_1(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel(player1, proposal_p1_uid));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel(player2, proposal_p2_uid));

    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, table1_1, stake));
    BOOST_REQUIRE_NO_THROW(buy_out_table(player2, table1_1, stake));

    next_maintenance();

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_1));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table1_2));
    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator2, table2));

    idump((room1(db)));
    idump((room2(db)));

    try {
        auto proposal1 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        auto proposal2 = buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version);
        proposal_p1_uid = proposal2.uid;
    } FC_LOG_AND_RETHROW()

    try {
        auto proposal1 = buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version);
        auto proposal2 = buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version);
        proposal_p2_uid = proposal2.uid;
    } FC_LOG_AND_RETHROW()

    generate_block();
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE(table1_1(db).is_pending_at_table(get_player(player2)));
    BOOST_REQUIRE(table2(db).is_pending_at_table(get_player(player1)));
    BOOST_REQUIRE(table2(db).is_pending_at_table(get_player(player2)));

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator2, table2, table2(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator2, table2, table2(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel(player1, proposal_p1_uid));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel(player2, proposal_p2_uid));

    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, table2, stake));
    BOOST_REQUIRE_NO_THROW(buy_out_table(player2, table2, stake));
}

BOOST_AUTO_TEST_SUITE_END()
}
