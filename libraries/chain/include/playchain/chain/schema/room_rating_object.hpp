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
#include <boost/multi_index/composite_key.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>

namespace graphene {
    namespace chain {
        class database;
    }
}

namespace playchain {
    namespace chain {

        using namespace graphene::chain;
        using namespace graphene::db;

        class room_rating_kpi_measurement_object : public graphene::db::abstract_object<room_rating_kpi_measurement_object>
        {
        public:
            static constexpr uint8_t space_id = implementation_for_playchain_ids;
            static constexpr uint8_t type_id = impl_room_rating_kpi_measurement_object_type;

            fc::time_point_sec created = time_point_sec::min();
            fc::time_point_sec expiration = time_point_sec::min();

            room_id_type room;

            table_id_type table;

            pending_buy_in_id_type associated_buyin;

            uint32_t weight;

            bool waiting_resolve = true;
        };

        class room_rating_standby_measurement_object : public graphene::db::abstract_object<room_rating_standby_measurement_object>
        {
        public:
            static constexpr uint8_t space_id = implementation_for_playchain_ids;
            static constexpr uint8_t type_id = impl_room_rating_standby_measurement_object_type;

            fc::time_point_sec created = time_point_sec::min();
            fc::time_point_sec updated = time_point_sec::min();
            fc::time_point_sec expiration = time_point_sec::min();

            room_id_type room;

            table_id_type table;

            uint32_t weight;
        };

        struct by_pending_buy_in;
        struct by_room;
        struct by_expiration;

        /**
         * @ingroup object_index
         */
        using room_rating_kpi_measurement_multi_index_type =
        multi_index_container <
            room_rating_kpi_measurement_object,
                indexed_by <
                ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,

                //constraint key
                ordered_unique< tag<by_pending_buy_in>,
                    member<room_rating_kpi_measurement_object, pending_buy_in_id_type, &room_rating_kpi_measurement_object::associated_buyin > >,

                ordered_non_unique< tag<by_room>,
                                member<room_rating_kpi_measurement_object, room_id_type, &room_rating_kpi_measurement_object::room > >,

                ordered_non_unique<tag<by_expiration>,
                                member<room_rating_kpi_measurement_object, time_point_sec, &room_rating_kpi_measurement_object::expiration > >
            >> ;

        using room_rating_kpi_measurement_index = generic_index<room_rating_kpi_measurement_object, room_rating_kpi_measurement_multi_index_type>;

        struct by_table;

        /**
         * @ingroup object_index
         */
        using room_rating_standby_measurement_multi_index_type =
        multi_index_container <
            room_rating_standby_measurement_object,
                indexed_by <
                ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,

                //constraint key
                ordered_unique<tag<by_table>,
                          member<room_rating_standby_measurement_object, table_id_type, &room_rating_standby_measurement_object::table> >,

                ordered_non_unique< tag<by_room>,
                                member<room_rating_standby_measurement_object, room_id_type, &room_rating_standby_measurement_object::room > >,

                ordered_non_unique<tag<by_expiration>,
                                member<room_rating_standby_measurement_object, time_point_sec, &room_rating_standby_measurement_object::expiration > >
            >> ;

        using room_rating_standby_measurement_index = generic_index<room_rating_standby_measurement_object, room_rating_standby_measurement_multi_index_type>;
    }
}

FC_REFLECT_DERIVED(playchain::chain::room_rating_kpi_measurement_object,
                    (graphene::db::object),
                    (created)
                    (expiration)
                    (room)
                    (associated_buyin)
                    (weight)
                    (waiting_resolve))

FC_REFLECT_DERIVED(playchain::chain::room_rating_standby_measurement_object,
                    (graphene::db::object),
                    (created)
                    (updated)
                    (expiration)
                    (room)
                    (table)
                    (weight))
