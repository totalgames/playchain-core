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

#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>

namespace playchain { namespace chain {

   using namespace graphene::chain;

   class playchain_committee_member_create_evaluator : public evaluator<playchain_committee_member_create_evaluator>
   {
      public:
         typedef playchain_committee_member_create_operation operation_type;

         void_result do_evaluate( const playchain_committee_member_create_operation& o );
         object_id_type do_apply( const playchain_committee_member_create_operation& o );
   };

   class playchain_committee_member_update_evaluator : public evaluator<playchain_committee_member_update_evaluator>
   {
      public:
         typedef playchain_committee_member_update_operation operation_type;

         void_result do_evaluate( const playchain_committee_member_update_operation& o );
         void_result do_apply( const playchain_committee_member_update_operation& o );
   };

   template<typename Operation>
   class playchain_committee_member_update_parameters_evaluator_impl : public evaluator<playchain_committee_member_update_parameters_evaluator_impl<Operation>>
   {
   public:
       using operation_type = Operation;

       void_result do_evaluate( const operation_type& o );
       void_result do_apply( const operation_type& o );
   };

   using playchain_committee_member_update_parameters_evaluator = playchain_committee_member_update_parameters_evaluator_impl<
           playchain_committee_member_update_parameters_operation>;

   using playchain_committee_member_update_parameters_v2_evaluator = playchain_committee_member_update_parameters_evaluator_impl<
           playchain_committee_member_update_parameters_v2_operation>;

   using playchain_committee_member_update_parameters_v3_evaluator = playchain_committee_member_update_parameters_evaluator_impl<
           playchain_committee_member_update_parameters_v3_operation>;
} } // graphene::chain
