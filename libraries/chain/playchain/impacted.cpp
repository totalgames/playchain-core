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

#include <playchain/chain/impacted.hpp>

#include <graphene/chain/protocol/authority.hpp>

namespace playchain { namespace chain {

using namespace fc;
using namespace graphene::chain;

struct get_impacted_account_visitor
{
   flat_set<account_id_type>& _impacted;
   get_impacted_account_visitor( flat_set<account_id_type>& impact ):_impacted(impact) {}
   typedef void result_type;

   void operator()( const transfer_operation& op )
   {
      _impacted.insert( op.to );
      _impacted.insert( op.fee_payer() ); // from
   }
   void operator()( const asset_claim_fees_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
   }
   void operator()( const asset_claim_pool_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
   }
   void operator()( const limit_order_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // seller
   }
   void operator()( const limit_order_cancel_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // fee_paying_account
   }
   void operator()( const call_order_update_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // funding_account
   }
   void operator()( const bid_collateral_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // bidder
   }
   void operator()( const fill_order_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account_id
   }
   void operator()( const execute_bid_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // bidder
   }
   void operator()( const account_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // registrar
      _impacted.insert( op.referrer );
      add_authority_accounts( _impacted, op.owner );
      add_authority_accounts( _impacted, op.active );
   }
   void operator()( const account_update_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account
      if( op.owner )
         add_authority_accounts( _impacted, *(op.owner) );
      if( op.active )
         add_authority_accounts( _impacted, *(op.active) );
   }
   void operator()( const account_whitelist_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // authorizing_account
      _impacted.insert( op.account_to_list );
   }
   void operator()( const account_upgrade_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account_to_upgrade
   }
   void operator()( const account_transfer_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account_id
   }
   void operator()( const asset_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
   }
   void operator()( const asset_update_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
      if( op.new_issuer )
         _impacted.insert( *(op.new_issuer) );
   }
   void operator()( const asset_update_issuer_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
      _impacted.insert( op.new_issuer );
   }
   void operator()( const asset_update_bitasset_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
   }
   void operator()( const asset_update_feed_producers_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
   }
   void operator()( const asset_issue_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
      _impacted.insert( op.issue_to_account );
   }
   void operator()( const asset_reserve_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // payer
   }
   void operator()( const asset_fund_fee_pool_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // from_account
   }
   void operator()( const asset_settle_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account
   }
   void operator()( const asset_global_settle_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // issuer
   }
   void operator()( const asset_publish_feed_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // publisher
   }
   void operator()( const witness_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // witness_account
   }
   void operator()( const witness_update_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // witness_account
   }
   void operator()( const proposal_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // fee_paying_account
      vector<authority> other;
      for( const auto& proposed_op : op.proposed_ops )
         operation_get_required_authorities( proposed_op.op, _impacted, _impacted, other );
      for( auto& o : other )
         add_authority_accounts( _impacted, o );
   }
   void operator()( const proposal_update_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // fee_paying_account
   }
   void operator()( const proposal_delete_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // fee_paying_account
   }
   void operator()( const withdraw_permission_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // withdraw_from_account
      _impacted.insert( op.authorized_account );
   }
   void operator()( const withdraw_permission_update_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // withdraw_from_account
      _impacted.insert( op.authorized_account );
   }
   void operator()( const withdraw_permission_claim_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // withdraw_to_account
      _impacted.insert( op.withdraw_from_account );
   }
   void operator()( const withdraw_permission_delete_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // withdraw_from_account
      _impacted.insert( op.authorized_account );
   }
   void operator()( const committee_member_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // committee_member_account
   }
   void operator()( const committee_member_update_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // committee_member_account
   }
   void operator()( const committee_member_update_global_parameters_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account_id_type()
   }
   void operator()( const vesting_balance_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // creator
      _impacted.insert( op.owner );
   }
   void operator()( const vesting_balance_withdraw_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // owner
   }
   void operator()( const worker_create_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // owner
   }
   void operator()( const custom_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // payer
   }
   void operator()( const assert_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // fee_paying_account
   }
   void operator()( const balance_claim_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // deposit_to_account
   }
   void operator()( const override_transfer_operation& op )
   {
      _impacted.insert( op.to );
      _impacted.insert( op.from );
      _impacted.insert( op.fee_payer() ); // issuer
   }
   void operator()( const transfer_to_blind_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // from
      for( const auto& out : op.outputs )
         add_authority_accounts( _impacted, out.owner );
   }
   void operator()( const blind_transfer_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // GRAPHENE_TEMP_ACCOUNT
      for( const auto& in : op.inputs )
         add_authority_accounts( _impacted, in.owner );
      for( const auto& out : op.outputs )
         add_authority_accounts( _impacted, out.owner );
   }
   void operator()( const transfer_from_blind_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // GRAPHENE_TEMP_ACCOUNT
      _impacted.insert( op.to );
      for( const auto& in : op.inputs )
         add_authority_accounts( _impacted, in.owner );
   }
   void operator()( const asset_settle_cancel_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account
   }
   void operator()( const fba_distribute_operation& op )
   {
      _impacted.insert( op.fee_payer() ); // account_id
   }
   void operator()( const htlc_create_operation& op )
   {
      _impacted.insert( op.fee_payer() );
      _impacted.insert( op.to );
   }
   void operator()( const htlc_redeem_operation& op )
   {
      _impacted.insert( op.fee_payer() );
   }
   void operator()( const htlc_redeemed_operation& op )
   {
      _impacted.insert( op.from );
      if ( op.to != op.redeemer )
         _impacted.insert( op.to );
   }
   void operator()( const htlc_extend_operation& op )
   {
      _impacted.insert( op.fee_payer() );
   }
   void operator()( const htlc_refund_operation& op )
   {
      _impacted.insert( op.fee_payer() );
   }

   void operator()( const bitshares_dump_operation_8& ) {}
   void operator()( const bitshares_dump_operation_9& ) {}

   void operator()( const player_invitation_create_operation& op )
   {
      _impacted.insert( op.inviter );
   }
   void operator()( const player_invitation_resolve_operation& op )
   {
      _impacted.insert( op.inviter );
   }
   void operator()( const player_invitation_cancel_operation& op )
   {
      _impacted.insert( op.inviter );
   }
   void operator()( const player_invitation_expire_operation& op )
   {
       _impacted.insert( op.inviter );
   }
   void operator()( const player_create_operation& op)
   {
       _impacted.insert( op.account );
       _impacted.insert( op.inviter );
   }
   void operator()( const game_witness_create_operation& op)
   {
       _impacted.insert( op.new_witness );
   }

   void operator()( const room_create_operation& op )
   {
       _impacted.insert( op.owner );
   }
   void operator()( const room_update_operation& op )
   {
       _impacted.insert( op.owner );
   }

   void operator()(const table_create_operation& op)
   {
       _impacted.insert(op.owner);
   }

   void operator()(const table_update_operation& op)
   {
       _impacted.insert(op.owner);
   }

   void operator()(const buy_in_table_operation& op)
   {
       _impacted.insert(op.player);
       _impacted.insert(op.table_owner);
   }
   void operator()( const buy_out_table_operation& op )
   {
      _impacted.insert( op.player );
      _impacted.insert(op.table_owner);
   }

   void operator()(const game_start_playing_check_operation& op)
   {
       _impacted.insert(op.voter);
       _impacted.insert(op.table_owner);
   }
   void operator()(const game_result_check_operation& op)
   {
       _impacted.insert(op.voter);
       _impacted.insert(op.table_owner);
   }
   void operator()(const game_reset_operation& op)
   {
       _impacted.insert(op.table_owner);
   }
   void operator()(const buy_in_reserve_operation& op)
   {
       _impacted.insert(op.player);
   }
   void operator()(const buy_in_reserving_cancel_operation& op)
   {
       _impacted.insert(op.player);
   }
   void operator()(const buy_in_reserving_expire_operation& op)
   {
       _impacted.insert(op.player);
       if (GRAPHENE_NULL_ACCOUNT != op.table_owner)
       {
           _impacted.insert(op.table_owner);
       }
   }
   void operator()(const buy_in_reserving_allocated_table_operation& op)
   {
       _impacted.insert(op.player);
       _impacted.insert(op.table_owner);
   }
   void operator()(const buy_in_reserving_resolve_operation& op)
   {
       _impacted.insert(op.table_owner);
   }

   void operator()( const game_event_operation& op )
   {
      _impacted.insert(op.table_owner);

      switch(op.event.which())
      {
      case game_event_type::tag<fraud_game_start_playing_check>::value:
          _impacted.insert(op.event.get<fraud_game_start_playing_check>().witness);
          break;
      case game_event_type::tag<fraud_game_result_check>::value:
          _impacted.insert(op.event.get<fraud_game_result_check>().witness);
          break;
      case game_event_type::tag<buy_out_allowed>::value:
          _impacted.insert(op.event.get<buy_out_allowed>().player);
          break;
      case game_event_type::tag<buy_in_return>::value:
          _impacted.insert(op.event.get<buy_in_return>().player);
          break;
      case game_event_type::tag<game_cash_return>::value:
          _impacted.insert(op.event.get<game_cash_return>().player);
          break;
      case game_event_type::tag<fraud_buy_out>::value:
          _impacted.insert(op.event.get<fraud_buy_out>().player);
          break;
      case game_event_type::tag<fail_vote>::value:
          _impacted.insert(op.event.get<fail_vote>().voter);
          break;
      default:;
      }
   }

   void operator()( const player_create_by_room_owner_operation& op)
   {
       _impacted.insert( op.account );
       _impacted.insert( op.room_owner );
   }

   void operator()(const donate_to_playchain_operation& op)
   {
       _impacted.insert(op.donator);
   }

   void operator()(const buy_in_reserving_cancel_all_operation& op)
   {
       _impacted.insert(op.player);
   }

   void operator()(const buy_in_expire_operation& op)
   {
       _impacted.insert(op.table_owner);
       _impacted.insert(op.player);
   }

   void operator()( const playchain_committee_member_create_operation& op )
   {
      _impacted.insert( op.committee_member_account );
   }
   void operator()( const playchain_committee_member_update_operation& op )
   {
      _impacted.insert( op.committee_member_account );
   }
   void operator()( const playchain_committee_member_update_parameters_operation& op ) {}
};

void operation_get_impacted_accounts( const operation& op, flat_set<account_id_type>& result )
{
   get_impacted_account_visitor vtor = get_impacted_account_visitor( result );
   op.visit( vtor );
}

void transaction_get_impacted_accounts( const transaction& tx, flat_set<account_id_type>& result )
{
   for( const auto& op : tx.operations )
      operation_get_impacted_accounts( op, result );
}

} }
