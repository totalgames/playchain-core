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

#include <graphene/chain/protocol/base.hpp>
#include <playchain/chain/protocol/playchain_types.hpp>

namespace playchain { namespace chain {

   using namespace graphene::chain;

   struct op_wrapper;

   struct playchain_proposal_create_operation : public base_operation
   {
       struct fee_parameters_type { 
          uint64_t fee            = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
          uint32_t price_per_kbyte = 1000;
       };

       asset              fee;
       account_id_type    fee_paying_account;
       vector<op_wrapper> proposed_ops;
       time_point_sec     expiration_time;
       optional<uint32_t> review_period_seconds;
       extensions_type    extensions;

       account_id_type fee_payer()const { return fee_paying_account; }
       void            validate()const;
       share_type      calculate_fee(const fee_parameters_type& k) const
       {
           return calculate_fee_with_kbyte(this, k);
       }
   };

   struct playchain_proposal_update_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t fee            = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte = 1000;
      };

      asset                      fee;
      account_id_type            fee_paying_account;
      proposal_id_type           proposal;
      flat_set<account_id_type>  active_approvals_to_add;
      flat_set<account_id_type>  active_approvals_to_remove;
      flat_set<account_id_type>  owner_approvals_to_add;
      flat_set<account_id_type>  owner_approvals_to_remove;
      flat_set<public_key_type>  key_approvals_to_add;
      flat_set<public_key_type>  key_approvals_to_remove;
      extensions_type            extensions;

      account_id_type fee_payer()const { return fee_paying_account; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k) const
      {
          return calculate_fee_with_kbyte(this, k);
      }
      void get_required_authorities( vector<authority>& )const;
      void get_required_active_authorities( flat_set<account_id_type>& )const;
      void get_required_owner_authorities( flat_set<account_id_type>& )const;
   };

   struct playchain_proposal_delete_operation : public base_operation
   {
      struct fee_parameters_type {
          uint64_t fee =  0;
      };

      asset             fee;
      account_id_type   fee_paying_account;
      bool              using_owner_authority = false;
      proposal_id_type  proposal;
      extensions_type   extensions;

      account_id_type fee_payer()const { return fee_paying_account; }
      void       validate()const;
   };
   
}} // playchain::chain

FC_REFLECT( playchain::chain::playchain_proposal_create_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( playchain::chain::playchain_proposal_update_operation::fee_parameters_type, (fee)(price_per_kbyte) )
FC_REFLECT( playchain::chain::playchain_proposal_delete_operation::fee_parameters_type, (fee) )

FC_REFLECT( playchain::chain::playchain_proposal_create_operation, (fee)(fee_paying_account)(expiration_time)
            (proposed_ops)(review_period_seconds)(extensions) )
FC_REFLECT( playchain::chain::playchain_proposal_update_operation, (fee)(fee_paying_account)(proposal)
            (active_approvals_to_add)(active_approvals_to_remove)(owner_approvals_to_add)(owner_approvals_to_remove)
            (key_approvals_to_add)(key_approvals_to_remove)(extensions) )
FC_REFLECT( playchain::chain::playchain_proposal_delete_operation, (fee)(fee_paying_account)(using_owner_authority)(proposal)(extensions) )
