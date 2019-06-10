#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <graphene/chain/hardfork.hpp>

#include <algorithm>

namespace table_alive_tests
{
struct table_alive_fixture: public playchain_common::playchain_fixture
{
    const int64_t registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator)

    table_alive_fixture()
    {
        actor(richregistrator).supply(asset(registrator_init_balance));

        init_fees();
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

BOOST_AUTO_TEST_SUITE_END()
}
