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

#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/database.hpp>

namespace playchain { namespace chain {

    using namespace graphene::chain;

    class table_create_evaluator : public evaluator<table_create_evaluator>
    {
       public:
          using operation_type = table_create_operation;

          void_result do_evaluate( const operation_type& o );
          object_id_type do_apply( const operation_type& o );
    };

    class table_update_evaluator : public evaluator<table_update_evaluator>
    {
       public:
          using operation_type = table_update_operation;

          void_result do_evaluate( const operation_type& o );
          void_result do_apply( const operation_type& o );
    };

    class table_alive_evaluator : public evaluator<table_alive_evaluator>
    {
       public:
          using operation_type = tables_alive_operation;

          void_result do_evaluate( const operation_type& o );
          operation_result do_apply( const operation_type& o );
    };

    operation_result alive_for_table(database& d, const table_id_type &);
}}
