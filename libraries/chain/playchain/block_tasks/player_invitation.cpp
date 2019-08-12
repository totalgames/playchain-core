/*
 * Copyright (c) 2018 Total Games LLC and contributors.
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

#include "player_invitation.hpp"

#include <graphene/chain/database.hpp>

#include <playchain/chain/schema/player_invitation_object.hpp>

namespace playchain { namespace chain {

void update_expired_invitations(database &d)
{
    auto& invitations_by_expiration= d.get_index_type<player_invitation_index>().indices().get<by_playchain_obj_expiration>();
    while( !invitations_by_expiration.empty() && invitations_by_expiration.begin()->expiration <= d.head_block_time() )
    {
        const player_invitation_object &invitation = *invitations_by_expiration.begin();
        d.push_applied_operation( player_invitation_expire_operation{ invitation.inviter, invitation.uid } );
        d.remove(invitation);
    }
}

} }
