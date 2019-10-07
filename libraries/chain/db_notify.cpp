#include <fc/container/flat.hpp>

#include <graphene/chain/protocol/authority.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/transaction.hpp>

#include <playchain/chain/impacted.hpp>
#include <playchain/chain/schema/objects.hpp>

using namespace fc;
using namespace graphene::chain;

namespace {

using namespace playchain::chain;

static void get_relevant_accounts(const object* obj, flat_set<account_id_type>& accounts)
{
   if( obj->id.space() == protocol_ids )
   {
      switch( (object_type)obj->id.type() )
      {
        case null_object_type:
        case base_object_type:
        case OBJECT_TYPE_COUNT:
           return;
        case account_object_type:{
           accounts.insert( obj->id );
           break;
        } case asset_object_type:{
           const auto& aobj = dynamic_cast<const asset_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->issuer );
           break;
        } case force_settlement_object_type:{
           const auto& aobj = dynamic_cast<const force_settlement_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->owner );
           break;
        } case committee_member_object_type:{
           const auto& aobj = dynamic_cast<const committee_member_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->committee_member_account );
           break;
        } case witness_object_type:{
           const auto& aobj = dynamic_cast<const witness_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->witness_account );
           break;
        } case limit_order_object_type:{
           const auto& aobj = dynamic_cast<const limit_order_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->seller );
           break;
        } case call_order_object_type:{
           const auto& aobj = dynamic_cast<const call_order_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->borrower );
           break;
        } case custom_object_type:{
          break;
        } case proposal_object_type:{
           const auto& aobj = dynamic_cast<const proposal_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           transaction_get_impacted_accounts( aobj->proposed_transaction, accounts );
           break;
        } case operation_history_object_type:{
           const auto& aobj = dynamic_cast<const operation_history_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           operation_get_impacted_accounts( aobj->op, accounts );
           break;
        } case withdraw_permission_object_type:{
           const auto& aobj = dynamic_cast<const withdraw_permission_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->withdraw_from_account );
           accounts.insert( aobj->authorized_account );
           break;
        } case vesting_balance_object_type:{
           const auto& aobj = dynamic_cast<const vesting_balance_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->owner );
           break;
        } case worker_object_type:{
           const auto& aobj = dynamic_cast<const worker_object*>(obj);
           FC_ASSERT( aobj != nullptr );
           accounts.insert( aobj->worker_account );
           break;
        } case balance_object_type:{
           /** these are free from any accounts */
           break;
        } case htlc_object_type:{
              const auto& htlc_obj = dynamic_cast<const htlc_object*>(obj);
              FC_ASSERT( htlc_obj != nullptr );
              accounts.insert( htlc_obj->transfer.from );
              accounts.insert( htlc_obj->transfer.to );
              break;
        }
      }
   }
   else if( obj->id.space() == implementation_ids )
   {
      switch( (impl_object_type)obj->id.type() )
      {
             case impl_global_property_object_type:
              break;
             case impl_dynamic_global_property_object_type:
              break;
             case impl_reserved0_object_type:
              break;
             case impl_asset_dynamic_data_type:
              break;
             case impl_asset_bitasset_data_type:
              break;
             case impl_account_balance_object_type:{
              const auto& aobj = dynamic_cast<const account_balance_object*>(obj);
              FC_ASSERT( aobj != nullptr );
              accounts.insert( aobj->owner );
              break;
           } case impl_account_statistics_object_type:{
              const auto& aobj = dynamic_cast<const account_statistics_object*>(obj);
              FC_ASSERT( aobj != nullptr );
              accounts.insert( aobj->owner );
              break;
           } case impl_transaction_object_type:{
              const auto& aobj = dynamic_cast<const transaction_object*>(obj);
              FC_ASSERT( aobj != nullptr );
              transaction_get_impacted_accounts( aobj->trx, accounts );
              break;
           } case impl_blinded_balance_object_type:{
              const auto& aobj = dynamic_cast<const blinded_balance_object*>(obj);
              FC_ASSERT( aobj != nullptr );
              for( const auto& a : aobj->owner.account_auths )
                accounts.insert( a.first );
              break;
           } case impl_block_summary_object_type:
              break;
             case impl_account_transaction_history_object_type: {
              const auto& aobj = dynamic_cast<const account_transaction_history_object*>(obj);
              FC_ASSERT( aobj != nullptr );
              accounts.insert( aobj->account );
              break;
           } case impl_chain_property_object_type:
              break;
             case impl_witness_schedule_object_type:
              break;
             case impl_budget_record_object_type:
              break;
             case impl_special_authority_object_type:
              break;
             case impl_buyback_object_type:
              break;
             case impl_fba_accumulator_object_type:
              break;
             case impl_collateral_bid_object_type:{
              const auto& aobj = dynamic_cast<const collateral_bid_object*>(obj);
              FC_ASSERT( aobj != nullptr );
              accounts.insert( aobj->bidder );
              break;
           }
      }
   }
    else if (obj->id.space() == implementation_for_playchain_ids)
    {
        switch((playchain_impl_object_type)obj->id.type())
        {
        case impl_player_invitation_object_type:
        {
            const auto& pobj = dynamic_cast<const player_invitation_object*>(obj);
            assert(pobj != nullptr);
            accounts.insert(pobj->inviter);
            break;
        }
        case impl_player_object_type:
        {
            const auto& pobj = dynamic_cast<const player_object*>(obj);
            assert(pobj != nullptr);
            accounts.insert(pobj->account);
            break;
        }
        case impl_room_object_type:
        {
            const auto& pobj = dynamic_cast<const room_object*>(obj);
            assert(pobj != nullptr);
            accounts.insert(pobj->owner);
            break;
        }
        case impl_game_witness_object_type:
        {
            const auto& pobj = dynamic_cast<const game_witness_object*>(obj);
            assert(pobj != nullptr);
            accounts.insert(pobj->account);
            break;
        }
        case impl_table_object_type:
            break;
        case impl_pending_buy_out_object_type:
            break;
        case impl_table_voting_object_type:
            break;
        case impl_pending_buy_in_object_type:
            break;
        case impl_playchain_property_object_type:
            break;
        case impl_buy_in_object_type:
            break;
        case impl_pending_table_vote_object_type:
            break;
        case impl_playchain_committee_member_object_type:
            break;
        case impl_table_alive_object_type:
            break;
        case impl_room_rating_kpi_measurement_object_type:
            break;
        case impl_room_rating_standby_measurement_object_type:
            break;
        case impl_dynamic_playchain_property_object_type:
            break;
        }
    }
} // end get_relevant_accounts( const object* obj, flat_set<account_id_type>& accounts )
}

namespace graphene { namespace chain {

void database::notify_applied_block( const signed_block& block )
{
   GRAPHENE_TRY_NOTIFY( applied_block, block )
}

void database::notify_on_pending_transaction( const signed_transaction& tx )
{
   GRAPHENE_TRY_NOTIFY( on_pending_transaction, tx )
}

void database::notify_changed_objects()
{ try {
   if( _undo_db.enabled() ) 
   {
      const auto& head_undo = _undo_db.head();

      // New
      if( !new_objects.empty() )
      {
        vector<object_id_type> new_ids;  new_ids.reserve(head_undo.new_ids.size());
        flat_set<account_id_type> new_accounts_impacted;
        for( const auto& item : head_undo.new_ids )
        {
          new_ids.push_back(item);
          auto obj = find_object(item);
          if(obj != nullptr)
            get_relevant_accounts(obj, new_accounts_impacted);
        }

        if( new_ids.size() )
           GRAPHENE_TRY_NOTIFY( new_objects, new_ids, new_accounts_impacted)
      }

      // Changed
      if( !changed_objects.empty() )
      {
        vector<object_id_type> changed_ids;  changed_ids.reserve(head_undo.old_values.size());
        flat_set<account_id_type> changed_accounts_impacted;
        for( const auto& item : head_undo.old_values )
        {
          changed_ids.push_back(item.first);
          get_relevant_accounts(item.second.get(), changed_accounts_impacted);
        }

        if( changed_ids.size() )
           GRAPHENE_TRY_NOTIFY( changed_objects, changed_ids, changed_accounts_impacted)
      }

      // Removed
      if( !removed_objects.empty() )
      {
        vector<object_id_type> removed_ids; removed_ids.reserve( head_undo.removed.size() );
        vector<const object*> removed; removed.reserve( head_undo.removed.size() );
        flat_set<account_id_type> removed_accounts_impacted;
        for( const auto& item : head_undo.removed )
        {
          removed_ids.emplace_back( item.first );
          auto obj = item.second.get();
          removed.emplace_back( obj );
          get_relevant_accounts(obj, removed_accounts_impacted);
        }

        if( removed_ids.size() )
           GRAPHENE_TRY_NOTIFY( removed_objects, removed_ids, removed, removed_accounts_impacted)
      }
   }
} FC_CAPTURE_AND_LOG( (0) ) }

} }
