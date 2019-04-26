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

#pragma once

#include <graphene/chain/protocol/operations.hpp>
#include <graphene/db/generic_index.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;
    using namespace graphene::db;

    class player_invitation_object : public graphene::db::abstract_object<player_invitation_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_player_invitation_object_type;

        account_id_type                     inviter;
        string                              uid;
        fc::time_point_sec                  created; //< for monitoring only
        fc::time_point_sec                  expiration;
        string                              metadata;

    };

    struct by_player_invitation_uid;

    /**
     * @ingroup object_index
     */
    using player_invitation_multi_index_type =
    multi_index_container<
       player_invitation_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_unique< tag<by_player_invitation_uid>,
       composite_key< player_invitation_object,
           member< player_invitation_object, account_id_type, &player_invitation_object::inviter >,
           member< player_invitation_object, string, &player_invitation_object::uid >
        >>,
          ordered_unique<tag<by_playchain_obj_expiration>,
       composite_key<player_invitation_object,
           member<player_invitation_object, time_point_sec, &player_invitation_object::expiration>,
           member<object, object_id_type, &object::id >
        >>
    >>;

    using player_invitation_index = generic_index<player_invitation_object, player_invitation_multi_index_type>;
}}

FC_REFLECT_DERIVED( playchain::chain::player_invitation_object,
                    (graphene::db::object),
                    (inviter)(uid)(created)(expiration)(metadata) )
