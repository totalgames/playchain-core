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
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/block.hpp>
#include <fc/io/raw.hpp>
#include <fc/bitutil.hpp>
#include <algorithm>
#include <limits>

#include <graphene/chain/protocol/sign_state.hpp>

#include <playchain/chain/playchain_config.hpp>

namespace graphene { namespace chain {

digest_type processed_transaction::merkle_digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::digest()const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, *this );
   return enc.result();
}

digest_type transaction::sig_digest( const chain_id_type& chain_id )const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return enc.result();
}

void transaction::validate() const
{
   FC_ASSERT( operations.size() > 0, "A transaction must have at least one operation", ("trx",*this) );
   for( const auto& op : operations )
      operation_validate(op);
}

uint64_t transaction::get_packed_size() const
{
   return fc::raw::pack_size(*this);
}

const transaction_id_type& transaction::id() const
{
   auto h = digest();
   memcpy(_tx_id_buffer._hash, h._hash, std::min(sizeof(_tx_id_buffer), sizeof(h)));
   return _tx_id_buffer;
}

const signature_type& graphene::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)
{
   digest_type h = sig_digest( chain_id );
   signatures.push_back(key.sign_compact(h));
   return signatures.back();
}

signature_type graphene::chain::signed_transaction::sign(const private_key_type& key, const chain_id_type& chain_id)const
{
   digest_type::encoder enc;
   fc::raw::pack( enc, chain_id );
   fc::raw::pack( enc, *this );
   return key.sign_compact(enc.result());
}

void transaction::set_expiration( fc::time_point_sec expiration_time )
{
    expiration = expiration_time;
}

void transaction::set_reference_block( const block_id_type& reference_block )
{
   ref_block_num = fc::endian_reverse_u32(reference_block._hash[0]);
   ref_block_prefix = reference_block._hash[1];
}

void transaction::get_required_authorities( flat_set<account_id_type>& active,
                                            flat_set<account_id_type>& owner,
                                            vector<authority>& other )const
{
   for( const auto& op : operations )
      operation_get_required_authorities( op, active, owner, other );
   for( const auto& account : owner )
      active.erase( account );
}

void verify_authority( const vector<operation>& ops, const flat_set<public_key_type>& sigs,
                       const std::function<const authority*(account_id_type)>& get_active,
                       const std::function<const authority*(account_id_type)>& get_owner,
                       bool allow_non_immediate_owner,
                       uint32_t max_recursion_depth,
                       bool  allow_committe,
                       const flat_set<account_id_type>& active_aprovals,
                       const flat_set<account_id_type>& owner_approvals )
{ try {
   flat_set<account_id_type> required_active;
   flat_set<account_id_type> required_owner;
   vector<authority> other;

   for( const auto& op : ops )
      operation_get_required_authorities( op, required_active, required_owner, other );

   if( !allow_committe )
   {
      GRAPHENE_ASSERT( required_active.find(GRAPHENE_COMMITTEE_ACCOUNT) == required_active.end(),
                       invalid_committee_approval, "Committee account may only propose transactions" );
      GRAPHENE_ASSERT( required_active.find(PLAYCHAIN_COMMITTEE_ACCOUNT) == required_active.end(),
                       invalid_committee_approval, "Playchain committee account may only propose transactions" );
   }

   sign_state s( sigs, get_active, get_owner, allow_non_immediate_owner, max_recursion_depth );
   for( auto& id : active_aprovals )
      s.approved_by.insert( id );
   for( auto& id : owner_approvals )
      s.approved_by.insert( id );

   for( const auto& auth : other )
   {
      GRAPHENE_ASSERT( s.check_authority(&auth), tx_missing_other_auth, "Missing Authority", ("auth",auth)("sigs",sigs) );
   }

   // fetch all of the top level authorities
   for( auto id : required_owner )
   {
      GRAPHENE_ASSERT( owner_approvals.find(id) != owner_approvals.end() ||
                       s.check_authority(get_owner(id)),
                       tx_missing_owner_auth, "Missing Owner Authority ${id}", ("id",id)("auth",*get_owner(id)) );
   }

   for( auto id : required_active )
   {
      GRAPHENE_ASSERT( s.check_authority(id) ||
                       s.check_authority(get_owner(id)),
                       tx_missing_active_auth, "Missing Active Authority ${id}",
                       ("id",id)("auth",*get_active(id))("owner",*get_owner(id)) );
   }

   GRAPHENE_ASSERT(
      !s.remove_unused_signatures(),
      tx_irrelevant_sig,
      "Unnecessary signature(s) detected"
      );
} FC_CAPTURE_AND_RETHROW( (ops)(sigs) ) }


const flat_set<public_key_type>& signed_transaction::get_signature_keys( const chain_id_type& chain_id )const
{ try {
   auto d = sig_digest( chain_id );
   flat_set<public_key_type> result;
   for( const auto&  sig : signatures )
   {
      GRAPHENE_ASSERT(
         result.insert( fc::ecc::public_key(sig,d) ).second,
            tx_duplicate_sig,
            "Duplicate Signature detected" );
   }
   _signees = std::move( result );
   return _signees;
} FC_CAPTURE_AND_RETHROW() }


set<public_key_type> signed_transaction::get_required_signatures(
   const chain_id_type& chain_id,
   const flat_set<public_key_type>& available_keys,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   bool allow_non_immediate_owner,
   uint32_t max_recursion_depth )const
{
   flat_set<account_id_type> required_active;
   flat_set<account_id_type> required_owner;
   vector<authority> other;
   get_required_authorities( required_active, required_owner, other );

   const flat_set<public_key_type>& signature_keys = get_signature_keys( chain_id );
   sign_state s( signature_keys, get_active, get_owner, allow_non_immediate_owner, max_recursion_depth, available_keys );

   for( const auto& auth : other )
      s.check_authority(&auth);
   for( auto& owner : required_owner )
      s.check_authority( get_owner( owner ) );
   for( auto& active : required_active )
      s.check_authority( active ) || s.check_authority( get_owner( active ) );

   s.remove_unused_signatures();

   set<public_key_type> result;

   for( auto& provided_sig : s.provided_signatures )
      if( available_keys.find( provided_sig.first ) != available_keys.end()
            && signature_keys.find( provided_sig.first ) == signature_keys.end() )
         result.insert( provided_sig.first );

   return result;
}

set<public_key_type> signed_transaction::minimize_required_signatures(
   const chain_id_type& chain_id,
   const flat_set<public_key_type>& available_keys,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   bool allow_non_immediate_owner,
   uint32_t max_recursion
   ) const
{
   set< public_key_type > s = get_required_signatures( chain_id, available_keys, get_active, get_owner, max_recursion );
   flat_set< public_key_type > result( s.begin(), s.end() );

   for( const public_key_type& k : s )
   {
      result.erase( k );
      try
      {
         graphene::chain::verify_authority( operations, result, get_active, get_owner, allow_non_immediate_owner, max_recursion );
         continue;  // element stays erased if verify_authority is ok
      }
      catch( const tx_missing_owner_auth& e ) {}
      catch( const tx_missing_active_auth& e ) {}
      catch( const tx_missing_other_auth& e ) {}
      result.insert( k );
   }
   return set<public_key_type>( result.begin(), result.end() );
}

const transaction_id_type& precomputable_transaction::id()const
{
   if( !_tx_id_buffer._hash[0] )
      transaction::id();
   return _tx_id_buffer;
}

void precomputable_transaction::validate() const
{
   if( _validated ) return;
   transaction::validate();
   _validated = true;
}

uint64_t precomputable_transaction::get_packed_size()const
{
   if( _packed_size == 0 )
      _packed_size = transaction::get_packed_size();
   return _packed_size;
}

const flat_set<public_key_type>& precomputable_transaction::get_signature_keys( const chain_id_type& chain_id )const
{
   // Strictly we should check whether the given chain ID is same as the one used to initialize the `signees` field.
   // However, we don't pass in another chain ID so far, for better performance, we skip the check.
   if( _signees.empty() )
      signed_transaction::get_signature_keys( chain_id );
   return _signees;
}

void signed_transaction::verify_authority(
   const chain_id_type& chain_id,
   const std::function<const authority*(account_id_type)>& get_active,
   const std::function<const authority*(account_id_type)>& get_owner,
   bool allow_non_immediate_owner,
   uint32_t max_recursion )const
{ try {
   graphene::chain::verify_authority( operations,
                                      get_signature_keys( chain_id ),
                                      get_active,
                                      get_owner,
                                      allow_non_immediate_owner,
                                      max_recursion );
} FC_CAPTURE_AND_RETHROW( (*this) ) }

} } // graphene::chain
