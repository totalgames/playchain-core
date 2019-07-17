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
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/operations.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <playchain/chain/protocol/playchain_types.hpp>
#include <playchain/chain/protocol/game_operations.hpp>

#include <set>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain {

    using namespace graphene::chain;
    using namespace graphene::db;

    class table_object : public graphene::db::abstract_object<table_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_table_object_type;

        room_id_type                                room;

        string                                      metadata;

        amount_type                                 required_witnesses = 0u;

        asset                                       min_accepted_proposal_asset;

        using pending_buy_in_type = flat_map<player_id_type, pending_buy_in_id_type>;

        ///buy-in cash that has just allocated
        pending_buy_in_type                         pending_proposals;

        using cash_data_type = flat_map<player_id_type, asset>;

        ///the cash indexed by players that used for buy-in/buy-out
        cash_data_type                              cash;

        ///the cash that was elected to start playing
        cash_data_type                              playing_cash;
        fc::time_point_sec                          game_created; //< for monitoring only
        fc::time_point_sec                          game_expiration;

        ///for playing state it is keep voted witnesses
        game_witnesses_type                         voted_witnesses;

        int32_t                                     weight = 0;

        uint32_t                                    occupied_places = 0;

        bool is_playing() const
        {
            return !is_free();
        }

        bool is_playing(const player_id_type &id) const
        {
            return playing_cash.end() != playing_cash.find(id);
        }

        bool is_free() const
        {
            return playing_cash.empty();
        }

        size_t get_waiting_players() const
        {
            return cash.size();
        }

        size_t get_pending_proposals() const
        {
            return pending_proposals.size();
        }

        bool is_waiting_at_table(const player_id_type &id) const
        {
            return cash.end() != cash.find(id);
        }

        static bool is_special_table(const table_id_type &);

        void set_weight(database &) const;
        void adjust_cash(const player_id_type &id, asset delta);
        void adjust_playing_cash(const player_id_type &id, asset delta);
        void clear_cash();
        void clear_playing_cash();
        //return previous if exist
        pending_buy_in_id_type add_pending_proposal(const player_id_type &id, const pending_buy_in_id_type &);
        void remove_pending_proposal(const player_id_type &id);

        asset get_pending_balance(const database &, const player_id_type &id) const;
        asset get_cash_balance(const player_id_type &id) const;
        asset get_playing_cash_balance(const player_id_type &id) const;
    };

    class pending_table_vote_object : public graphene::db::abstract_object<pending_table_vote_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_pending_table_vote_object_type;

        table_id_type                               table;
        account_id_type                             voter;
        op_wrapper                                  vote;
    };

    class table_voting_object: public graphene::db::abstract_object<table_voting_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_table_voting_object_type;

        table_id_type                               table;

        fc::time_point_sec                          created; //< for monitoring only
        fc::time_point_sec                          expiration;

        flat_map<account_id_type, voting_data_type> votes;

        size_t                                      witnesses_allowed_for_substitution = 0u;

        game_accounts_type                          required_player_voters;
        game_witnesses_type                         required_witness_voters;
        game_witnesses_type                         voted_witnesses;
        optional<voting_data_type>                  etalon_vote;

        bool is_voting_for_playing() const
        {
            return !votes.empty() && (votes.begin()->second).which() == voting_data_type::tag<game_initial_data>::value;
        }

        bool is_voting_for_results() const
        {
            return !votes.empty() && (votes.begin()->second).which() == voting_data_type::tag<game_result>::value;
        }
    };

    class table_alive_object : public graphene::db::abstract_object<table_alive_object>
    {
    public:
        static constexpr uint8_t space_id = implementation_for_playchain_ids;
        static constexpr uint8_t type_id  = impl_table_alive_object_type;

        table_id_type                       table;
        fc::time_point_sec                  created; //< for monitoring only
        fc::time_point_sec                  expiration;
    };

    /**
     *  @brief This secondary index will allow lookup of all table by room owner.
     */
    class table_owner_index : public secondary_index
    {
       public:
          table_owner_index(const database&);
          ~table_owner_index();

          virtual void object_inserted( const object& obj ) override;
          virtual void object_removed( const object& obj ) override;

          using tables_type = flat_set<table_id_type>;

          flat_map< account_id_type, tables_type > tables_by_owner;
          flat_map< room_id_type, account_id_type > rooms_owners;

        private:

          const database& _db;
    };

    /**
     *  @brief This secondary index will allow lookup of all tables those player sits at.
     */
    class table_players_index : public secondary_index
    {
       public:
          table_players_index(const database&);
          ~table_players_index();

          virtual void about_to_modify( const object& before );
          virtual void object_modified( const object& after  );

          using tables_type = std::set<table_id_type>;

          flat_map< player_id_type, tables_type > tables_with_pending_proposals_by_player;
          flat_map< player_id_type, tables_type > tables_with_cash_by_player;
          flat_map< player_id_type, tables_type > tables_with_playing_cash_by_player;

        private:

          using player_set_type = std::set<player_id_type>;

          player_set_type  get_pending_proposals( const table_object& )const;
          player_set_type  get_cash( const table_object& )const;
          player_set_type  get_playing_cash( const table_object& )const;

          player_set_type                         before_pending_proposals;
          player_set_type                         before_cash;
          player_set_type                         before_playing_cash;

          const database& _db;
    };

    /**
     *  @brief This secondary index will allow lookup vote data by tables.
     */
    class table_voting_statistics_index : public secondary_index
    {
        public:
           table_voting_statistics_index(const database&);
           ~table_voting_statistics_index();

            virtual void object_removed( const object& obj );

            using accounts_type = std::vector<account_id_type>;

            flat_map< table_id_type, accounts_type > voted_last_time_players_by_table;
            flat_map< table_id_type, accounts_type > not_voted_last_time_players_by_table;

        private:
            const database& _db;
    };

    struct by_room;
    struct by_room_and_metadata;
    struct by_table_choose_algorithm;

    /**
     * @ingroup object_index
     */
    using table_multi_index_type =
    multi_index_container<
       table_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_unique<tag<by_room>,
                    composite_key<table_object,
                    member<table_object, room_id_type, &table_object::room>,
                    member<object, object_id_type, &object::id > >>,
          ordered_non_unique<tag<by_room_and_metadata>,
                    composite_key<table_object,
                    member<table_object, room_id_type, &table_object::room>,
                    member<table_object, string, &table_object::metadata > >>,
          ordered_unique<tag<by_playchain_obj_expiration>,
                    composite_key<table_object,
                    member<table_object, time_point_sec, &table_object::game_expiration>,
                    member<object, object_id_type, &object::id > >>,
          ordered_unique<tag<by_table_choose_algorithm>,
                         composite_key<table_object,
                                member<table_object,
                                       string,
                                       &table_object::metadata>, //first by metadata
                                member<table_object,
                                       uint32_t,
                                       &table_object::occupied_places>, //first with greater free places
                                member<table_object,
                                       int32_t,
                                       &table_object::weight>, //sorted by weight
                                member<object,
                                       object_id_type,
                                       &object::id>>, //unique
                         composite_key_compare<std::less<string>,
                                               std::less<uint32_t>,
                                               std::greater<int32_t>,
                                               std::less<object_id_type>>>
    >>;

    using table_index = generic_index<table_object, table_multi_index_type>;

    struct by_table;
    struct by_table_expiration;

    /**
     * @ingroup object_index
     */
    using table_voting_multi_index_type =
    multi_index_container<
       table_voting_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_unique< tag<by_table>,
                    member<table_voting_object, table_id_type, &table_voting_object::table> >,
          ordered_unique<tag<by_table_expiration>,
                    composite_key<table_voting_object,
                    member<table_voting_object, time_point_sec, &table_voting_object::expiration>,
                    member<object, object_id_type, &object::id > >>
    >>;

    using table_voting_index = generic_index<table_voting_object, table_voting_multi_index_type>;

    struct by_table_voter;

    /**
     * @ingroup object_index
     */
    using pending_table_vote_multi_index_type =
    multi_index_container<
       pending_table_vote_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_non_unique< tag<by_table>,
                    member<pending_table_vote_object, table_id_type, &pending_table_vote_object::table> >,
          ordered_unique<tag<by_table_voter>,
                    composite_key<pending_table_vote_object,
                    member<pending_table_vote_object, table_id_type, &pending_table_vote_object::table>,
                    member<pending_table_vote_object, account_id_type, &pending_table_vote_object::voter> >>
    >>;

    using pending_table_vote_index = generic_index<pending_table_vote_object, pending_table_vote_multi_index_type>;

    /**
     * @ingroup object_index
     */
    using table_alive_multi_index_type =
    multi_index_container<
       table_alive_object,
       indexed_by<
          ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
          ordered_unique< tag<by_table>,
                    member<table_alive_object, table_id_type, &table_alive_object::table> >,
          ordered_unique<tag<by_table_expiration>,
                    composite_key<table_alive_object,
                    member<table_alive_object, time_point_sec, &table_alive_object::expiration>,
                    member<object, object_id_type, &object::id > >>
    >>;

    using table_alive_index = generic_index<table_alive_object, table_alive_multi_index_type>;
}}

FC_REFLECT_DERIVED( playchain::chain::table_object,
                    (graphene::db::object),
                    (room)
                    (metadata)
                    (required_witnesses)
                    (min_accepted_proposal_asset)
                    (pending_proposals)
                    (cash)
                    (playing_cash)
                    (game_created)
                    (game_expiration)
                    (voted_witnesses)
                    (weight)
                    (occupied_places))

FC_REFLECT_DERIVED( playchain::chain::pending_table_vote_object,
                    (graphene::db::object),
                    (table)
                    (voter)
                    (vote))

FC_REFLECT_DERIVED( playchain::chain::table_voting_object,
                    (graphene::db::object),
                    (table)
                    (created)
                    (expiration)
                    (votes)
                    (witnesses_allowed_for_substitution)
                    (required_player_voters)
                    (required_witness_voters)
                    (voted_witnesses)
                    (etalon_vote))

FC_REFLECT_DERIVED( playchain::chain::table_alive_object,
                    (graphene::db::object),
                    (table)
                    (created)
                    (expiration))
