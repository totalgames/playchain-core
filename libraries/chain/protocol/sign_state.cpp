#include <graphene/chain/protocol/sign_state.hpp>

namespace graphene { namespace chain {

bool sign_state::signed_by( const public_key_type& k )
{
   auto itr = provided_signatures.find(k);
   if( itr == provided_signatures.end() )
   {
      auto pk = available_keys.find(k);
      if( pk  != available_keys.end() )
         return provided_signatures[k] = true;
      return false;
   }
   return itr->second = true;
}

bool sign_state::signed_by( const address& a ) {
   if( !available_address_sigs ) {
      available_address_sigs = std::map<address,public_key_type>();
      provided_address_sigs = std::map<address,public_key_type>();
      for( auto& item : available_keys ) {
       (*available_address_sigs)[ address(pts_address(item, false, 56) ) ] = item;
       (*available_address_sigs)[ address(pts_address(item, true, 56) ) ] = item;
       (*available_address_sigs)[ address(pts_address(item, false, 0) ) ] = item;
       (*available_address_sigs)[ address(pts_address(item, true, 0) ) ] = item;
       (*available_address_sigs)[ address(item) ] = item;
      }
      for( auto& item : provided_signatures ) {
       (*provided_address_sigs)[ address(pts_address(item.first, false, 56) ) ] = item.first;
       (*provided_address_sigs)[ address(pts_address(item.first, true, 56) ) ] = item.first;
       (*provided_address_sigs)[ address(pts_address(item.first, false, 0) ) ] = item.first;
       (*provided_address_sigs)[ address(pts_address(item.first, true, 0) ) ] = item.first;
       (*provided_address_sigs)[ address(item.first) ] = item.first;
      }
   }
   auto itr = provided_address_sigs->find(a);
   if( itr == provided_address_sigs->end() )
   {
      auto aitr = available_address_sigs->find(a);
      if( aitr != available_address_sigs->end() ) {
         auto pk = available_keys.find(aitr->second);
         if( pk != available_keys.end() )
            return provided_signatures[aitr->second] = true;
         return false;
      }
   }
   return provided_signatures[itr->second] = true;
}

bool sign_state::check_authority( account_id_type id )
{
   if( approved_by.find(id) != approved_by.end() ) return true;
   return check_authority( get_active(id) );
}

bool sign_state::check_authority( const authority* au, uint32_t depth )
{
   if( au == nullptr ) return false;
   const authority& auth = *au;

   uint32_t total_weight = 0;
   for( const auto& k : auth.key_auths )
      if( signed_by( k.first ) )
      {
         total_weight += k.second;
         if( total_weight >= auth.weight_threshold )
            return true;
      }

   for( const auto& k : auth.address_auths )
      if( signed_by( k.first ) )
      {
         total_weight += k.second;
         if( total_weight >= auth.weight_threshold )
            return true;
      }

   for( const auto& a : auth.account_auths )
   {
      if( approved_by.find(a.first) == approved_by.end() )
      {
         if( depth == max_recursion )
            continue;
         if( check_authority( get_active( a.first ), depth+1 ) )
         {
            approved_by.insert( a.first );
            total_weight += a.second;
            if( total_weight >= auth.weight_threshold )
               return true;
         }
      }
      else
      {
         total_weight += a.second;
         if( total_weight >= auth.weight_threshold )
            return true;
      }
   }
   return total_weight >= auth.weight_threshold;
}

bool sign_state::remove_unused_signatures()
{
   vector<public_key_type> remove_sigs;
   for( const auto& sig : provided_signatures )
      if( !sig.second ) remove_sigs.push_back( sig.first );

   for( auto& sig : remove_sigs )
      provided_signatures.erase(sig);

   return remove_sigs.size() != 0;
}

sign_state::sign_state( const flat_set<public_key_type>& sigs,
            const std::function<const authority*(account_id_type)>& a,
            const flat_set<public_key_type>& keys )
:get_active(a),available_keys(keys)
{
   for( const auto& key : sigs )
      provided_signatures[ key ] = false;
   approved_by.insert( GRAPHENE_TEMP_ACCOUNT  );
}
}}
