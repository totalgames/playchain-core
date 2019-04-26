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

#include <playchain/chain/protocol/player_invitation_operations.hpp>
#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/account_object.hpp>

namespace playchain { namespace chain {

    void player_invitation_create_operation::validate() const
    {
        FC_ASSERT( !account_object::is_special_account(inviter) );
        FC_ASSERT(!uid.empty());
        auto delta =fc::seconds(lifetime_in_sec);
        FC_ASSERT(delta >= PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD);
        FC_ASSERT(delta <= PLAYCHAIN_MAXIMUM_INVITATION_EXPIRATION_PERIOD);
    }

    void player_invitation_resolve_operation::validate() const
    {
        FC_ASSERT( !account_object::is_special_account(inviter) );
        FC_ASSERT(!uid.empty());
        FC_ASSERT(!name.empty());
    }

    void player_invitation_cancel_operation::validate() const
    {
        FC_ASSERT( !account_object::is_special_account(inviter) );
        FC_ASSERT(!uid.empty());
    }

}}
