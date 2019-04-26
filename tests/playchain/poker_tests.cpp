/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>

#include <graphene/utilities/tempdir.hpp>
#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"
#include "actor.hpp"
#include "defines.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;
using namespace playchain::chain;

struct poker_fixture : public  database_fixture
{
    static const int64_t player_init_balance = 1000000;

    DECLARE_ACTOR(player)
    DECLARE_ACTOR(gameserver)

    poker_fixture()
    {
        actor(player).supply(asset(player_init_balance));
        actor(gameserver).supply(asset(player_init_balance));
    }

    ActorActions actor(const Actor& a)
    {
        return ActorActions(*this, a);
    }

    buy_in_operation create_valid_buyin_op()
    {
        buy_in_operation op;
        op.player = actor(player);
        op.table_owner = actor(gameserver);
        op.big_blind_price = asset(10);
        op.chips_per_big_blind = 10;
        op.purchased_big_blind_amount = 100;

        return std::move(op);
    }

    game_operation create_valid_game_op()
    {
        game_operation op;
        op.players.push_back(actor(player));
        op.table_owner = actor(gameserver);
        op.big_blind_price = asset(10);
        op.chips_per_big_blind = 10;

        return std::move(op);
    }
};

BOOST_FIXTURE_TEST_SUITE( poker_tests, poker_fixture)

/////////////////////////////game/////////////////////////////////////////////
PLAYCHAIN_TEST_CASE(empty_buyin)
{
    buy_in_operation op;
    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op), fc::exception);
}

PLAYCHAIN_TEST_CASE(buyin_for_unknown_player)
{
    {
        buy_in_operation op = create_valid_buyin_op();
        op.table_owner = account_id_type(100);
        BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op), fc::exception);
    }
    {
        buy_in_operation op = create_valid_buyin_op();
        op.player = account_id_type(100);
        BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op), fc::exception);
    }
}

PLAYCHAIN_TEST_CASE(buyin_price_must_not_be_zero)
{
    buy_in_operation op = create_valid_buyin_op();
    op.big_blind_price = asset(0);

    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op, gameserver), fc::exception);
}

PLAYCHAIN_TEST_CASE(buyin_big_blind_amount_must_not_be_zero)
{
    buy_in_operation op = create_valid_buyin_op();
    op.purchased_big_blind_amount = 0;

    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op, gameserver), fc::exception);
}

PLAYCHAIN_TEST_CASE(buyin_chips_per_big_blind_must_not_be_zero)
{
    buy_in_operation op = create_valid_buyin_op();
    op.chips_per_big_blind = 0;

    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op, gameserver), fc::exception);
}

PLAYCHAIN_TEST_CASE(buyin_requre_player_and_table_owner_signatures)
{
    {
        buy_in_operation op = create_valid_buyin_op();
        BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op), fc::exception);
    }
    {
        buy_in_operation op = create_valid_buyin_op();
        BOOST_REQUIRE_NO_THROW(actor(player).push_operations_with_additional_signers(op, gameserver));
    }
}

PLAYCHAIN_TEST_CASE(check_balances_after_buyin)
{
    buy_in_operation op = create_valid_buyin_op();
    op.big_blind_price = asset(10);
    op.chips_per_big_blind = 1;
    op.purchased_big_blind_amount = 56;

    BOOST_REQUIRE_NO_THROW(actor(player).push_operations_with_additional_signers(op, gameserver));

    generate_block();

    BOOST_REQUIRE_EQUAL(actor(player).balance(), player_init_balance - op.big_blind_price.amount.value * op.purchased_big_blind_amount);
    BOOST_REQUIRE_EQUAL(actor(gameserver).balance(), player_init_balance + op.big_blind_price.amount.value * op.purchased_big_blind_amount);
}

PLAYCHAIN_TEST_CASE(check_big_blind_amount)
{
    auto op = create_valid_buyin_op();
    op.purchased_big_blind_amount = 39;
    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op, gameserver), fc::exception);

    op.purchased_big_blind_amount = 40;
    BOOST_REQUIRE_NO_THROW(actor(player).push_operations_with_additional_signers(op, gameserver));

    op.purchased_big_blind_amount = 100;
    BOOST_REQUIRE_NO_THROW(actor(player).push_operations_with_additional_signers(op, gameserver));

    op.purchased_big_blind_amount = 101;
    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op, gameserver), fc::exception);
}
/////////////////////////////game/////////////////////////////////////////////
PLAYCHAIN_TEST_CASE(empty_game)
{
    game_operation op;
    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op), fc::exception);
}

PLAYCHAIN_TEST_CASE(game_for_unknown_player)
{
    {
        auto op = create_valid_game_op();
        op.table_owner = account_id_type(100);
        BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op), fc::exception);
    }
    {
        auto op = create_valid_game_op();
        op.players.push_back(account_id_type(100));
        BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op), fc::exception);
    }
}

PLAYCHAIN_TEST_CASE(game_price_must_not_be_zero)
{
    auto op = create_valid_game_op();
    op.big_blind_price = asset(0);

    BOOST_REQUIRE_THROW(actor(player).push_operations_with_additional_signers(op, gameserver), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()
