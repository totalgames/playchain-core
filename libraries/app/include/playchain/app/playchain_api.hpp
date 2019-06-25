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

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/block.hpp>

#include <playchain/chain/schema/player_invitation_object.hpp>
#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/playchain_property_object.hpp>
#include <playchain/chain/schema/playchain_committee_member_object.hpp>

#include <fc/api.hpp>

#include <memory>
#include <vector>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace app {

using namespace graphene::chain;
using namespace playchain::chain;

class playchain_api_impl;

struct playchain_member_balance_info
{
    asset account_balance;
    asset referral_balance;
    asset rake_balance;
    asset witness_balance;

    optional<vesting_balance_id_type> referral_balance_id;
    optional<vesting_balance_id_type> rake_balance_id;
    optional<vesting_balance_id_type> witness_balance_id;
};

struct playchain_room_info
{
    room_id_type                        id;
    account_id_type                     owner;
    string                              owner_name;
    string                              metadata;
    string                              server_url;
    int32_t                             rating = 0;
    string                              protocol_version;
    fc::time_point_sec                  last_rating_update;
    asset                               rake_balance;
    optional<vesting_balance_id_type>   rake_balance_id;
};

enum class playchain_table_state
{
    nop = 0,
    free,
    playing,
    voting_for_playing,
    voting_for_results
};

struct playchain_table_info
{
    playchain_table_info() = default;

    table_id_type         id;
    account_id_type       owner;
    string                owner_name;
    string                metadata;
    string                server_url;
    amount_type           required_witnesses = 0;
    asset                 min_accepted_proposal_asset;
    playchain_table_state state = playchain_table_state::nop;
    bool                  alive = false;
};

struct playchain_pending_buy_in_proposal_info
{
    string                              name;
    pending_buy_in_id_type              id;
    string                              uid;
    asset                               amount;
};

struct player_cash_info
{
    player_cash_info() = default;
    player_cash_info(const string &name,
                     const asset &amount):
        name(name),
        amount(amount)
    {
    }

    string                              name;
    asset                               amount;
};

struct playchain_table_info_ext: public playchain_table_info
{
    using pending_buy_in_type = flat_map<account_id_type, playchain_pending_buy_in_proposal_info>;
    using cash_data_type = flat_map<account_id_type, player_cash_info>;
    using accounts_data_type = flat_map<account_id_type, string>;

    pending_buy_in_type  pending_proposals;
    cash_data_type       cash;
    cash_data_type       playing_cash;
    accounts_data_type   missed_voters;
    accounts_data_type   voters;
};

using player_invitation_object_with_blockchain_time = std::pair<player_invitation_object, fc::time_point_sec>;
using player_invitation_objects_list_with_blockchain_time = std::pair< vector<player_invitation_object>, fc::time_point_sec>;

enum class playchain_player_table_state
{
    nop = 0,
    pending,
    attable,
    playing,
};

struct playchain_player_table_info
{
    playchain_player_table_info() = default;
    playchain_player_table_info(const playchain_player_table_state state,
                                const asset &balance,
                                const asset &buyouting_balance,
                                playchain_table_info &&other):
        state(state),
        balance(balance),
        buyouting_balance(buyouting_balance)
    {
        table = std::move(other);
    }

    playchain_player_table_state state = playchain_player_table_state::nop;
    asset balance;
    asset buyouting_balance;
    playchain_table_info table;
};

struct playchain_account_info
{
    account_id_type account;
    public_key_type login_key;
    player_id_type  player;
};

struct version_info
{
    string playchain_ver;
    string bitshares_ver;
    string git_revision_description;
    string git_revision_sha;
    string git_revision_unix_timestamp;
    string openssl_ver;
    string boost_ver;
    string websocket_ver;
};

/**
 * @brief The playchain_api class implements the RPC API for PlayChain
 *
 * This API contains methods to access playchain object model
 */
class playchain_api
{
   public:
      playchain_api(graphene::chain::database& db);
      ~playchain_api();

      /** List invitation objects for certain inviter.
       *
       * @param inviter_name_or_id Accept inviter: player ID, account ID, account name.
       * @param last_page_uid The invitation UID from list starting (not including).
       *                      Empty string for first page
       * @param limit The limitation (or page size)
       * @returns the invitations list
       */
      player_invitation_objects_list_with_blockchain_time list_player_invitations(
                                                          const string &inviter_name_or_id,
                                                          const string &last_page_uid,
                                                          const uint32_t limit) const;

      /** Get certain invitation object.
       *
       * @param inviter_name_or_id Accept inviter: player ID, account ID, account name.
       * @param uid The invitation UID
       * @returns the invitation object if invitation exists
       */
      optional< player_invitation_object_with_blockchain_time > get_player_invitation(
                                                    const string &inviter_name_or_id,
                                                    const string &uid) const;


      /** List invitation objects.
       *
       * @param inviter_name_or_id Accept inviter: player ID, account ID, account name.
       * @param last_page_id The invitation ID from list starting in ascending order (not including)
       *              Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the invitations list
       */
      player_invitation_objects_list_with_blockchain_time list_all_player_invitations(
                                                          const string &last_page_id,
                                                          const uint32_t limit) const;

      /** List invited accounts.
       *
       * @param last_page_id The invitation ID from list starting in ascending order (not including)
       *              Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the invitations list
       */
      vector< std::pair< player_object, account_object > > list_invited_players(
                                                          const string &inviter_name_or_id,
                                                          const string &last_page_id,
                                                          const uint32_t limit) const;

      /** Get certain player object.
       *
       * @param account_name_or_id Accept: player ID, account ID, account name (assert if player doesn't exist).
       * @returns the player object if account is player
       */
      optional< player_object > get_player(const string &account_name_or_id) const;

      /** List player objects.
       *
       * @param last_page_id The player ID from list starting in ascending order (not including)
       *                     Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the playes list
       */
      vector< player_object > list_all_players(const string &last_page_id,
                                               const uint32_t limit) const;

      /** Get rooms of owner.
       *
       * @param owner_name_or_id Accept owner: player ID, account ID, account name.
       * @param last_page_id The room ID from list starting in ascending order (not including)
       *              Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the rooms list
       */
      vector< room_object > list_rooms(const string &owner_name_or_id,
                                       const string &last_page_id,
                                       const uint32_t limit) const;

      /** List rooms.
       *
       * @param last_page_id The room ID from list starting in ascending order (not including)
       *                     Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the rooms list
       */
      vector< room_object > list_all_rooms(
                                       const string &last_page_id,
                                       const uint32_t limit) const;

      /**
       * @brief Light version for database_api::get_objects
       * Check if returned info include field Id corresponds to input Id
       * to detect room existence.
       * @return vector of room info if it is found.
       */
      vector< playchain_room_info > get_rooms_info_by_id(const flat_set<room_id_type>& ids) const;

      /**
       * @brief Get room info by owner and metadata.
       * @return room info if it is found.
       */
      optional< playchain_room_info > get_room_info(const string &owner_name_or_id, const string &metadata) const;

      /** Get game witness object.
       *
       * @param account_name_or_id Accept owner: game witness ID, player ID, account ID, account name.
       * @returns the game witness object if account is game witness
       */
      optional< game_witness_object > get_game_witness(const string &account_name_or_id) const;

      /** List game witnesses.
       *
       * @param last_page_id The game witness ID from list starting in ascending order (not including)
       *                     Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the game witnesses list
       */
      vector< game_witness_object > list_all_game_witnesses(
                                       const string &last_page_id,
                                       const uint32_t limit) const;

      /**
       * @brief Get a list of committee_members by ID
       * @param committee_member_ids IDs of the committee_members to retrieve
       * @return The committee_members corresponding to the provided IDs
       *
       * This function has semantics identical to @ref get_objects
       */
      vector<optional<playchain_committee_member_object>> get_committee_members(const vector<playchain_committee_member_id_type>& committee_member_ids) const;

      /**
       * @brief Get the committee_member owned by a given account
       * @param account_name_or_id
       * @return The committee_member object, or null if the account does not have a committee_member
       */
      optional< playchain_committee_member_object > get_committee_member_by_account(const string &account_name_or_id) const;

      /**
       * @brief Get names and IDs for registered committee_members
       * @param lower_bound_name Lower bound of the first name to return
       * @param limit Maximum number of results to return -- must not exceed 1000
       * @returns the playchain committee members list
       */
      vector< playchain_committee_member_object > list_all_committee_members(const string& last_page_id, uint32_t limit)const;

      /** Get table of owner.
       *
       * @param owner_name_or_room_id Accept owner: room ID, player ID, account ID, account name.
       * @param last_page_id The table ID from list starting in ascending order (not including)
       *                     Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the tables list
       */
      vector< table_object > list_tables(const string &owner_name_or_room_id,
                                         const string &last_page_id,
                                         const uint32_t limit) const;

      /** Get tables of owner with filter.
       *
       * @param room_id Accept room ID.
       * @param metadata.
       * @param limit The limitation (or page size).
       * @returns the tables list
       */
      vector< playchain_table_info > get_tables_info_by_metadata(const string &room_id,
                                                       const string &metadata,
                                                       const uint32_t limit) const;

      /** List tables.
       *
       * @param last_page_id The table ID from list starting in ascending order (not including)
       *                     Empty string for first page
       * @param limit The limitation (or page size).
       * @returns the tables list
       */
      vector< table_object > list_all_tables(
                                       const string &last_page_id,
                                       const uint32_t limit) const;

      /**
       * @brief Retrieve a block header for last_irreversible_block_num. Helper to build transaction
       * @return header of the referenced block
       */
      block_header get_last_irreversible_block_header() const;

      /**
       * @brief Retrieve balance info for assets that take part in PlayChain ecosystem
       * @param name_or_id Accept player ID or account ID or account name.
       * @return info or error if account not found
       */
      playchain_member_balance_info get_playchain_balance_info(const string &name_or_id) const;

      /**
       * @brief Light version for database_api::get_account_by_name with multiple names
       * @return account Id(s) by names if it is found
       */
      flat_map< string, account_id_type > get_account_id_by_name(const flat_set<string> &names) const;

      /**
       * @brief Light version for database_api::get_objects
       * Check if returned info include field Id corresponds to input Id
       * to detect table existence.
       * @return vector of table info if it is found.
       */
      vector< playchain_table_info_ext > get_tables_info_by_id(const flat_set<table_id_type>& ids) const;

      /**
       * @brief Register a callback handle which then can be used to subscribe to table info changes
       * @param ids list of tables
       * @param cb The callback handle to register
       */
      void set_tables_subscribe_callback( std::function<void(const variant&)> cb, const flat_set<table_id_type>& ids ) ;

      /**
       * @brief Reset callback for some tables info notifications
       * @param ids list of tables
       */
      void cancel_tables_subscribe_callback( const flat_set<table_id_type>& ids );

      /**
       * @brief Reset callback for all tables info notifications
       */
      void cancel_all_tables_subscribe_callback( );

      /**
       * @brief Check if pending buy-in proposal has been allocated to table
       * @param name_or_id Accept player ID or account ID or account name.
       * @param uid UID field for buy_in_reserve_operation
       * @returns the table object with pending buyin id if proposal has been allocated
       */
      optional< playchain_table_info > get_table_info_for_pending_buy_in_proposal(const string &name_or_id, const string &uid) const;

      /**
       * @brief Retrieve the current @ref playchain_property_object
       */
      playchain_property_object get_playchain_properties() const;

      /** Get rooms in which player there is
       *
       * @param owner_name_or_id Accept owner: player ID, account ID, account name.
       * @param limit The limitation (or single page size).
       * @returns the rooms list
       */
      vector< playchain_player_table_info > list_tables_with_player(const string &name_or_id, const uint32_t limit) const;

      /** Get account info if core account and playchain account (player) exist
       *
       * @param name Core account name.
       * @returns Info if exist
       */
      optional< playchain_account_info > get_player_account_by_name(const string &name ) const;

      /** Get PlayChain version
       *
       * @returns version
       */
      version_info get_version() const;

    private:
        std::shared_ptr< playchain_api_impl > _impl;
};

}}

FC_REFLECT(playchain::app::playchain_member_balance_info,
           (account_balance)
           (referral_balance)
           (rake_balance)
           (witness_balance)
           (referral_balance_id)
           (rake_balance_id)
           (witness_balance_id))

FC_REFLECT(playchain::app::playchain_room_info,
           (id)
           (owner)
           (owner_name)
           (metadata)
           (server_url)
           (rating)
           (protocol_version)
           (last_rating_update)
           (rake_balance)
           (rake_balance_id))

FC_REFLECT_ENUM(playchain::app::playchain_table_state,
                (nop)
                (free)
                (playing)
                (voting_for_playing)
                (voting_for_results))

FC_REFLECT(playchain::app::playchain_table_info,
           (id)
           (owner)
           (owner_name)
           (metadata)
           (server_url)
           (required_witnesses)
           (min_accepted_proposal_asset)
           (state)
           (alive))

FC_REFLECT(playchain::app::playchain_pending_buy_in_proposal_info,
           (name)
           (id)
           (uid)
           (amount))

FC_REFLECT(playchain::app::player_cash_info,
           (name)
           (amount))

FC_REFLECT_DERIVED(playchain::app::playchain_table_info_ext, (playchain::app::playchain_table_info),
                   (pending_proposals)
                   (cash)
                   (playing_cash)
                   (missed_voters)
                   (voters))

FC_REFLECT_ENUM(playchain::app::playchain_player_table_state,
                (nop)
                (pending)
                (attable)
                (playing))

FC_REFLECT(playchain::app::playchain_player_table_info,
           (state)
           (balance)
           (buyouting_balance)
           (table))

FC_REFLECT(playchain::app::playchain_account_info,
           (account)
           (login_key)
           (player))

FC_REFLECT(playchain::app::version_info,
    (playchain_ver)
    (bitshares_ver)
    (git_revision_description)
    (git_revision_sha)
    (git_revision_unix_timestamp)
    (openssl_ver)
    (boost_ver)
    (websocket_ver))

FC_API(playchain::app::playchain_api,
    (list_player_invitations)
    (get_player_invitation)
    (list_all_player_invitations)
    (list_invited_players)
    (get_player)
    (list_all_players)
    (list_rooms)
    (list_all_rooms)
    (get_rooms_info_by_id)
    (get_room_info)
    (get_game_witness)
    (list_all_game_witnesses)
    (get_committee_members)
    (get_committee_member_by_account)
    (list_all_committee_members)
    (list_tables)
    (get_tables_info_by_metadata)
    (list_all_tables)
    (get_last_irreversible_block_header)
    (get_playchain_balance_info)
    (get_account_id_by_name)
    (get_tables_info_by_id)
    (set_tables_subscribe_callback)
    (cancel_tables_subscribe_callback)
    (cancel_all_tables_subscribe_callback)
    (get_table_info_for_pending_buy_in_proposal)
    (get_playchain_properties)
    (list_tables_with_player)
    (get_player_account_by_name)
    (get_version)
)
