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

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;
    using namespace graphene::db;

    class room_object : public graphene::db::abstract_object<room_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_room_object_type;

        account_id_type                     owner;

        string                              server_url;
        string                              metadata;
        version_ext                         protocol_version;
        int32_t                             rating = 0;
        ///used for rating calculation
        fc::time_point_sec                  last_rating_update = time_point_sec::min();
        int32_t                             prev_rating = 0;
        table_id_type                       last_updated_table;

        ///Not disseminated rake
        asset pending_rake;

        /**
         * Vesting balance which receives profit (from game results) deposits.
         */
        optional<vesting_balance_id_type>   balance;

        bool is_not_changed_rating() const
        {
            return rating == prev_rating;
        }

        static bool is_special_room(const room_id_type &);
    };

    struct by_room_owner;
    struct by_room_owner_and_id;
    struct by_room_owner_and_metadata;
    struct by_last_rating_update;
    struct by_room_rating;

    /**
     * @ingroup object_index
     */
    using room_multi_index_type =
    multi_index_container<
       room_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_non_unique< tag<by_room_owner>,
                        member<room_object, account_id_type, &room_object::owner > >,
          ordered_unique<tag<by_room_owner_and_id>,
                    composite_key<room_object,
                        member<room_object, account_id_type, &room_object::owner>,
                        member<object, object_id_type, &object::id > >>,
          ordered_unique<tag<by_room_owner_and_metadata>,
                    composite_key<room_object,
                        member<room_object, account_id_type, &room_object::owner>,
                        member<room_object, string, &room_object::metadata > >>,
          ordered_unique<tag<by_last_rating_update>,
                     composite_key<room_object,
                          member<room_object, fc::time_point_sec, &room_object::last_rating_update>,
                          member<object, object_id_type, &object::id > >>,
          ordered_unique<tag<by_room_rating>,
                      composite_key<room_object,
                           const_mem_fun<room_object,
                           bool,
                           &room_object::is_not_changed_rating>,
                           member<object, object_id_type, &object::id > >,
                      composite_key_compare<std::less<bool>,
                                            std::less<object_id_type>>>
    >>;

    using room_index = generic_index<room_object, room_multi_index_type>;
}}

FC_REFLECT_DERIVED( playchain::chain::room_object,
                    (graphene::db::object),
                    (owner)(server_url)(metadata)(protocol_version)(rating)(last_rating_update)(prev_rating)(last_updated_table)(pending_rake)(balance) )

