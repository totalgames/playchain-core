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
#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/account.hpp>
#include <graphene/chain/protocol/assert.hpp>
#include <graphene/chain/protocol/asset_ops.hpp>
#include <graphene/chain/protocol/balance.hpp>
#include <graphene/chain/protocol/custom.hpp>
#include <graphene/chain/protocol/committee_member.hpp>
#include <graphene/chain/protocol/confidential.hpp>
#include <graphene/chain/protocol/fba.hpp>
#include <graphene/chain/protocol/market.hpp>
#include <graphene/chain/protocol/proposal.hpp>
#include <graphene/chain/protocol/transfer.hpp>
#include <graphene/chain/protocol/vesting.hpp>
#include <graphene/chain/protocol/withdraw_permission.hpp>
#include <graphene/chain/protocol/witness.hpp>
#include <graphene/chain/protocol/worker.hpp>

#include <playchain/chain/protocol/compatibility_with_core.hpp>
#include <playchain/chain/protocol/buy_in_buy_out_operations.hpp>
#include <playchain/chain/protocol/game_operations.hpp>
#include <playchain/chain/protocol/table_operations.hpp>
#include <playchain/chain/protocol/player_invitation_operations.hpp>
#include <playchain/chain/protocol/player_operations.hpp>
#include <playchain/chain/protocol/room_operations.hpp>
#include <playchain/chain/protocol/game_event_operations.hpp>
#include <playchain/chain/protocol/game_witness_operations.hpp>
#include <playchain/chain/protocol/common_operations.hpp>
#include <playchain/chain/protocol/playchain_committee_member.hpp>

namespace graphene { namespace chain {

    using namespace playchain::chain;

   /**
    * @ingroup operations
    *
    * Defines the set of valid operations as a discriminated union type.
    */
   typedef fc::static_variant<
            transfer_operation, // <- 0
            limit_order_create_operation,
            limit_order_cancel_operation,
            call_order_update_operation,
            fill_order_operation,                               // VIRTUAL
            account_create_operation,
            account_update_operation,
            account_whitelist_operation,
            account_upgrade_operation,
            account_transfer_operation,
            asset_create_operation,
            asset_update_operation,
            asset_update_bitasset_operation,
            asset_update_feed_producers_operation,
            asset_issue_operation,
            asset_reserve_operation,
            asset_fund_fee_pool_operation,
            asset_settle_operation,
            asset_global_settle_operation,
            asset_publish_feed_operation,
            witness_create_operation,
            witness_update_operation,
            proposal_create_operation,
            proposal_update_operation,
            proposal_delete_operation,
            withdraw_permission_create_operation,
            withdraw_permission_update_operation,
            withdraw_permission_claim_operation,
            withdraw_permission_delete_operation,
            committee_member_create_operation,
            committee_member_update_operation,
            committee_member_update_global_parameters_operation,
            vesting_balance_create_operation,
            vesting_balance_withdraw_operation,
            worker_create_operation,
            custom_operation,
            assert_operation,
            balance_claim_operation,
            override_transfer_operation,
            transfer_to_blind_operation,
            blind_transfer_operation,
            transfer_from_blind_operation,
            asset_settle_cancel_operation,                          // VIRTUAL
            asset_claim_fees_operation,
            fba_distribute_operation,                               // VIRTUAL
            bid_collateral_operation,
            execute_bid_operation,                                  // VIRTUAL <- 46
            bitshares_dump_operation_1,
            bitshares_dump_operation_2,
            bitshares_dump_operation_3,
            bitshares_dump_operation_4,
            bitshares_dump_operation_5,
            bitshares_dump_operation_6,
            buy_in_operation, // <- 53 - !!! playchain operations start from here !!!
            buy_out_operation,
            game_operation,
            player_invitation_create_operation, // <- 56
            player_invitation_resolve_operation,
            player_invitation_cancel_operation,
            player_invitation_expire_operation,                     // VIRTUAL
            player_create_operation,                                // VIRTUAL
            game_witness_create_operation,                          // VIRTUAL
            room_create_operation, // <- 62
            room_update_operation,
            table_create_operation,
            table_update_operation,
            buy_in_table_operation,
            buy_out_table_operation,
            game_start_playing_check_operation, // <- 68
            game_result_check_operation, // <- 69
            game_event_operation,                                   // VIRTUAL
            player_create_by_room_owner_operation,
            game_reset_operation,
            buy_in_reserve_operation,
            buy_in_reserving_cancel_operation,
            buy_in_reserving_resolve_operation,
            buy_in_reserving_allocated_table_operation,             // VIRTUAL
            buy_in_reserving_expire_operation,                      // VIRTUAL
            donate_to_playchain_operation,
            buy_in_reserving_cancel_all_operation,
            buy_in_expire_operation,                                // VIRTUAL
            playchain_committee_member_create_operation,
            playchain_committee_member_update_operation,
            playchain_committee_member_update_parameters_operation
         > operation;

   /// @} // operations group

   /**
    *  Appends required authorites to the result vector.  The authorities appended are not the
    *  same as those returned by get_required_auth
    *
    *  @return a set of required authorities for @ref op
    */
   void operation_get_required_authorities( const operation& op,
                                            flat_set<account_id_type>& active,
                                            flat_set<account_id_type>& owner,
                                            vector<authority>&  other );

   void operation_validate( const operation& op );

   /**
    *  @brief necessary to support nested operations inside the proposal_create_operation
    */
   struct op_wrapper
   {
      public:
         op_wrapper(const operation& op = operation()):op(op){}
         operation op;
   };

} } // graphene::chain

FC_REFLECT_TYPENAME( graphene::chain::operation )
FC_REFLECT( graphene::chain::op_wrapper, (op) )
