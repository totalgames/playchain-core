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

#include "committee_applying.hpp"

#include <graphene/chain/database.hpp>
#include <graphene/chain/db_with.hpp>
#include <graphene/chain/chain_property_object.hpp>

#include <playchain/chain/schema/playchain_committee_member_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>

#include <boost/multiprecision/integer.hpp>

#include <graphene/chain/vote_count.hpp>

#include <algorithm>

namespace playchain { namespace chain {

playchain_committee_applying_database_impl::playchain_committee_applying_database_impl(database &d_): d(d_)
{
}

void playchain_committee_applying_database_impl::update_active_members()
{
    try {
       assert( d._committee_count_histogram_buffer.size() > 0 );
       share_type stake_target = (d._total_voting_stake-d._committee_count_histogram_buffer[0]) / 2;

       /// accounts that vote for 0 or 1 witness do not get to express an opinion on
       /// the number of witnesses to have (they abstain and are non-voting accounts)
       uint64_t stake_tally = 0; // _committee_count_histogram_buffer[0];
       size_t committee_member_count = 0;
       if( stake_target > 0 )
          while( (committee_member_count < d._committee_count_histogram_buffer.size() - 1)
                 && (stake_tally <= stake_target) )
             stake_tally += d._committee_count_histogram_buffer[++committee_member_count];

       const chain_property_object& cpo = d.get_chain_properties();
       auto committee_members = sort_votable_objects<playchain_committee_member_index>(std::max(committee_member_count*2+1,
                                                                                                (size_t)cpo.immutable_parameters.min_committee_member_count));

       for( const playchain_committee_member_object& del : committee_members )
       {
          d.modify( del, [&]( playchain_committee_member_object& obj ){
                  obj.total_votes = d._vote_tally_buffer[del.vote_id];
                  });
       }

       // Update committee authorities
       if( !committee_members.empty() )
       {
          d.modify(d.get(PLAYCHAIN_COMMITTEE_ACCOUNT), [&](account_object& a)
          {
            vote_counter vc;
            for( const playchain_committee_member_object& cm : committee_members )
               vc.add( cm.committee_member_game_witness(d).account, d._vote_tally_buffer[cm.vote_id] );
            vc.finish( a.active );
          } );
       }

       d.modify(get_playchain_properties(d), [&](playchain_property_object& gp) {
          gp.active_games_committee_members.clear();
          std::transform(committee_members.begin(), committee_members.end(),
                         std::inserter(gp.active_games_committee_members, gp.active_games_committee_members.begin()),
                         [](const playchain_committee_member_object& d)
                         {
                            return d.committee_member_game_witness;
                         });
       });
    } FC_CAPTURE_AND_RETHROW()
}

void playchain_committee_applying_database_impl::update_playchain_parameters()
{
    d.modify(get_playchain_properties(d), [](playchain_property_object& p) {
       if( p.pending_parameters )
       {
          p.parameters = std::move(*p.pending_parameters);
          p.pending_parameters.reset();
       }
    });
}

template<class Index>
vector<std::reference_wrapper<const typename Index::object_type>> playchain_committee_applying_database_impl::sort_votable_objects(size_t count) const
{
   using ObjectType = typename Index::object_type;
   const auto& all_objects = d.get_index_type<Index>().indices();
   count = std::min(count, all_objects.size());
   vector<std::reference_wrapper<const ObjectType>> refs;
   refs.reserve(all_objects.size());
   std::transform(all_objects.begin(), all_objects.end(),
                  std::back_inserter(refs),
                  [](const ObjectType& o) { return std::cref(o); });
   std::partial_sort(refs.begin(), refs.begin() + count, refs.end(),
                   [&](const ObjectType& a, const ObjectType& b)->bool {
      share_type oa_vote = d._vote_tally_buffer[a.vote_id];
      share_type ob_vote = d._vote_tally_buffer[b.vote_id];
      if( oa_vote != ob_vote )
         return oa_vote > ob_vote;
      return a.vote_id < b.vote_id;
   });

   refs.resize(count, refs.front());
   return refs;
}

}}
