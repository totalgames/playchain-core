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

    struct player_create_operation : public base_operation
    {
        struct fee_parameters_type {};

        player_create_operation(){}
        player_create_operation( const account_id_type &account, const account_id_type &inviter )
           :account(account), inviter(inviter) {}

        asset                     fee; // always zero
        account_id_type           account;
        account_id_type           inviter;

        account_id_type fee_payer() const { return GRAPHENE_TEMP_ACCOUNT; }
        void            validate() const { FC_ASSERT( !"virtual operation" ); }
        share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };

    struct player_create_by_room_owner_operation : public base_operation
    {
       struct fee_parameters_type {
         int64_t fee       = GRAPHENE_BLOCKCHAIN_PRECISION;
       };

       asset                     fee;
       account_id_type           account;
       account_id_type           room_owner;

       void get_required_active_authorities( flat_set<account_id_type>& a) const
       {
           a.insert(fee_payer());
       }

       account_id_type   fee_payer() const { return room_owner; }
       void              validate() const;
    };
}}

FC_REFLECT( playchain::chain::player_create_operation::fee_parameters_type, )
FC_REFLECT( playchain::chain::player_create_by_room_owner_operation::fee_parameters_type, (fee))

FC_REFLECT( playchain::chain::player_create_operation, (fee)(account)(inviter))
FC_REFLECT( playchain::chain::player_create_by_room_owner_operation, (fee)(account)(room_owner))
