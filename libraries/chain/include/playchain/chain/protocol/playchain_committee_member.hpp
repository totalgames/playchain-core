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
#include <playchain/chain/schema/playchain_parameters.hpp>

namespace playchain { namespace chain {

   using namespace graphene::chain;

   struct playchain_committee_member_create_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 50 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset                                 fee;

      account_id_type                       committee_member_account;
      string                                url;

      account_id_type fee_payer()const { return committee_member_account; }
      void            validate()const;
   };

   struct playchain_committee_member_update_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset                                 fee;

      playchain_committee_member_id_type    committee_member;

      account_id_type                       committee_member_account;
      optional< string >                    new_url;

      account_id_type fee_payer()const { return committee_member_account; }
      void            validate()const;
   };

   struct playchain_committee_member_update_parameters_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset                 fee;
      playchain_parameters_v1  new_parameters;

      account_id_type fee_payer()const { return PLAYCHAIN_COMMITTEE_ACCOUNT; }
      void            validate()const;
   };

   struct playchain_committee_member_update_parameters_v2_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset                 fee;
      playchain_parameters_v2  new_parameters;

      account_id_type fee_payer()const { return PLAYCHAIN_COMMITTEE_ACCOUNT; }
      void            validate()const;
   };

} } // playchain::chain
FC_REFLECT( playchain::chain::playchain_committee_member_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( playchain::chain::playchain_committee_member_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( playchain::chain::playchain_committee_member_update_parameters_operation::fee_parameters_type, (fee) )
FC_REFLECT( playchain::chain::playchain_committee_member_update_parameters_v2_operation::fee_parameters_type, (fee) )

FC_REFLECT( playchain::chain::playchain_committee_member_create_operation,
            (fee)(committee_member_account)(url) )
FC_REFLECT( playchain::chain::playchain_committee_member_update_operation,
            (fee)(committee_member)(committee_member_account)(new_url) )
FC_REFLECT( playchain::chain::playchain_committee_member_update_parameters_operation, (fee)(new_parameters) );
FC_REFLECT( playchain::chain::playchain_committee_member_update_parameters_v2_operation, (fee)(new_parameters) );
