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

namespace playchain { namespace chain {

    using namespace graphene::chain;

    class game_start_playing_check_evaluator : public evaluator<game_start_playing_check_evaluator>
    {
       public:
          using operation_type = game_start_playing_check_operation;

          void_result do_evaluate( const operation_type& o );
          operation_result do_apply( const operation_type& o );
    };

    class game_result_check_evaluator : public evaluator<game_result_check_evaluator>
    {
       public:
          using operation_type = game_result_check_operation;

          void_result do_evaluate( const operation_type& o );
          operation_result do_apply( const operation_type& o );
    };

    class game_reset_evaluator : public evaluator<game_reset_evaluator>
    {
       public:
          using operation_type = game_reset_operation;

          void_result do_evaluate( const operation_type& o );
          operation_result do_apply( const operation_type& o );
    };

    void rollback_table(database& d, const table_object &table);
    void rollback_game(database& d, const table_object &table);

    void expire_voting_for_playing(database& d, const table_voting_object &table_voting, const table_object &table);
    void expire_voting_for_results(database& d, const table_voting_object &table_voting, const table_object &table);
}}
