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

    struct room_create_operation: public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
            uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        asset                     fee;
        account_id_type           owner;
        string                    server_url;
        string                    metadata;
        string                    protocol_version;

        account_id_type   fee_payer() const { return owner; }
        void              validate() const;
        share_type        calculate_fee(const fee_parameters_type& k) const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };

    struct room_update_operation: public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 100 * GRAPHENE_BLOCKCHAIN_PRECISION;
            uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
        };

        asset                     fee;
        account_id_type           owner;
        room_id_type              room;
        string                    server_url;
        string                    metadata;
        string                    protocol_version;

        account_id_type   fee_payer() const { return owner; }
        void              validate() const;
        share_type        calculate_fee(const fee_parameters_type& k) const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };
}}

FC_REFLECT( playchain::chain::room_create_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::room_update_operation::fee_parameters_type, (fee)(price_per_kbyte))

FC_REFLECT( playchain::chain::room_create_operation, (fee)(owner)(server_url)(metadata)(protocol_version))
FC_REFLECT( playchain::chain::room_update_operation, (fee)(owner)(room)(server_url)(metadata)(protocol_version))

