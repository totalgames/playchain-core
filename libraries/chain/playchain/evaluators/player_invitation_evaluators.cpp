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

#include <playchain/chain/evaluators/player_invitation_evaluators.hpp>
#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <playchain/chain/schema/player_invitation_object.hpp>

#include <playchain/chain/protocol/player_operations.hpp>
#include <playchain/chain/schema/player_object.hpp>

#include <playchain/chain/playchain_utils.hpp>

#include <playchain/chain/evaluators/validators.hpp>
#include <playchain/chain/evaluators/player_evaluators.hpp>

namespace playchain { namespace chain{

    namespace
    {
        void check_inviter_exists(const database& d, const account_id_type &inviter)
        {
            FC_ASSERT(is_account_exists(d, inviter), "Inviter account does not exist");
        }

        void check_invitation(const database& d, bool exists, const account_id_type &inviter, const string &uid)
        {
            const auto& invitations_by_uid = d.get_index_type<player_invitation_index>().indices().get<by_player_invitation_uid>();
            auto check = exists ^ (invitations_by_uid.end() == invitations_by_uid.find(std::make_tuple(inviter, uid)));
            if (exists)
            {
                FC_ASSERT(check, "Invitation must exist. Client Code = 20180006");
            }else
            {
                FC_ASSERT(check, "Invitation must not exist");
            }
        }

        void check_invitation_exists(const database& d, const account_id_type &inviter, const string &uid)
        {
            check_invitation(d, true, inviter, uid);
        }

        void check_invitation_not_exists(const database& d, const account_id_type &inviter, const string &uid)
        {
            check_invitation(d, false, inviter, uid);
        }

        void check_invitation_authority(const database& d, const account_id_type &inviter, const string &uid, const signature_type &mandat)
        {
            check_authority(d, inviter, utils::create_digest_by_invitation(d.get_chain_id(), inviter(d).name, uid), mandat);
        }
    }

    void_result player_invitation_create_evaluator::do_evaluate(const player_invitation_create_operation& op) {
        try {
            const database& d = db();

            check_invitation_not_exists(d, op.inviter, op.uid);

            _upgrade_account = account_upgrade_operation();
            if (!op.inviter(d).is_lifetime_member())
            {
                _upgrade_account.account_to_upgrade = op.inviter;
                _upgrade_account.upgrade_to_lifetime_member = true;
                _upgrade_account.fee = asset{0};
                _upgrade_account.validate();
            }

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    object_id_type player_invitation_create_evaluator::do_apply(const player_invitation_create_operation& op) {
        try {
            database& d = db();

            if (!_upgrade_account.account_to_upgrade(d).is_special_account())
            {
                transaction_evaluation_state state(&d);
                state.skip_fee_schedule_check = true;
                state.skip_fee = true;

                d.apply_operation(state, _upgrade_account);
            }

            const auto& dyn_props = d.get_dynamic_global_properties();
            return d.create<player_invitation_object>([&](player_invitation_object& invitation) {
               invitation.inviter = op.inviter;
               invitation.uid = op.uid;
               invitation.created = dyn_props.time;
               invitation.expiration = invitation.created + fc::seconds(op.lifetime_in_sec);
               invitation.metadata = op.metadata;
            }).id;
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result player_invitation_cancel_evaluator::do_evaluate(const player_invitation_cancel_operation& op) {
        try {

            const database& d = db();

            check_invitation_exists(d, op.inviter, op.uid);

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result player_invitation_cancel_evaluator::do_apply(const player_invitation_cancel_operation& op) {
        try {

            database& d = db();

            auto& invitations_by_uid = d.get_index_type<player_invitation_index>().indices().get<by_player_invitation_uid>();
            d.remove(*invitations_by_uid.find(std::make_tuple(op.inviter, op.uid)));

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    void_result player_invitation_resolve_evaluator::do_evaluate(const player_invitation_resolve_operation& op) {
        try {
            const database& d = db();

            check_inviter_exists(d, op.inviter);
            check_invitation_exists(d, op.inviter, op.uid);
            check_invitation_authority(d, op.inviter, op.uid, op.mandat);

            _create_account = account_create_operation();
            _create_account.registrar = op.inviter;
            _create_account.referrer = GRAPHENE_TEMP_ACCOUNT;
            _create_account.name = op.name;
            _create_account.owner = op.owner;
            _create_account.active = op.active;
            auto keys = op.active.get_keys();
            if (keys.empty())
                keys = op.owner.get_keys();
            FC_ASSERT(!keys.empty());
            _create_account.options.memo_key = (*keys.begin());
            _create_account.fee = asset{0};
            _create_account.validate();

            return void_result();
        } FC_CAPTURE_AND_RETHROW((op))
    }

    object_id_type player_invitation_resolve_evaluator::do_apply(const player_invitation_resolve_operation& op) {
        try {
            database& d = db();

            auto inviter_like_player = PLAYCHAIN_NULL_PLAYER;
            const auto& player_by_account = d.get_index_type<player_index>().indices().get<by_playchain_account>();
            auto p_player = player_by_account.find(op.inviter);
            if (player_by_account.end() != p_player)
            {
                inviter_like_player = p_player->id;
            }

            transaction_evaluation_state state(&d);
            state.skip_fee_schedule_check = true;
            state.skip_fee = true;

            account_id_type new_account_id = d.apply_operation(state, _create_account).get<object_id_type>();

            auto &&new_player_id = create_player(d, new_account_id, inviter_like_player);

            d.set_applied_operation_result(
                        d.push_applied_operation( player_create_operation{ new_account_id, op.inviter} ),
                        operation_result{(object_id_type)new_player_id});

            auto& invitations_by_uid = d.get_index_type<player_invitation_index>().indices().get<by_player_invitation_uid>();
            d.remove(*invitations_by_uid.find(std::make_tuple(op.inviter, op.uid)));

            return new_account_id;
        } FC_CAPTURE_AND_RETHROW((op))
    }
}}
