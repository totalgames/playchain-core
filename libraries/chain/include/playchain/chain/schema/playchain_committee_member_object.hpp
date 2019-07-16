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
#include <graphene/db/generic_index.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>

#include <playchain/chain/schema/game_witness_object.hpp>

#include <graphene/chain/protocol/vote.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;
    using namespace graphene::db;

   class playchain_committee_member_object : public graphene::db::abstract_object<playchain_committee_member_object>
   {
      public:
         static constexpr uint8_t space_id = implementation_for_playchain_ids;
         static constexpr uint8_t type_id  = impl_playchain_committee_member_object_type;

         game_witness_id_type  committee_member_game_witness;
         vote_id_type          vote_id;
         uint64_t              total_votes = 0;
         string                url;
   };

   struct by_game_witness;
   struct by_vote_id;

   /**
    * @ingroup object_index
    */
   using playchain_committee_member_multi_index_type = multi_index_container<
      playchain_committee_member_object,
      indexed_by<
         ordered_unique< tag<by_id>,
            member<object, object_id_type, &object::id>
         >,
         ordered_unique< tag<by_game_witness>,
            member<playchain_committee_member_object, game_witness_id_type, &playchain_committee_member_object::committee_member_game_witness>
         >,
         ordered_unique< tag<by_vote_id>,
            member<playchain_committee_member_object, vote_id_type, &playchain_committee_member_object::vote_id>
         >
      >
   >;
   using playchain_committee_member_index = generic_index<playchain_committee_member_object, playchain_committee_member_multi_index_type>;
} } // playchain::chain

FC_REFLECT_DERIVED( playchain::chain::playchain_committee_member_object, (graphene::db::object),
                    (committee_member_game_witness)
                    (vote_id)
                    (total_votes)
                    (url))
