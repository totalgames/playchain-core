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
#include <graphene/chain/protocol/asset.hpp>

#include <graphene/db/generic_index.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;
    using namespace graphene::db;

    class pending_buy_out_object : public graphene::db::abstract_object<pending_buy_out_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_pending_buy_out_object_type;

        player_id_type                      player;
        table_id_type                       table;
        asset                               amount;
    };

    struct by_player_at_table;
    struct by_table;

    /**
     * @ingroup object_index
     */
    using pending_buy_out_multi_index_type =
    multi_index_container<
       pending_buy_out_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_unique< tag<by_player_at_table>,
                        composite_key< pending_buy_out_object,
                            member< pending_buy_out_object, player_id_type, &pending_buy_out_object::player >,
                            member< pending_buy_out_object, table_id_type, &pending_buy_out_object::table >
                         >>,
          ordered_non_unique< tag<by_table>,
                            member< pending_buy_out_object, table_id_type, &pending_buy_out_object::table > >
    >>;

    using pending_buy_out_index = generic_index<pending_buy_out_object, pending_buy_out_multi_index_type>;
}}

FC_REFLECT_DERIVED( playchain::chain::pending_buy_out_object,
                    (graphene::db::object),
                    (player)
                    (table)
                    (amount))
