/*
 * Copyright (c) 2018 Total Games LLC, and contributors.
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

#include <playchain/chain/protocol/game_operations.hpp>
#include <playchain/chain/protocol/game_event_operations.hpp>

#include <graphene/chain/account_object.hpp>

namespace playchain { namespace chain {

void game_start_playing_check_operation::validate() const
{
    FC_ASSERT( !account_object::is_special_account(table_owner) );
    FC_ASSERT( !account_object::is_special_account(voter) );
    FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
    FC_ASSERT( initial_data.cash.empty() || initial_data.cash.size() >= 2u );
}

void game_result_check_operation::validate() const
{
    FC_ASSERT( !account_object::is_special_account(table_owner) );
    FC_ASSERT( !account_object::is_special_account(voter) );
    FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
}

void game_reset_operation::validate() const
{
    FC_ASSERT( !account_object::is_special_account(table_owner) );
    FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
}

} }
