#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/validators.hpp>
#include <graphene/chain/hardfork.hpp>

#include <algorithm>

namespace table_alive_tests
{
struct table_alive_fixture: public playchain_common::playchain_fixture
{
    const int64_t registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator)
    DECLARE_ACTOR(richregistrator2)
    DECLARE_ACTOR(richregistrator3)

    table_alive_fixture()
    {
        actor(richregistrator).supply(asset(registrator_init_balance));
        actor(richregistrator2).supply(asset(registrator_init_balance));
        actor(richregistrator3).supply(asset(registrator_init_balance));

        init_fees();
    }

    void set_playchain_parameters( const uint32_t buy_in_expiration_seconds,
                                   const uint32_t table_alive_expiration_seconds )
    {
        playchain_parameters new_params = get_playchain_properties(db).parameters;
        if (new_params.buy_in_expiration_seconds == buy_in_expiration_seconds &&
            new_params.table_alive_expiration_seconds == table_alive_expiration_seconds)
            return;

        ilog("Playchain Commitee: ${l}", ("l", get_playchain_properties(db).active_games_committee_members));

        create_witness(richregistrator);

        BOOST_CHECK_NO_THROW(playchain_committee_member_create(richregistrator));

        generate_block();

        elect_member(richregistrator);

        new_params.buy_in_expiration_seconds = buy_in_expiration_seconds;
        new_params.table_alive_expiration_seconds = table_alive_expiration_seconds;

        auto proposal_info = propose_playchain_params_change(richregistrator, new_params);

        approve_proposal(proposal_info.id, richregistrator, true);
        approve_proposal(proposal_info.id, richregistrator2, true);
        approve_proposal(proposal_info.id, richregistrator3, true);

        generate_blocks(proposal_info.expiration_time);

        next_maintenance();

        BOOST_REQUIRE_EQUAL(get_playchain_properties(db).parameters.buy_in_expiration_seconds, buy_in_expiration_seconds);
        BOOST_REQUIRE_EQUAL(get_playchain_properties(db).parameters.table_alive_expiration_seconds, table_alive_expiration_seconds);
    }
};

BOOST_FIXTURE_TEST_SUITE( table_alive_tests, table_alive_fixture)

PLAYCHAIN_TEST_CASE(check_table_alive_no_maintenance)
{
    generate_blocks(HARDFORK_PLAYCHAIN_2_TIME);

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room");

    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table(db).get_pending_proposals(), 0u);

    try
    {
        table_alive(richregistrator, table);
    } FC_LOG_AND_RETHROW()

    idump((table(db)));

    generate_block();

    BOOST_CHECK_EQUAL(table(db).get_pending_proposals(), 2u);
}

PLAYCHAIN_TEST_CASE(check_table_alive_object_lifetime)
{
    generate_blocks(HARDFORK_PLAYCHAIN_2_TIME);

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room");

    next_maintenance();

    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table(db).get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table));

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table(db).get_pending_proposals(), 1u);

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    std::max(fc::seconds(params.table_alive_expiration_seconds),
                             fc::seconds(params.pending_buyin_proposal_lifetime_limit_in_seconds)) );

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));

    generate_block();

    BOOST_CHECK_EQUAL(table(db).get_pending_proposals(), 0u);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table));

    generate_block();

    BOOST_CHECK_EQUAL(table(db).get_pending_proposals(), 2u);
}

PLAYCHAIN_TEST_CASE(check_table_update_when_alive)
{
    const std::string meta1 = "Game1";
    const std::string meta2 = "Game2";

    room_id_type room = create_new_room(richregistrator, "room");

    table_id_type table = create_new_table(richregistrator, room, 0u, meta1);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table));

    BOOST_CHECK_NO_THROW(update_table(richregistrator, table, 0u, meta2));

    generate_blocks(HARDFORK_PLAYCHAIN_3_TIME);

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator, table));

    BOOST_CHECK_THROW(update_table(richregistrator, table, 0u, meta1), fc::exception);

    //metada doesn't change
    BOOST_CHECK_NO_THROW(update_table(richregistrator, table, 10u, meta2));
}

PLAYCHAIN_TEST_CASE(check_table_become_alive_while_operated)
{
    generate_blocks(HARDFORK_PLAYCHAIN_2_TIME);

    set_playchain_parameters((uint32_t)fc::minutes(5).to_seconds(), (uint32_t)fc::minutes(2).to_seconds());

    const auto& params = get_playchain_parameters(db);

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room");

    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    generate_block();

    BOOST_REQUIRE_EQUAL( pplaychain_api->get_tables_info_by_id({table}).size(), 1u );
    BOOST_CHECK( !pplaychain_api->get_tables_info_by_id({table})[0].alive );

    auto op = tables_alive_op(actor(richregistrator), {table});

    auto &&trx = actor(richregistrator).push_operation(op);

    BOOST_REQUIRE_EQUAL(trx.operation_results.size(), 1u);
    table_alive_id_type alive_id = (*trx.operation_results.begin()).get<object_id_type>();

    BOOST_CHECK_EQUAL((alive_id(db).expiration - alive_id(db).created).to_seconds(), params.table_alive_expiration_seconds);

    auto prev_ping = db.get_dynamic_global_properties().time;

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "p3", asset(player_init_balance));

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta));

    generate_block();

    const table_object &table_obj = table(db);

    {
        std::vector<pending_buy_in_id_type> proposals;

        for(const auto &ps: table_obj.pending_proposals)
        {
            proposals.emplace_back(ps.second);
        }

        for(const auto &id: proposals)
        {
            BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table_obj.id, id));
        }
    }

    BOOST_CHECK_EQUAL((alive_id(db).expiration - db.get_dynamic_global_properties().time).to_seconds(), params.table_alive_expiration_seconds);

    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(player1)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(player2)));

    generate_blocks(prev_ping + fc::seconds(params.table_alive_expiration_seconds));

    BOOST_CHECK(is_table_alive(db, table));

    game_initial_data initial1;
    initial1.cash[actor(player1)] = stake;
    initial1.cash[actor(player2)] = stake;
    initial1.info = "player1 is diller";

    prev_ping = db.get_dynamic_global_properties().time;

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator, table, initial1));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(player1, table, initial1));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(player2, table, initial1));

    BOOST_CHECK_EQUAL((alive_id(db).expiration - prev_ping).to_seconds(), params.table_alive_expiration_seconds);

    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK(is_table_alive(db, table));

    generate_blocks(prev_ping + fc::seconds(params.table_alive_expiration_seconds / 2 ));

    BOOST_REQUIRE_NO_THROW(game_reset(richregistrator, table, false));

    prev_ping = db.get_dynamic_global_properties().time;

    BOOST_CHECK_EQUAL((alive_id(db).expiration - prev_ping).to_seconds(), params.table_alive_expiration_seconds);

    BOOST_REQUIRE(table_obj.is_free());

    generate_blocks(prev_ping + fc::seconds(params.table_alive_expiration_seconds / 2 ));

    BOOST_CHECK(is_table_alive(db, table));

    BOOST_REQUIRE_NO_THROW(buy_in_table(player3, richregistrator, table, stake));

    prev_ping = db.get_dynamic_global_properties().time;

    BOOST_CHECK_EQUAL((alive_id(db).expiration - prev_ping).to_seconds(), params.table_alive_expiration_seconds);

    game_initial_data initial2;
    initial2.cash[actor(player1)] = stake;
    initial2.cash[actor(player2)] = stake;
    initial2.cash[actor(player3)] = stake;
    initial2.info = "player2 is diller";

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator, table, initial2));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(player1, table, initial2));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(player2, table, initial2));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(player3, table, initial2));

    BOOST_CHECK_EQUAL((alive_id(db).expiration - prev_ping).to_seconds(), params.table_alive_expiration_seconds);

    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK(is_table_alive(db, table));

    generate_blocks(prev_ping + fc::seconds(params.table_alive_expiration_seconds / 2 ));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &player1_result = result.cash[actor(player1)];
    player1_result.cash = stake + win - win_rake;
    player1_result.rake = win_rake;
    auto &player2_result = result.cash[actor(player2)];
    player2_result.cash = stake - win;
    player2_result.rake = asset(0);
    auto &player3_result = result.cash[actor(player3)];
    player3_result.cash = stake;
    player3_result.rake = asset(0);
    result.log = "player has A 4";

    prev_ping = db.get_dynamic_global_properties().time;

    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(player1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(player2, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(player3, table, result));

    BOOST_CHECK_EQUAL((alive_id(db).expiration - prev_ping).to_seconds(), params.table_alive_expiration_seconds);

    BOOST_REQUIRE(table_obj.is_free());

    generate_blocks(prev_ping + fc::seconds(params.table_alive_expiration_seconds / 2 ));

    BOOST_CHECK(is_table_alive(db, table));

    BOOST_REQUIRE_EQUAL( pplaychain_api->get_tables_info_by_id({table}).size(), 1u );
    BOOST_CHECK( pplaychain_api->get_tables_info_by_id({table})[0].alive );
}

PLAYCHAIN_TEST_CASE(check_negative_tables_alive_operation)
{
    room_id_type room = create_new_room(richregistrator);

    std::set<table_id_type> tables;

    tables.insert(create_new_table(richregistrator, room));
    tables.insert(create_new_table(richregistrator, room));

    BOOST_CHECK_THROW(tables_alive_op(GRAPHENE_COMMITTEE_ACCOUNT, tables).validate(), fc::exception);
    BOOST_CHECK_THROW(tables_alive_op(richregistrator, {}).validate(), fc::exception);

    tables.insert(PLAYCHAIN_NULL_TABLE);

    BOOST_CHECK_THROW(tables_alive_op(richregistrator, tables).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_tables_alive_evaluator_asserts)
{
    room_id_type room1 = create_new_room(richregistrator);

    std::set<table_id_type> tables;

    tables.insert(create_new_table(richregistrator, room1));
    tables.insert(create_new_table(richregistrator, room1));

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));

    BOOST_CHECK_THROW(tables_alive(player1, tables), fc::exception);

    BOOST_CHECK_NO_THROW(tables_alive(richregistrator, tables));

    room_id_type room2 = create_new_room(richregistrator2);

    tables.insert(create_new_table(richregistrator2, room2));

    BOOST_CHECK_THROW(tables_alive(richregistrator, tables), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_table_find_with_game_witnesses)
{
    const std::string protocol_version = "1.3.0+20190610";

    Actor player1 = create_new_player(richregistrator, "p1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "p2", asset(player_init_balance));

    room_id_type main_room = create_new_room(richregistrator, "room1", protocol_version);
    room_id_type w1_room = create_new_room(richregistrator2, "w1", protocol_version);
    room_id_type w2_room = create_new_room(richregistrator3, "w2", protocol_version);

    const std::string meta = "Game";

    table_id_type table1 = create_new_table(richregistrator, main_room, 0u, meta);
    table_id_type table2 = create_new_table(richregistrator, main_room, 0u, meta);
    table_id_type table3 = create_new_table(richregistrator, main_room, 0u, meta);

    generate_blocks(HARDFORK_PLAYCHAIN_2_TIME);

    BOOST_REQUIRE_NO_THROW(tables_alive(richregistrator, {table1, table2, table3}));

    BOOST_REQUIRE(is_table_alive(db, table1));
    BOOST_REQUIRE(is_table_alive(db, table2));
    BOOST_REQUIRE(is_table_alive(db, table3));

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));

    generate_block();

    idump((table1(db)));
    idump((table2(db)));
    idump((table3(db)));

    BOOST_CHECK(table1(db).get_pending_proposals() == 2u ||
                table2(db).get_pending_proposals() == 2u ||
                table3(db).get_pending_proposals() == 2u);
}

BOOST_AUTO_TEST_SUITE_END()
}
