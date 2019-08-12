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

    class pending_buy_in_object : public graphene::db::abstract_object<pending_buy_in_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_pending_buy_in_object_type;

        account_id_type                     player;
        string                              uid;
        asset                               amount;
        string                              metadata;
        version_ext                         protocol_version;
        fc::time_point_sec                  created; //< for monitoring only
        fc::time_point_sec                  expiration;
        table_id_type                       table = PLAYCHAIN_NULL_TABLE; //< if allocated
        player_id_type                      player_iternal;

        bool is_allocated() const
        {
            return table != table_id_type{};
        }
    };

    class buy_in_object : public graphene::db::abstract_object<buy_in_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_buy_in_object_type;

        player_id_type                      player;
        table_id_type                       table;
        fc::time_point_sec                  created; //< for monitoring only
        fc::time_point_sec                  updated; //< for monitoring only
        fc::time_point_sec                  expiration;
    };

    struct by_pending_buy_in_uid;
    struct by_pending_buy_in_player;
    struct by_pending_buy_in_allocation_status_and_expiration;

    /**
     * @ingroup object_index
     */
    using pending_buy_in_multi_index_type =
    multi_index_container<
        pending_buy_in_object,
        indexed_by<
           ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
           ordered_unique< tag<by_pending_buy_in_uid>,
        composite_key< pending_buy_in_object,
            member< pending_buy_in_object, account_id_type, &pending_buy_in_object::player >,
            member< pending_buy_in_object, string, &pending_buy_in_object::uid >
         >>,
            ordered_unique<tag<by_playchain_obj_expiration>,
        composite_key<pending_buy_in_object,
         member<pending_buy_in_object, time_point_sec, &pending_buy_in_object::expiration>,
         member<object, object_id_type, &object::id >
        >>,
           ordered_unique<tag<by_pending_buy_in_allocation_status_and_expiration>,
        composite_key<pending_buy_in_object,
            const_mem_fun<pending_buy_in_object,
                   bool,
                   &pending_buy_in_object::is_allocated>,
            member<pending_buy_in_object, time_point_sec, &pending_buy_in_object::expiration>,
            member<object, object_id_type, &object::id >
        >>,
        ordered_non_unique< tag<by_pending_buy_in_player>, member<pending_buy_in_object, account_id_type, &pending_buy_in_object::player > >
    >>;

    using pending_buy_in_index = generic_index<pending_buy_in_object, pending_buy_in_multi_index_type>;

    struct by_buy_in_player;
    struct by_buy_in_table_and_player;
    struct by_buy_in_table;

    /**
     * @ingroup object_index
     */
    using buy_in_multi_index_type =
    multi_index_container<
        buy_in_object,
        indexed_by<
           ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
           ordered_unique<tag<by_playchain_obj_expiration>,
        composite_key<buy_in_object,
           member<buy_in_object, time_point_sec, &buy_in_object::expiration>,
           member<object, object_id_type, &object::id >
        >>,
           ordered_unique<tag<by_buy_in_player>,
        composite_key<buy_in_object,
            member<buy_in_object, player_id_type, &buy_in_object::player>,
            member<object, object_id_type, &object::id >
        >>,
            ordered_unique<tag<by_buy_in_table_and_player>,
         composite_key<buy_in_object,
             member<buy_in_object, table_id_type, &buy_in_object::table>,
             member<buy_in_object, player_id_type, &buy_in_object::player>
         >>,
        ordered_non_unique< tag<by_buy_in_table>,
                  member<buy_in_object, table_id_type, &buy_in_object::table >>
    >>;

    using buy_in_index = generic_index<buy_in_object, buy_in_multi_index_type>;
}}

FC_REFLECT_DERIVED( playchain::chain::pending_buy_in_object,
                    (graphene::db::object),
                    (player)
                    (uid)
                    (amount)
                    (metadata)
                    (protocol_version)
                    (created)
                    (expiration)
                    (table)
                    (player_iternal))
FC_REFLECT_DERIVED( playchain::chain::buy_in_object,
                    (graphene::db::object),
                    (player)
                    (table)
                    (created)
                    (updated)
                    (expiration))
