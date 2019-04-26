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

#include <graphene/db/object.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/generic_index.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;
    using namespace graphene::db;

    class game_witness_object : public graphene::db::abstract_object<game_witness_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_game_witness_object_type;

        account_id_type                     account;

        /**
         * Vesting balance which receives profit (from game results) deposits.
         */
        optional<vesting_balance_id_type>   balance;

        static bool is_special_witness(const game_witness_id_type &);

    };

    /**
     * @ingroup object_index
     */
    using game_witness_multi_index_type =
    multi_index_container<
       game_witness_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_unique< tag<by_playchain_account>,
                        member<game_witness_object, account_id_type, &game_witness_object::account>
       >
    >>;

    using game_witness_index = generic_index<game_witness_object, game_witness_multi_index_type>;
}}

FC_REFLECT_DERIVED( playchain::chain::game_witness_object,
                    (graphene::db::object),
                    (account)(balance) )

