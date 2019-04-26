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

#include <playchain/chain/protocol/buy_in_buy_out_operations.hpp>

#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/account_object.hpp>

namespace playchain { namespace chain {

    void buy_in_operation::validate() const
    {
        FC_ASSERT(fee.amount >= 0);
        FC_ASSERT(player != table_owner);
        FC_ASSERT(big_blind_price.amount > 0);
        FC_ASSERT(chips_per_big_blind > 0);
        FC_ASSERT(purchased_big_blind_amount > 0);
    }

    void buy_out_operation::validate() const
    {
        FC_ASSERT( fee.amount >= 0 );
        FC_ASSERT( amount.amount >= 0 );
        FC_ASSERT( player != table_owner );
    }

    //^ legacy
    //=====================================================
    // playchain:

    void buy_in_table_operation::validate() const
    {
        FC_ASSERT( fee.amount >= 0 );
        FC_ASSERT( amount.amount > 0 );
        FC_ASSERT( !account_object::is_special_account(player) );
        FC_ASSERT( !account_object::is_special_account(table_owner) );
        FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
        FC_ASSERT( player != table_owner, "Table owner can't play at the own table" );
    }

    void buy_out_table_operation::validate() const
    {
        FC_ASSERT( fee.amount >= 0 );
        FC_ASSERT( amount.amount > 0 );
        //'reason' must be limited to protect table_owner from overpayments for kilobytes
        FC_ASSERT( reason.size() <= PLAYCHAIN_MAXIMUM_BUYOUT_REASON_SIZE );
        FC_ASSERT( !account_object::is_special_account(player) );
        FC_ASSERT( !account_object::is_special_account(table_owner) );
        FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
        FC_ASSERT( player != table_owner, "Table owner can't play at the own table" );
    }

    void buy_out_table_operation::get_required_authorities( vector<authority>& auths)const
    {
        //return combined authority: active public key required from player OR table owner
        auths.emplace_back(1, player, 1, table_owner, 1);
    }

    void buy_in_reserve_operation::validate() const
    {
        FC_ASSERT( fee.amount >= 0 );
        FC_ASSERT( !account_object::is_special_account(player) );
        FC_ASSERT(!uid.empty());
        FC_ASSERT( amount.amount > 0 );
        FC_ASSERT(!metadata.empty());
        FC_ASSERT( !protocol_version.empty() );
    }

    void buy_in_reserving_cancel_operation::validate() const
    {
        FC_ASSERT( fee.amount >= 0 );
        FC_ASSERT( !account_object::is_special_account(player) );
        FC_ASSERT(!uid.empty());
    }

    void buy_in_reserving_resolve_operation::validate() const
    {
        FC_ASSERT( pending_buyin != PLAYCHAIN_NULL_PENDING_BUYIN );
        FC_ASSERT( fee.amount >= 0 );
        FC_ASSERT( !account_object::is_special_account(table_owner) );
        FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
    }

    void buy_in_reserving_cancel_all_operation::validate() const
    {
        FC_ASSERT( fee.amount >= 0 );
        FC_ASSERT( !account_object::is_special_account(player) );
    }
} }
