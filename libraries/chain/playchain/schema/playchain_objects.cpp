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

#include <playchain/chain/playchain_config.hpp>

#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/playchain_property_object.hpp>
#include <playchain/chain/schema/playchain_committee_member_object.hpp>

#include <graphene/chain/database.hpp>

namespace playchain { namespace chain {

    bool player_object::is_special_player(const player_id_type &id)
    {
        return PLAYCHAIN_NULL_PLAYER == id;
    }

    bool game_witness_object::is_special_witness(const game_witness_id_type &id)
    {
        return PLAYCHAIN_NULL_GAME_WITNESS == id;
    }

    bool room_object::is_special_room(const room_id_type &id)
    {
        return PLAYCHAIN_NULL_ROOM == id;
    }

    bool table_object::is_special_table(const table_id_type &id)
    {
        return PLAYCHAIN_NULL_TABLE == id;
    }

    void playchain_parameters::validate()const
    {
       FC_ASSERT( player_referrer_percent_of_fee <= GRAPHENE_100_PERCENT );
       FC_ASSERT( player_referrer_parent_percent_of_fee <= GRAPHENE_100_PERCENT );
       FC_ASSERT( game_witness_percent_of_fee <= GRAPHENE_100_PERCENT );
       FC_ASSERT( voting_for_playing_requied_percent <= GRAPHENE_100_PERCENT );
       FC_ASSERT( voting_for_results_requied_percent <= GRAPHENE_100_PERCENT );
       FC_ASSERT( percentage_of_voter_witness_substitution_while_voting_for_playing <= GRAPHENE_100_PERCENT );
       FC_ASSERT( percentage_of_voter_witness_substitution_while_voting_for_results <= GRAPHENE_100_PERCENT );
       FC_ASSERT( minimum_desired_number_of_players_for_tables_allocation < maximum_desired_number_of_players_for_tables_allocation );
    }

    const playchain_committee_member_object &playchain_property_object::get_committee_member(const database &d, const game_witness_id_type &id)
    {
        const auto& committee_members_by_witness = d.get_index_type<playchain_committee_member_index>().indices().get<by_game_witness>();
        auto it = committee_members_by_witness.find(id);
        FC_ASSERT(committee_members_by_witness.end() != it);
        return *it;
    }
}}
