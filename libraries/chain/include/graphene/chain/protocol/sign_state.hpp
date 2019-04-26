#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/address.hpp>
#include <graphene/chain/protocol/authority.hpp>

namespace graphene { namespace chain {

const flat_set<public_key_type> empty_keyset;

struct sign_state
{
      /** returns true if we have a signature for this key or can
       * produce a signature for this key, else returns false.
       */
      bool signed_by( const public_key_type& k );

      optional<map<address,public_key_type>> available_address_sigs;
      optional<map<address,public_key_type>> provided_address_sigs;

      bool signed_by( const address& a );

      bool check_authority( account_id_type id );

      /**
       *  Checks to see if we have signatures of the active authorites of
       *  the accounts specified in authority or the keys specified.
       */
      bool check_authority( const authority* au, uint32_t depth = 0 );

      bool remove_unused_signatures();

      sign_state( const flat_set<public_key_type>& sigs,
                  const std::function<const authority*(account_id_type)>& a,
                  const flat_set<public_key_type>& keys = empty_keyset );

      const std::function<const authority*(account_id_type)>& get_active;
      const flat_set<public_key_type>&                        available_keys;

      flat_map<public_key_type,bool>   provided_signatures;
      flat_set<account_id_type>        approved_by;
      uint32_t                         max_recursion = GRAPHENE_MAX_SIG_CHECK_DEPTH;
};

}}
