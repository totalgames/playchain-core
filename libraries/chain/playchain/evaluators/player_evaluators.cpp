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

#include <playchain/chain/evaluators/player_evaluators.hpp>
#include <playchain/chain/playchain_config.hpp>

#include <playchain/chain/schema/player_object.hpp>

#include <playchain/chain/evaluators/validators.hpp>

namespace playchain { namespace chain{

    void_result player_create_by_room_owner_evaluator::do_evaluate(const player_create_by_room_owner_operation& op) {
        try {

            const database& d = db();

            FC_ASSERT(is_account_exists(d, op.account), "Account doesn't exist");
            FC_ASSERT(is_room_owner(d, op.room_owner), "Table owner doesn't own table");
            FC_ASSERT(!is_player_exists(d, op.account), "Account is already player");

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    object_id_type player_create_by_room_owner_evaluator::do_apply(const player_create_by_room_owner_operation& op) {
        try {

            database& d = db();

            return create_player(d, op.account, PLAYCHAIN_NULL_PLAYER);
        } FC_CAPTURE_AND_RETHROW((op))
    }

    player_id_type create_player(database& d, const account_id_type &account, const player_id_type &inviter)
    {
        PLAYCHAIN_NULL_PLAYER(d);

        const auto &new_player = d.create<player_object>([&](player_object& player) {
                       player.account = account;
                       player.inviter = inviter;
                    });

        dlog("new player: ${p}", ("p", new_player));

        return new_player.id;
    }
}}
