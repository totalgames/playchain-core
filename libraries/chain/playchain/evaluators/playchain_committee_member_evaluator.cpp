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

#include <playchain/chain/evaluators/playchain_committee_member_evaluator.hpp>
#include <playchain/chain/schema/playchain_committee_member_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/playchain_property_object.hpp>

#include <playchain/chain/evaluators/validators.hpp>
#include <playchain/chain/evaluators/db_helpers.hpp>

#include <graphene/chain/database.hpp>

namespace playchain { namespace chain {

    void_result playchain_committee_member_create_evaluator::do_evaluate( const playchain_committee_member_create_operation& op )
    {
        try {
            const database& d = db();

            FC_ASSERT(is_game_witness(d, op.committee_member_account), "Account must be the game witness");

            return void_result();
        } FC_CAPTURE_AND_RETHROW( (op) )
    }

    object_id_type playchain_committee_member_create_evaluator::do_apply( const playchain_committee_member_create_operation& op )
    {
        try {
            database& d = db();

            vote_id_type vote_id;
            d.modify(d.get_global_properties(), [&vote_id](global_property_object& p)
            {
                vote_id = get_next_vote_id(p, vote_id_type::playchain_committee);
            });

            const auto &witness = get_game_witness(d, op.committee_member_account);
            const auto& new_object = d.create<playchain_committee_member_object>( [&]( playchain_committee_member_object& obj ){

                 obj.committee_member_game_witness = witness.id;
                 obj.vote_id            = vote_id;
                 obj.url                = op.url;
            });

            dlog("new committee member: ${m}", ("m", new_object));

            return new_object.id;
        } FC_CAPTURE_AND_RETHROW( (op) )
    }

    void_result playchain_committee_member_update_evaluator::do_evaluate( const playchain_committee_member_update_operation& op )
    {
        try {
            const database& d = db();

            FC_ASSERT(is_game_witness(d, op.committee_member_account), "Account must be the game witness");

            const auto &witness = get_game_witness(d, op.committee_member_account);

            FC_ASSERT(d.get(op.committee_member).committee_member_game_witness == witness.id);

            return void_result();
        } FC_CAPTURE_AND_RETHROW( (op) )
    }

    void_result playchain_committee_member_update_evaluator::do_apply( const playchain_committee_member_update_operation& op )
    {
        try {
            database& d = db();
            d.modify(
                d.get(op.committee_member), [&]( playchain_committee_member_object& com )
            {
                if( op.new_url.valid() )
                    com.url = *op.new_url;
            });

            return void_result();
        } FC_CAPTURE_AND_RETHROW( (op) )
    }

    template<typename Operation>
    void_result playchain_committee_member_update_parameters_evaluator_impl<Operation>::do_evaluate(const operation_type& o)
    {
        try {
            FC_ASSERT(this->trx_state->_is_proposed_trx);

            return void_result();
        } FC_CAPTURE_AND_RETHROW( (o) )
    }

    template<typename Operation>
    void_result playchain_committee_member_update_parameters_evaluator_impl<Operation>::do_apply(const operation_type& o)
    {
        try {
            database& d = this->db();
            d.modify(d.get( playchain_property_id_type() ), [&o](playchain_property_object& p)
            {
                playchain_parameters params = o.new_parameters;
                p.pending_parameters = params;

                ilog("${p1} -> ${p2}", ("p1", p.parameters)("p2", params));
            });

            return void_result();
        } FC_CAPTURE_AND_RETHROW( (o) )
    }

    template class playchain_committee_member_update_parameters_evaluator_impl<playchain_committee_member_update_parameters_operation>;
    template class playchain_committee_member_update_parameters_evaluator_impl<playchain_committee_member_update_parameters_v2_operation>;
    template class playchain_committee_member_update_parameters_evaluator_impl<playchain_committee_member_update_parameters_v3_operation>;
    template class playchain_committee_member_update_parameters_evaluator_impl<playchain_committee_member_update_parameters_v4_operation>;

} } // graphene::chain
