#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

#include <graphene/chain/hardfork.hpp>

namespace voting_tests
{
struct voting_fixture: public playchain_common::playchain_fixture
{
    const int64_t rich_registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;
    const int64_t new_player_balance = 500*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator1)
    DECLARE_ACTOR(richregistrator2)
    DECLARE_ACTOR(richregistrator3)
    //for simple tests:
    DECLARE_ACTOR(alice)
    DECLARE_ACTOR(bob)
    DECLARE_ACTOR(sam)
    DECLARE_ACTOR(jon)

    voting_fixture()
    {
        actor(richregistrator1).supply(asset(rich_registrator_init_balance));
        actor(richregistrator2).supply(asset(rich_registrator_init_balance));
        actor(richregistrator3).supply(asset(rich_registrator_init_balance));

        init_fees();

        CREATE_PLAYER(richregistrator1, alice);
        CREATE_PLAYER(richregistrator1, bob);
        CREATE_PLAYER(richregistrator1, sam);
        CREATE_PLAYER(richregistrator1, jon);

        //test only with latest voting algorithm!!!
        generate_blocks(HARDFORK_PLAYCHAIN_8_TIME);
    }
};

BOOST_FIXTURE_TEST_SUITE( voting_tests, voting_fixture)

PLAYCHAIN_TEST_CASE(check_negative_start_playing_check_operation)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    game_initial_data data;
    auto stake = asset(player_init_balance/2);
    data.cash[actor(alice)] = stake;
    data.cash[actor(bob)] = stake;
    data.info = "alice is diller";

    BOOST_CHECK_THROW(game_start_playing_check_op(GRAPHENE_COMMITTEE_ACCOUNT, actor(richregistrator1), table, data).validate(), fc::exception);
    //vote to roolback is possible
    BOOST_CHECK_NO_THROW(game_start_playing_check_op(richregistrator1, richregistrator1, table, game_initial_data()).validate());
    BOOST_CHECK_THROW(game_start_playing_check_op(actor(richregistrator1), GRAPHENE_COMMITTEE_ACCOUNT, table, data).validate(), fc::exception);

    data.cash.erase(actor(alice));

    BOOST_CHECK_THROW(game_start_playing_check_op(richregistrator1, richregistrator1, table, data).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_result_check_operation)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    game_result data;
    auto stake = asset(player_init_balance/2);
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = data.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = data.cash[actor(bob)];
    bob_result.cash = stake - win;
    data.log = "alice has A 4";

    BOOST_CHECK_THROW(game_result_check_op(GRAPHENE_COMMITTEE_ACCOUNT, actor(richregistrator1), table, data).validate(), fc::exception);
    BOOST_CHECK_THROW(game_result_check_op(actor(richregistrator1), GRAPHENE_COMMITTEE_ACCOUNT, table, data).validate(), fc::exception);
    //vote to cancel the game is possible
    BOOST_CHECK_NO_THROW(game_result_check_op(richregistrator1, richregistrator1, table, game_result()).validate());
}

PLAYCHAIN_TEST_CASE(check_with_negative_start_playing_check_evaluate)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    game_initial_data data;
    auto stake = asset(player_init_balance/2);
    data.cash[actor(alice)] = stake;
    data.cash[actor(bob)] = stake;
    data.info = "alice is diller";

    BOOST_REQUIRE(table(db).is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(bob)));

    //table is not exist
    BOOST_CHECK_THROW(game_start_playing_check(richregistrator1, table_id_type(), data), fc::exception);

    //alice is not table owner and vote twice
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, data));
    BOOST_CHECK_THROW(game_start_playing_check(alice, table, data), fc::exception);

    //sam has not done buy-in
    data.cash[actor(sam)] = stake;
    BOOST_CHECK_THROW(game_start_playing_check(richregistrator1, table, data), fc::exception);
    data.cash.erase(actor(sam));

    //Ok
    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, data));

    //sam has not done buy-in
    BOOST_CHECK_THROW(game_start_playing_check(sam, table, data), fc::exception);

    //finish voting
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, data));

    generate_block();

    BOOST_REQUIRE(table(db).is_playing(get_player(alice)));
    BOOST_REQUIRE(table(db).is_playing(get_player(bob)));
}

PLAYCHAIN_TEST_CASE(check_with_negative_result_check_evaluate)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    game_initial_data initial;
    auto stake = asset(player_init_balance/2);
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table(db).is_playing(get_player(alice)));
    BOOST_REQUIRE(table(db).is_playing(get_player(bob)));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = stake - win;
    result.log = "alice has A 4";

    //table is not exist
    BOOST_CHECK_THROW(game_result_check(richregistrator1, table_id_type(), result), fc::exception);

    //alice is not table owner and vote twice
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_THROW(game_result_check(alice, table, result), fc::exception);

    //sam has not done buy-in
    result.cash[actor(sam)] = alice_result;
    BOOST_CHECK_THROW(game_result_check(richregistrator1, table, result), fc::exception);
    result.cash.erase(actor(sam));

    //Ok
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));

    //sam is not table owner
    BOOST_CHECK_THROW(game_result_check(sam, table, result), fc::exception);

    //finish voting
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));

    generate_block();

    BOOST_CHECK(table(db).is_free());

    BOOST_CHECK(table(db).is_waiting_at_table(get_player(alice)));
    BOOST_CHECK(table(db).is_waiting_at_table(get_player(bob)));
}

PLAYCHAIN_TEST_CASE(check_table_free_state)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    auto balance = db.get_balance(actor(bob), asset_id_type());

    BOOST_CHECK_NO_THROW(buy_out_table(bob, table, asset(stake.amount/2)));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(bob), asset_id_type())), to_string(balance + asset(stake.amount/2)) );

    BOOST_CHECK_NO_THROW(buy_out_table(bob, table, stake - asset(stake.amount/2)));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(bob), asset_id_type())), to_string(balance + stake) );
}

PLAYCHAIN_TEST_CASE(check_validated_with_one_fraud)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    game_initial_data initial_wrong1;
    initial_wrong1.cash[actor(alice)] = asset(stake.amount / 2);
    initial_wrong1.cash[actor(bob)] = stake;
    initial_wrong1.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(bob)));
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(jon)));
    BOOST_REQUIRE(!table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(!table_obj.is_playing(get_player(bob)));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial_wrong1));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_playing());
    BOOST_REQUIRE_EQUAL(table_obj.cash.size(), 1u);
    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(jon))), to_string(stake));
    BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(bob)));
    BOOST_REQUIRE(!table_obj.is_playing(get_player(jon)));
    BOOST_REQUIRE(!table(db).is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(!table(db).is_waiting_at_table(get_player(bob)));
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(jon)));

//    print_last_operations(actor(richregistrator1), next_history_record);
    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fraud_game_start_playing_check_id));
}

PLAYCHAIN_TEST_CASE(check_game_roolback_by_vote_for_result)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(bob)));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(bob)));

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, game_result()));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, game_result()));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, game_result()));
    //extra vote
    BOOST_CHECK_THROW(game_result_check(richregistrator2, table, game_result()), fc::exception);

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK(table(db).is_waiting_at_table(get_player(alice)));
    BOOST_CHECK(table(db).is_waiting_at_table(get_player(bob)));

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_rollback_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
}

PLAYCHAIN_TEST_CASE(check_failed_consensus_to_start_playing)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    game_initial_data initial_wrong1;
    initial_wrong1.cash[actor(alice)] = asset(stake.amount / 2);
    initial_wrong1.cash[actor(bob)] = stake;
    initial_wrong1.info = "alice is diller";

    game_initial_data initial_wrong2;
    initial_wrong2.cash[actor(alice)] = stake;
    initial_wrong2.cash[actor(bob)] = stake;
    initial_wrong2.info = "bob is diller";

    game_initial_data initial_wrong_not_acceptable_for_voting;
    initial_wrong_not_acceptable_for_voting.cash[actor(alice)] = stake;
    initial_wrong_not_acceptable_for_voting.cash[actor(bob)] = stake;
    initial_wrong_not_acceptable_for_voting.cash[actor(sam)] = stake; //there is no at the table
    initial_wrong_not_acceptable_for_voting.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial_wrong1));
    BOOST_CHECK_THROW(game_start_playing_check(bob, table, initial_wrong_not_acceptable_for_voting), fc::exception);
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial_wrong2));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    //cash is not reset if consensus failed when start playing voting
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(bob)));
    BOOST_REQUIRE(table(db).is_waiting_at_table(get_player(jon)));

//    print_block_operations();
    generate_blocks(2);

//    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fail_consensus_game_start_playing_id));
}

PLAYCHAIN_TEST_CASE(check_game_lifetime_expiration_with_special_starting)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    create_witness(richregistrator2);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    auto alice_balance = db.get_balance(actor(alice), asset_id_type());
    auto bob_balance = db.get_balance(actor(bob), asset_id_type());

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator2, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.game_lifetime_limit_in_seconds));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(alice), asset_id_type())), to_string(alice_balance));
    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(bob), asset_id_type())), to_string(bob_balance));

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fail_expire_game_lifetime_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_rollback_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.buy_in_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.buy_in_return_id));
}

PLAYCHAIN_TEST_CASE(check_game_start_playing_voting_expiration)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(bob)));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_playing_expiration_seconds));

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(bob)));

    //voting round has started again and owner required
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(bob)));

    //print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fail_expire_game_start_playing_id));
}

PLAYCHAIN_TEST_CASE(check_game_result_voting_expiration)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(bob)));

    const auto& params = get_playchain_parameters(db);

    //the game lasted half of lifetime
    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.game_lifetime_limit_in_seconds / 2));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = stake - win;
    result.log = "alice has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_results_expiration_seconds));

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(bob)));

    generate_blocks(2);

    //print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fail_expire_game_result_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_rollback_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
}

PLAYCHAIN_TEST_CASE(check_two_games_ok)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    const auto& params = get_playchain_parameters(db);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial1;
    initial1.cash[actor(alice)] = stake;
    initial1.cash[actor(bob)] = stake;
    initial1.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE(table_obj.is_free());

    auto alice_balance = db.get_balance(actor(alice), asset_id_type());
    auto bob_balance = db.get_balance(actor(bob), asset_id_type());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(alice), asset_id_type())), to_string(alice_balance - stake) );
    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(bob), asset_id_type())), to_string(bob_balance - stake) );

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial1));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial1));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial1));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());
    BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(bob)));

    //the game 1 lasted half of lifetime
    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.game_lifetime_limit_in_seconds / 2));

    game_result result1;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result1.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result1.cash[actor(bob)];
    bob_result.cash = stake - win;
    bob_result.rake = asset(0);
    result1.log = "alice has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result1));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result1));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result1));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(bob)));

    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(alice))), to_string(result1.cash[actor(alice)].cash));
    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(bob))), to_string(result1.cash[actor(bob)].cash));

    BOOST_CHECK_NO_THROW(buy_out_table(bob, table, result1.cash[actor(bob)].cash));

    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(!table_obj.is_waiting_at_table(get_player(bob)));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(bob), asset_id_type())), to_string(bob_balance - stake + result1.cash[actor(bob)].cash) );

    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(sam)));

    game_initial_data initial2;
    initial2.cash[actor(alice)] = result1.cash[actor(alice)].cash;
    initial2.cash[actor(sam)] = stake;
    initial2.info = "sam is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial2));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial2));
    BOOST_CHECK_NO_THROW(game_start_playing_check(sam, table, initial2));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());
    BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(sam)));

    //the game 2 lasted half of lifetime
    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.game_lifetime_limit_in_seconds / 2));

    game_result result2;
    win = result1.cash[actor(alice)].cash;
    win_rake = asset(win.amount/20);
    auto &alice_result2 = result2.cash[actor(alice)];
    alice_result2.cash = asset(0);
    alice_result2.rake = asset(0);
    auto &sam_result = result2.cash[actor(sam)];
    sam_result.cash = stake + win - win_rake;
    sam_result.rake = win_rake;
    result2.log = "sam has 10 10 10";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result2));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result2));
    BOOST_CHECK_NO_THROW(game_result_check(sam, table, result2));

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(!table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(sam)));

    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(sam))), to_string(result2.cash[actor(sam)].cash));

//    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));
}

PLAYCHAIN_TEST_CASE(check_buy_in_buy_out_when_voting_for_start_playing)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(buy_out_table(jon, table, stake));

    BOOST_CHECK_NO_THROW(buy_out_table(alice, table, stake));
    BOOST_CHECK_THROW(buy_out_table(alice, table, asset(stake.amount / 2)), fc::exception);

    //initial was changed
    BOOST_CHECK_THROW(game_start_playing_check(alice, table, initial), fc::exception);
    BOOST_CHECK_THROW(game_start_playing_check(bob, table, initial), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_buy_in_buy_out_special_when_playing)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    const auto& params = get_playchain_parameters(db);

    auto alice_balance_rest = db.get_balance(actor(alice), asset_id_type());
    auto bob_balance_rest = db.get_balance(actor(bob), asset_id_type());
    auto sam_balance_rest = db.get_balance(actor(sam), asset_id_type());

    auto stake = asset(player_init_balance/4);

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    alice_balance_rest -= stake;
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    bob_balance_rest -= stake;
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));
    sam_balance_rest -= stake;

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.cash[actor(sam)] = stake;
    initial.info = "alice is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(sam, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(bob)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(sam)));

    //add to exist stake
    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, asset(stake.amount / 2)));
    alice_balance_rest -= asset(stake.amount / 2);
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    bob_balance_rest -= stake;

    //buy-out for exist
    BOOST_CHECK_NO_THROW(buy_out_table(bob, table, stake));
    BOOST_CHECK_NO_THROW(buy_out_table(sam, table, stake));

    //the game lasted half of lifetime
    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.game_lifetime_limit_in_seconds / 2));

    game_result result;
    auto win = asset(stake.amount + stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result.cash[actor(alice)];
    alice_result.cash = asset(stake.amount);
    alice_result.cash += win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = asset(stake.amount/2);
    bob_result.rake = asset(0);
    auto &sam_result = result.cash[actor(sam)];
    sam_result.cash = asset(0);
    sam_result.rake = asset(0);
    result.log = "alice has K K A";

    try {
        game_result_check(richregistrator1, table, result);
    } FC_LOG_AND_RETHROW()
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(sam, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(bob)));

    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(alice))), to_string(asset(stake.amount * 3) - win_rake));
    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(bob))), to_string(asset(stake.amount / 2)));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(alice), asset_id_type())), to_string(alice_balance_rest) );
    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(bob), asset_id_type())), to_string(bob_balance_rest + stake) );
    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(sam), asset_id_type())), to_string(sam_balance_rest) );

    generate_blocks(2);

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);

    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fraud_buy_out_id));

    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));
}

PLAYCHAIN_TEST_CASE(check_buy_in_buy_out_forbiddance_for_players_when_voting_for_result)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = stake - win;
    bob_result.rake = asset(0);
    result.log = "alice has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    //pending buy-out
    BOOST_CHECK_NO_THROW(buy_out_table(alice, table, stake));
    //modify pending buy-out
    BOOST_CHECK_NO_THROW(buy_out_table(alice, table, asset(4 * stake.amount)));

    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    try
    {
        game_result_check(bob, table, result);
    } FC_LOG_AND_RETHROW()

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_free());

//    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.buy_out_allowed_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fraud_buy_out_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));
}

PLAYCHAIN_TEST_CASE(check_double_buy_out_for_pending)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    create_witness(richregistrator2);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    auto alice_balance_rest = db.get_balance(actor(alice), asset_id_type());
    auto bob_balance_rest = db.get_balance(actor(bob), asset_id_type());

    auto stake = asset(player_init_balance/2);

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    alice_balance_rest -= stake;
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    bob_balance_rest -= stake;

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK_NO_THROW(buy_out_table(alice, table, stake));
    //buy-out change exist buy-out
    BOOST_CHECK_NO_THROW(buy_out_table(alice, table, asset(stake.amount/2)));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = stake - win;
    bob_result.rake = asset(0);
    result.log = "alice has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(alice), asset_id_type())), to_string(alice_balance_rest + asset(stake.amount/2)) );
    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(bob), asset_id_type())), to_string(bob_balance_rest) );

//    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.buy_out_allowed_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));
}

PLAYCHAIN_TEST_CASE(check_buy_in_buy_out_for_free_players)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    auto sam_balance = db.get_balance(actor(sam), asset_id_type());
    auto jon_balance = db.get_balance(actor(jon), asset_id_type());

    auto stake = asset(player_init_balance/2);

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    //new player
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    //exist player but his is not in the game
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_out_table(jon, table, stake));

    const auto& params = get_playchain_parameters(db);

    //the game lasted half of buy-in lifetime
    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds / 2));

    //free player buy-out
    BOOST_CHECK_NO_THROW(buy_out_table(sam, table, asset(stake.amount / 3)));
    BOOST_CHECK_NO_THROW(buy_out_table(jon, table, asset(stake.amount / 3)));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = stake - win;
    bob_result.rake = asset(0);
    result.log = "alice has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));

    //no buy-in forbiddance for free player(s)
    BOOST_CHECK_NO_THROW(buy_out_table(sam, table, asset(stake.amount / 3)));
    BOOST_CHECK_NO_THROW(buy_out_table(jon, table, asset(stake.amount / 3)));

    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_free());

    auto stake_rest = stake - asset(stake.amount / 3) - asset(stake.amount / 3);

    BOOST_CHECK_NO_THROW(buy_out_table(sam, table, stake_rest));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(sam), asset_id_type())), to_string(sam_balance) );
    BOOST_CHECK_EQUAL(to_string(db.get_balance(actor(jon), asset_id_type())),
                      to_string(jon_balance - stake_rest) );

//    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
}

PLAYCHAIN_TEST_CASE(check_game_reset_game)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance)));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(bob)), to_string(asset(player_init_balance)));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(sam)), to_string(asset(player_init_balance)));

    const table_object &table_obj = table(db);

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance) - stake));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(bob)), to_string(asset(player_init_balance) - stake));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(sam)), to_string(asset(player_init_balance) - stake));

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_CHECK_NO_THROW(game_reset(richregistrator1, table, false));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance) - stake));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(bob)), to_string(asset(player_init_balance) - stake));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(sam)), to_string(asset(player_init_balance) - stake));

    generate_blocks(2);

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), game_reset_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_rollback_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
}

PLAYCHAIN_TEST_CASE(check_game_reset_table)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance)));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(bob)), to_string(asset(player_init_balance)));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(sam)), to_string(asset(player_init_balance)));

    const table_object &table_obj = table(db);

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance) - stake));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(bob)), to_string(asset(player_init_balance) - stake));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(sam)), to_string(asset(player_init_balance) - stake));

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_CHECK_NO_THROW(game_reset(richregistrator1, table, true));

    generate_blocks(2);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance)));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(bob)), to_string(asset(player_init_balance)));
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(sam)), to_string(asset(player_init_balance)));

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), game_reset_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_rollback_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.buy_in_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.buy_in_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.buy_in_return_id));
}

PLAYCHAIN_TEST_CASE(check_game_result_voting_expiration_with_min_votes)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.cash[actor(sam)] = stake;
    initial.cash[actor(jon)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(sam, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(jon, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(bob)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(sam)));
    BOOST_REQUIRE(table_obj.is_playing(get_player(jon)));

    const auto& params = get_playchain_parameters(db);

    //the game lasted half of lifetime
    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.game_lifetime_limit_in_seconds / 2));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &alice_result = result.cash[actor(alice)];
    alice_result.cash = stake + win - win_rake;
    alice_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    auto rest_part = asset(win.amount/3);
    bob_result.cash = stake - rest_part;
    auto &sam_result = result.cash[actor(sam)];
    sam_result.cash = stake - rest_part;
    auto &jon_result = result.cash[actor(jon)];
    jon_result.cash = stake - win + rest_part + rest_part;
    result.log = "alice has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(jon, table, result));
    // alice and jon have lag with connection

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_results_expiration_seconds));

    BOOST_REQUIRE(table_obj.is_free());

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));
}

PLAYCHAIN_TEST_CASE(check_reset_game_by_voters)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    const auto& params = get_playchain_parameters(db);

    Actor b1 = create_new_player(richregistrator1, "b1", asset(new_player_balance));
    Actor b2 = create_new_player(richregistrator1, "b2", asset(new_player_balance));
    Actor b3 = create_new_player(richregistrator1, "b3", asset(new_player_balance));

    auto stake = asset(new_player_balance/4);

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(b1, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(b2, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(b3, richregistrator1, table, stake));

    game_initial_data initial;
    initial.cash[actor(b1)] = stake;
    initial.cash[actor(b2)] = stake;
    initial.info = "b1 is diller";

    BOOST_REQUIRE(table_obj.is_free());

    try {
        game_start_playing_check(richregistrator1, table, initial);
        game_start_playing_check(b1, table, initial);
        game_start_playing_check(b2, table, initial);
    } FC_LOG_AND_RETHROW()

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &winner_result = result.cash[actor(b1)];
    winner_result.cash = stake + win - win_rake;
    winner_result.rake = win_rake;
    auto &last_result = result.cash[actor(b2)];
    last_result.cash = stake - win;
    result.log = "**** has A 4";

    game_result reset_result;

    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b1, table, reset_result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b2, table, reset_result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_REQUIRE(table_obj.cash.count(get_player(b1)));
    BOOST_REQUIRE(table_obj.cash.count(get_player(b2)));
    BOOST_REQUIRE(table_obj.cash.count(get_player(b3)));

    BOOST_CHECK(table_obj.playing_cash.empty());

    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(b1))), to_string(stake));
    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(b2))), to_string(stake));
    BOOST_CHECK_EQUAL(to_string(table_obj.cash.at(get_player(b3))), to_string(stake));

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_rollback_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_cash_return_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fraud_game_result_check_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_expire_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_expire_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_expire_id);
}

PLAYCHAIN_TEST_CASE(check_buy_out_after_reset)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    Actor b1 = create_new_player(richregistrator1, "b1", asset(new_player_balance));
    Actor b2 = create_new_player(richregistrator1, "b2", asset(new_player_balance));
    Actor b3 = create_new_player(richregistrator1, "b3", asset(new_player_balance));

    auto stake = asset(new_player_balance/4);

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(b1, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(b2, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(b3, richregistrator1, table, stake));

    game_initial_data initial;
    initial.cash[actor(b1)] = stake;
    initial.cash[actor(b2)] = stake;
    initial.cash[actor(b3)] = stake;
    initial.info = "b1 is diller";

    BOOST_REQUIRE(table_obj.is_free());

    try {
        game_start_playing_check(richregistrator1, table, initial);
        game_start_playing_check(b1, table, initial);
        game_start_playing_check(b2, table, initial);
        game_start_playing_check(b3, table, initial);
    } FC_LOG_AND_RETHROW()

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK_NO_THROW(buy_out_table(b3, table, stake));

    game_reset(richregistrator1, table, false);

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK(table_obj.cash.count(get_player(b1)));
    BOOST_CHECK(table_obj.cash.count(get_player(b2)));
    BOOST_CHECK(!table_obj.cash.count(get_player(b3)));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(b1)), to_string(asset(new_player_balance) - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(b2)), to_string(asset(new_player_balance) - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(b3)), to_string(asset(new_player_balance)));
}

PLAYCHAIN_TEST_CASE(check_rollback_game_when_consensus_broken)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);

    const auto& params = get_playchain_parameters(db);

    Actor b1 = create_new_player(richregistrator1, "b1", asset(new_player_balance));
    Actor b2 = create_new_player(richregistrator1, "b2", asset(new_player_balance));
    Actor b3 = create_new_player(richregistrator1, "b3", asset(new_player_balance));

    auto stake = asset(new_player_balance/4);

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(b1, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(b2, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(b3, richregistrator1, table, stake));

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(b1)), to_string(asset(new_player_balance) - stake));

    game_initial_data initial;
    initial.cash[actor(b1)] = stake;
    initial.cash[actor(b2)] = stake;
    initial.cash[actor(b3)] = stake;
    initial.info = "b1 is diller";

    BOOST_REQUIRE(table_obj.is_free());

    try {
        game_start_playing_check(richregistrator1, table, initial);
        game_start_playing_check(b1, table, initial);
        game_start_playing_check(b2, table, initial);
        game_start_playing_check(b3, table, initial);
    } FC_LOG_AND_RETHROW()

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK_NO_THROW(buy_out_table(b2, table, stake));

    game_result result1;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &b1_result = result1.cash[actor(b1)];
    b1_result.cash = stake + win - win_rake;
    b1_result.rake = win_rake;
    auto &b2_result = result1.cash[actor(b2)];
    b2_result.cash = stake - win;
    result1.cash[actor(b3)].cash = stake;
    result1.log = "**** has A 4";

    game_result result2 = result1;
    result2.log =  "**** has A 5";

    game_result result3 = result1;
    result3.log =  "**** has A 6";

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(b1)), to_string(asset(new_player_balance) - stake));

    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator1, table, result1));
    BOOST_REQUIRE_NO_THROW(game_result_check(b1, table, result2));
    try
    {
        game_result_check(b3, table, result3);
    }FC_LOG_AND_RETHROW()

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_results_expiration_seconds));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(b1)), to_string(asset(new_player_balance) - stake));
}

PLAYCHAIN_TEST_CASE(check_buy_in_expiration_for_buy_out_rest)
{
    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u, meta);

    const auto& params = get_playchain_parameters(db);

    Actor b1 = create_new_player(richregistrator1, "b1", asset(new_player_balance));
    Actor b2 = create_new_player(richregistrator1, "b2", asset(new_player_balance));
    Actor b3 = create_new_player(richregistrator1, "b3", asset(new_player_balance));
    Actor b4 = create_new_player(richregistrator1, "b4", asset(new_player_balance));
    Actor b5 = create_new_player(richregistrator1, "b5", asset(new_player_balance));

    const table_object &table_obj = table(db);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b1, get_next_uid(actor(b1)), asset(900), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b2, get_next_uid(actor(b2)), asset(530), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b3, get_next_uid(actor(b3)), asset(780), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b4, get_next_uid(actor(b4)), asset(630), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b5, get_next_uid(actor(b5)), asset(580), meta));

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator1, table));

    generate_block();

    pending_buy_in_resolve_all(richregistrator1, table_obj);

    game_initial_data initial;
    initial.cash[actor(b1)] = asset(900);
    initial.cash[actor(b2)] = asset(530);
    initial.cash[actor(b3)] = asset(780);
    initial.info = "210";

    BOOST_REQUIRE(table_obj.is_free());

    idump((table_obj));

    try {
        game_start_playing_check(richregistrator1, table, initial);
        game_start_playing_check(b1, table, initial);
        game_start_playing_check(b2, table, initial);
        game_start_playing_check(b3, table, initial);
    } FC_LOG_AND_RETHROW()

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK_NO_THROW(buy_out_table(b3, table, asset(760)));

    game_result result;
    result.cash[actor(b1)].cash = asset(895);
    result.cash[actor(b2)].cash = asset(545);
    result.cash[actor(b3)].cash = asset(770);
    result.log = "$bot10_1.2.31:0:900;bot4_1.2.27:1:530;bot3_1.2.13:2:780|D2:10:4.0|L0B1R220;F0E2|W1:25;K0S1C7H3|0:895;1:545;2:770";

    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b2, table, result));

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_results_expiration_seconds));

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK(table_obj.cash.count(get_player(b1)));
    BOOST_CHECK(table_obj.cash.count(get_player(b2)));
    BOOST_CHECK(table_obj.cash.count(get_player(b3)));
    BOOST_CHECK(table_obj.cash.count(get_player(b4)));
    BOOST_CHECK(table_obj.cash.count(get_player(b5)));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(b3)), to_string(asset(new_player_balance) - asset(780) + asset(760)));

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    BOOST_CHECK(!table_obj.cash.count(get_player(b1)));
    BOOST_CHECK(!table_obj.cash.count(get_player(b2)));
    BOOST_CHECK(!table_obj.cash.count(get_player(b3)));
    BOOST_CHECK(!table_obj.cash.count(get_player(b4)));
    BOOST_CHECK(!table_obj.cash.count(get_player(b5)));

    print_last_operations(actor(richregistrator1), next_history_record);
}

PLAYCHAIN_TEST_CASE(sticked_pending_votes_bug_check)
{
    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u, meta);

    const auto& params = get_playchain_parameters(db);

    Actor b1 = create_new_player(richregistrator1, "b1", asset(new_player_balance));
    Actor b2 = create_new_player(richregistrator1, "b2", asset(new_player_balance));
    Actor b3 = create_new_player(richregistrator1, "b3", asset(new_player_balance));

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b1, get_next_uid(actor(b1)), asset(470), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b2, get_next_uid(actor(b2)), asset(460), meta));

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator1, table));

    generate_block();

    pending_buy_in_resolve_all(richregistrator1, table_obj);

    game_initial_data initial;
    initial.cash[actor(b1)] = asset(470);
    initial.cash[actor(b2)] = asset(460);
    initial.info = "210";

    BOOST_REQUIRE(table_obj.is_free());

    try {
        game_start_playing_check(richregistrator1, table, initial);
        game_start_playing_check(b1, table, initial);
        game_start_playing_check(b2, table, initial);
    } FC_LOG_AND_RETHROW()

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    game_result result;
    result.cash[actor(b1)].cash = asset(460);
    result.cash[actor(b2)].cash = asset(470);
    result.log = "$bot25_1.2.11:0:460;bot21_1.2.45:1:470|D0:10:4.0|L0B1R015;F1|W0:20;K0S0C2CQ|0:470;1:460";

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b2, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    game_initial_data initial2;
    initial2.cash[actor(b1)] = asset(460);
    initial2.cash[actor(b2)] = asset(470);
    initial2.info = "110";

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator1, table, initial2));

    BOOST_REQUIRE_NO_THROW(game_reset(richregistrator1, table, false));

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b1, table, initial2));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b2, table, initial2));

    BOOST_CHECK_NO_THROW(buy_out_table(b2, table, asset(470)));
    BOOST_CHECK_NO_THROW(buy_in_table(b3, richregistrator1, table, asset(450)));

    game_initial_data initial3;
    initial3.cash[actor(b1)] = asset(460);
    initial3.cash[actor(b3)] = asset(450);
    initial3.info = "110";

    idump((table));

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator1, table, initial3));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b3, table, initial3));

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_playing_expiration_seconds));

    BOOST_REQUIRE(table_obj.is_free());

    print_last_operations(actor(richregistrator1), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_reset_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_rollback_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fail_vote_id));
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fail_vote_id));
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fail_expire_game_start_playing_id));
}

PLAYCHAIN_TEST_CASE(wrong_players_composition_votes_bug_check)
{
    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u, meta);

    Actor b1 = create_new_player(richregistrator1, "b1", asset(new_player_balance));
    Actor b2 = create_new_player(richregistrator1, "b2", asset(new_player_balance));
    Actor b3 = create_new_player(richregistrator1, "b3", asset(new_player_balance));
    Actor b4 = create_new_player(richregistrator1, "b4", asset(new_player_balance));

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b1, get_next_uid(actor(b1)), asset(470), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b2, get_next_uid(actor(b2)), asset(460), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b3, get_next_uid(actor(b3)), asset(400), meta));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(b4, get_next_uid(actor(b4)), asset(500), meta));

    BOOST_REQUIRE_NO_THROW(table_alive(richregistrator1, table));

    generate_block();

    pending_buy_in_resolve_all(richregistrator1, table_obj);

    BOOST_REQUIRE(table_obj.is_free());

    const auto& params = get_playchain_parameters(db);

    game_initial_data initial;
    initial.cash[actor(b1)] = asset(470);
    initial.cash[actor(b2)] = asset(460);
    initial.cash[actor(b3)] = asset(400);
    initial.info = "110";

    game_initial_data initial2;
    initial2.cash[actor(b1)] = asset(470);
    initial2.cash[actor(b3)] = asset(400);
    initial2.info = "110";

    game_initial_data initial3;
    initial3.cash[actor(b1)] = asset(470);
    initial3.cash[actor(b2)] = asset(460);
    initial3.cash[actor(b4)] = asset(500);
    initial3.info = "110";

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

    idump((table_obj));

    try {
        game_start_playing_check(richregistrator1, table, initial);
        game_start_playing_check(b3, table, initial);
    } FC_LOG_AND_RETHROW()

    BOOST_CHECK_THROW(game_start_playing_check(b1, table, initial2), fc::exception);
    BOOST_CHECK_THROW(game_start_playing_check(b2, table, initial3), fc::exception);

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_playing_expiration_seconds));

    print_last_operations(actor(richregistrator1), next_history_record);
}

BOOST_AUTO_TEST_SUITE_END()
}
