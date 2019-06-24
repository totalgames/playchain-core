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

#include <playchain/chain/evaluators/buy_in_buy_out_evaluators.hpp>
#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/room_rating_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/pending_buy_out_object.hpp>
#include <playchain/chain/schema/pending_buy_in_object.hpp>

#include <playchain/chain/evaluators/validators.hpp>
#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/table_evaluators.hpp>

namespace playchain { namespace chain {

namespace
{
    void check_pending_buy_in(const database& d, bool exists, const account_id_type &player, const string &uid)
    {
        const auto& by_uid = d.get_index_type<pending_buy_in_index>().indices().get<by_pending_buy_in_uid>();
        auto check = exists ^ (by_uid.end() == by_uid.find(std::make_tuple(player, uid)));
        if (exists)
        {
            FC_ASSERT(check, "Buy-in must exist");
        }else
        {
            FC_ASSERT(check, "Buy-in must not exist");
        }
    }

    void check_pending_buy_in_exists(const database& d, const account_id_type &player, const string &uid)
    {
        check_pending_buy_in(d, true, player, uid);
    }

    void check_pending_buy_in_not_exists(const database& d, const account_id_type &player, const string &uid)
    {
        check_pending_buy_in(d, false, player, uid);
    }

    void cancel_pending_buyin(database& d, const pending_buy_in_object& buy_in)
    {
        if (buy_in.is_allocated())
        {
            const table_object& table = buy_in.table(d);
            const player_object &player = get_player(d, buy_in.player);

            assert(table.pending_proposals.find(player.id) != table.pending_proposals.end());

            auto& measurements_by_buyin = d.get_index_type<room_rating_measurement_index>().indices().get<by_pending_buy_in>();
            auto it = measurements_by_buyin.find(buy_in.id);
            if (it != measurements_by_buyin.end())
            {
                wlog("______________Remove measurement ${m} - buyin cancelled", ("m", *it));

                d.remove(*it);
            }
            else
            {
                elog("Buyin ${b} doesn't take part in room rating calculation! Something is wrong.", ("b", buy_in));
            }

            d.modify(table, [&](table_object &obj)
            {
                 obj.remove_pending_proposal(player.id);
            });
        }

        d.adjust_balance(buy_in.player, buy_in.amount);

        d.remove(buy_in);
    }

    operation_result register_buy_in(database& d, const player_object & player,
                       const table_object &table)
    {
        object_id_type ret_id;

        auto& index = d.get_index_type<buy_in_index>().indices().get<by_buy_in_table_and_player>();
        auto it = index.find(boost::make_tuple(table.id, player.id));
        if (it == index.end())
        {
            const auto& dyn_props = d.get_dynamic_global_properties();
            const auto& parameters = get_playchain_parameters(d);

            ret_id = d.create<buy_in_object>([&](buy_in_object& buyin) {
               buyin.player = player.id;
               buyin.table = table.id;
               buyin.created = dyn_props.time;
               buyin.expiration = buyin.created + fc::seconds(parameters.buy_in_expiration_seconds);
            }).id;
        }
        else
        {
            prolong_life_for_by_in(d, *it);
        }

        object_id_type alive_id = alive_for_table(d, table.id).get<object_id_type>();
        if (ret_id == object_id_type{})
            ret_id = alive_id;

        return ret_id;
    }

    operation_result buy_out_table(database& d, const player_object & player,
                       const table_object &table,
                       const asset &amount)
    {
        bool allow_buy_out = !table.is_playing(player.id);
        if (!allow_buy_out)
        {
            auto it = table.cash.find(player.id);
            if (table.cash.end() != it)
            {
                allow_buy_out = (amount <= it->second);
            }
        }

        const auto& idx = d.get_index_type<pending_buy_out_index>().indices().get<by_player_at_table>();
        auto it = idx.find(std::make_tuple(player.id, table.id));

        if (allow_buy_out)
        {
            d.adjust_balance(player.account, amount);

            d.modify(table, [&](table_object &obj)
            {
                obj.adjust_cash(player.id, -amount);
            });

            if (idx.end() != it)
            {
                d.remove(*it);
            }
        }else
        {
            if (amount.amount > 0)
            {
                if (idx.end() == it)
                {
                    return d.create<pending_buy_out_object>([&](pending_buy_out_object &obj)
                    {
                         obj.player = player.id;
                         obj.table = table.id;
                         obj.amount = amount;
                    }).id;
                }else
                {
                    d.modify(*it, [&](pending_buy_out_object &obj)
                    {
                        obj.amount = amount;
                    });
                }
            }else if (idx.end() != it)
            {
                d.remove(*it);
            }
        }

        return {};
    }
}

    void_result buy_in_table_evaluator::do_evaluate(const operation_type& op)
    {
        try {
            const database& d = db();

            FC_ASSERT(is_player_exists(d, op.player), "Only player can participate game");
            FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");
            FC_ASSERT(is_table_owner(d, op.table_owner, op.table), "Wrong table owner");

            const table_object &table = op.table(d);

            FC_ASSERT(table.room(d).owner != op.player, "Can't buyin to own table");

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    operation_result buy_in_table_evaluator::do_apply(const operation_type& op)
    {
        try {
            database& d = db();

            const player_object &player = get_player(d, op.player);

            d.adjust_balance(op.player, -op.amount);

            const table_object &table = op.table(d);

            d.modify(table, [&](table_object &obj)
            {
                obj.adjust_cash(player.id, op.amount);
            });

            return register_buy_in(d, player, table);
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result buy_out_table_evaluator::do_evaluate(const operation_type& op)
    {
        try {
            const database& d = db();

            FC_ASSERT(is_player_exists(d, op.player), "Only player can out game");
            FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");
            FC_ASSERT(is_table_owner(d, op.table_owner, op.table), "Wrong table owner");

            const player_object &player = get_player(d, op.player);

            const table_object &table = op.table(d);

            FC_ASSERT(table.is_waiting_at_table(player.id) || table.is_playing(player.id), "There is no the player at the table");

            if (!table.is_playing(player.id))
            {
                asset cash = (*table.cash.find(player.id)).second;
                FC_ASSERT(cash >= op.amount, "Insufficient funds");
            }

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    operation_result buy_out_table_evaluator::do_apply(const operation_type& op)
    {
        try {
            database& d = db();

            const player_object &player = get_player(d, op.player);

            const table_object &table = op.table(d);

            return buy_out_table(d, player, table, op.amount);
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result buy_in_reserve_evaluator::do_evaluate(const operation_type& op)
    {
        try {
            const database& d = db();

            version_ext checked_protocol_version;
            fc::from_variant(op.protocol_version, checked_protocol_version);

            FC_ASSERT(is_player_exists(d, op.player), "Only player can reserve place");

            check_pending_buy_in_not_exists(d, op.player, op.uid);

            const auto& parameters = get_playchain_parameters(d);

            auto& buy_in_by_player = d.get_index_type<pending_buy_in_index>().indices().get<by_pending_buy_in_player>();
            FC_ASSERT(buy_in_by_player.count(op.player) < parameters.amount_reserving_places_per_user, "Limit is exceeded. Client Code = 20180011");

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    operation_result buy_in_reserve_evaluator::do_apply(const operation_type& op)
    {
        try {
            database& d = db();

            PLAYCHAIN_NULL_PENDING_BUYIN(d);

            const auto& playchain_parameters = get_playchain_parameters(d);
            const auto& dyn_props = d.get_dynamic_global_properties();
            const auto& player = get_player(d, op.player);

            d.adjust_balance(op.player, -op.amount);

            return d.create<pending_buy_in_object>([&](pending_buy_in_object& buyin) {
               buyin.player = op.player;
               buyin.player_iternal = player.id;
               buyin.uid = op.uid;
               buyin.amount = op.amount;
               buyin.metadata = op.metadata;
               fc::from_variant(op.protocol_version, buyin.protocol_version);
               buyin.created = dyn_props.time;
               buyin.expiration = buyin.created + fc::seconds(playchain_parameters.pending_buyin_proposal_lifetime_limit_in_seconds);
            }).id;
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result buy_in_cancel_evaluator::do_evaluate(const operation_type& op)
    {
        try {
            const database& d = db();

            FC_ASSERT(is_player_exists(d, op.player), "Only player can cancel place reserving");

            check_pending_buy_in_exists(d, op.player, op.uid);

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result buy_in_cancel_evaluator::do_apply(const operation_type& op)
    {
        try {
            database& d = db();

            auto& by_uid = d.get_index_type<pending_buy_in_index>().indices().get<by_pending_buy_in_uid>();

            const auto &buy_in = *by_uid.find(std::make_tuple(op.player, op.uid));

            cancel_pending_buyin(d, buy_in);

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result buy_in_reserving_resolve_evaluator::do_evaluate(const operation_type& op)
    {
        try {
            const database& d = db();

            FC_ASSERT(is_table_exists(d, op.table), "Table does not exist");

            const table_object& table = op.table(d);

            FC_ASSERT(is_table_owner(d, op.table_owner, op.table), "Wrong table owner");
            FC_ASSERT(is_pending_buy_in_exists(d, op.pending_buyin), "Pending buy-in does not exist");

            const pending_buy_in_object& buyin = op.pending_buyin(d);
            const player_object &player = get_player(d, buyin.player);

            FC_ASSERT(table.pending_proposals.count(player.id) && table.pending_proposals.at(player.id) == buyin.id, "Pending buy-in does not exist at table");

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    operation_result buy_in_reserving_resolve_evaluator::do_apply(const operation_type& op)
    {
        try {
            database& d = db();

            const table_object& table = op.table(d);
            const pending_buy_in_object& buyin = op.pending_buyin(d);
            const player_object &player = get_player(d, buyin.player);

            assert(table.pending_proposals.find(player.id) != table.pending_proposals.end());

            auto& measurements_by_buyin = d.get_index_type<room_rating_measurement_index>().indices().get<by_pending_buy_in>();
            auto it = measurements_by_buyin.find(buyin.id);
            if (it != measurements_by_buyin.end())
            {
                d.modify(*it, [&](room_rating_measurement_object &obj)
                {
                    wlog("______________Push Succeeded measurement ${m}, expiration_secconds ${e}", ("m", obj)("e", get_playchain_parameters(d).room_rating_measurements_alive_periods * d.get_global_properties().parameters.maintenance_interval));

                    obj.waiting_resolve = false;
                    obj.weight = 1; // in this case poker server works as expected, so we push its rating up by assigning mark with value 1
                                    // If poker room does more useful work than only servicing client games(e.g it's also a poker witness for other rooms)
                                    // we can reward it in the future by assigning greater value to obj.weight
                });
            }
            else
            {
                elog("Buyin ${b} doesn't take part in room rating calculation! Something is wrong.", ("b", buyin));
            }

            d.modify(table, [&](table_object &obj)
            {
                obj.remove_pending_proposal(player.id);

                obj.adjust_cash(player.id, buyin.amount);
            });

            d.remove(buyin);

            return register_buy_in(d, player, table);
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result buy_in_reserving_cancel_all_evaluator::do_evaluate(const operation_type& op)
    {
        try {
            const database& d = db();

            FC_ASSERT(is_player_exists(d, op.player), "Only player can cancel place reserving");

            auto& buy_in_by_player = d.get_index_type<pending_buy_in_index>().indices().get<by_pending_buy_in_player>();
            FC_ASSERT(buy_in_by_player.count(op.player), "There no any pending buyin for '${i}'", ("i", op.player));

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result buy_in_reserving_cancel_all_evaluator::do_apply(const operation_type& op)
    {
        try {
            database& d = db();

            auto& buy_in_by_player = d.get_index_type<pending_buy_in_index>().indices().get<by_pending_buy_in_player>();

            auto it = buy_in_by_player.find(op.player);
            while (it != buy_in_by_player.end())
            {
                cancel_pending_buyin(d, *it);
                it = buy_in_by_player.find(op.player);
            }

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void prolong_life_for_by_in(database& d, const buy_in_object & buy_in)
    {
        const auto& dyn_props = d.get_dynamic_global_properties();
        const auto& parameters = get_playchain_parameters(d);

        d.modify(buy_in, [&](buy_in_object &obj)
        {
            obj.expiration = dyn_props.time + fc::seconds(parameters.buy_in_expiration_seconds);
        });
    }

    void expire_buy_in(database& d, const buy_in_object &buy_in, const table_object &table)
    {
        const player_object &player = buy_in.player(d);

        asset amount = table.get_cash_balance(player.id);
        if (amount.amount > 0)
        {
            buy_out_table(d, player, table, amount);
        }

        asset new_amount = table.get_cash_balance(player.id);
        if (new_amount.amount < 1)
        {
            d.push_applied_operation( buy_in_expire_operation{ buy_in.player(d).account, table.id, table.room(d).owner, amount } );
            d.remove(buy_in);
        }else
        {
            prolong_life_for_by_in(d, buy_in);
        }
    }

}}
