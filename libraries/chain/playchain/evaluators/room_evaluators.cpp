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

#include <playchain/chain/evaluators/room_evaluators.hpp>

#include <playchain/chain/evaluators/validators.hpp>

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>

#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/hardfork.hpp>


namespace playchain { namespace chain {

void_result room_create_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();

        version_ext checked_protocol_version;
        fc::from_variant(op.protocol_version, checked_protocol_version);

        const auto& rooms_by_owner_and_metadata = d.get_index_type<room_index>().indices().get<by_room_owner_and_metadata>();
        FC_ASSERT(rooms_by_owner_and_metadata.end() == rooms_by_owner_and_metadata.find(boost::make_tuple(op.owner, op.metadata)),
                  "There is the same room that is owned by account");

        return void_result();
    } FC_CAPTURE_AND_RETHROW((op))
}

object_id_type room_create_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();

        PLAYCHAIN_NULL_ROOM(d);
        PLAYCHAIN_NULL_GAME_WITNESS(d);

        const auto& idx = d.get_index_type<game_witness_index>().indices().get<by_playchain_account>();
        auto it = idx.find(op.owner);
        if (idx.end() == it)
        {
            const auto &new_witness = d.create<game_witness_object>([&](game_witness_object& witness) {
                                                witness.account = op.owner;
                                            });

            dlog("new game witness: ${p}", ("p", new_witness));

            d.set_applied_operation_result(
                        d.push_applied_operation( game_witness_create_operation{ op.owner } ),
                        operation_result{new_witness.id});
        }

        const auto &new_room = d.create<room_object>([&](room_object& room) {
                       room.owner = op.owner;
                       room.server_url = op.server_url;
                       room.metadata = op.metadata;
                       fc::from_variant(op.protocol_version, room.protocol_version);

                       if (d.head_block_time() >= HARDFORK_PLAYCHAIN_5_TIME)
                       {
                           room.rating = d.get_dynamic_global_properties().average_room_rating;
                       }
                       else
                       {
                           room.rating = 0;
                       }
                    });

        dlog("new room: ${r}", ("r", new_room));

        return new_room.id;
    } FC_CAPTURE_AND_RETHROW((op))
}

void_result room_update_evaluator::do_evaluate(const operation_type& op)
{
    try {
        const database& d = db();

        version_ext checked_protocol_version;
        fc::from_variant(op.protocol_version, checked_protocol_version);

        FC_ASSERT(is_room_exists(d, op.room), "Room does not exist");
        FC_ASSERT(is_room_owner(d, op.owner, op.room), "Wrong room owner");
        return void_result();
    } FC_CAPTURE_AND_RETHROW((op))

}

void_result room_update_evaluator::do_apply(const operation_type& op)
{
    try {
        database& d = db();

        d.modify(op.room(d), [&](room_object& room) {
                       room.server_url = op.server_url;
                       room.metadata = op.metadata;
                       fc::from_variant(op.protocol_version, room.protocol_version);
                    });

        return void_result();
    } FC_CAPTURE_AND_RETHROW((op))
}

}}
