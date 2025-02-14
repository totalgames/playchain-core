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

    struct table_create_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 100 * GRAPHENE_BLOCKCHAIN_PRECISION;
            uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        asset                                       fee;
        account_id_type                             owner;
        room_id_type                                room;
        string                                      metadata;
        amount_type                                 required_witnesses = 0u;
        asset                                       min_accepted_proposal_asset;

        account_id_type   fee_payer()const { return owner; }
        void              validate()const;
        share_type        calculate_fee(const fee_parameters_type& k) const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };

    struct table_update_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION;
            uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        asset                                       fee;
        account_id_type                             owner;
        table_id_type                               table;
        string                                      metadata;
        amount_type                                 required_witnesses = 0u;
        asset                                       min_accepted_proposal_asset;

        account_id_type   fee_payer()const { return owner; }
        void              validate()const;
        share_type        calculate_fee(const fee_parameters_type& k) const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };

    struct tables_alive_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 0;
        };

        asset                                       fee;
        account_id_type                             owner;
        flat_set<table_id_type>                     tables;

        account_id_type   fee_payer()const { return owner; }
        void              validate()const;
    };
}}

FC_REFLECT( playchain::chain::table_create_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::table_update_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::tables_alive_operation::fee_parameters_type, (fee))

FC_REFLECT( playchain::chain::table_create_operation, (fee)(owner)(room)(metadata)(required_witnesses)(min_accepted_proposal_asset))
FC_REFLECT( playchain::chain::table_update_operation, (fee)(owner)(table)(metadata)(required_witnesses)(min_accepted_proposal_asset))
FC_REFLECT( playchain::chain::tables_alive_operation, (fee)(owner)(tables))
