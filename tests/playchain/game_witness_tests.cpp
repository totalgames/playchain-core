#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

#include <graphene/chain/hardfork.hpp>

namespace game_witness_tests
{
struct game_witness_fixture: public playchain_common::playchain_fixture
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
    DECLARE_ACTOR(mike)

    game_witness_fixture()
    {
        actor(richregistrator1).supply(asset(rich_registrator_init_balance));
        actor(richregistrator2).supply(asset(rich_registrator_init_balance));
        actor(richregistrator3).supply(asset(rich_registrator_init_balance));

        init_fees();

        CREATE_PLAYER(richregistrator1, alice);
        CREATE_PLAYER(richregistrator1, bob);
        CREATE_PLAYER(richregistrator1, sam);
        CREATE_PLAYER(richregistrator1, jon);
        CREATE_PLAYER(richregistrator1, mike);

        //test only with latest voting algorithm!!!
        generate_blocks(HARDFORK_PLAYCHAIN_11_TIME);
    }
};

BOOST_FIXTURE_TEST_SUITE( game_witness_tests, game_witness_fixture)

PLAYCHAIN_TEST_CASE(check_consensus_with_game_witnesses)
{
     room_id_type room = create_new_room(richregistrator1);
     table_id_type table = create_new_table(richregistrator1, room);
     create_witness(richregistrator2);

     auto stake = asset(player_init_balance/2);

     game_initial_data initial;
     initial.cash[actor(alice)] = stake;
     initial.cash[actor(bob)] = stake;
     initial.info = "alice is diller";

     game_initial_data initial_wrong;
     initial_wrong.cash[actor(alice)] = stake;
     initial_wrong.cash[actor(bob)] = stake;
     initial_wrong.info = "bob is diller";

     const table_object &table_obj = table(db);

     auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator1));

     BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
     BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
     BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));

     BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
     BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator2, table, initial));
     BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial_wrong));
     BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

     generate_block();

     BOOST_REQUIRE(table_obj.is_playing(get_player(alice)));
     BOOST_REQUIRE(table_obj.is_playing(get_player(bob)));

//     print_block_operations();
     generate_blocks(2);

//     print_last_operations(actor(richregistrator1), next_history_record);

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
     BOOST_CHECK(check_game_event_type(history, ++record_offset, game_event_type_id.fraud_game_start_playing_check_id));
}

PLAYCHAIN_TEST_CASE(check_game_required_witnesses_which_voted_for_playing)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 1u);
    create_witness(richregistrator2);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator2, table, initial));

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
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));

    //waiting for richregistrator2
    BOOST_REQUIRE(table_obj.is_playing());

    //wrong witness
    BOOST_CHECK_THROW(game_result_check(richregistrator3, table, result), fc::exception);

    //ok
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_game_required_n_witnesses_for_voting)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 2u);
    create_witness(richregistrator2);
    create_witness(richregistrator3);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    //bob has already voted
    BOOST_CHECK_THROW(game_start_playing_check(bob, table, initial), fc::exception);
    //waiting for 2 witnesses

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator2, table, initial));
    //waiting for 2 witnesses

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator3, table, initial));

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
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));

    //waiting for richregistrator2, richregistrator3
    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));

    //waiting for richregistrator2, richregistrator3
    BOOST_REQUIRE(table_obj.is_playing());

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator3, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_game_new_witness_only_for_result_when_table_0)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);
    create_witness(richregistrator2);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

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
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    //new witness
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_game_new_witness_substitute_player_while_voting_for_result_2_players)
{
    auto &&parameters = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(parameters.percentage_of_voter_witness_substitution_while_voting_for_results, 50*GRAPHENE_1_PERCENT);

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);
    create_witness(richregistrator2);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

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

    //one player can be replaced
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));

    //bob can fly out

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_game_new_witness_substitute_player_while_voting_for_result_2_players_different_witness)
{
    auto &&parameters = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(parameters.percentage_of_voter_witness_substitution_while_voting_for_results, 50*GRAPHENE_1_PERCENT);

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 1u);
    create_witness(richregistrator2);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator2, table, initial));
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

    //one player can be replaced
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));

    //bob can fly out

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_game_new_witness_substitute_player_while_voting_for_result_3_players)
{
    auto &&parameters = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(parameters.percentage_of_voter_witness_substitution_while_voting_for_results, 50*GRAPHENE_1_PERCENT);

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);
    create_witness(richregistrator2);
    create_witness(richregistrator3);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.cash[actor(sam)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(sam, table, initial));

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
    auto &sam_result = result.cash[actor(sam)];
    sam_result.cash = stake;
    sam_result.rake = asset(0);
    result.log = "alice has A 4";

    //one player can be replaced (due to rounding down)
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator3, table, result)); //irrelevant !
    BOOST_CHECK_NO_THROW(game_result_check(sam, table, result));

    //bob can fly out

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_game_new_witness_substitute_player_while_voting_for_result_4_players)
{
    auto &&parameters = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(parameters.percentage_of_voter_witness_substitution_while_voting_for_results, 50*GRAPHENE_1_PERCENT);

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);
    create_witness(richregistrator2);
    create_witness(richregistrator3);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.cash[actor(sam)] = stake;
    initial.cash[actor(jon)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

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
    auto &sam_result = result.cash[actor(sam)];
    sam_result.cash = stake;
    sam_result.rake = asset(0);
    auto &jon_result = result.cash[actor(jon)];
    jon_result.cash = stake;
    jon_result.rake = asset(0);
    result.log = "alice has A 4";

    //two players can be replaced
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator3, table, result));

    //sam, jon can fly out

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_game_new_witness_substitute_player_while_voting_for_result_5_players)
{
    auto &&parameters = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(parameters.percentage_of_voter_witness_substitution_while_voting_for_results, 50*GRAPHENE_1_PERCENT);

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);
    create_witness(richregistrator2);
    create_witness(richregistrator3);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.cash[actor(sam)] = stake;
    initial.cash[actor(jon)] = stake;
    initial.cash[actor(mike)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(mike, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(sam, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(jon, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(mike, table, initial));

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
    auto &sam_result = result.cash[actor(sam)];
    sam_result.cash = stake;
    sam_result.rake = asset(0);
    auto &jon_result = result.cash[actor(jon)];
    jon_result.cash = stake;
    jon_result.rake = asset(0);
    auto &mike_result = result.cash[actor(mike)];
    mike_result.cash = stake;
    mike_result.rake = asset(0);
    result.log = "alice has A 4";

    //two players can be replaced (due to rounding down)
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(jon, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator3, table, result));

    //sam, mike can fly out

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
}

PLAYCHAIN_TEST_CASE(check_witnesses_using_to_save_consensus_for_more_flew_out_players_by_expiration)
{
    //this future controlled by option 'min_votes_for_results'

    auto &&params = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(params.min_votes_for_results, 2); //default for test mode

    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance)));

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);
    create_witness(richregistrator2);
    create_witness(richregistrator3);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.cash[actor(sam)] = stake;
    initial.cash[actor(jon)] = stake;
    initial.cash[actor(mike)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(mike, richregistrator1, table, stake));

    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(sam, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(jon, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(mike, table, initial));

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
    auto &sam_result = result.cash[actor(sam)];
    sam_result.cash = stake;
    sam_result.rake = asset(0);
    auto &jon_result = result.cash[actor(jon)];
    jon_result.cash = stake;
    jon_result.rake = asset(0);
    auto &mike_result = result.cash[actor(mike)];
    mike_result.cash = stake;
    mike_result.rake = asset(0);
    result.log = "alice has A 4";

    BOOST_CHECK_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance) - stake));

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result)); //# 1
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result)); //# 2
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator2, table, result)); //witness has enough time to interfere with the voting
    BOOST_CHECK_NO_THROW(game_result_check(richregistrator3, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_results_expiration_seconds));

    //sam, mike, jon can fly out

    BOOST_REQUIRE(table_obj.is_free());

    auto alice_new_stake =  stake + win - win_rake;

    BOOST_CHECK_NO_THROW(buy_out_table(alice, table, alice_new_stake));

    //was consensus
    BOOST_CHECK_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance) + win - win_rake));
}

PLAYCHAIN_TEST_CASE(check_consensus_saving_for_more_flew_out_players_by_expiration_without_witnesses)
{
    auto &&params = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(params.min_votes_for_results, 2); //default for test mode
    BOOST_REQUIRE_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance)));

    //Only if we works without witness:

    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u); //# 1 no one witness required
    create_witness(richregistrator2);
    create_witness(richregistrator3);

    auto stake = asset(player_init_balance/2);

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.cash[actor(sam)] = stake;
    initial.cash[actor(jon)] = stake;
    initial.cash[actor(mike)] = stake;
    initial.info = "alice is diller";

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    BOOST_CHECK_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(jon, richregistrator1, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(mike, richregistrator1, table, stake));

    //#2 no one witness voted
    BOOST_CHECK_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(bob, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(sam, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(jon, table, initial));
    BOOST_CHECK_NO_THROW(game_start_playing_check(mike, table, initial));

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
    auto &sam_result = result.cash[actor(sam)];
    sam_result.cash = stake;
    sam_result.rake = asset(0);
    auto &jon_result = result.cash[actor(jon)];
    jon_result.cash = stake;
    jon_result.rake = asset(0);
    auto &mike_result = result.cash[actor(mike)];
    mike_result.cash = stake;
    mike_result.rake = asset(0);
    result.log = "alice has A 4";

    BOOST_CHECK_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance) - stake));

    BOOST_CHECK_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_CHECK_NO_THROW(game_result_check(alice, table, result)); //# 1
    BOOST_CHECK_NO_THROW(game_result_check(bob, table, result)); //# 2

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(params.voting_for_results_expiration_seconds));

    //sam, mike, jon can fly out

    BOOST_REQUIRE(table_obj.is_free());

    auto alice_new_stake =  stake + win - win_rake;

    BOOST_CHECK_NO_THROW(buy_out_table(alice, table, alice_new_stake));

    //was consensus
    BOOST_CHECK_EQUAL(to_string(get_account_balance(alice)), to_string(asset(player_init_balance) + win - win_rake));
}

PLAYCHAIN_TEST_CASE(check_game_new_witness_substitute_player_while_voting_for_playing)
{
    auto &&parameters = get_playchain_parameters(db);

    BOOST_REQUIRE_EQUAL(parameters.percentage_of_voter_witness_substitution_while_voting_for_playing, 0*GRAPHENE_1_PERCENT);

    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
}
