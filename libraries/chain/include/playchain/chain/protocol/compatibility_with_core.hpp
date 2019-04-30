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

#include <boost/preprocessor/repetition/repeat_from_to.hpp>

namespace playchain { namespace chain {

    using namespace graphene::chain;

}}

#define BITSHARES_DUMP_OPERATION(_1, N, _2)                                             \
namespace playchain { namespace chain {                                                 \
                                                                                        \
struct bitshares_dump_operation_##N: public base_operation                              \
{                                                                                       \
    struct fee_parameters_type {                                                        \
    };                                                                                  \
                                                                                        \
    asset                     fee;                                                      \
                                                                                        \
    account_id_type fee_payer() const { return GRAPHENE_TEMP_ACCOUNT; }                 \
    void            validate() const { FC_ASSERT( !"dump operation" ); }                \
    share_type      calculate_fee(const fee_parameters_type& )const { return 0; }       \
};                                                                                      \
}}                                                                                      \
FC_REFLECT( playchain::chain::bitshares_dump_operation_##N::fee_parameters_type, )      \
FC_REFLECT( playchain::chain::bitshares_dump_operation_##N, (fee))

//declare 6 dump operations: 1 <= N < 7
BOOST_PP_REPEAT_FROM_TO(8, 10, BITSHARES_DUMP_OPERATION, _)
