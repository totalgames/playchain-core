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

#include <playchain/chain/evaluators/legacy_game_evaluators.hpp>

#include <graphene/chain/database.hpp>

namespace playchain { namespace chain {

void_result game_evaluator::do_evaluate(const game_operation& op)
{
    try
    {
        FC_ASSERT(op.big_blind_price.amount > 0, "Big blind price must be greater than zero!");

        const database& d = db();

        //check asset and accounts existence
        op.big_blind_price.asset_id(d);
        op.table_owner(d);
        for (auto& id: op.players)
        {
            id(d);
        }
        return void_result();
    }
    FC_CAPTURE_AND_RETHROW((op))
}

void_result game_evaluator::do_apply(const game_operation& )
{
    return void_result();
}

}}
