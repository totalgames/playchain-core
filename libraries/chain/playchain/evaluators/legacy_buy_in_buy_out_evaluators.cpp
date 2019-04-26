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

#include <playchain/chain/evaluators/legacy_buy_in_buy_out_evaluators.hpp>
#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/database.hpp>

#define PLAYCHAIN_POKER_MIN_BIG_BLIND_AMOUNT    40
#define PLAYCHAIN_POKER_MAX_BIG_BLIND_AMOUNT    100

namespace playchain { namespace chain {

void_result buy_in_evaluator::do_evaluate(const buy_in_operation& op) {
    try {

        const database& d = db();
        const asset_id_type asset_type = asset_id_type(0);
        FC_ASSERT(op.big_blind_price.asset_id == asset_type);

        try {
            //check table_owner existence
            op.table_owner(d);

            FC_ASSERT(op.big_blind_price.amount > 0, "Big blind price must be greater than zero!");

            const auto valid_big_blind_amount = (op.purchased_big_blind_amount >= PLAYCHAIN_POKER_MIN_BIG_BLIND_AMOUNT && op.purchased_big_blind_amount <= PLAYCHAIN_POKER_MAX_BIG_BLIND_AMOUNT);
            FC_ASSERT(valid_big_blind_amount, "Big blind amount must be between ${1} and ${2}", ("1", PLAYCHAIN_POKER_MIN_BIG_BLIND_AMOUNT)("2", PLAYCHAIN_POKER_MAX_BIG_BLIND_AMOUNT));

            const auto total_amount = op.big_blind_price.amount * op.purchased_big_blind_amount /*+ op.rake_amount.amount*/; // ignore rake
            const bool insufficient_balance = d.get_balance(op.player, asset_type).amount >= total_amount;

            FC_ASSERT(insufficient_balance,
                "Insufficient Balance: ${balance}, unable to do the BUYIN operation for account '${a}' on tabel owner ${b}",
                ("a", op.player)("b", op.table_owner)("balance",
                    d.to_pretty_string(d.get_balance(op.player, asset_type))));

            return void_result();
        } FC_RETHROW_EXCEPTIONS(error, "Player ${p} cannot buy in to ${t}", ("p", op.player)("t", op.table_owner));

    } FC_CAPTURE_AND_RETHROW((op))
}

void_result buy_in_evaluator::do_apply(const buy_in_operation& o) {
    try {
        const auto total_amount = o.big_blind_price.amount * o.purchased_big_blind_amount;

        db().adjust_balance(o.player, -total_amount);
        db().adjust_balance(o.table_owner, total_amount);

        return void_result();
    } FC_CAPTURE_AND_RETHROW((o))
}

void_result buy_out_evaluator::do_evaluate(const buy_out_operation& op) {
    try {

        const database& d = db();
        const account_object& table_owner_account = op.table_owner(d);
        const account_object& player_account = op.player(d);
        const asset_object& asset_type = op.amount.asset_id(d);

        try {

            const bool insufficient_balance = d.get_balance(table_owner_account, asset_type).amount >= op.amount.amount;

            FC_ASSERT(insufficient_balance,
                      "Insufficient Balance: ${balance}, unable to transfer '${total_transfer}' from account '${a}' to '${t}'",
                      ("a", table_owner_account.name)("t", player_account.name)
                              ("total_transfer", d.to_pretty_string(op.amount))
                              ("balance", d.to_pretty_string(d.get_balance(table_owner_account, asset_type))));

            return void_result();
        } FC_RETHROW_EXCEPTIONS(error, "Unable to transfer ${a} from ${f} to ${t}", ("a", d.to_pretty_string(op.amount))
                ("f", op.table_owner(d).name)("t", op.player(d).name));

    } FC_CAPTURE_AND_RETHROW((op))
}

void_result buy_out_evaluator::do_apply(const buy_out_operation& o) {
    try {
        db().adjust_balance(o.player, o.amount);
        db().adjust_balance(o.table_owner, -o.amount);
        return void_result();
    } FC_CAPTURE_AND_RETHROW((o))
}

}}
