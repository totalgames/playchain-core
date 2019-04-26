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

    struct player_invitation_create_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 2 * GRAPHENE_BLOCKCHAIN_PRECISION;
            uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        asset                     fee;
        account_id_type           inviter;
        string                    uid;
        uint32_t                  lifetime_in_sec;
        string                    metadata;

        //we don't need get_required_active_authorities because 'fee_payer' active authority required

        account_id_type   fee_payer()const { return inviter; }
        void              validate()const;
        share_type        calculate_fee(const fee_parameters_type& k) const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };

    struct player_invitation_resolve_operation : public base_operation
    {
        struct fee_parameters_type {
        };

        asset                     fee; // always zero
        account_id_type           inviter;
        string                    uid;
        signature_type            mandat;
        string                    name;
        authority                 owner;
        authority                 active;

        //this operation does not require any authorities, but check in an unusual way

        account_id_type fee_payer()const { return GRAPHENE_TEMP_ACCOUNT; }
        void            validate()const;
        share_type      calculate_fee(const fee_parameters_type& )const { return 0; }
    };

    struct player_invitation_cancel_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; // 1.000000 PLC
            uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        asset                     fee;
        account_id_type           inviter;
        string                    uid;

        //we don't need get_required_active_authorities because 'fee_payer' active authority required

        account_id_type fee_payer()const { return inviter; }
        void            validate()const;
        share_type      calculate_fee(const fee_parameters_type& k)const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };

    struct player_invitation_expire_operation : public base_operation
    {
       struct fee_parameters_type {};

       player_invitation_expire_operation(){}
       player_invitation_expire_operation( const account_id_type &inviter, const string &uid )
          :inviter(inviter),uid(uid) {}

       asset                     fee; // always zero
       account_id_type           inviter;
       string                    uid;

       account_id_type fee_payer()const { return GRAPHENE_TEMP_ACCOUNT; }
       void            validate()const { FC_ASSERT( !"virtual operation" ); }
       share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };

}}

FC_REFLECT( playchain::chain::player_invitation_create_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::player_invitation_resolve_operation::fee_parameters_type, )
FC_REFLECT( playchain::chain::player_invitation_cancel_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::player_invitation_expire_operation::fee_parameters_type,  ) // VIRTUAL

FC_REFLECT( playchain::chain::player_invitation_create_operation, (fee)(inviter)(uid)(lifetime_in_sec)(metadata))
FC_REFLECT( playchain::chain::player_invitation_resolve_operation, (fee)(inviter)(uid)(mandat)(name)(owner)(active))
FC_REFLECT( playchain::chain::player_invitation_cancel_operation, (fee)(inviter)(uid))
FC_REFLECT( playchain::chain::player_invitation_expire_operation, (fee)(inviter)(uid))
