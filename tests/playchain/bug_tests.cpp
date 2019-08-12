#include "playchain_common.hpp"

#include <playchain/chain/evaluators/db_helpers.hpp>

#include <graphene/chain/hardfork.hpp>

namespace bug_tests {

struct bug_tests_fixture: public playchain_common::playchain_fixture
{
    const int64_t rich_registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(tableowner)
    DECLARE_ACTOR(witness1)
    DECLARE_ACTOR(witness2)

    DECLARE_ACTOR(alice)
    DECLARE_ACTOR(bob)
    DECLARE_ACTOR(sam)

    room_id_type test_room;
    table_id_type test_table;

    bug_tests_fixture()
    {
        try {
            actor(tableowner).supply(asset(rich_registrator_init_balance));
            actor(witness1).supply(asset(rich_registrator_init_balance));
            actor(witness2).supply(asset(rich_registrator_init_balance));
        } FC_LOG_AND_RETHROW()

        init_fees();

        CREATE_PLAYER(tableowner, alice);
        CREATE_PLAYER(tableowner, bob);
        CREATE_PLAYER(tableowner, sam);

        create_witness(witness1);
        create_witness(witness2);

        test_room = create_new_room(tableowner);
        test_table = create_new_table(tableowner, test_room, 0u, default_table_metadata);
    }

    asset get_account_balance(const Actor& account)
    {
        return db.get_balance(actor(account), asset_id_type());
    }

    void stuck_asset_when_game_undo_bug(const asset alice_bet)
    {
        const table_object &table_obj = test_table(db);

        BOOST_REQUIRE(table_obj.is_free());

        auto alice_initial = asset(470);
        auto bob_initial = asset(460);
        auto sam_initial = asset(460);

        BOOST_REQUIRE_NO_THROW(buy_in_reserve(alice, get_next_uid(actor(alice)), alice_initial, default_table_metadata));
        BOOST_REQUIRE_NO_THROW(buy_in_reserve(bob, get_next_uid(actor(bob)), bob_initial, default_table_metadata));
        BOOST_REQUIRE_NO_THROW(buy_in_reserve(sam, get_next_uid(actor(sam)), sam_initial, default_table_metadata));

        generate_block();

        pending_buy_in_resolve_all(tableowner, table_obj);

        BOOST_REQUIRE(table_obj.is_free());

        game_initial_data initial;
        initial.cash[actor(alice)] = alice_initial;
        initial.cash[actor(bob)] = bob_initial;
        initial.cash[actor(sam)] = sam_initial;
        initial.info = "110";

        BOOST_REQUIRE_NO_THROW(game_start_playing_check(tableowner, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness1, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness2, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(sam, test_table, initial));

        BOOST_REQUIRE(table_obj.is_playing());

        generate_block();

        auto bob_bet = asset(5);

        BOOST_REQUIRE(alice_initial > alice_bet);
        BOOST_REQUIRE(bob_initial > bob_bet);

        BOOST_CHECK_NO_THROW(buy_out_table(alice, test_table, alice_initial - alice_bet));
        BOOST_CHECK_NO_THROW(buy_out_table(bob, test_table, bob_initial - bob_bet));
        BOOST_CHECK_NO_THROW(buy_out_table(sam, test_table, sam_initial));

        generate_block();

        BOOST_REQUIRE(table_obj.is_playing());

        game_result result;
        result.cash[actor(alice)].cash = alice_initial - alice_bet;
        result.cash[actor(bob)].cash = bob_initial - bob_bet;
        result.cash[actor(sam)].cash = sam_initial + alice_bet + bob_bet;
        result.log = "game1 with faul result";

        BOOST_REQUIRE_NO_THROW(game_result_check(tableowner, test_table, result));

        const auto& params = get_playchain_parameters(db);

        generate_blocks(db.get_dynamic_global_properties().time +
                        fc::seconds(params.voting_for_results_expiration_seconds));

        BOOST_REQUIRE(table_obj.is_free());

        idump((table_obj));

        BOOST_REQUIRE(table_obj.get_cash_balance(get_player(alice)) == alice_bet); //that is ok
        BOOST_REQUIRE(table_obj.get_cash_balance(get_player(bob)) == bob_bet); //that is ok
        BOOST_REQUIRE(table_obj.get_cash_balance(get_player(sam)) == asset(0));

        auto last_prolong_life_point = db.get_dynamic_global_properties().time;

        BOOST_REQUIRE_NO_THROW(buy_in_table(bob, tableowner, test_table, bob_initial));
        BOOST_REQUIRE_NO_THROW(buy_in_reserve(sam, get_next_uid(actor(sam)), sam_initial, default_table_metadata));

        generate_block();

        pending_buy_in_resolve_all(tableowner, table_obj);

        idump((table_obj));

        initial.cash.clear();
        initial.cash[actor(bob)] = bob_initial + bob_bet;
        initial.cash[actor(sam)] = sam_initial;
        initial.info = "110";

        try {
            game_start_playing_check(tableowner, test_table, initial);
        }FC_LOG_AND_RETHROW()
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness1, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness2, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, test_table, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(sam, test_table, initial));

        BOOST_REQUIRE(table_obj.is_playing());

        generate_block();

        idump((table_obj));

        result.cash.clear();
        result.cash[actor(bob)].cash = bob_initial;
        result.cash[actor(sam)].cash = sam_initial + bob_bet;
        result.log = "game2 with ok result";

        BOOST_REQUIRE_NO_THROW(game_result_check(tableowner, test_table, result));
        BOOST_REQUIRE_NO_THROW(game_result_check(witness1, test_table, result));
        BOOST_REQUIRE_NO_THROW(game_result_check(witness2, test_table, result));
        BOOST_REQUIRE_NO_THROW(game_result_check(sam, test_table, result));

        BOOST_REQUIRE(table_obj.is_free());

        idump((table_obj));

        generate_blocks(last_prolong_life_point +
                        fc::seconds(params.buy_in_expiration_seconds));

        idump((table_obj));
    }
};

BOOST_FIXTURE_TEST_SUITE( bug_tests, bug_tests_fixture)

PLAYCHAIN_TEST_CASE(stuck_asset_when_game_undo_bug_reproduction)
{
    const table_object &table_obj = test_table(db);

    auto alice_bet = asset(10);
    auto alice_initial_balance = get_account_balance(alice);

    stuck_asset_when_game_undo_bug(alice_bet);

    BOOST_REQUIRE(table_obj.get_cash_balance(get_player(alice)) == alice_bet); //it should be expired!
    BOOST_REQUIRE(get_account_balance(alice) == alice_initial_balance - alice_bet);
}

PLAYCHAIN_TEST_CASE(stuck_asset_when_game_undo_bug_fix)
{
    generate_blocks(HARDFORK_PLAYCHAIN_7_TIME);

    const table_object &table_obj = test_table(db);

    auto alice_bet = asset(10);
    auto alice_initial_balance = get_account_balance(alice);

    stuck_asset_when_game_undo_bug(alice_bet);

    BOOST_REQUIRE(table_obj.get_cash_balance(get_player(alice)) == asset(0)); //it has expired
    BOOST_REQUIRE(get_account_balance(alice) == alice_initial_balance);
}

BOOST_AUTO_TEST_SUITE_END()
}
