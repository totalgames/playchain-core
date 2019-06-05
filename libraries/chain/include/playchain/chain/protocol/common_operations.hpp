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

    struct donate_to_playchain_operation : public base_operation
    {
        struct fee_parameters_type
        {
            uint64_t fee = 0; //set minimum limit for donate
        };

        asset                     fee; ///< donate amount
        account_id_type           donator;

        void get_required_active_authorities(flat_set<account_id_type>& a) const {
            a.insert(donator);
        }

        account_id_type   fee_payer()const { return donator; }
        void              validate()const;
    };

    enum class playchain_deposit_type: uint16_t
    {
        unknown = 0,
        inviter,
        room,
        witness
    };

    struct playchain_deposit_cashback_operation : public base_operation
    {
        struct fee_parameters_type {};

        playchain_deposit_cashback_operation(){}
        playchain_deposit_cashback_operation(
                             const playchain_deposit_type deposit,
                             const account_id_type &getter,
                             const account_id_type &recipient,
                             const asset &amount,
                             const string &metadata)
           : deposit((uint16_t)deposit),
             getter(getter),
             recipient(recipient),
             amount(amount),
             metadata(metadata)
        {}

        asset                     fee; // always zero
        uint16_t                  deposit;
        account_id_type           getter;
        account_id_type           recipient;
        asset                     amount;
        string                    metadata;

        account_id_type fee_payer()const { return GRAPHENE_TEMP_ACCOUNT; }
        void            validate()const { FC_ASSERT( !"virtual operation" ); }
        share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
    };
}}

FC_REFLECT( playchain::chain::donate_to_playchain_operation::fee_parameters_type, (fee))
FC_REFLECT( playchain::chain::playchain_deposit_cashback_operation::fee_parameters_type, )

FC_REFLECT( playchain::chain::donate_to_playchain_operation, (fee)(donator))
FC_REFLECT( playchain::chain::playchain_deposit_cashback_operation, (fee)(deposit)(getter)(recipient)(amount)(metadata))
