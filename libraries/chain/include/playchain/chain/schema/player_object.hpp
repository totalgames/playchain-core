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
#include <graphene/db/generic_index.hpp>

#include <graphene/chain/protocol/asset.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <vector>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;
    using namespace graphene::db;

    struct player_pending_fee_data
    {
        player_pending_fee_data(){}

        player_pending_fee_data(const room_id_type &room,
                                const share_type &amount,
                                const game_witnesses_type &witnesses):
        room(room),
        amount(amount),
        witnesses(witnesses){}

        room_id_type                        room;
        share_type                          amount;
        game_witnesses_type                 witnesses;
    };

    class player_object : public graphene::db::abstract_object<player_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_player_object_type;

        account_id_type                     account;

        ///The account that receives a percentage of referral rewards.
        player_id_type                      inviter;

        using fees_data_type = std::vector<player_pending_fee_data>;
        using fees_by_rooms_type = flat_map<room_id_type, share_type>;

        /**
         * Tracks the table provider fees paid by this account which have not been disseminated
         * to the various inviters that receive them yet. This is used as an optimization to avoid
         * doing massive amounts of uint128 arithmetic on each and every inviter (process_fees).
         *
         * These fees will be paid out as vesting cash-back, and this counter will reset during the maintenance
         * interval.
         *
         * Cash-back wiil be immediately available for withdrawal.
         */
        fees_data_type                      pending_fees;

        ///inviters inheritance remainder
        fees_by_rooms_type                  pending_parent_invitation_fees;

        /**
         * Vesting balance which receives cashback from games.
         */
        optional<vesting_balance_id_type>   balance;

        bool is_not_pending() const
        {
            return pending_fees.empty() && pending_parent_invitation_fees.empty();
        }

        static bool is_special_player(const player_id_type &);
    };

    struct by_inviter;
    struct by_pending_fees;

    /**
     * @ingroup object_index
     */
    using player_multi_index_type =
    multi_index_container<
       player_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_unique< tag<by_playchain_account>,
                                member<player_object, account_id_type, &player_object::account>>,
          ordered_unique< tag<by_inviter>,
                            composite_key<player_object,
                                    member<player_object, player_id_type, &player_object::inviter>,
                                    member< object, object_id_type, &object::id >>>,
          ordered_unique<tag<by_pending_fees>,
                            composite_key<player_object,
                                     const_mem_fun<player_object,
                                            bool,
                                            &player_object::is_not_pending>,
                                     member<object,
                                            object_id_type,
                                            &object::id>>,
                       composite_key_compare<std::less<bool>,
                                             std::less<object_id_type>>>
    >>;

    using player_index = generic_index<player_object, player_multi_index_type>;
}}

FC_REFLECT( playchain::chain::player_pending_fee_data, (room)(amount)(witnesses))

FC_REFLECT_DERIVED( playchain::chain::player_object,
                    (graphene::db::object),
                    (account)(inviter)(pending_fees)(pending_parent_invitation_fees)(balance) )
