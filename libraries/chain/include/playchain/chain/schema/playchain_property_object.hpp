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

#include <graphene/db/object.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <graphene/db/generic_index.hpp>

#include <playchain/chain/schema/playchain_parameters.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

   class playchain_property_object : public graphene::db::abstract_object<playchain_property_object>
   {
      public:
         static constexpr uint8_t space_id = implementation_for_playchain_ids;
         static constexpr uint8_t type_id  = impl_playchain_property_object_type;

         playchain_parameters           parameters;
         optional<playchain_parameters> pending_parameters;

         vector<game_witness_id_type>   active_games_committee_members;

         static const playchain_committee_member_object &get_committee_member(const database &, const game_witness_id_type &);
   };

   class dynamic_playchain_property_object : public abstract_object<dynamic_playchain_property_object>
   {
     public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_dynamic_playchain_property_object_type;

        int64_t average_room_rating = 0;
   };
}}

FC_REFLECT_DERIVED( playchain::chain::playchain_property_object, (graphene::db::object),
                    (parameters)
                    (pending_parameters)
                    (active_games_committee_members)
                  )

FC_REFLECT_DERIVED( playchain::chain::dynamic_playchain_property_object, (graphene::db::object),
                    (average_room_rating)
                  )
