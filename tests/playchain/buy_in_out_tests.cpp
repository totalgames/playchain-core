#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/detail/unbounded.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

namespace buy_in_out_tests
{
struct buy_in_out_fixture: public playchain_common::playchain_fixture
{
    const int64_t registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator)
    DECLARE_ACTOR(temp)

    buy_in_out_fixture()
    {
        actor(richregistrator).supply(asset(registrator_init_balance));
        actor(temp).supply(asset(player_init_balance));

        init_fees();
    }
};

BOOST_FIXTURE_TEST_SUITE( buy_in_out_tests, buy_in_out_fixture)

PLAYCHAIN_TEST_CASE(check_negative_buy_in_operation)
{
    account_id_type owner = actor(richregistrator);
    room_id_type room = create_new_room(richregistrator);
    account_id_type player = actor(temp);
    table_id_type table = create_new_table(richregistrator, room);
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);

    BOOST_CHECK_THROW(buy_in_table_op(GRAPHENE_COMMITTEE_ACCOUNT, owner, table, amount).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_in_table_op(player, GRAPHENE_COMMITTEE_ACCOUNT, table, amount).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_in_table_op(player, player, table, amount).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_in_table_op(owner, owner, table, asset()).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_buy_out_operation)
{
    account_id_type owner = actor(richregistrator);
    room_id_type room = create_new_room(richregistrator);
    account_id_type player = actor(temp);
    table_id_type table = create_new_table(richregistrator, room);
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);

    BOOST_CHECK_THROW(buy_out_table_op(GRAPHENE_COMMITTEE_ACCOUNT, owner, table, amount).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_out_table_op(player, GRAPHENE_COMMITTEE_ACCOUNT, table, asset()).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_out_table_op(player, player, table, amount).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_out_table_op(owner, owner, table, asset()).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_buy_in_out_operation)
{
    Actor player1 = create_new_player(richregistrator, "andrew", asset(player_init_balance));

    room_id_type room = create_new_room(richregistrator);
    table_id_type table = create_new_table(richregistrator, room);
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);

    account_id_type player = actor(player1);

    auto balance = db.get_balance(player, asset_id_type());

    BOOST_REQUIRE_NO_THROW(buy_in_table(player1, richregistrator, table, amount));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(player, asset_id_type())), to_string(balance -amount) );

    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, table, amount));

    BOOST_CHECK_EQUAL(to_string(db.get_balance(player, asset_id_type())), to_string(balance) );
}

PLAYCHAIN_TEST_CASE(check_negative_buy_in_evaluate)
{
    Actor player1 = create_new_player(richregistrator, "andrew", asset(player_init_balance));

    room_id_type room = create_new_room(richregistrator);
    table_id_type table = create_new_table(richregistrator, room);
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);

    //temp is not player
    BOOST_CHECK_THROW(buy_in_table(temp, richregistrator, table, amount), fc::exception);

    //table does not exist
    BOOST_CHECK_THROW(buy_in_table(player1, richregistrator, table_id_type{object_id_type{table.space_id, table.type_id, 9}}, amount), fc::exception);

    //Wrong table owner
    BOOST_CHECK_THROW(buy_in_table(player1, temp, table, amount), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_buy_out_evaluate)
{
    Actor player1 = create_new_player(richregistrator, "andrew", asset(player_init_balance));

    room_id_type room = create_new_room(richregistrator);
    table_id_type table = create_new_table(richregistrator, room);
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);

    //temp is not player
    BOOST_CHECK_THROW(buy_out_table(temp, table, amount), fc::exception);

    //too large reason
    BOOST_CHECK_THROW(buy_out_table(temp, table, amount, std::string(PLAYCHAIN_MAXIMUM_BUYOUT_REASON_SIZE + 1, '!')), fc::exception);

    //table does not exist
    buy_out_table_operation op = buy_out_table_op(player1, richregistrator, table_id_type{object_id_type{table.space_id, table.type_id, 9}}, amount);
    BOOST_CHECK_THROW(actor(player1).push_operation(op), fc::exception);

    //Wrong table owner
    op = buy_out_table_op(player1, temp, table, amount);
    BOOST_CHECK_THROW(actor(player1).push_operation(op), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_sign_buy_out)
{
    Actor player1 = create_new_player(richregistrator, "andrew", asset(player_init_balance));

    room_id_type room = create_new_room(richregistrator);
    table_id_type table = create_new_table(richregistrator, room);
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);

    BOOST_REQUIRE_NO_THROW(buy_in_table(player1, richregistrator, table, amount));

    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, richregistrator, player1, table, amount, "By player"));

    BOOST_REQUIRE_NO_THROW(buy_in_table(player1, richregistrator, table, amount));

    BOOST_REQUIRE_NO_THROW(buy_out_table(player1, richregistrator, richregistrator, table, amount, "By table owner"));
}

PLAYCHAIN_TEST_CASE(check_negative_sign_buy_out)
{
    Actor player1 = create_new_player(richregistrator, "bob", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "alice", asset(player_init_balance));

    room_id_type room = create_new_room(richregistrator);
    table_id_type table = create_new_table(richregistrator, room);
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);

    BOOST_REQUIRE_NO_THROW(buy_in_table(player1, richregistrator, table, amount));

    BOOST_CHECK_THROW(buy_out_table(player1, richregistrator, player2, table, amount, "By not authorized player"),  fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_buy_in_reserve_operation)
{
    room_id_type room = create_new_room(richregistrator);
    account_id_type player = actor(temp);
    string uid = get_next_uid(actor(temp));
    asset amount = asset(1*GRAPHENE_BLOCKCHAIN_PRECISION);
    table_id_type table = create_new_table(richregistrator, room);

    BOOST_CHECK_THROW(buy_in_reserve_op(GRAPHENE_COMMITTEE_ACCOUNT, uid, amount, table(db).metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_in_reserve_op(player, "", amount, table(db).metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_in_reserve_op(player, uid, asset{}, table(db).metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_in_reserve_op(player, uid, amount, "").validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_resolve)
{
    const std::string meta = "Game";

    Actor player1 = create_new_player(richregistrator, "andrew", asset(player_init_balance));
    auto &&room = create_new_room(richregistrator);
    create_new_table(richregistrator, room, 0u, meta);

    auto stake = asset(player_init_balance/2);

    auto next_history_record = scroll_history_to_case_start_point(actor(player1));

    try {
        buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta);
    } FC_LOG_AND_RETHROW()

    generate_blocks(2);

    print_last_operations(actor(player1), next_history_record);

    auto history = phistory_api->get_account_history(player1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_reserve_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_cancel)
{
    const std::string meta = "Game";

    Actor player1 = create_new_player(richregistrator, "andrew", asset(player_init_balance));

    auto stake = asset(player_init_balance/2);

    auto next_history_record = scroll_history_to_case_start_point(actor(player1));

    string last_uid = get_next_uid(actor(player1));

    try {
        buy_in_reserve(player1, last_uid, stake, meta);
    } FC_LOG_AND_RETHROW()

    generate_block();

    try {
        buy_in_reserving_cancel(player1, last_uid);
    } FC_LOG_AND_RETHROW()

    generate_blocks(2);

    print_last_operations(actor(player1), next_history_record);

    auto history = phistory_api->get_account_history(player1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_reserve_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_cancel_id);
}

PLAYCHAIN_TEST_CASE(check_buy_in_reserving_expired)
{
    const std::string meta = "Game";

    Actor player1 = create_new_player(richregistrator, "andrew", asset(player_init_balance));

    auto stake = asset(player_init_balance/2);

    auto next_history_record = scroll_history_to_case_start_point(actor(player1));

    try {
        buy_in_reserve(player1, get_next_uid(actor(player1)), stake, "Unknown Game");
    } FC_LOG_AND_RETHROW()

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.pending_buyin_proposal_lifetime_limit_in_seconds));

    print_last_operations(actor(player1), next_history_record);

    auto history = phistory_api->get_account_history(player1.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_reserve_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_expire_id);
}

PLAYCHAIN_TEST_CASE(check_negative_buy_in_reserving_cancel_operation)
{
    string uid = get_next_uid(actor(temp));

    BOOST_CHECK_THROW(buy_in_reserving_cancel_op(GRAPHENE_COMMITTEE_ACCOUNT, uid).validate(), fc::exception);
    BOOST_CHECK_THROW(buy_in_reserving_cancel_op(actor(temp), "").validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_buy_in_reserving_resolve_operation)
{
    room_id_type room = create_new_room(richregistrator);
    table_id_type table = create_new_table(richregistrator, room, 0u, "Game");

    BOOST_CHECK_THROW(buy_in_reserving_resolve_op(GRAPHENE_COMMITTEE_ACCOUNT, table, pending_buy_in_id_type{}).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_buy_in_reserving_cancel_all_operation)
{
    BOOST_CHECK_THROW(buy_in_reserving_cancel_all_op(GRAPHENE_COMMITTEE_ACCOUNT).validate(), fc::exception);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    BOOST_CHECK_THROW(buy_in_reserving_cancel_all(player1), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_direct_buy_in_to_own_table)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    Actor player_and_owner = create_new_player(richregistrator, "blade", asset(player_init_balance * 5));

    room_id_type player_room = create_new_room(player_and_owner, "room", protocol_version);
    table_id_type player_table = create_new_table(player_and_owner, player_room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));

    auto stake = asset(player_init_balance/3);

    BOOST_CHECK_THROW(buy_in_table(player_and_owner, player_and_owner, player_table, stake), fc::exception);
    BOOST_REQUIRE_NO_THROW(buy_in_table(player1, player_and_owner, player_table, stake));
    BOOST_REQUIRE_NO_THROW(buy_in_table(player2, player_and_owner, player_table, stake));

}

PLAYCHAIN_TEST_CASE(check_buy_in_expiration_for_buyin_resolving)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    generate_block();

    auto stake = asset(player_init_balance/3);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));

    generate_block();

    BOOST_REQUIRE(!table(db).pending_proposals.empty());

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));

    BOOST_REQUIRE(!table(db).cash.empty());

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));
}

PLAYCHAIN_TEST_CASE(check_buy_in_expiration_for_buyin_direct)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));

    generate_block();

    auto stake = asset(player_init_balance/3);

    BOOST_REQUIRE_NO_THROW(buy_in_table(player1, richregistrator, table, stake));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));

    generate_block();

    BOOST_REQUIRE(!table(db).cash.empty());

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance)));
}

PLAYCHAIN_TEST_CASE(check_buy_in_expiration_for_buyin_with_prolong)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));

    generate_block();

    auto stake = asset(player_init_balance/3);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));

    generate_block();

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));

    game_initial_data initial;
    initial.cash[actor(player1)] = stake;
    initial.cash[actor(player2)] = stake;
    initial.info = "lucky1 is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(player1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(player2, table, initial));

    BOOST_REQUIRE(table(db).is_playing());

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &result_p1 = result.cash[actor(player1)];
    result_p1.cash = stake + win - win_rake;
    result_p1.rake = win_rake;
    auto &result_p2 = result.cash[actor(player2)];
    result_p2.cash = stake - win;
    result_p2.rake = asset(0);
    result.log = "player1 has A 4";

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(player1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(player2, table, result));

    BOOST_REQUIRE(!table(db).is_playing());

    const table_object &table_obj = table(db);

    player_id_type player1_id = playchain::chain::get_player(db, actor(player1)).id;
    player_id_type player2_id = playchain::chain::get_player(db, actor(player2)).id;

    BOOST_REQUIRE(table_obj.cash.count(player1_id));
    BOOST_REQUIRE(table_obj.cash.count(player2_id));

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) +  win - win_rake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player2)), to_string(asset(player_init_balance) - win));
}

PLAYCHAIN_TEST_CASE(check_buy_in_expiration_for_buyin_with_pending_buyout)
{
    const std::string protocol_version = "1.0.0+20190223";

    const std::string meta = "Game";

    room_id_type room = create_new_room(richregistrator, "room", protocol_version);
    table_id_type table = create_new_table(richregistrator, room, 0u, meta);

    Actor player1 = create_new_player(richregistrator, "player1", asset(player_init_balance));
    Actor player2 = create_new_player(richregistrator, "player2", asset(player_init_balance));
    Actor player3 = create_new_player(richregistrator, "player3", asset(player_init_balance));

    generate_block();

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator));

    auto stake = asset(player_init_balance/5);

    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player1, get_next_uid(actor(player1)), stake, meta, protocol_version));
    BOOST_REQUIRE_NO_THROW(buy_in_reserve(player2, get_next_uid(actor(player2)), stake, meta, protocol_version));

    generate_block();

    BOOST_REQUIRE_EQUAL(table(db).get_pending_proposals(), 2u);

    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));
    BOOST_REQUIRE_NO_THROW(buy_in_reserving_resolve(richregistrator, table, table(db).pending_proposals.begin()->second));

    game_initial_data initial;
    initial.cash[actor(player1)] = stake;
    initial.cash[actor(player2)] = stake;
    initial.info = "lucky1 is diller";

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(player1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(player2, table, initial));

    BOOST_REQUIRE(table(db).is_playing());

    BOOST_CHECK_NO_THROW(buy_in_table(player2, richregistrator, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(player3, richregistrator, table, stake));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player2)), to_string(asset(player_init_balance) - stake - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player3)), to_string(asset(player_init_balance) - stake));

    const auto& params = get_playchain_parameters(db);

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player2)), to_string(asset(player_init_balance) - stake - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player3)), to_string(asset(player_init_balance)));

    BOOST_CHECK_NO_THROW(buy_in_table(player2, richregistrator, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(player3, richregistrator, table, stake));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player2)), to_string(asset(player_init_balance) - stake - stake - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player3)), to_string(asset(player_init_balance) - stake));

    BOOST_CHECK_NO_THROW(buy_out_table(player2, table, stake));
    BOOST_CHECK_NO_THROW(buy_out_table(player3, table, stake));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player2)), to_string(asset(player_init_balance) - stake - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player3)), to_string(asset(player_init_balance)));

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/20);
    auto &result_p1 = result.cash[actor(player1)];
    result_p1.cash = stake + win - win_rake;
    result_p1.rake = win_rake;
    auto &result_p2 = result.cash[actor(player2)];
    result_p2.cash = stake - win;
    result_p2.rake = asset(0);
    result.log = "player1 has A 4";

    try
    {
        game_result_check(richregistrator, table, result);
    }FC_LOG_AND_RETHROW()
    BOOST_CHECK_NO_THROW(game_result_check(player1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(player2, table, result));

    BOOST_REQUIRE(!table(db).is_playing());

    const table_object &table_obj = table(db);

    player_id_type player1_id = playchain::chain::get_player(db, actor(player1)).id;
    player_id_type player2_id = playchain::chain::get_player(db, actor(player2)).id;

    BOOST_REQUIRE(table_obj.cash.count(player1_id));
    BOOST_REQUIRE(table_obj.cash.count(player2_id));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player2)), to_string(asset(player_init_balance) - stake - stake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player3)), to_string(asset(player_init_balance)));

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.buy_in_expiration_seconds));

    BOOST_CHECK_EQUAL(to_string(get_account_balance(player1)), to_string(asset(player_init_balance) +  win - win_rake));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player2)), to_string(asset(player_init_balance) - win));
    BOOST_CHECK_EQUAL(to_string(get_account_balance(player3)), to_string(asset(player_init_balance)));

//    print_last_operations(actor(richregistrator), next_history_record);

    auto history = phistory_api->get_account_history(richregistrator.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), buy_in_reserving_allocated_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_allocated_table_id);

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_resolve_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_reserving_resolve_id);

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_start_playing_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_start_playing_validated_id));

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_expire_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_out_table_id);

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_result_check_id);
    BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.game_result_validated_id));

    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_expire_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), buy_in_expire_id);
}

BOOST_AUTO_TEST_SUITE_END()
}
