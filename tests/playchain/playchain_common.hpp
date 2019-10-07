#pragma once

#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

#include <graphene/app/api.hpp>
#include <graphene/app/database_api.hpp>

#include "actor.hpp"
#include "defines.hpp"

#include <set>

namespace playchain_common
{
    using namespace graphene::chain;
    using namespace graphene::app;
    using namespace graphene::chain::test;
    using namespace playchain::chain;
    using namespace playchain::app;

    struct get_operation_name
    {
        using result_type = std::string;

        template <typename T> result_type operator()(const T& v) const
        {
            return name_from_type(fc::get_typename<T>::name());
        }

    private:

        std::string name_from_type(const std::string& type_name) const;
    };

    struct playchain_fixture: public database_fixture
    {
        const int object_id_type_id = operation_result::tag<object_id_type>::value;

        const int account_create_id = operation::tag<account_create_operation>::value;
        const int player_create_id = operation::tag<player_create_operation>::value;
        const int game_witness_create_id = operation::tag<game_witness_create_operation>::value;
        const int account_upgrade_id = operation::tag<account_upgrade_operation>::value;
        const int player_invitation_create_id = operation::tag<player_invitation_create_operation>::value;
        const int player_invitation_cancel_id = operation::tag<player_invitation_cancel_operation>::value;
        const int player_invitation_resolve_id = operation::tag<player_invitation_resolve_operation>::value;
        const int player_invitation_expire_id = operation::tag<player_invitation_expire_operation>::value;
        const int room_create_id = operation::tag<room_create_operation>::value;
        const int room_update_id = operation::tag<room_update_operation>::value;
        const int table_create_id = operation::tag<table_create_operation>::value;
        const int game_start_playing_check_id = operation::tag<game_start_playing_check_operation>::value;
        const int game_result_check_id = operation::tag<game_result_check_operation>::value;
        const int game_reset_id = operation::tag<game_reset_operation>::value;
        const int game_event_id = operation::tag<game_event_operation>::value;
        const int buy_in_table_id = operation::tag<buy_in_table_operation>::value;
        const int buy_out_table_id = operation::tag<buy_out_table_operation>::value;
        const int player_create_by_room_owner_id = operation::tag<player_create_by_room_owner_operation>::value;
        const int buy_in_reserve_id = operation::tag<buy_in_reserve_operation>::value;
        const int buy_in_reserving_cancel_id = operation::tag<buy_in_reserving_cancel_operation>::value;
        const int buy_in_reserving_resolve_id = operation::tag<buy_in_reserving_resolve_operation>::value;
        const int buy_in_reserving_expire_id = operation::tag<buy_in_reserving_expire_operation>::value;
        const int buy_in_reserving_allocated_table_id = operation::tag<buy_in_reserving_allocated_table_operation>::value;
        const int buy_in_reserving_cancel_all_id = operation::tag<buy_in_reserving_cancel_all_operation>::value;
        const int buy_in_expire_id = operation::tag<buy_in_expire_operation>::value;

        struct game_event
        {
            const int game_start_playing_validated_id = game_event_type::tag<game_start_playing_validated>::value;
            const int game_result_validated_id = game_event_type::tag<game_result_validated>::value;
            const int game_rollback_id = game_event_type::tag<game_rollback>::value;
            const int fail_consensus_game_start_playing_id = game_event_type::tag<fail_consensus_game_start_playing>::value;
            const int fail_consensus_game_result_id = game_event_type::tag<fail_consensus_game_result>::value;
            const int fail_expire_game_start_playing_id = game_event_type::tag<fail_expire_game_start_playing>::value;
            const int fail_expire_game_result_id = game_event_type::tag<fail_expire_game_result>::value;
            const int fail_expire_game_lifetime_id = game_event_type::tag<fail_expire_game_lifetime>::value;
            const int fraud_game_start_playing_check_id = game_event_type::tag<fraud_game_start_playing_check>::value;
            const int fraud_game_result_check_id = game_event_type::tag<fraud_game_result_check>::value;
            const int buy_out_allowed_id = game_event_type::tag<buy_out_allowed>::value;
            const int buy_in_return_id = game_event_type::tag<buy_in_return>::value;
            const int game_cash_return_id = game_event_type::tag<game_cash_return>::value;
            const int fraud_buy_out_id = game_event_type::tag<fraud_buy_out>::value;
            const int fail_vote_id = game_event_type::tag<fail_vote>::value;
        } game_event_type_id;

        const string default_invitation_metadata = "{}";
        const string default_room_metadata = "";
        const string default_protocol_version = "0.0.0+20190223";
        const string default_table_metadata = "{}";
        const string default_room_server = "localhost:8888";

        const int64_t player_init_balance = 13*GRAPHENE_BLOCKCHAIN_PRECISION;

        std::unique_ptr<history_api>  phistory_api;
        std::unique_ptr<database_api> pdatabase_api;
        std::unique_ptr<playchain_api> pplaychain_api;

        playchain_fixture();

        ///////////////////////////////
        /// \brief helpers

        virtual void init_fees();

        ActorActions actor(const Actor& a)
        {
            return ActorActions(*this, a);
        }

        template <typename T> std::string id_to_string(const T &) {
          BOOST_REQUIRE(false);
          return "";
        }

        std::string id_to_string(
            const account_id_type &id) {
          return (std::string)(object_id_type)id;
        }

        std::string id_to_string(const player_id_type &id) {
          return (std::string)(object_id_type)id;
        }

        std::string id_to_string(
            const player_invitation_id_type &id) {
          return (std::string)(object_id_type)id;
        }

        std::string id_to_string(const room_id_type &id) {
          return (std::string)(object_id_type)id;
        }

        std::string id_to_string(const table_id_type &id) {
          return (std::string)(object_id_type)id;
        }

        std::string id_to_string(
            const game_witness_id_type &id) {
          return (std::string)(object_id_type)id;
        }

        std::string id_to_string(const Actor &a);

        string to_string(const asset& a);

        string get_next_uid(const account_id_type &invitor);

        void pending_buy_in_resolve_all(Actor &table_owner, const table_object &table);

        asset get_account_balance(const Actor& account);

        fc::ecc::private_key create_private_key_from_password(const string &password, const string &salt);

        void print_block_operations();

        operation_history_id_type print_last_operations(const account_id_type& who,
                                                        const operation_history_id_type &from = operation_history_id_type());

        operation_history_id_type scroll_history_to_case_start_point(const account_id_type& who);

        bool check_game_event_type(const vector<operation_history_object> &history, const size_t record, const int event_id);

        void next_maintenance();

        ///////////////////////////////
        /// \brief player invitations

        player_invitation_create_operation create_invitation_op(const account_id_type& invitor, uint32_t lifetime_in_sec);

        player_invitation_create_operation create_invitation_op(const Actor& invitor, uint32_t lifetime_in_sec)
        {
            return create_invitation_op(actor(invitor), lifetime_in_sec);
        }

        player_invitation_create_operation create_invitation(const Actor& invitor, uint32_t lifetime_in_sec);

        void create_invitation(const Actor& invitor, uint32_t lifetime_in_sec, player_invitation_create_operation& op);

        player_invitation_cancel_operation cancel_invitation_op(const account_id_type& invitor, const string &uid);

        player_invitation_cancel_operation cancel_invitation_op(const Actor& invitor, const string &uid)
        {
            return cancel_invitation_op(actor(invitor), uid);
        }

        player_invitation_cancel_operation cancel_invitation(const Actor& invitor, const string &uid);

        void cancel_invitation(const Actor& invitor, const string &uid, player_invitation_cancel_operation &op);

        player_invitation_resolve_operation resolve_invitation_op(const account_id_type& invitor, const string &uid, const string &name);

        player_invitation_resolve_operation resolve_invitation_op(const Actor& invitor, const string &uid, const string &name)
        {
            return resolve_invitation_op(actor(invitor), uid, name);
        }

        player_invitation_resolve_operation resolve_invitation(const Actor& invitor, const string &uid, const string &name);

        void resolve_invitation(const Actor& invitor, const string &uid, const string &name, player_invitation_resolve_operation &op);

        Actor create_new_player(const Actor& inviter, const string &name, const asset &supply_amount);

        player_id_type get_player(const Actor& player);

        game_witness_id_type get_witness(const Actor& account);

        ///////////////////////////////
        /// \brief rooms

        room_create_operation create_room_op(const account_id_type& owner, const std::string &metadata = {},
                                             const std::string &protocol_version = {});

        room_create_operation create_room_op(const Actor& owner, const std::string &metadata = {},
                                             const std::string &protocol_version = {})
        {
            return create_room_op(actor(owner), metadata, protocol_version);
        }

        room_create_operation create_room(const Actor& owner, const std::string &metadata = {},
                                          const std::string &protocol_version = {});

        room_update_operation update_room_op(const account_id_type& owner, const room_id_type& room, const std::string &room_server, const std::string &metadata,
                                             const std::string &protocol_version = {});

        room_update_operation update_room_op(const Actor& owner, const room_id_type& room, const std::string &room_server, const std::string &metadata,
                                             const std::string &protocol_version = {})
        {
            return update_room_op(actor(owner), room, room_server, metadata, protocol_version);
        }

        room_update_operation update_room(const Actor& owner, const room_id_type& room, const std::string &room_server, const std::string &room_metadata,
                                          const std::string &protocol_version = {});

        ///////////////////////////////
        /// \brief tables

        table_create_operation create_table_op(const account_id_type& owner, const room_id_type &room, const amount_type = 0u, const std::string &metadata = {});

        table_create_operation create_table_op(const Actor& owner, const room_id_type& room, const amount_type required_witnesses = 0u, const std::string &metadata = {})
        {
            return create_table_op(actor(owner), room, required_witnesses, metadata);
        }

        table_create_operation create_table(const Actor& owner, const room_id_type &room, const amount_type = 0u, const std::string &metadata = {});

        table_update_operation update_table_op(const account_id_type& owner, const table_id_type &table, const amount_type = 0u, const std::string &metadata = {});

        table_update_operation update_table_op(const Actor& owner, const table_id_type& table, const amount_type required_witnesses = 0u, const std::string &metadata = {})
        {
            return update_table_op(actor(owner), table, required_witnesses, metadata);
        }

        table_update_operation update_table(const Actor& owner, const table_id_type &table, const amount_type = 0u, const std::string &metadata = {});

        //
        room_id_type create_new_room(const Actor& owner, const std::string &metadata = {}, const std::string &protocol_version = {});
        table_id_type create_new_table(const Actor& owner, const room_id_type &room, const amount_type = 0u, const std::string &metadata = {});

        const game_witness_object &create_witness(const Actor &account);

        ///////////////////////////////
        /// \brief buy-in/buy-out

        buy_in_table_operation buy_in_table_op(const account_id_type& player, const account_id_type& owner,
                                               const table_id_type &table, const asset &amount);

        buy_in_table_operation buy_in_table_op(const Actor& player, const Actor& owner,
                                               const table_id_type& table, const asset &amount)
        {
            return buy_in_table_op(actor(player), actor(owner), table, amount);
        }

        buy_in_table_operation buy_in_table(const Actor& player, const Actor& owner,
                                            const table_id_type& table, const asset &amount);

        buy_out_table_operation buy_out_table_op(const account_id_type& player, const account_id_type& owner,
                                                 const table_id_type &table, const asset &amount,
                                                 const std::string &reason  = "");

        buy_out_table_operation buy_out_table_op(const Actor& player, const Actor& owner,
                                                 const table_id_type& table, const asset &amount,
                                                 const std::string &reason  = "")
        {
            return buy_out_table_op(actor(player), actor(owner), table, amount, reason);
        }

        buy_out_table_operation buy_out_table(const Actor& player,
                                              const table_id_type& table, const asset &amount,
                                              const std::string &reason  = "");

        buy_out_table_operation buy_out_table(const Actor& player,
                                              const Actor& owner,
                                              const Actor& authority,
                                              const table_id_type& table,
                                              const asset &amount,
                                              const std::string &reason  = "");

        buy_in_reserve_operation buy_in_reserve_op(const account_id_type& player, const string& uid,
                                               const asset &amount, const string &metadata,
                                               const std::string &protocol_version = {});

        buy_in_reserve_operation buy_in_reserve_op(const Actor& player, const string& uid,
                                                   const asset &amount, const string &metadata,
                                                   const std::string &protocol_version = {})
        {
            return buy_in_reserve_op(actor(player), uid, amount, metadata,
                                     protocol_version);
        }

        buy_in_reserve_operation buy_in_reserve(const Actor& player, const string& uid,
                                                const asset &amount, const string &metadata,
                                                const std::string &protocol_version = {});

        buy_in_reserving_cancel_operation buy_in_reserving_cancel_op(const account_id_type& player, const string& uid);

        buy_in_reserving_cancel_operation buy_in_reserving_cancel_op(const Actor& player, const string& uid)
        {
            return buy_in_reserving_cancel_op(actor(player), uid);
        }

        buy_in_reserving_cancel_operation buy_in_reserving_cancel(const Actor& player, const string& uid);

        buy_in_reserving_resolve_operation buy_in_reserving_resolve_op(const account_id_type& owner, const table_id_type& table,
                                                                       const pending_buy_in_id_type& id);

        buy_in_reserving_resolve_operation buy_in_reserving_resolve_op(const Actor& owner, const table_id_type& table, const pending_buy_in_id_type& id)
        {
            return buy_in_reserving_resolve_op(actor(owner), table,  id);
        }

        buy_in_reserving_resolve_operation buy_in_reserving_resolve(const Actor& owner, const table_id_type& table, const pending_buy_in_id_type& id);

        buy_in_reserving_cancel_all_operation buy_in_reserving_cancel_all_op(const account_id_type& player);

        buy_in_reserving_cancel_all_operation buy_in_reserving_cancel_all_op(const Actor& player)
        {
            return buy_in_reserving_cancel_all_op(actor(player));
        }

        buy_in_reserving_cancel_all_operation buy_in_reserving_cancel_all(const Actor& player);

        ///////////////////////////////
        /// \brief voting

        game_start_playing_check_operation game_start_playing_check_op(const account_id_type& voter,
                                                                       const account_id_type& owner,
                                                                       const table_id_type &table,
                                                                       const game_initial_data &initial_data);

        game_start_playing_check_operation game_start_playing_check_op(const Actor& voter,
                                                                       const Actor& owner,
                                                                       const table_id_type &table,
                                                                       const game_initial_data &initial_data)
        {
            return game_start_playing_check_op(actor(voter), actor(owner), table, initial_data);
        }

        game_start_playing_check_operation game_start_playing_check(const Actor& voter,
                                                                    const table_id_type &table,
                                                                    const game_initial_data &initial_data);

        game_result_check_operation  game_result_check_op(const account_id_type& voter,
                                                          const account_id_type& owner,
                                                          const table_id_type &table,
                                                          const game_result &result);

        game_result_check_operation  game_result_check_op(const Actor& voter,
                                                          const Actor& owner,
                                                          const table_id_type &table,
                                                          const game_result &result)
        {
            return  game_result_check_op(actor(voter), actor(owner), table, result);
        }

        game_result_check_operation  game_result_check(const Actor& voter,
                                                       const table_id_type &table,
                                                       const game_result &result);

        game_reset_operation  game_reset_op(const account_id_type& owner,
                                            const table_id_type &table,
                                            const bool rollback_table);

        game_reset_operation  game_reset_op(const Actor& owner,
                                            const table_id_type &table,
                                            const bool rollback_table)
        {
            return  game_reset_op(actor(owner), table, rollback_table);
        }

        game_reset_operation  game_reset(const Actor& owner,
                                         const table_id_type &table,
                                         const bool rollback_table);

        player_create_by_room_owner_operation player_create_by_room_owner_op(const account_id_type& room_owner,
                                                                               const account_id_type& account);

        player_create_by_room_owner_operation player_create_by_room_owner_op(const Actor& room_owner,
                                                                               const Actor& account)
        {
            return player_create_by_room_owner_op(actor(room_owner), actor(account));
        }

        player_create_by_room_owner_operation player_create_by_room_owner(const Actor& room_owner,
                                                                            const account_id_type &account);

        player_create_by_room_owner_operation player_create_by_room_owner(const Actor& room_owner,
                                                                            const Actor &account)
        {
            return player_create_by_room_owner(room_owner, actor(account));
        }

        //
        playchain_committee_member_create_operation playchain_committee_member_create_op(const account_id_type& committee_member_account,
                                                                                         const string& url = {});

        playchain_committee_member_create_operation playchain_committee_member_create_op(const Actor& committee_member_account,
                                                                               const string& url = {})
        {
            return playchain_committee_member_create_op(actor(committee_member_account), url);
        }

        playchain_committee_member_create_operation playchain_committee_member_create(const Actor& committee_member_account,
                                                                            const string& url = {});

        //
        playchain_committee_member_update_operation playchain_committee_member_update_op(const account_id_type& committee_member_account,
                                                                               const playchain_committee_member_id_type& committee_member,
                                                                                         const optional<string>& url = {});

        playchain_committee_member_update_operation playchain_committee_member_update_op(const Actor& committee_member_account,
                                                                               const playchain_committee_member_id_type& committee_member,
                                                                               const optional<string>& url = {})
        {
            return playchain_committee_member_update_op(actor(committee_member_account), committee_member, url);
        }

        playchain_committee_member_update_operation playchain_committee_member_update(const Actor& committee_member_account,
                                                                            const playchain_committee_member_id_type& committee_member,
                                                                            const optional<string>& url = {});

        void vote_for(const vote_id_type &vote_id, const Actor &voter, bool approve = true);

        tables_alive_operation tables_alive_op(const account_id_type& room_owner,
                                              const std::set<table_id_type> &tables);

        tables_alive_operation tables_alive_op(const Actor& room_owner,
                                              const std::set<table_id_type> &tables)
        {
            return tables_alive_op(actor(room_owner), tables);
        }

        tables_alive_operation table_alive(const Actor& room_owner,
                                          const table_id_type &table);

        tables_alive_operation tables_alive(const Actor& room_owner,
                                          const std::set<table_id_type> &tables);

        const playchain_committee_member_object &get_playchain_committee_member(const Actor &member) const;

        bool is_active_playchain_committee_member(const Actor &member) const;

        void vote_for_playchain_committee_member(const Actor &member, const Actor &voter, bool approve = true);

        void approve_proposal(const object_id_type &proposal_id, const Actor &member, bool approve = true);

        void elect_member(const Actor &member);

        struct proposal_info
        {
            proposal_info() = default;

            object_id_type id;
            fc::time_point_sec expiration_time;
        };

        proposal_info propose_playchain_params_change(const Actor &member, const playchain_parameters &new_params,
                                                      bool review = true,
                                                      const fc::microseconds &ext_review = fc::minutes(1));
    };
}

#define CREATE_PLAYER(richregistrator, actor) \
    actor = create_new_player(richregistrator, actor.name, asset(player_init_balance))
