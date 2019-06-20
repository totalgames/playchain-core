#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

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

PLAYCHAIN_TEST_CASE(check_initial_room_rating_equals_0)
{
    const std::string protocol_version = "1.0.0+20190223";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);

    BOOST_CHECK_EQUAL(0, room(db).rating);
}

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


//PLAYCHAIN_TEST_CASE(check_failed_resolve_results_in_rating_degrodation)
//{
//    const std::string protocol_version = "1.0.0+20190223";
//
//    const std::string meta = "Game";
//
//    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
//    table_id_type table = create_new_table(richregistrator, room, 0u, meta);
//
//    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
//    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
//
//    auto stake = asset(player_init_balance / 3);
//
//    generate_block();
//
//    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
//    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
//
//    generate_block();
//
//    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
//    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
//    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
//
//    BOOST_TEST_MESSAGE("\nBEFORE MAINTANCE BLOCK");
//    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - GRAPHENE_DEFAULT_BLOCK_INTERVAL);
//    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
//    generate_block(); //perform maintain
//
//    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);
//
//    ///Save 100% rating
//    auto room_100percent_rating = room(db).rating;
//
//    //////////////////////////////////////////////////////////////////////////
//    generate_block();
//
//    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
//    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));
//
//    generate_block();
//
//    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
//    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));//resolve only 1 buy_in
//
//    BOOST_TEST_MESSAGE("\nGENERATE_UNTIL_EXPIRATION");
//    generate_blocks(db.get_dynamic_global_properties().time + fc::seconds(get_playchain_parameters(db).buy_in_expiration_seconds));
//
//    BOOST_TEST_MESSAGE("\nBEFORE MAINTANCE BLOCK");
//    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - GRAPHENE_DEFAULT_BLOCK_INTERVAL);
//    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
//    generate_block(); //perform maintain
//
//    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);
//
//    ///Get 50% rating
//    auto room_50percent_rating = room(db).rating;
//
//    BOOST_TEST_MESSAGE("\nroom_50percent_rating == " << room_50percent_rating << ", room_100percent_rating == "<< room_100percent_rating);
//
//    BOOST_CHECK_LT(room_50percent_rating, room_100percent_rating/2);
//}
//


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

    BOOST_TEST_MESSAGE("\nroom_50percent_rating == " << room_1fail_rating << ", room_100percent_rating == " << room_exelent_rating);

    BOOST_CHECK_LT(room_1fail_rating, room_exelent_rating);
}

PLAYCHAIN_TEST_CASE(check_least_recent_buyins_give_most_contribution_in_rating_value)
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

    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - fc::minutes(TIME_FACTOR_FADE_START));

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 2;
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 2u);



    BOOST_TEST_MESSAGE("\nSTART BUYIN ON TABLE1");

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));

    generate_block();

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));

    generate_block();




    //buyins during the same minute are equal,
    generate_blocks(db.get_dynamic_global_properties().time + fc::minutes(TIME_FACTOR_FADE_START).count()/2);

    // update parameter for test needs
    db.modify(get_playchain_properties(db), [&](playchain_property_object& p) {
        p.parameters.maximum_desired_number_of_players_for_tables_allocation = 2;
    });
    BOOST_REQUIRE_EQUAL(get_playchain_parameters(db).maximum_desired_number_of_players_for_tables_allocation, 2u);





    BOOST_TEST_MESSAGE("\nSTART BUYIN ON TABLE2");

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta, protocol_version));

    generate_block();

    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 2u);
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table2, table2(db).pending_proposals.begin()->second));

    generate_block();


    BOOST_TEST_MESSAGE("\nBEFORE MAINTANCE BLOCK");
    generate_blocks(db.get_dynamic_global_properties().next_maintenance_time - GRAPHENE_DEFAULT_BLOCK_INTERVAL);
    BOOST_TEST_MESSAGE("\nGENERATE MAINTANCE");
    generate_block(); //perform maintain

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2(db).get_pending_proposals(), 0u);

    auto room1_exelent_rating = room(db).rating;
    auto room2_exelent_rating = room2(db).rating;

    BOOST_TEST_MESSAGE("\nroom1_exelent_rating == " << room1_exelent_rating << ", room2_exelent_rating == " << room2_exelent_rating);

    BOOST_CHECK_LT(room1_exelent_rating, room2_exelent_rating);
}

BOOST_AUTO_TEST_SUITE_END()
}












