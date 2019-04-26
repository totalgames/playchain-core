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

#include "deposit_pending_fees.hpp"

#include <graphene/chain/database.hpp>
#include <graphene/chain/db_with.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/worker_object.hpp>

#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>

#include <playchain/chain/playchain_config.hpp>
#include <playchain/chain/evaluators/db_helpers.hpp>

#include <fc/uint128.hpp>

#include <boost/range/iterator_range.hpp>

namespace playchain { namespace chain {

namespace
{
    share_type cut_fee(share_type a, uint16_t p)
    {
       if( a < 1 || p < 1 )
          return 0;
       if( p >= GRAPHENE_100_PERCENT )
          return a;

       fc::uint128 r(a.value);
       r *= p;
       r /= GRAPHENE_100_PERCENT;
       return r.to_uint64();
    }

    template<typename T>
    void deposit_cashback(database &d,
                          const room_object &room,
                          const T& cashback_parent,
                          const account_id_type &owner,
                          const share_type &amount)
    {
        if( amount < 1 )
           return;

#ifdef NDEBUG
        dlog("deposit cashback ${a} to '${name}'", ("a", amount)("name", owner(d).name));
#else
        ilog("deposit cashback ${a} to '${name}'", ("a", amount)("name", owner(d).name));
#endif

        d.modify(room, [&amount](room_object &obj)
        {
            obj.pending_rake -= asset(amount, obj.pending_rake.asset_id);
        });

        optional< vesting_balance_id_type > new_vbid = d.deposit_lazy_vesting(
           cashback_parent.balance,
           amount,
           d.get_global_properties().parameters.cashback_vesting_period_seconds,
           owner,
           true );

        if( new_vbid.valid() )
        {
           d.modify( cashback_parent, [&]( T& cashback_parent_ )
           {
              cashback_parent_.balance = *new_vbid;
           } );
        }

        return;
    }

    share_type get_total_balance(database &d,
                                 const playchain_parameters &parameters,
                                 const player_object &player)
    {
        const auto threshold = parameters.player_referrer_balance_max_threshold;
        const auto &account = player.account(d);

        share_type result;

        result += d.get_balance(account.id, asset_id_type()).amount;
        if (threshold >= result)
            return result;

        flat_set<vesting_balance_id_type> playchain_vbs;

        if (player.balance.valid())
        {
            const vesting_balance_object &vb = (*player.balance)(d);
            result += vb.balance.amount;
            if (threshold >= result)
                return result;

            playchain_vbs.emplace(vb.id);
        }

        const auto& witness_by_account = d.get_index_type<game_witness_index>().indices().get<by_playchain_account>();
        auto witness_by_account_it = witness_by_account.find(account.id);
        if (witness_by_account.end() != witness_by_account_it)
        {
            const auto &witness = (*witness_by_account_it);
            if (witness.balance.valid())
            {
                const vesting_balance_object &vb = (*witness.balance)(d);
                result += vb.balance.amount;
                if (threshold >= result)
                    return result;

                playchain_vbs.emplace(vb.id);
            }
        }

        const auto& rooms_by_owner = d.get_index_type<room_index>().indices().get<by_room_owner>();
        auto rooms_range = rooms_by_owner.equal_range(account.id);
        for( const room_object& room: boost::make_iterator_range( rooms_range.first, rooms_range.second ) )
        {
            if (room.balance.valid())
            {
                const vesting_balance_object &vb = (*room.balance)(d);
                result += vb.balance.amount;
                if (threshold >= result)
                    return result;

                playchain_vbs.emplace(vb.id);
            }
        }

        if (parameters.take_into_account_graphene_balances)
        {
            auto vesting_range =
                    d.get_index_type<vesting_balance_index>().indices().get<graphene::chain::by_account>().equal_range(account.id);
            for( const vesting_balance_object& vb: boost::make_iterator_range( vesting_range.first, vesting_range.second ) )
            {
                if (playchain_vbs.find(vb.id) != playchain_vbs.end())
                    continue;

                result += vb.balance.amount;
                if (threshold >= result)
                    return result;
            }
        }

        return result;
    }

    void disseminate_for_inviters(database &d,
                                  const playchain_parameters &parameters,
                                  const room_object &room,
                                  const player_object &inviter,
                                  share_type &amount,
                                  int max_depth)
    {
        if (inviter.id == PLAYCHAIN_NULL_PLAYER || amount < 1)
            return;

        if (max_depth < 1)
        {
            d.modify( inviter, [&](player_object &obj)
            {
                obj.pending_parent_invitation_fees[room.id] += amount;
            });

            amount = 0;
            return;
        }

        auto parents_cut = cut_fee(amount, parameters.player_referrer_parent_percent_of_fee);
        auto max_my_cut = amount - parents_cut;
        if (max_my_cut > 0)
        {
            share_type my_cut{max_my_cut};
            auto balance = get_total_balance(d, parameters, inviter);
            if (balance < parameters.player_referrer_balance_min_threshold)
            {
                parents_cut += my_cut;
                my_cut = 0;
            }else if (balance < parameters.player_referrer_balance_max_threshold)
            {
                fc::uint128 my_cut_128(max_my_cut.value);
                my_cut_128 *= balance.value;
                my_cut_128 /= parameters.player_referrer_balance_max_threshold.value;
                my_cut = my_cut_128.to_uint64();
                parents_cut += max_my_cut - my_cut;
            }

            deposit_cashback(d, room, inviter, inviter.account, my_cut);
        }

        amount = parents_cut;
        disseminate_for_inviters(d, parameters, room,
                                 inviter.inviter(d), amount, --max_depth);
    }
}

void deposit_pending_fees(database &d)
{
    const auto& parameters = get_playchain_parameters(d);
    const auto& idx = d.get_index_type<player_index>().indices().get<by_pending_fees>();
    using cref_type = std::reference_wrapper<const player_object>;
    std::vector<cref_type> players;
    for( const player_object& player : idx )
    {
        if (player.is_not_pending())
            break;

        players.emplace_back(std::cref(player));
    }

    for( const player_object& player : players )
    {
        for(const auto &pending_fees_p: player.pending_parent_invitation_fees)
        {
            auto room_id = pending_fees_p.first;
            const auto &room = room_id(d);
            auto pending_fees = pending_fees_p.second;

            disseminate_for_inviters(d, parameters, room,
                                     player, pending_fees, PLAYCHAIN_MAXIMUM_INVITERS_DEPTH);

            deposit_cashback(d, room, room, room.owner, pending_fees);
        }

        d.modify(player, [](player_object& obj) {
           obj.pending_parent_invitation_fees.clear();
        });

        for(const auto &data: player.pending_fees)
        {
            const auto &room = data.room(d);
            const auto &pending_fees = data.amount;

            auto inviters_cut = cut_fee(pending_fees, parameters.player_referrer_percent_of_fee );
            auto witness_cut = cut_fee(pending_fees, parameters.game_witness_percent_of_fee );
            auto game_owner_cut = pending_fees - inviters_cut - witness_cut;

            disseminate_for_inviters(d, parameters, room,
                                     player.inviter(d), inviters_cut, PLAYCHAIN_MAXIMUM_INVITERS_DEPTH);
            game_owner_cut += inviters_cut;

            auto witnesses = data.witnesses.size();
            if (witnesses > 0u)
            {
                auto witness_one_cut = witness_cut / witnesses;
                if (witness_one_cut > 0)
                {
                    for(const game_witness_id_type &witness_id: data.witnesses)
                    {
                        const auto &witness = witness_id(d);
                        deposit_cashback(d, room, witness, witness.account, witness_one_cut);
                        witness_cut -= witness_one_cut;
                    }
                }
                if (witness_cut > 0)
                {
                    const game_witness_id_type &lucky_witness_id = (*data.witnesses.begin());
                    const auto &witness = lucky_witness_id(d);
                    deposit_cashback(d, room, witness, witness.account, witness_cut);
                }
            }else
            {
                game_owner_cut += witness_cut;
            }

            deposit_cashback(d, room, room, room.owner, game_owner_cut);
        }

        d.modify(player, [](player_object& obj) {
           obj.pending_fees.clear();
        });
    }
}

}}
