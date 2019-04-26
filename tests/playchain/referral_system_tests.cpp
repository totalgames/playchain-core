#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

namespace referral_system_tests
{
struct referral_system_fixture: public playchain_common::playchain_fixture
{
    const int64_t rich_registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator1)
    DECLARE_ACTOR(richregistrator2)
    DECLARE_ACTOR(richregistrator3)
    //for simple tests:
    DECLARE_ACTOR(alice)
    DECLARE_ACTOR(bob)
    DECLARE_ACTOR(sam)
    DECLARE_ACTOR(jon)
    //for mass tests:
    DECLARE_ACTOR(player1)
    DECLARE_ACTOR(player2)
    DECLARE_ACTOR(player3)
    DECLARE_ACTOR(player4)
    DECLARE_ACTOR(player5)
    DECLARE_ACTOR(player6)
    DECLARE_ACTOR(player7)
    DECLARE_ACTOR(player8)

    referral_system_fixture()
    {
        actor(richregistrator1).supply(asset(rich_registrator_init_balance));
        actor(richregistrator2).supply(asset(rich_registrator_init_balance));
        actor(richregistrator3).supply(asset(rich_registrator_init_balance));

        init_fees();

        CREATE_PLAYER(richregistrator1, alice);
        CREATE_PLAYER(richregistrator1, bob);
    }

    Actor & create_embedded_playes_depth_8(const Actor& inviter, const asset &value)
    {
        CREATE_PLAYER(inviter, player1);
        CREATE_PLAYER(player1, player2);
        CREATE_PLAYER(player2, player3);
        CREATE_PLAYER(player3, player4);
        CREATE_PLAYER(player4, player5);
        CREATE_PLAYER(player5, player6);
        CREATE_PLAYER(player6, player7);
        CREATE_PLAYER(player7, player8);

        supply(player1, value);
        supply(player2, value);
        supply(player3, value);
        supply(player4, value);
        supply(player5, value);
        supply(player6, value);
        supply(player7, value);
        supply(player8, value);

        return player8;
    }

    asset percent(asset a, uint16_t p)
    {
       BOOST_REQUIRE( a.amount >= 0 );
       BOOST_REQUIRE( p >=0 && p <= GRAPHENE_100_PERCENT );

       fc::uint128 r(a.amount.value);
       r *= p;
       r /= GRAPHENE_100_PERCENT;
       return asset(r.to_uint64(), a.asset_id);
    }

    void supply(const Actor& account, const asset &value)
    {
        actor(account).supply(value);
    }

    asset get_table_owner_reward_balance(const Actor& owner)
    {
        asset result;
        const auto& rooms_by_owner = db.get_index_type<room_index>().indices().get<by_room_owner>();
        auto rooms_range = rooms_by_owner.equal_range(actor(owner));
        for( const room_object& room: boost::make_iterator_range( rooms_range.first, rooms_range.second ) )
        {
            const vesting_balance_object &vb = (*room.balance)(db);
            result += vb.balance;
        }

        return result;
    }

    asset get_witness_reward_balance(const Actor& account)
    {
        asset result;

        const auto &witness_obj = get_witness(account)(db);

        if (witness_obj.balance.valid())
        {
            const vesting_balance_object &vb = (*witness_obj.balance)(db);
            result = vb.balance.amount;
        }
        return result;
    }

    asset get_player_reward_balance(const Actor& player)
    {
        asset result;

        const auto &player_obj = get_player(player)(db);

        if (player_obj.balance.valid())
        {
            const vesting_balance_object &vb = (*player_obj.balance)(db);
            result  = vb.balance.amount;
        }
        return result;
    }

    asset create_rake_with_bob(
            const table_id_type &table,
            const Actor& owner,
            const Actor& witness,
            const Actor& player_with_rake)
    {
        const table_object &table_obj = table(db);

        auto stake = asset(player_init_balance/2);

        BOOST_REQUIRE(table_obj.is_free());

        BOOST_CHECK_NO_THROW(buy_in_table(player_with_rake, owner, table, stake));
        BOOST_CHECK_NO_THROW(buy_in_table(bob, owner, table, stake));

        game_initial_data initial;
        initial.cash[actor(player_with_rake)] = stake;
        initial.cash[actor(bob)] = stake;
        initial.info = "**** is diller";

        try
        {
            game_start_playing_check(owner, table, initial);
            game_start_playing_check(witness, table, initial);
            game_start_playing_check(bob, table, initial);
            game_start_playing_check(player_with_rake, table, initial);
        }FC_LOG_AND_RETHROW()

        BOOST_REQUIRE(table_obj.is_playing());

        generate_block();

        game_result result;
        auto win = asset(stake.amount/2);
        auto win_rake = asset(win.amount/5);
        auto &winner_result = result.cash[actor(player_with_rake)];
        auto player_with_rake_new_cash = stake + win - win_rake;
        winner_result.cash = player_with_rake_new_cash;
        winner_result.rake = win_rake;
        auto &bob_result = result.cash[actor(bob)];
        bob_result.cash = stake - win;
        result.log = "**** has A 4";

        try
        {
            game_result_check(owner, table, result);
            game_result_check(witness, table, result);
            game_result_check(bob, table, result);
        }FC_LOG_AND_RETHROW()

        BOOST_REQUIRE(table_obj.is_free());

        BOOST_REQUIRE_NO_THROW(buy_out_table(player_with_rake, table, player_with_rake_new_cash));

        BOOST_REQUIRE(!table_obj.is_waiting_at_table(get_player(player_with_rake)));

        return win_rake;
    }
};

BOOST_FIXTURE_TEST_SUITE( referral_system_tests, referral_system_fixture)

PLAYCHAIN_TEST_CASE(check_fixture_helper)
{
    BOOST_CHECK_EQUAL(to_string(percent(asset(1000), 50 * GRAPHENE_1_PERCENT)), to_string(asset(500)));
    BOOST_CHECK_EQUAL(to_string(percent(asset(0), 50 * GRAPHENE_1_PERCENT)), to_string(asset(0)));
    BOOST_CHECK_EQUAL(to_string(percent(asset(1000), 0 * GRAPHENE_1_PERCENT)), to_string(asset(0)));
    BOOST_CHECK_EQUAL(to_string(percent(asset(1000), 10 * GRAPHENE_1_PERCENT)), to_string(asset(100)));
    BOOST_CHECK_EQUAL(to_string(percent(asset(1000), 100 * GRAPHENE_1_PERCENT)), to_string(asset(1000)));
}

PLAYCHAIN_TEST_CASE(check_table_owners_reward)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    create_new_room(richregistrator2);

    auto rake = create_rake_with_bob(table, richregistrator1, richregistrator2, alice);

    next_maintenance();

    asset expected_witness_reward = percent(rake, PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE);
    asset expected_table_owner_reward = rake;
    expected_table_owner_reward -= expected_witness_reward;

    BOOST_CHECK_EQUAL(to_string(get_table_owner_reward_balance(richregistrator1) + get_witness_reward_balance(richregistrator2)),
                      to_string(rake));

    BOOST_CHECK_EQUAL(to_string(get_table_owner_reward_balance(richregistrator1)),
                      to_string(expected_table_owner_reward));

    BOOST_CHECK_EQUAL(to_string(get_witness_reward_balance(richregistrator2)),
                      to_string(expected_witness_reward));
}

PLAYCHAIN_TEST_CASE(check_inviter_reward)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    create_new_room(richregistrator2);

    const auto& params = get_playchain_parameters(db);

    CREATE_PLAYER(alice, sam);
    CREATE_PLAYER(bob, jon);

    auto rake = create_rake_with_bob(table, richregistrator1, richregistrator2, sam);

    //to ensure the full reward for Alice
    supply(alice, asset(params.player_referrer_balance_max_threshold) - get_account_balance(alice));

    next_maintenance();

    asset expected_inviter_reward = percent(rake, PLAYCHAIN_DEFAULT_REFERRER_PERCENT_OF_FEE);
    expected_inviter_reward -= percent(expected_inviter_reward, PLAYCHAIN_DEFAULT_REFERRER_PARENT_PERCENT_OF_FEE);

    asset witness_reward = percent(rake, PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE);
    asset expected_table_owner_reward1 = rake - expected_inviter_reward;
    expected_table_owner_reward1-= witness_reward;

    BOOST_CHECK_EQUAL(to_string(get_player_reward_balance(alice)),
                      to_string(expected_inviter_reward) );
    BOOST_CHECK_EQUAL(to_string(get_table_owner_reward_balance(richregistrator1)),
                      to_string(expected_table_owner_reward1) );

    rake = create_rake_with_bob(table, richregistrator1, richregistrator2, jon);

    //to ensure the 50% reward for Bob
    supply(bob, asset(params.player_referrer_balance_max_threshold / 2) - get_account_balance(bob));

    next_maintenance();

    //inviter reward depend on inviter balance (50% from threshold)
    expected_inviter_reward.amount /= 2;

    asset expected_table_owner_reward2 = rake - expected_inviter_reward;
    expected_table_owner_reward2 -= witness_reward;

    BOOST_CHECK_EQUAL(to_string(get_player_reward_balance(bob)),
                      to_string(expected_inviter_reward) );
    BOOST_CHECK_EQUAL(to_string(get_table_owner_reward_balance(richregistrator1)),
                      to_string(expected_table_owner_reward1 + expected_table_owner_reward2) );
}

PLAYCHAIN_TEST_CASE(check_embedded_inviters_reward)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    create_new_room(richregistrator2);

    const auto& params = get_playchain_parameters(db);

    //create player chain with ensurence the full reward for each one
    auto &latest_player = create_embedded_playes_depth_8(alice, asset(params.player_referrer_balance_max_threshold));

    create_rake_with_bob(table, richregistrator1, richregistrator2, latest_player);

    //to ensure the full reward for Alice
    supply(alice, asset(params.player_referrer_balance_max_threshold) - get_account_balance(alice));

    for(auto ci = 0; ci < 2; ++ci)
    {
        next_maintenance();

        BOOST_CHECK_EQUAL(to_string(get_player_reward_balance(alice)),
                          to_string(asset(0)) );
    }
}

PLAYCHAIN_TEST_CASE(check_several_witnesses_reward)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 2u);
    create_new_room(richregistrator2);
    create_new_room(richregistrator3);

    CREATE_PLAYER(richregistrator1, sam);

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_REQUIRE_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_REQUIRE_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "**** is diller";

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator2, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator3, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));

    BOOST_REQUIRE(table_obj.is_playing());

    generate_block();

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/5);
    auto &winner_result = result.cash[actor(alice)];
    winner_result.cash = stake + win - win_rake;
    winner_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = stake - win;
    result.log = "**** has A 4";

    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator2, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator3, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(alice, table, result));

    BOOST_REQUIRE(table_obj.is_free());

    next_maintenance();

    asset expected_witness_reward = percent(win_rake, PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE);

    BOOST_CHECK_EQUAL(to_string(get_witness_reward_balance(richregistrator2)),
                      to_string(asset(expected_witness_reward.amount / 2)));

    BOOST_CHECK_EQUAL(to_string(get_witness_reward_balance(richregistrator3)),
                      to_string(expected_witness_reward - asset(expected_witness_reward.amount / 2)));
}

PLAYCHAIN_TEST_CASE(check_several_witnesses_reward_new_witnesses_in_result)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room, 0u);
    create_new_room(richregistrator2);
    create_new_room(richregistrator3);

    CREATE_PLAYER(richregistrator1, sam);

    const table_object &table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    auto stake = asset(player_init_balance/2);

    BOOST_REQUIRE_NO_THROW(buy_in_table(alice, richregistrator1, table, stake));
    BOOST_REQUIRE_NO_THROW(buy_in_table(bob, richregistrator1, table, stake));
    BOOST_REQUIRE_NO_THROW(buy_in_table(sam, richregistrator1, table, stake));

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "**** is diller";

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator1, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));

    BOOST_REQUIRE(table_obj.is_playing());

    generate_block();

    game_result result;
    auto win = asset(stake.amount/2);
    auto win_rake = asset(win.amount/5);
    auto &winner_result = result.cash[actor(alice)];
    winner_result.cash = stake + win - win_rake;
    winner_result.rake = win_rake;
    auto &bob_result = result.cash[actor(bob)];
    bob_result.cash = stake - win;
    result.log = "**** has A 4";

    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator2, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator3, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(alice, table, result));

    BOOST_REQUIRE(table_obj.is_free());

    next_maintenance();

    asset expected_witness_reward = percent(win_rake, PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE);

    BOOST_CHECK_EQUAL(to_string(get_witness_reward_balance(richregistrator2)),
                      to_string(asset(expected_witness_reward.amount / 2)));

    BOOST_CHECK_EQUAL(to_string(get_witness_reward_balance(richregistrator3)),
                      to_string(expected_witness_reward - asset(expected_witness_reward.amount / 2)));
}

PLAYCHAIN_TEST_CASE(check_no_rewards_for_poor_inviter)
{
    room_id_type room = create_new_room(richregistrator1);
    table_id_type table = create_new_table(richregistrator1, room);
    create_new_room(richregistrator2);

    const auto& params = get_playchain_parameters(db);

    CREATE_PLAYER(alice, sam);

    auto rake = create_rake_with_bob(table, richregistrator1, richregistrator2, sam);

    //to ensure that Alice will no have enough asset
    transfer(actor(alice), actor(sam), get_account_balance(alice));

    BOOST_REQUIRE(get_account_balance(alice).amount == 0);

    BOOST_REQUIRE(params.player_referrer_balance_min_threshold.value > 1);

    supply(alice, asset(params.player_referrer_balance_min_threshold - 1));

    next_maintenance();

    BOOST_CHECK_EQUAL(to_string(get_player_reward_balance(alice)),
                      to_string(asset(0)) );

    BOOST_CHECK_EQUAL(to_string(get_table_owner_reward_balance(richregistrator1) + get_witness_reward_balance(richregistrator2)),
                      to_string(rake));
}

PLAYCHAIN_TEST_CASE(check_rewards_in_several_rooms)
{
    room_id_type room1 = create_new_room(richregistrator1);
    table_id_type table_in_room1 = create_new_table(richregistrator1, room1);
    room_id_type room2 = create_new_room(richregistrator2);
    table_id_type table_in_room2 = create_new_table(richregistrator2, room2);

    CREATE_PLAYER(alice, sam);

    const auto& params = get_playchain_parameters(db);

    //bob will be inviter but his children will not playing
    create_embedded_playes_depth_8(bob, asset(params.player_referrer_balance_max_threshold));

    auto stake = asset(player_init_balance/2);

    auto win_at_room1 = asset(stake.amount/2);
    auto win_rake_at_room1 = asset(win_at_room1.amount/20);

    {
        const table_object &table_obj = table_in_room1(db);

        BOOST_REQUIRE_NO_THROW(buy_in_table(sam, richregistrator1, table_in_room1, stake));
        BOOST_REQUIRE_NO_THROW(buy_in_table(bob, richregistrator1, table_in_room1, stake));

        game_initial_data initial;
        initial.cash[actor(sam)] = stake;
        initial.cash[actor(bob)] = stake;
        initial.info = "**** is diller";

        BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator1, table_in_room1, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(sam, table_in_room1, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table_in_room1, initial));

        BOOST_REQUIRE(table_obj.is_playing());

        game_result result;
        auto &winner_result = result.cash[actor(sam)];
        winner_result.cash = stake + win_at_room1 - win_rake_at_room1;
        winner_result.rake = win_rake_at_room1;
        auto &bob_result = result.cash[actor(bob)];
        bob_result.cash = stake - win_at_room1;
        result.log = "**** has A 4";

        BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator1, table_in_room1, result));
        BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator2, table_in_room1, result));
        BOOST_REQUIRE_NO_THROW(game_result_check(sam, table_in_room1, result));

        BOOST_REQUIRE(table_obj.is_free());

        BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(sam)));
        BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(bob)));
    }

    generate_block();

    auto win_at_room2 = asset(stake.amount);
    auto win_rake_at_room2 = asset(win_at_room2.amount/20);

    {
        const table_object &table_obj = table_in_room2(db);

        BOOST_REQUIRE_NO_THROW(buy_in_table(sam, richregistrator2, table_in_room2, stake));
        BOOST_REQUIRE_NO_THROW(buy_in_table(bob, richregistrator2, table_in_room2, stake));

        game_initial_data initial;
        initial.cash[actor(sam)] = stake;
        initial.cash[actor(bob)] = stake;
        initial.info = "**** is diller";

        BOOST_REQUIRE_NO_THROW(game_start_playing_check(richregistrator2, table_in_room2, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(sam, table_in_room2, initial));
        BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table_in_room2, initial));

        BOOST_REQUIRE(table_obj.is_playing());

        game_result result;
        auto &winner_result = result.cash[actor(sam)];
        winner_result.cash = stake + win_at_room2 - win_rake_at_room2;
        winner_result.rake = win_rake_at_room2;
        auto &bob_result = result.cash[actor(bob)];
        bob_result.cash = stake - win_at_room2;
        result.log = "**** has A 4";

        BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator2, table_in_room2, result));
        BOOST_REQUIRE_NO_THROW(game_result_check(richregistrator1, table_in_room2, result));
        BOOST_REQUIRE_NO_THROW(game_result_check(bob, table_in_room2, result));

        BOOST_REQUIRE(table_obj.is_free());

        BOOST_REQUIRE(table_obj.is_waiting_at_table(get_player(sam)));
        BOOST_REQUIRE(!table_obj.is_waiting_at_table(get_player(bob)));
    }

    //to ensure the full reward for Alice
    supply(alice, asset(params.player_referrer_balance_max_threshold) - get_account_balance(alice));

    next_maintenance();

    asset expected_inviter_reward_at_room1 = percent(win_rake_at_room1, PLAYCHAIN_DEFAULT_REFERRER_PERCENT_OF_FEE);
    expected_inviter_reward_at_room1 -= percent(expected_inviter_reward_at_room1, PLAYCHAIN_DEFAULT_REFERRER_PARENT_PERCENT_OF_FEE);
    asset witness_reward_at_room1 = percent(win_rake_at_room1, PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE);
    asset expected_table1_owner_reward = win_rake_at_room1 - expected_inviter_reward_at_room1;
    expected_table1_owner_reward-= witness_reward_at_room1;

    asset expected_inviter_reward_at_room2 = percent(win_rake_at_room2, PLAYCHAIN_DEFAULT_REFERRER_PERCENT_OF_FEE);
    expected_inviter_reward_at_room2 -= percent(expected_inviter_reward_at_room2, PLAYCHAIN_DEFAULT_REFERRER_PARENT_PERCENT_OF_FEE);
    asset witness_reward_at_room2 = percent(win_rake_at_room2, PLAYCHAIN_DEFAULT_WITNESS_PERCENT_OF_FEE);
    asset expected_table2_owner_reward = win_rake_at_room2 - expected_inviter_reward_at_room2;
    expected_table2_owner_reward-= witness_reward_at_room2;

    BOOST_CHECK_EQUAL(to_string(get_player_reward_balance(alice)),
                      to_string(expected_inviter_reward_at_room1 + expected_inviter_reward_at_room2) );

    BOOST_CHECK_EQUAL(to_string(get_table_owner_reward_balance(richregistrator1)),
                      to_string(expected_table1_owner_reward) );
    BOOST_CHECK_EQUAL(to_string(get_table_owner_reward_balance(richregistrator2)),
                      to_string(expected_table2_owner_reward) );

    BOOST_CHECK_EQUAL(to_string(get_witness_reward_balance(richregistrator2)),
                      to_string(witness_reward_at_room1) );
    BOOST_CHECK_EQUAL(to_string(get_witness_reward_balance(richregistrator1)),
                      to_string(witness_reward_at_room2) );
}

BOOST_AUTO_TEST_SUITE_END()
}
