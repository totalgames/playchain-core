/*
 * Copyright (c) 2018 Total Games LLC, and contributors.
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

#include <graphene/chain/protocol/operations.hpp>

#include <graphene/chain/evaluator.hpp>

namespace playchain { namespace chain {

    using namespace graphene::chain;

    class buy_in_table_evaluator : public evaluator<buy_in_table_evaluator>
    {
    public:
        typedef buy_in_table_operation operation_type;

        void_result do_evaluate(const operation_type& o);
        operation_result do_apply(const operation_type& o);
   };

    class buy_out_table_evaluator : public evaluator<buy_out_table_evaluator>
    {
    public:
        typedef buy_out_table_operation operation_type;

        void_result do_evaluate(const operation_type& o);
        operation_result do_apply(const operation_type& o);
    };

    class buy_in_reserve_evaluator : public evaluator<buy_in_reserve_evaluator>
    {
    public:
        typedef buy_in_reserve_operation operation_type;

        void_result do_evaluate(const operation_type& o);
        operation_result do_apply(const operation_type& o);
   };

    class buy_in_cancel_evaluator : public evaluator<buy_in_cancel_evaluator>
    {
    public:
        typedef buy_in_reserving_cancel_operation operation_type;

        void_result do_evaluate(const operation_type& o);
        void_result do_apply(const operation_type& o);
    };

    class buy_in_reserving_resolve_evaluator : public evaluator<buy_in_reserving_resolve_evaluator>
    {
    public:
        typedef buy_in_reserving_resolve_operation operation_type;

        void_result do_evaluate(const operation_type& o);
        operation_result do_apply(const operation_type& o);
    };

    class buy_in_reserving_cancel_all_evaluator : public evaluator<buy_in_reserving_cancel_all_evaluator>
    {
    public:
        typedef buy_in_reserving_cancel_all_operation operation_type;

        void_result do_evaluate(const operation_type& o);
        void_result do_apply(const operation_type& o);
    };

    void prolong_life_for_by_in(database& d, const buy_in_object &);
    void expire_buy_in(database& d, const buy_in_object &, const table_object &);

}}
