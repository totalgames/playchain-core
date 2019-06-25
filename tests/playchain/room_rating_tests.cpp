#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <graphene/chain/hardfork.hpp>

namespace room_rating_tests
{
struct room_rating_fixture : public playchain_common::playchain_fixture
{
    const int64_t registrator_init_balance = 3000 * GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator)
    DECLARE_ACTOR(temp)

    room_rating_fixture()
    {
        actor(richregistrator).supply(asset(registrator_init_balance));
        actor(temp).supply(asset(player_init_balance));

        init_fees();

        generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
    }
};

BOOST_FIXTURE_TEST_SUITE(room_rating_tests, room_rating_fixture)


PLAYCHAIN_TEST_CASE(check_rating_changes_only_in_maintance)
{
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    generate_block();
    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 1u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    generate_block();


    //move to begin of no-fade period
    generate_blocks(db.get_dynamic_global_properties().time + fc::minutes(TIME_FACTOR_FADE_START-1));


    BOOST_TEST_MESSAGE("\nSTART BUYIN ON TABLE2");
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
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
        return GRAPHENE_BLOCKCHAIN_PRECISION * val;
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
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
    generate_blocks(HARDFORK_PLAYCHAIN_5_TIME);

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
    BOOST_REQUIRE_NE(0, db.get_dynamic_global_properties().average_room_rating);
    BOOST_REQUIRE_EQUAL(room(db).rating, db.get_dynamic_global_properties().average_room_rating);


    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version);
    BOOST_REQUIRE_EQUAL(room2(db).rating, db.get_dynamic_global_properties().average_room_rating);
}

BOOST_AUTO_TEST_SUITE_END()
}
