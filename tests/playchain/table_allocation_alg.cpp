#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

#include <graphene/chain/hardfork.hpp>

namespace table_allocation_alg_tests
{
struct table_allocation_alg_fixture: public playchain_common::playchain_fixture
{
    const int64_t registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator)
    DECLARE_ACTOR(temp)

    table_allocation_alg_fixture()
    {
        actor(richregistrator).supply(asset(registrator_init_balance));
        actor(temp).supply(asset(player_init_balance));

        init_fees();

        //test only with latest voting algorithm!!!
        generate_blocks(HARDFORK_PLAYCHAIN_11_TIME);
    }
};

BOOST_FIXTURE_TEST_SUITE( table_allocation_alg_tests, table_allocation_alg_fixture)

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_allocation_alg)
{
    const std::string meta1 = "Game1";
    const std::string meta2 = "Game2";

    auto stake = asset(player_init_balance/2);

    room_id_type room = create_new_room(richregistrator);
    table_id_type table1 = create_new_table(richregistrator, room, 0u, meta1);
    table_id_type table2 = create_new_table(richregistrator, room, 0u, meta2);

    const table_object &table1_obj = table1(db);
    const table_object &table2_obj = table2(db);

    Actor lucky1 = create_new_player(richregistrator, "lucky1", asset(player_init_balance));
    Actor lucky2 = create_new_player(richregistrator, "lucky2", asset(player_init_balance));
    Actor lucky3 = create_new_player(richregistrator, "lucky3", asset(player_init_balance));
    Actor lucky4 = create_new_player(richregistrator, "lucky4", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table1);
    table_alive(richregistrator, table2);

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(lucky1, get_next_uid(actor(lucky1)), stake, meta1));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(lucky2, get_next_uid(actor(lucky2)), stake, meta1));

    generate_block();

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 2u);

    Actor loser1 = create_new_player(richregistrator, "loser1", asset(player_init_balance));
    Actor loser2 = create_new_player(richregistrator, "loser2", asset(player_init_balance));
    Actor loser3 = create_new_player(richregistrator, "loser3", asset(player_init_balance));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(loser1, get_next_uid(actor(loser1)), stake, "Unknown Game"));

    generate_block();

    BOOST_REQUIRE(!table1_obj.is_playing());

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 2u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_pending_proposals(), 0u);

    pending_buy_in_resolve_all(richregistrator, table1_obj);
    pending_buy_in_resolve_all(richregistrator, table2_obj);

    BOOST_REQUIRE_EQUAL(table1_obj.get_waiting_players(), 2u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_waiting_players(), 0u);

    game_initial_data initial;
    initial.cash[actor(lucky1)] = stake;
    initial.cash[actor(lucky2)] = stake;
    initial.info = "lucky1 is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator, table1, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(lucky1, table1, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(lucky2, table1, initial));

    generate_block();

    BOOST_REQUIRE(table1_obj.is_playing());

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(loser3, get_next_uid(actor(loser2)), stake, meta1));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &result_p1 = result.cash[actor(lucky1)];
    result_p1.cash = stake + win - win_rake;
    result_p1.rake = win_rake;
    auto &result_p2 = result.cash[actor(lucky2)];
    result_p2.cash = stake - win;
    result_p2.rake = asset(0);
    result.log = "lucky1 has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator, table1, result));
    BOOST_CHECK_NO_THROW(game_result_check(lucky1, table1, result));
    BOOST_CHECK_NO_THROW(game_result_check(lucky2, table1, result));

    generate_block();

    BOOST_REQUIRE(!table1_obj.is_playing());

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(lucky3, get_next_uid(actor(lucky3)), stake, meta1));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(lucky4, get_next_uid(actor(lucky4)), stake, meta2));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(lucky1, get_next_uid(actor(lucky1)), stake, meta2));

    generate_block();

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 2u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_pending_proposals(), 2u);

    pending_buy_in_resolve_all(richregistrator, table1_obj);
    pending_buy_in_resolve_all(richregistrator, table2_obj);

    BOOST_REQUIRE_EQUAL(table1_obj.get_waiting_players(), 4u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_waiting_players(), 2u);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_allocation_consider_protocol_version)
{
    const std::string protocol_version1 = "1.0.0+20190223";
    const std::string protocol_version2 = "1.0.1+20190308";
    const std::string protocol_version3 = "1.2.0+20190501";

    const std::string meta = "Game";

    room_id_type room1 = create_new_room(richregistrator, "room1", protocol_version1);
    table_id_type table1 = create_new_table(richregistrator, room1, 0u, meta);

    room_id_type room2 = create_new_room(richregistrator, "room2", protocol_version2);
    table_id_type table2 = create_new_table(richregistrator, room2, 0u, meta);

    room_id_type room3 = create_new_room(richregistrator, "room3", protocol_version3);
    table_id_type table3 = create_new_table(richregistrator, room3, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "player3", asset(player_init_balance));
    Actor player4 = create_new_player(richregistrator, "player4", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table1);
    table_alive(richregistrator, table2);
    table_alive(richregistrator, table3);

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version1));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version2));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta, protocol_version3));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta, protocol_version3));

    generate_block();

    if (table1(db).weight < table2(db).weight)
    {
        //because protocol_version1 == protocol_version2 (by protocol) and alg_conf_min == 1
        BOOST_CHECK_EQUAL(table1(db).get_pending_proposals(), 0u);
        //because protocol_version1 == protocol_version2 (by protocol) and alg_conf_min == 1
        BOOST_CHECK_EQUAL(table2(db).get_pending_proposals(), 2u);
    }else
    {
        BOOST_CHECK_EQUAL(table1(db).get_pending_proposals(), 2u);
        BOOST_CHECK_EQUAL(table2(db).get_pending_proposals(), 0u);
    }

    //because protocol_version3 != protocol_version1 == protocol_version2
    BOOST_CHECK_EQUAL(table3(db).get_pending_proposals(), 2u);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_cancel_all_without_table)
{
    const std::string protocol_version1 = "1.0.0+20190223";
    const std::string protocol_version2 = "2.0.0+20190308";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version2);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table);

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));

    auto stake = asset(player_init_balance/4);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version1));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version1));
    BOOST_CHECK_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version1), fc::exception);

    generate_block();

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake - stake));

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel_all(player1));
    BOOST_CHECK_THROW(buy_in_reserving_cancel_all(player1), fc::exception);

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version1));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_cancel_all_single_table)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table);

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));

    auto stake = asset(player_init_balance/4);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_CHECK_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version), fc::exception);

    generate_block();

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel_all(player1));
    BOOST_CHECK_THROW(buy_in_reserving_cancel_all(player1), fc::exception);

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_cancel_all_different_tables)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta1 = "Game1";
    const std::string meta2 = "Game2";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type table1 = create_new_table(richregistrator, room, 0u, meta1);
    table_id_type table2 = create_new_table(richregistrator, room, 0u, meta2);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table1);
    table_alive(richregistrator, table2);

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));

    auto stake = asset(player_init_balance/4);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta1, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta2, protocol_version));
    BOOST_CHECK_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta1, protocol_version), fc::exception);
    BOOST_CHECK_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta2, protocol_version), fc::exception);

    generate_block();

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake - stake));

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_cancel_all(player1));
    BOOST_CHECK_THROW(buy_in_reserving_cancel_all(player1), fc::exception);

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta1, protocol_version));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_allocation_alg_for_table_owner)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta1 = "Game1";
    const std::string meta2 = "Game2";

    room_id_type richregistrator_room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type richregistrator_table = create_new_table(richregistrator, richregistrator_room, 0u, meta1);

    Actor player_and_owner = create_new_player(richregistrator, "blade", asset(player_init_balance * 5));

    room_id_type player_room = create_new_room(player_and_owner, "room", protocol_version);
    table_id_type player_table = create_new_table(player_and_owner, player_room, 0u, meta2);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, richregistrator_table);
    table_alive(player_and_owner, player_table);

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));

    auto stake = asset(player_init_balance/3);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta1, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta1, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player_and_owner, get_next_uid(actor(player_and_owner)), stake, meta1, protocol_version));

    generate_block();

    BOOST_REQUIRE_EQUAL(richregistrator_table(db).get_pending_proposals(), 3u);
    BOOST_REQUIRE_EQUAL(player_table(db).get_pending_proposals(), 0u);

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.pending_buyin_proposal_lifetime_limit_in_seconds));

    table_alive(richregistrator, richregistrator_table);
    table_alive(player_and_owner, player_table);

    BOOST_REQUIRE_EQUAL(richregistrator_table(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(player_table(db).get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta2, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta2, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player_and_owner, get_next_uid(actor(player_and_owner)), stake, meta2, protocol_version));

    generate_block();

    BOOST_REQUIRE_EQUAL(richregistrator_table(db).get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(player_table(db).get_pending_proposals(), 2u);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_allocation_alg_for_full_table)
{
    const std::string meta = "Game";

    auto stake = asset(player_init_balance/2);

    room_id_type room = create_new_room(richregistrator);
    table_id_type table1 = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table3 = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table4 = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table5 = create_new_table(richregistrator, room, 0u, meta);

    const table_object &table1_obj = table1(db);
    const table_object &table2_obj = table2(db);
    const table_object &table3_obj = table3(db);
    const table_object &table4_obj = table4(db);
    const table_object &table5_obj = table5(db);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "p3", asset(player_init_balance));
    Actor player4 = create_new_player(richregistrator, "p4", asset(player_init_balance));
    Actor player5 = create_new_player(richregistrator, "p5", asset(player_init_balance));
    Actor player6 = create_new_player(richregistrator, "p6", asset(player_init_balance));
    Actor player7 = create_new_player(richregistrator, "p7", asset(player_init_balance));
    Actor player8 = create_new_player(richregistrator, "p8", asset(player_init_balance));
    Actor player9 = create_new_player(richregistrator, "p9", asset(player_init_balance));
    Actor player10 = create_new_player(richregistrator, "p10", asset(player_init_balance));
    Actor player11 = create_new_player(richregistrator, "p11", asset(player_init_balance));
    Actor player12 = create_new_player(richregistrator, "p12", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table1);
    table_alive(richregistrator, table2);
    table_alive(richregistrator, table3);
    table_alive(richregistrator, table4);
    table_alive(richregistrator, table5);

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table3_obj.get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table4_obj.get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table5_obj.get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta));
    generate_block();
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player5, get_next_uid(actor(player5)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player6, get_next_uid(actor(player6)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player7, get_next_uid(actor(player7)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player8, get_next_uid(actor(player8)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player9, get_next_uid(actor(player9)), stake, meta));
    generate_block();
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player10, get_next_uid(actor(player10)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player11, get_next_uid(actor(player11)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player12, get_next_uid(actor(player12)), stake, meta));
    generate_block();

    BOOST_CHECK_EQUAL(table1_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table1_obj.occupied_places, 5u);

    BOOST_CHECK_EQUAL(table2_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table2_obj.occupied_places, 5u);

    BOOST_CHECK_EQUAL(table3_obj.get_pending_proposals(), 2u);
    BOOST_CHECK_EQUAL(table3_obj.occupied_places, 2u);

    BOOST_CHECK_EQUAL(table4_obj.get_pending_proposals(), 0u);
    BOOST_CHECK_EQUAL(table4_obj.occupied_places, 0u);

    BOOST_CHECK_EQUAL(table5_obj.get_pending_proposals(), 0u);
    BOOST_CHECK_EQUAL(table5_obj.occupied_places, 0u);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_allocation_alg_for_busy_tables)
{
    const std::string meta = "Game";

    auto stake = asset(player_init_balance/2);

    room_id_type room = create_new_room(richregistrator);
    table_id_type table1 = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room, 0u, meta);

    const table_object &table1_obj = table1(db);
    const table_object &table2_obj = table2(db);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "p3", asset(player_init_balance));
    Actor player4 = create_new_player(richregistrator, "p4", asset(player_init_balance));
    Actor player5 = create_new_player(richregistrator, "p5", asset(player_init_balance));
    Actor player6 = create_new_player(richregistrator, "p6", asset(player_init_balance));
    Actor player7 = create_new_player(richregistrator, "p7", asset(player_init_balance));
    Actor player8 = create_new_player(richregistrator, "p8", asset(player_init_balance));
    Actor player9 = create_new_player(richregistrator, "p9", asset(player_init_balance));
    Actor player10 = create_new_player(richregistrator, "p10", asset(player_init_balance));
    Actor player11 = create_new_player(richregistrator, "p11", asset(player_init_balance));
    Actor player12 = create_new_player(richregistrator, "p12", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table1);
    table_alive(richregistrator, table2);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator));

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player5, get_next_uid(actor(player5)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player6, get_next_uid(actor(player6)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table1_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table1_obj.occupied_places, 5u);
    BOOST_CHECK_EQUAL(table2_obj.get_pending_proposals(), 1u);
    BOOST_CHECK_EQUAL(table2_obj.occupied_places, 1u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player7, get_next_uid(actor(player7)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player8, get_next_uid(actor(player8)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player9, get_next_uid(actor(player9)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player10, get_next_uid(actor(player10)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player11, get_next_uid(actor(player11)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player12, get_next_uid(actor(player12)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table1_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table1_obj.occupied_places, 5u);
    BOOST_CHECK_EQUAL(table2_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table2_obj.occupied_places, 5u);

//    print_last_operations(actor(richregistrator), next_history_record);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_allocation_alg_for_busy_will_free_tables)
{
    const std::string meta = "Game";

    const auto& params = get_playchain_parameters(db);

    auto stake = asset(player_init_balance/2);

    room_id_type room = create_new_room(richregistrator);
    table_id_type table1 = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room, 0u, meta);

    const table_object &table1_obj = table1(db);
    const table_object &table2_obj = table2(db);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "p3", asset(player_init_balance));
    Actor player4 = create_new_player(richregistrator, "p4", asset(player_init_balance));
    Actor player5 = create_new_player(richregistrator, "p5", asset(player_init_balance));
    Actor player6 = create_new_player(richregistrator, "p6", asset(player_init_balance));
    Actor player7 = create_new_player(richregistrator, "p7", asset(player_init_balance));
    Actor player8 = create_new_player(richregistrator, "p8", asset(player_init_balance));
    Actor player9 = create_new_player(richregistrator, "p9", asset(player_init_balance));
    Actor player10 = create_new_player(richregistrator, "p10", asset(player_init_balance));
    Actor player11 = create_new_player(richregistrator, "p11", asset(player_init_balance));
    Actor player12 = create_new_player(richregistrator, "p12", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table1);
    table_alive(richregistrator, table2);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator));

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));

    auto time = db.get_dynamic_global_properties().time;

    generate_block();

    BOOST_CHECK_EQUAL(table1_obj.get_pending_proposals(), 2u);
    BOOST_CHECK_EQUAL(table1_obj.occupied_places, 2u);
    BOOST_CHECK_EQUAL(table2_obj.get_pending_proposals(), 0u);
    BOOST_CHECK_EQUAL(table2_obj.occupied_places, 0u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player5, get_next_uid(actor(player5)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player6, get_next_uid(actor(player6)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player7, get_next_uid(actor(player7)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player8, get_next_uid(actor(player8)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player9, get_next_uid(actor(player9)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player10, get_next_uid(actor(player10)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player11, get_next_uid(actor(player11)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player12, get_next_uid(actor(player12)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table1_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table1_obj.occupied_places, 5u);
    BOOST_CHECK_EQUAL(table2_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table2_obj.occupied_places, 5u);

    //free 2 proposals
    generate_blocks(time + fc::seconds(params.pending_buyin_proposal_lifetime_limit_in_seconds));

    BOOST_CHECK_EQUAL(table1_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table1_obj.occupied_places, 5u);
    BOOST_CHECK_EQUAL(table2_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table2_obj.occupied_places, 5u);

    print_last_operations(actor(richregistrator), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_expire_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_expire_id);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_allocation_alg_for_replace_allocation)
{
    const std::string meta = "Game";

    auto stake = asset(player_init_balance/2);

    room_id_type room = create_new_room(richregistrator);
    table_id_type table1 = create_new_table(richregistrator, room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, room, 0u, meta);

    const table_object &table1_obj = table1(db);
    const table_object &table2_obj = table2(db);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "p3", asset(player_init_balance));
    Actor player4 = create_new_player(richregistrator, "p4", asset(player_init_balance));
    Actor player5 = create_new_player(richregistrator, "p5", asset(player_init_balance));
    Actor player6 = create_new_player(richregistrator, "p6", asset(player_init_balance));
    Actor player7 = create_new_player(richregistrator, "p7", asset(player_init_balance));

    next_maintenance();

    table_alive(richregistrator, table1);
    table_alive(richregistrator, table2);

    idump((room(db)));

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator));

    BOOST_REQUIRE_EQUAL(table1_obj.get_pending_proposals(), 0u);
    BOOST_REQUIRE_EQUAL(table2_obj.get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player5, get_next_uid(actor(player5)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player6, get_next_uid(actor(player4)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player7, get_next_uid(actor(player5)), stake, meta));

    generate_block();

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player3, get_next_uid(actor(player3)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player4, get_next_uid(actor(player4)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player5, get_next_uid(actor(player5)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table1_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table1_obj.occupied_places, 5u);
    BOOST_CHECK_EQUAL(table2_obj.get_pending_proposals(), 5u);
    BOOST_CHECK_EQUAL(table2_obj.occupied_places, 5u);

//    print_last_operations(actor(richregistrator), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
}

BOOST_AUTO_TEST_SUITE_END()
}
