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

    struct game_start_playing_check_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 0;
            uint32_t price_per_kbyte = 0;
        };

        asset                                       fee;
        table_id_type                               table;
        account_id_type                             table_owner;
        account_id_type                             voter;

        game_initial_data                           initial_data;

        const auto &data() const
        {
            return initial_data;
        }

        account_id_type   fee_payer()const { return voter; }
        void              validate()const;
        share_type        calculate_fee(const fee_parameters_type& k) const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };

    struct game_result_check_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 0;
            uint32_t price_per_kbyte = 0;
        };

        asset                                       fee;
        table_id_type                               table;
        account_id_type                             table_owner;
        account_id_type                             voter;

        game_result                                 result;

        const auto &data() const
        {
            return result;
        }

        account_id_type   fee_payer()const { return voter; }
        void              validate()const;
        share_type        calculate_fee(const fee_parameters_type& k) const
        {
            return calculate_fee_with_kbyte(this, k);
        }
    };

    struct game_reset_operation : public base_operation
    {
        struct fee_parameters_type {
            uint64_t fee = 0;
        };

        asset                                       fee;
        table_id_type                               table;
        account_id_type                             table_owner;

        ///reset all cash on table not only reserved for current game
        bool                                        rollback_table = false;

        account_id_type   fee_payer()const { return table_owner; }
        void              validate()const;
    };
}}

FC_REFLECT( playchain::chain::game_start_playing_check_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::game_result_check_operation::fee_parameters_type, (fee)(price_per_kbyte))
FC_REFLECT( playchain::chain::game_reset_operation::fee_parameters_type, (fee))

FC_REFLECT( playchain::chain::game_start_playing_check_operation, (fee)(table)(table_owner)(voter)(initial_data))
FC_REFLECT( playchain::chain::game_result_check_operation, (fee)(table)(table_owner)(voter)(result))
FC_REFLECT( playchain::chain::game_reset_operation, (fee)(table)(table_owner)(rollback_table))
