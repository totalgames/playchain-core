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

#include <playchain/chain/evaluators/table_evaluators.hpp>
#include <playchain/chain/playchain_config.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/validators.hpp>

#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/room_rating_object.hpp>

#include <graphene/chain/hardfork.hpp>

#include <limits>

namespace playchain{ namespace chain{

    namespace
    {
        void set_rating_measurement_by_alive_table(database& d, const table_id_type &table)
        {
            auto room_id = table(d).room;

            auto& measurements_by_room = d.get_index_type<room_rating_measurement_index>().indices().template get<by_room>();

            auto range = measurements_by_room.equal_range(room_id);
            if (range.first == range.second)
            {
                const auto& idx = d.get_index_type<room_rating_measurement2_index>().indices().get<by_table>();
                auto it = idx.find(table);
                if (idx.end() != it)
                {
                    d.modify(*it, [&](room_rating_measurement2_object& obj) {
                        obj.updated = d.head_block_time();
                                });
                }else
                {
                    d.create<room_rating_measurement2_object>([&](room_rating_measurement2_object& obj) {
                                               obj.updated = d.head_block_time();
                                               obj.room = room_id;
                                               obj.table = table;
                                               obj.weight = 10;
                    });
                }
            }
        }
    }

    void_result table_create_evaluator::do_evaluate( const operation_type& op )
    {
        try {
            const database& d = db();
            FC_ASSERT(is_room_exists(d, op.room), "Room does not exist");
            FC_ASSERT(is_room_owner(d, op.owner, op.room), "Wrong room owner");

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    object_id_type table_create_evaluator::do_apply( const operation_type& op )
    {
        try {
            database& d = db();

            PLAYCHAIN_NULL_TABLE(d);

            const auto &new_table = d.create<table_object>([&](table_object& table) {
                           table.room = op.room;
                           table.metadata = op.metadata;
                           table.required_witnesses = op.required_witnesses;
                           table.min_accepted_proposal_asset = op.min_accepted_proposal_asset;
                           table.game_created = fc::time_point_sec::min();
                           table.game_expiration = fc::time_point_sec::maximum();
                        });

            new_table.set_weight(d);

            dlog("new table: ${t}", ("t", new_table));

            return new_table.id;
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result table_update_evaluator::do_evaluate( const operation_type& op )
    {
        try {
            const database& d = db();
            FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");
            FC_ASSERT(is_table_owner(d, op.owner, op.table), "Wrong table owner");

            auto &&table_obj = op.table(d);

            FC_ASSERT(table_obj.is_free(), "Can't update table while playing");
            FC_ASSERT(!is_table_voting(d, op.table), "Can't update table while voting");
            if (d.head_block_time() >= HARDFORK_PLAYCHAIN_3_TIME &&
                    table_obj.metadata != op.metadata)
            {
                FC_ASSERT(!is_table_alive(d, table_obj.id), "Can't update table metadata while table alive");
            }

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result table_update_evaluator::do_apply( const operation_type& op )
    {
        try {
            database& d = db();

            auto &&table_obj = op.table(d);

            d.modify(table_obj, [&](table_object& table) {
                            table.metadata = op.metadata;
                            table.required_witnesses = op.required_witnesses;
                            table.min_accepted_proposal_asset = op.min_accepted_proposal_asset;
                        });

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result table_alive_evaluator::do_evaluate( const operation_type& op )
    {
        try {
            const database& d = db();
            for (const auto &table: op.tables)
            {
                FC_ASSERT(is_table_exists(d, table), "Table does not exist");
                FC_ASSERT(is_table_owner(d, op.owner, table), "Wrong table owner");
            }

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    operation_result table_alive_evaluator::do_apply( const operation_type& op )
    {
        try {
            database& d = db();

            operation_result last_alive_id;
            for (const auto &table: op.tables)
            {
                last_alive_id = alive_for_table(d, table);
            }
            return last_alive_id;
        } FC_CAPTURE_AND_RETHROW((op))
    }

    operation_result alive_for_table(database& d, const table_id_type &table)
    {
        auto &&table_obj = table(d);

        const auto& dyn_props = d.get_dynamic_global_properties();
        const auto& parameters = get_playchain_parameters(d);

        object_id_type alive_id;

        const auto& idx = d.get_index_type<table_alive_index>().indices().get<by_table>();
        auto it = idx.find(table_obj.id);
        if (idx.end() != it)
        {
            d.modify(*it, [&](table_alive_object& alive) {
                alive.expiration = dyn_props.time + fc::seconds(parameters.table_alive_expiration_seconds);
                        });
            alive_id = (*it).id;
        }else
        {
            alive_id = d.create<table_alive_object>([&](table_alive_object& alive) {
                                       alive.table = table_obj.id;
                                       alive.created = dyn_props.time;
                                       alive.expiration = alive.created + fc::seconds(parameters.table_alive_expiration_seconds);
            }).id;
        }

        table_obj.set_weight(d);

        if (d.head_block_time() >= HARDFORK_PLAYCHAIN_12_TIME)
        {
            set_rating_measurement_by_alive_table(d, table);
        }
        return alive_id;
    }

}}
