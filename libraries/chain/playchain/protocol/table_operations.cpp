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

#include <playchain/chain/protocol/table_operations.hpp>

#include <graphene/chain/account_object.hpp>

namespace playchain { namespace chain {

void table_create_operation::validate() const
{
    FC_ASSERT( !account_object::is_special_account(owner) );
    FC_ASSERT( room != PLAYCHAIN_NULL_ROOM );
    FC_ASSERT( min_accepted_proposal_asset.amount > 0 );
}

void table_update_operation::validate() const
{
    FC_ASSERT( !account_object::is_special_account(owner) );
    FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
    FC_ASSERT( min_accepted_proposal_asset.amount > 0 );
}

void tables_alive_operation::validate() const
{
    FC_ASSERT( !account_object::is_special_account(owner) );
    FC_ASSERT( !tables.empty() );
    FC_ASSERT( tables.size() <= PLAYCHAIN_MAX_SIZE_FOR_TABLES_ALIVE_PER_OP );
    for (const auto &table: tables)
    {
        FC_ASSERT( table != PLAYCHAIN_NULL_TABLE );
    }
}
}}
