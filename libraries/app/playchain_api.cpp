/*
 * Copyright (c) 2018 Total Games LLC, and contributors.
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

#include <playchain/app/playchain_api.hpp>

#include <playchain/chain/playchain_config.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>

#include <playchain/chain/evaluators/validators.hpp>
#include <playchain/chain/evaluators/db_helpers.hpp>

#include <playchain/chain/schema/pending_buy_in_object.hpp>
#include <playchain/chain/schema/pending_buy_out_object.hpp>
#include <playchain/chain/schema/player_object.hpp>

#include <graphene/utilities/git_revision.hpp>
#include <boost/version.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <websocketpp/version.hpp>

namespace playchain { namespace app {

namespace
{
    template<class T>
    optional<T> maybe_id( const string& name_or_id )
    {
        if( std::isdigit( name_or_id.front() ) )
        {
            try
            {
                return fc::variant(name_or_id, 1).as<T>(1);
            }
            catch (const fc::exception&)
            { // not an ID
            }
        }
        return optional<T>();
    }
}

class playchain_api_impl: public std::enable_shared_from_this<playchain_api_impl>
{
public:
    explicit playchain_api_impl( graphene::chain::database& db ): _db(db)
    {
        dlog("creating playchain API ${x}", ("x",int64_t(this)) );

        _change_connection = _db.changed_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                     on_objects_changed(ids, impacted_accounts);
                                     });
        _new_connection = _db.new_objects.connect([this](const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts) {
                                     on_sub_objects_created(ids, impacted_accounts);
                                     });
        _removed_connection = _db.removed_objects.connect([this](const vector<object_id_type>& ids, const vector<const object*>& removed, const flat_set<account_id_type>& impacted_accounts) {
                                     on_sub_objects_removed(ids, removed, impacted_accounts);
                                     });
    }
    ~playchain_api_impl()
    {
        dlog("freeing playchain API ${x}", ("x",int64_t(this)) );
    }

    playchain_room_info get_room_info(const room_object& room) const
    {
        playchain_room_info info;

        info.id = room.id;
        info.owner = room.owner;
        info.owner_name = room.owner(_db).name;
        info.metadata = room.metadata;
        info.server_url = room.server_url;
        info.rating = room.rating;
        info.protocol_version = room.protocol_version;
        info.last_rating_update = room.last_rating_update;
        info.rake_balance_id = room.balance;
        if (info.rake_balance_id.valid())
        {
            auto &&now = _db.get_dynamic_global_properties().time;
            const vesting_balance_object &vb = (*info.rake_balance_id)(_db);
            info.rake_balance = vb.get_allowed_withdraw(now);
        }

        return info;
    }

    template<typename TableInfo>
    TableInfo get_table_info(const table_object& table) const
    {
        TableInfo info;

        const auto &room = table.room(_db);

        info.id = table.id;
        info.owner = room.owner;
        info.owner_name = room.owner(_db).name;
        info.metadata = table.metadata;
        info.server_url = room.server_url;
        info.required_witnesses = table.required_witnesses;
        info.min_accepted_proposal_asset = table.min_accepted_proposal_asset;

        const auto& voting_by_table = _db.get_index_type<table_voting_index>().indices().get<by_table>();
        auto voting_by_table_it = voting_by_table.find(table.id);
        if (voting_by_table.end() != voting_by_table_it)
        {
            const auto &voting = (*voting_by_table_it);
            if (voting.is_voting_for_playing())
                info.state = playchain_table_state::voting_for_playing;
            else if (voting.is_voting_for_results())
                info.state = playchain_table_state::voting_for_results;
        }
        else if (table.is_playing())
        {
            info.state = playchain_table_state::playing;
        }
        else if (table.is_free())
        {
            info.state = playchain_table_state::free;
        }

        info.alive = is_table_alive(_db, table.id);
        return info;
    }

    playchain_pending_buy_in_proposal_info get_pending_buy_in_proposal_info_by_id(const pending_buy_in_id_type& id, const account_object &account) const
    {
        const pending_buy_in_object &pending_buy_in = id(_db);

        playchain_pending_buy_in_proposal_info info;
        info.name = account.name;
        info.id = pending_buy_in.id;
        info.uid = pending_buy_in.uid;
        info.amount = pending_buy_in.amount;

        return info;
    }

    void extend_table_info(playchain_table_info_ext &info,
                           const table_object& table) const
    {
        if (!table.pending_proposals.empty())
        {
            info.pending_proposals.reserve(table.pending_proposals.size());
            for (const auto &pr: table.pending_proposals)
            {
                const player_id_type &player_id = pr.first;
                const account_object &account = player_id(_db).account(_db);
                info.pending_proposals.emplace(std::make_pair(player_id(_db).account, get_pending_buy_in_proposal_info_by_id(pr.second, account)));
            }
        }
        if (!table.cash.empty())
        {
            info.cash.reserve(table.cash.size());
            for (const auto &pr: table.cash)
            {
                const player_id_type &player_id = pr.first;
                const account_object &account = player_id(_db).account(_db);
                info.cash.emplace(std::make_pair(account.id, player_cash_info{account.name, pr.second}));
            }
        }
        if (!table.playing_cash.empty())
        {
            info.playing_cash.reserve(table.playing_cash.size());
            for (const auto &pr: table.playing_cash)
            {
                const player_id_type &player_id = pr.first;
                const account_object &account = player_id(_db).account(_db);
                info.playing_cash.emplace(std::make_pair(account.id, player_cash_info{account.name, pr.second}));
            }
        }

        const auto& idx = _db.get_index_type<table_voting_index>();
        const auto& idx_impl = dynamic_cast<const base_primary_index&>(idx);
        const auto& second_index = idx_impl.get_secondary_index<table_voting_statistics_index>();

        auto it = second_index.not_voted_last_time_players_by_table.find(table.id);
        if (second_index.not_voted_last_time_players_by_table.end() != it)
        {
            auto &&not_voted_last_time_players_by_table = it->second;
            info.missed_voters.reserve(not_voted_last_time_players_by_table.size());

            for (const auto &account_id: not_voted_last_time_players_by_table)
            {
                const account_object &account = account_id(_db);
                info.missed_voters.emplace(std::make_pair(account_id, account.name));
            }
        }

        it = second_index.voted_last_time_players_by_table.find(table.id);
        if (second_index.voted_last_time_players_by_table.end() != it)
        {
            auto &&voted_last_time_players_by_table = it->second;
            info.voters.reserve(voted_last_time_players_by_table.size());

            for (const auto &account_id: voted_last_time_players_by_table)
            {
                const account_object &account = account_id(_db);
                info.voters.emplace(std::make_pair(account_id, account.name));
            }
        }
    }

    playchain_table_info get_table_info_by_id(const table_id_type& id) const
    {
        auto &&table = id(_db);

        return get_table_info<playchain_table_info>(table);
    }

    playchain_table_info_ext get_table_info_ext_by_id(const table_id_type& id) const
    {
        auto &&table = id(_db);

        playchain_table_info_ext info = get_table_info<playchain_table_info_ext>(table);
        extend_table_info(info, table);
        return info;
    }

    optional<table_id_type> find_subscribe_table (const object_id_type &id)
    {
        table_id_type table_id;

        if (id.is<table_object>())
        {
            table_id = table_id_type{id};
        }
        else
        {
            return {};
        }

        auto it = _subscribe_tables_filter.find(table_id);
        if (it == _subscribe_tables_filter.end())
            return {};

        if (!playchain::chain::is_table_exists(_db, table_id))
            return {};

        return {std::move(table_id)};
    }

    void on_sub_objects_created(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
    {
        if( _subscribe_tables_callback )
        {
           vector<object_id_type> updates;
           updates.resize(ids.size());

           size_t ci = 0;
           for(auto id : ids)
           {
               if (id.is<table_voting_object>())
               {
                   const table_voting_object& obj =  table_voting_id_type{id}(_db);

                   updates.emplace_back( obj.table );
               }
               ci++;
           }

           if (!updates.empty())
               on_objects_changed(updates, impacted_accounts);
        }
    }

    void on_sub_objects_removed(const vector<object_id_type>& ids, const vector<const object*>& removed, const flat_set<account_id_type>& impacted_accounts)
    {
        if( _subscribe_tables_callback )
        {
           vector<object_id_type> updates;
           updates.resize(ids.size());

           size_t ci = 0;
           for(auto id : ids)
           {
               if (id.is<table_voting_object>())
               {
                   const table_voting_object* p = dynamic_cast<const table_voting_object *>(removed.at(ci));
                   if (p)
                   {
                       updates.emplace_back( p->table );
                   }
               }
               ci++;
           }

           if (!updates.empty())
               on_objects_changed(updates, impacted_accounts);
        }
    }

    void on_objects_changed(const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts)
    {
       handle_table_object_changed(ids, impacted_accounts,
          std::bind(&playchain_api_impl::find_subscribe_table, this, std::placeholders::_1)
       );
    }

    void broadcast_updates( const vector<table_id_type>& ids )
    {
       if( ids.size() && _subscribe_tables_callback ) {
          auto self = shared_from_this();
          fc::async([self, ids](){
              if(self && self->_subscribe_tables_callback)
              {
                  vector<fc::variant> updates;
                  updates.reserve(ids.size());
                  for(const table_id_type &id: ids)
                  {
                      updates.emplace_back(self->get_table_info_ext_by_id(id), GRAPHENE_MAX_NESTED_OBJECTS);
                  }
                  self->_subscribe_tables_callback(fc::variant(updates));
              }
          });
       }
    }

    void handle_table_object_changed(const vector<object_id_type>& ids,
                               const flat_set<account_id_type>& ,
                               std::function<optional<table_id_type> (const object_id_type &id)> find_table)
    {
        if( _subscribe_tables_callback )
        {
           vector<table_id_type> updates;

           for(auto id : ids)
           {
               auto obj = find_table(id);
               if( obj.valid() )
               {
                  updates.emplace_back( *obj );
               }
           }

           if( updates.size() )
              broadcast_updates(updates);
        }
    }

    template<typename TObject,
             typename TIndexById,
             typename TObjectIdType>
    vector< TObject > list_all_impl(
                        const string &last_page_id,
                        uint32_t limit,
                        int last_special_id_instance = -1) const
    {
        check_limit(limit);

        optional<TObjectIdType> opt_id = maybe_id<TObjectIdType>(last_page_id);
        FC_ASSERT(last_page_id.empty() || opt_id.valid(), "Invalid ID");

        const auto& objects_by_id = _db.get_index_type<TIndexById>().indices().template get<by_id>();
        vector<TObject> result;
        result.reserve(limit);

        auto itr = objects_by_id.begin();

        if (opt_id.valid())
        {
            itr = objects_by_id.lower_bound(*opt_id);
            if (itr != objects_by_id.end())
                ++itr;
        }

        while(limit-- && itr != objects_by_id.end())
        {
            const TObject &obj = *itr++;
            if (last_special_id_instance >= 0 &&
                (uint64_t)(obj.id.instance())
                    <= (uint64_t)last_special_id_instance)
            {
                ++limit;
                continue;
            }
            result.emplace_back(obj);
        }

        return result;
    }

    player_invitation_objects_list_with_blockchain_time list_player_invitations(const string &inviter_name_or_id,
                                                               const string &last_page_uid,
                                                               uint32_t limit) const
    {
        check_limit(limit);

        account_id_type id = get_account_id(inviter_name_or_id);

        const auto& invitations_by_uid = _db.get_index_type<playchain::chain::player_invitation_index>().indices().get<by_player_invitation_uid>();
        player_invitation_objects_list_with_blockchain_time result =
                std::make_pair(vector<player_invitation_object>{}, _db.get_dynamic_global_properties().time);
        vector<player_invitation_object> &result_list = result.first;
        result_list.reserve(limit);

        auto itr = invitations_by_uid.begin();

        if( last_page_uid.empty() )
            itr = invitations_by_uid.lower_bound(std::make_tuple(id, ""));
        else
        {
            itr = invitations_by_uid.lower_bound(std::make_tuple(id, last_page_uid));
            if (itr != invitations_by_uid.end())
                ++itr;
        }

        while(limit-- && itr != invitations_by_uid.end())
        {
            const auto &obj = *itr++;
            if (obj.inviter != id)
            {
                ++limit;
                continue;
            }
           result_list.emplace_back(obj);
        }

        return result;
    }

    optional<player_invitation_object_with_blockchain_time> get_player_invitation(
                                                  const string &inviter_name_or_id,
                                                  const string &uid) const

    {
        account_id_type id = get_account_id(inviter_name_or_id);

        const auto& invitations_by_uid = _db.get_index_type<playchain::chain::player_invitation_index>().indices().get<by_player_invitation_uid>();

        auto itr = invitations_by_uid.find(std::make_tuple(id, uid));
        if (itr == invitations_by_uid.end())
            return {};

        return std::make_pair(*itr, _db.get_dynamic_global_properties().time);
    }

    player_invitation_objects_list_with_blockchain_time list_all_player_invitations(
                                                        const string &last_page_id,
                                                        uint32_t limit) const
    {
        player_invitation_objects_list_with_blockchain_time result =
                std::make_pair(vector<player_invitation_object>{}, _db.get_dynamic_global_properties().time);
        vector<player_invitation_object> &result_list = result.first;

        result_list = list_all_impl<player_invitation_object, player_invitation_index, player_invitation_id_type>(last_page_id, limit);
        return result;
    }

    vector< std::pair<player_object, account_object> > list_invited_players(
                                                        const string &inviter_name_or_id,
                                                        const string &last_page_id,
                                                        uint32_t limit) const
    {
        check_limit(limit);

        optional<player_id_type> opt_id = get_player_id(inviter_name_or_id);

        FC_ASSERT(opt_id.valid(), "Inviter is not player");

        player_id_type next_player_id;

        if (!last_page_id.empty())
            next_player_id = (*get_player_id(last_page_id));

        const auto& players_by_inviter = _db.get_index_type<playchain::chain::player_index>().indices().get<by_inviter>();
        vector< std::pair<player_object, account_object> > result;
        result.reserve(limit);

        auto itr = players_by_inviter.lower_bound(std::make_tuple(*opt_id, next_player_id));

        if( next_player_id != player_id_type{} &&
                itr != players_by_inviter.end())
        {
            ++itr;
        }

        while(limit-- && itr != players_by_inviter.end())
        {
            const auto &obj = *itr++;
            if (obj.inviter != *opt_id)
            {
                ++limit;
                continue;
            }
           result.emplace_back(std::make_pair(obj, obj.account(_db)));
        }

        return result;
    }

    optional< player_object > get_player(const string &account_name_or_id) const
    {
        optional<player_id_type> opt_id = get_player_id(account_name_or_id);

        if (!opt_id.valid())
            return {};

        return _db.get(*opt_id);
    }

    vector< player_object > list_all_players(const string &last_page_id,
                                             uint32_t limit) const
    {
        return list_all_impl<player_object, player_index, player_id_type>(last_page_id, limit,
                                                                          PLAYCHAIN_NULL_PLAYER.instance);
    }

    vector< room_object > list_rooms(const string &owner_name_or_id,
                                     const string &last_page_id,
                                     uint32_t limit) const
    {
        check_limit(limit);

        account_id_type id = get_account_id(owner_name_or_id);

        optional<room_id_type> opt_id = maybe_id<room_id_type>(last_page_id);
        FC_ASSERT(last_page_id.empty() || opt_id.valid(), "Invalid ID");

        const auto& rooms_by_owner = _db.get_index_type<room_index>().indices().get<by_room_owner_and_id>();
        vector<room_object> result;
        result.reserve(limit);

        auto itr = rooms_by_owner.begin();

        if( last_page_id.empty() )
            itr = rooms_by_owner.lower_bound(std::make_tuple(id, room_id_type{}));
        else
        {
            itr = rooms_by_owner.lower_bound(std::make_tuple(id, *opt_id));
            if (itr != rooms_by_owner.end())
                ++itr;
        }

        while(limit-- && itr != rooms_by_owner.end())
        {
            const auto &obj = *itr++;
            if (obj.owner != id)
            {
                ++limit;
                continue;
            }
            result.emplace_back(obj);
        }

        return result;
    }

    vector< room_object > list_all_rooms(
                                     const string &last_page_id,
                                     uint32_t limit) const
    {
        return list_all_impl<room_object, room_index, room_id_type>(last_page_id, limit,
                                                                    PLAYCHAIN_NULL_ROOM.instance);
    }

    vector< playchain_room_info > get_rooms_info_by_id(const flat_set<room_id_type>& ids) const
    {
        check_limit(ids.size());

        vector<playchain_room_info> result;
        result.reserve(ids.size());

        for(const room_id_type &id: ids)
        {
            if (!playchain::chain::is_room_exists(_db, id))
                continue;

            playchain_room_info info = get_room_info(id(_db));

            result.emplace_back(std::move(info));
        }

        return result;
    }

    optional< playchain_room_info > get_room_info(const string &owner_name_or_id, const string &metadata) const
    {
        account_id_type owner = get_account_id(owner_name_or_id);

        const auto& rooms_by_owner_and_metadata = _db.get_index_type<room_index>().indices().get<by_room_owner_and_metadata>();
        auto itr = rooms_by_owner_and_metadata.find(boost::make_tuple(owner, metadata));
        if( itr == rooms_by_owner_and_metadata.end() )
            return {};

        return get_room_info(*itr);
    }

    optional< game_witness_object > get_game_witness(const string &account_name_or_id) const
    {
        optional<game_witness_id_type> opt_id = get_game_witness_id(account_name_or_id);

        if (!opt_id.valid())
            return {};

        return _db.get(*opt_id);
    }

    vector< game_witness_object > list_all_game_witnesses(
                                     const string &last_page_id,
                                     uint32_t limit) const
    {
        return list_all_impl<game_witness_object, game_witness_index, game_witness_id_type>(last_page_id, limit,
                                                                                            PLAYCHAIN_NULL_GAME_WITNESS.instance);
    }

    vector<playchain_table_info> get_tables_info_by_metadata(const string &room_id_name,
                                                     const string &metadata,
                                                     uint32_t limit) const
    {
        check_limit(limit);

        vector<playchain_table_info> result;

        result.reserve(limit);

        optional<room_id_type> room_id = maybe_id<room_id_type>(room_id_name);
        FC_ASSERT(room_id.valid());

        auto table_range = _db.get_index_type<table_index>().indices().get<by_room_and_metadata>().equal_range(std::make_tuple(*room_id, metadata));
        for (auto itr = table_range.first; itr !=table_range.second && limit--;)
        {
            const auto &table = *itr++;
            playchain_table_info info = get_table_info_by_id(table.id);
            result.emplace_back(std::move(info));
        }

        return result;
    }

    vector< table_object > list_tables(const string &owner_name_or_room_id,
                                      const string &last_page_id,
                                      uint32_t limit) const
    {
        check_limit(limit);

        vector<table_object> result;

        optional<table_id_type> opt_id = maybe_id<table_id_type>(last_page_id);
        FC_ASSERT(last_page_id.empty() || opt_id.valid(), "Invalid ID");

        result.reserve(limit);

        optional<room_id_type> room_id = maybe_id<room_id_type>(owner_name_or_room_id);
        if (!room_id.valid())
        {
            account_id_type account_id = get_account_id(owner_name_or_room_id);

            const auto& idx = _db.get_index_type<table_index>();
            const auto& idx_impl = dynamic_cast<const base_primary_index&>(idx);
            const auto& second_index = idx_impl.get_secondary_index<table_owner_index>();

            auto it = second_index.tables_by_owner.find(account_id);
            if (second_index.tables_by_owner.end() == it)
                return {};

            const table_owner_index::tables_type &tables = it->second;

            auto itr = tables.begin();

            if( !last_page_id.empty() )
            {
                itr = tables.lower_bound(*opt_id);
                if (itr != tables.end())
                    ++itr;
            }

            while(limit-- && itr != tables.end())
            {
                auto table_id = *itr++;
                result.emplace_back(table_id(_db));
            }
        }else
        {
            const auto &id = *room_id;

            const auto& tables_by_room = _db.get_index_type<table_index>().indices().get<by_room>();

            auto itr = tables_by_room.begin();

            if( last_page_id.empty() )
                itr = tables_by_room.lower_bound(std::make_tuple(id, table_id_type{}));
            else
            {
                itr = tables_by_room.lower_bound(std::make_tuple(id, *opt_id));
                if (itr != tables_by_room.end())
                    ++itr;
            }

            while(limit-- && itr != tables_by_room.end())
            {
                const auto &obj = *itr++;
                if (obj.room != id)
                {
                    ++limit;
                    continue;
                }
               result.emplace_back(obj);
            }
        }

        return result;
    }

    vector< table_object > list_all_tables(
                                     const string &last_page_id,
                                     uint32_t limit) const
    {
        return list_all_impl<table_object, table_index, table_id_type>(last_page_id, limit,
                                                                       PLAYCHAIN_NULL_TABLE.instance);
    }

    block_header get_last_irreversible_block_header() const
    {
        const auto &dgp_object = _db.get_dynamic_global_properties();
        auto last_block = dgp_object.last_irreversible_block_num;
        FC_ASSERT(last_block > 0, "There is not blocks in history");
        auto result = _db.fetch_block_by_number(last_block);
        FC_ASSERT(result.valid(), "Block history is inaccessible");
        return *result;
    }

    playchain_member_balance_info get_playchain_balance_info(const string &name_or_id) const
    {
        account_id_type id = get_account_id(name_or_id);

        playchain_member_balance_info result;

        result.account_balance  = _db.get_balance(id, asset_id_type()).amount;

        auto &&now = _db.get_dynamic_global_properties().time;

        const auto& player_by_account = _db.get_index_type<player_index>().indices().get<by_playchain_account>();
        auto player_by_account_it = player_by_account.find(id);
        if (player_by_account.end() != player_by_account_it)
        {
            const auto &player = (*player_by_account_it);
            if (player.balance.valid())
            {
                const vesting_balance_object &vb = (*player.balance)(_db);
                result.referral_balance = vb.get_allowed_withdraw(now);
                result.referral_balance_id = vb.id;
            }
        }

        const auto& witness_by_account = _db.get_index_type<game_witness_index>().indices().get<by_playchain_account>();
        auto witness_by_account_it = witness_by_account.find(id);
        if (witness_by_account.end() != witness_by_account_it)
        {
            const auto &witness = (*witness_by_account_it);
            if (witness.balance.valid())
            {
                const vesting_balance_object &vb = (*witness.balance)(_db);
                result.witness_balance = vb.get_allowed_withdraw(now);
                result.witness_balance_id = vb.id;
            }
        }

        const auto& rooms_by_owner = _db.get_index_type<room_index>().indices().get<by_room_owner>();
        auto rooms_range = rooms_by_owner.equal_range(id);
        for( const room_object& room: boost::make_iterator_range( rooms_range.first, rooms_range.second ) )
        {
            if (room.balance.valid())
            {
                const vesting_balance_object &vb = (*room.balance)(_db);
                result.rake_balance += vb.get_allowed_withdraw(now);
                result.rake_balance_id = vb.id;
            }
        }

        return result;
    }

    flat_map<string, account_id_type> get_account_id_by_name(const flat_set<string> &names) const
    {
        check_limit(names.size());

        flat_map<string, account_id_type> result;
        result.reserve(names.size());

        for (const string &name: names)
        {
            auto opt_account_object = get_account_by_name(name);
            if (!opt_account_object.valid())
                continue;

            result.emplace(name, opt_account_object->id);
        }

        return result;
    }

    vector<playchain_table_info_ext> get_tables_info_by_id(const flat_set<table_id_type>& ids) const
    {
        check_limit(ids.size());

        vector<playchain_table_info_ext> result;
        result.reserve(ids.size());

        for(const table_id_type &id: ids)
        {
            if (!playchain::chain::is_table_exists(_db, id))
                continue;

            playchain_table_info_ext info = get_table_info_ext_by_id(id);

            result.emplace_back(std::move(info));
        }

        return result;
    }

    void set_tables_subscribe_callback( std::function<void(const variant&)> cb, const flat_set<table_id_type>& ids )
    {
        check_limit(ids.size());

        _subscribe_tables_filter = ids;
        _subscribe_tables_callback = cb;
    }

    void cancel_tables_subscribe_callback( const flat_set<table_id_type>& ids )
    {
        if (!_subscribe_tables_filter.empty())
        {
            decltype (_subscribe_tables_filter) diff;
            std::set_difference(begin(_subscribe_tables_filter), end(_subscribe_tables_filter), begin(ids), end(ids),
                                    std::inserter(diff, diff.begin()));
            _subscribe_tables_filter = diff;
        }
    }

    void cancel_all_tables_subscribe_callback( )
    {
        _subscribe_tables_filter.clear();
        _subscribe_tables_callback = {};
    }

    optional< playchain_table_info > get_table_info_for_pending_buy_in_proposal(const string &name_or_id, const string &uid) const
    {
        FC_ASSERT(!uid.empty(), "UID is empty");

        account_id_type id = get_account_id(name_or_id);

        const auto &index_by_uid = _db.get_index_type<pending_buy_in_index>().indices().get<by_pending_buy_in_uid>();
        auto it = index_by_uid.find(std::make_tuple(id, uid));

        FC_ASSERT(it != index_by_uid.end(), "Proposal is not found. Check it for expiration. Client Code = 20180012");

        const pending_buy_in_object &proposal = (*it);

        if (!proposal.is_allocated())
            return {};

        return get_table_info_by_id(proposal.table);
    }

    playchain_property_object get_playchain_properties() const
    {
        return _db.get(playchain_property_id_type());
    }

    vector< playchain_player_table_info > list_tables_with_player(const string &name_or_id, uint32_t limit) const
    {
        check_limit(limit);

        vector<playchain_player_table_info> result;

        result.reserve(limit);

        optional<player_id_type> player_id = get_player_id(name_or_id);

        FC_ASSERT(player_id.valid(), "Invalid player");

        const auto& idx = _db.get_index_type<table_index>();
        const auto& idx_impl = dynamic_cast<const base_primary_index&>(idx);
        const auto& second_index = idx_impl.get_secondary_index<table_players_index>();

        {
            auto it = second_index.tables_with_playing_cash_by_player.find(*player_id);
            if (second_index.tables_with_playing_cash_by_player.end() != it)
            {
                auto && tables = it->second;
                for(const auto &table: tables)
                {
                    if (!(limit--))
                        return result;
                    const table_object &table_object = table(_db);

                    asset buyouting_balance;
                    const auto &index_buyouting = _db.get_index_type<pending_buy_out_index>().indices().get<by_player_at_table>();
                    auto it = index_buyouting.find(std::make_tuple(*player_id, table));
                    if (it != index_buyouting.end())
                    {
                        buyouting_balance = it->amount;
                    }

                    result.emplace_back(playchain_player_table_state::playing,
                                        table_object.get_playing_cash_balance(*player_id), buyouting_balance,
                                        std::move(get_table_info<playchain_table_info>(table_object)));
                }
            }
        }

        {
            auto it = second_index.tables_with_cash_by_player.find(*player_id);
            if (second_index.tables_with_cash_by_player.end() != it)
            {
                auto && tables = it->second;
                for(const auto &table: tables)
                {
                    if (!(limit--))
                        return result;
                    const table_object &table_object = table(_db);
                    result.emplace_back(playchain_player_table_state::attable,
                                        table_object.get_cash_balance(*player_id), asset{},
                                        std::move(get_table_info<playchain_table_info>(table_object)));
                }
            }
        }

        {
            auto it = second_index.tables_with_pending_proposals_by_player.find(*player_id);
            if (second_index.tables_with_pending_proposals_by_player.end() != it)
            {
                auto && tables = it->second;
                for(const auto &table: tables)
                {
                    if (!(limit--))
                        return result;
                    const table_object &table_object = table(_db);
                    result.emplace_back(playchain_player_table_state::pending,
                                        table_object.get_pending_balance(_db, *player_id), asset{},
                                        std::move(get_table_info<playchain_table_info>(table_object)));
                }
            }
        }

        return result;
    }

    optional< playchain_account_info > get_player_account_by_name(const string &name) const
    {
        const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
        auto itr = idx.find(name);
        if (itr == idx.end())
            return {};

        const auto &account = *itr;

        if (!playchain::chain::is_player_exists(_db, account_id_type{account.id}))
            return {};

        const auto& player = playchain::chain::get_player(_db, account.id);

        playchain_account_info info;

        FC_ASSERT(!account.active.key_auths.empty());

        info.account = account.id;
        info.login_key = account.active.key_auths.begin()->first;
        info.player = player.id;
        return {info};
    }

    version_info get_version() const
    {
        version_info ret;

        ret.playchain_ver = fc::string(PLAYCHAIN_VERSION);
        ret.bitshares_ver = GRAPHENE_CURRENT_DB_VERSION;
        ret.git_revision_description = graphene::utilities::git_revision_description;
        ret.git_revision_sha = graphene::utilities::git_revision_sha;
        ret.git_revision_unix_timestamp = fc::get_approximate_relative_time_string(fc::time_point_sec(graphene::utilities::git_revision_unix_timestamp));
        ret.openssl_ver = OPENSSL_VERSION_TEXT;
        ret.boost_ver = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
        {
            std::stringstream ss;
            ss << websocketpp::major_version << "." << websocketpp::minor_version << "." << websocketpp::patch_version << "\n";
            ret.websocket_ver = ss.str();
        }

        return ret;
    }

    vector<optional<playchain_committee_member_object>> get_committee_members(const vector<playchain_committee_member_id_type>& committee_member_ids) const
    {
        vector<optional<playchain_committee_member_object>> result; result.reserve(committee_member_ids.size());
        std::transform(committee_member_ids.begin(), committee_member_ids.end(), std::back_inserter(result),
                       [this](playchain_committee_member_id_type id) -> optional<playchain_committee_member_object> {
           if(auto o = _db.find(id))
              return *o;
           return {};
        });
        return result;
    }

    optional<playchain_committee_member_object> get_committee_member_by_account(const string &account_name_or_id) const
    {
        account_id_type account_id = get_account_id(account_name_or_id);

        game_witness_id_type witness;

        {
            const auto& idx = _db.get_index_type<game_witness_index>().indices().get<by_playchain_account>();
            auto itr = idx.find(account_id);
            if( itr == idx.end() )
                return {};

            witness = (*itr).id;
        }

        {
            const auto& idx = _db.get_index_type<playchain_committee_member_index>().indices().get<by_game_witness>();
            auto itr = idx.find(witness);
            if( itr != idx.end() )
               return *itr;
        }

        return {};
    }

    vector< playchain_committee_member_object > list_all_committee_members(
                                     const string &last_page_id,
                                     uint32_t limit) const
    {
        return list_all_impl<playchain_committee_member_object, playchain_committee_member_index, playchain_committee_member_id_type>(last_page_id, limit);
    }

private:

    template<typename T>
    void check_limit(const T limit) const
    {
        FC_ASSERT( limit <= PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST, "Limit can't be greater than ${l}", ("l", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST));
    }

    account_id_type get_account_id(const string &account_name_or_id) const
    {
        optional<account_id_type> opt_account_id;
        optional<player_id_type> opt_player_id = maybe_id<player_id_type>(account_name_or_id);
        if (opt_player_id.valid())
        {
            FC_ASSERT(!player_object::is_special_player(*opt_player_id), "Invalid player");
            FC_ASSERT(is_player_exists(_db, *opt_player_id), "Player doesn't exist");

            opt_account_id = (*opt_player_id)(_db).account;
        }
        else
        {
            opt_account_id = maybe_id<account_id_type>(account_name_or_id);
            if (!opt_account_id.valid())
            {
                auto opt_account_object = get_account_by_name(account_name_or_id);
                FC_ASSERT(opt_account_object.valid(), "Account not found");

                opt_account_id = (*opt_account_object).id;
            }
        }

        if (opt_account_id.valid())
        {
            FC_ASSERT(!account_object::is_special_account(*opt_account_id), "Invalid account");
            FC_ASSERT(is_account_exists(_db, *opt_account_id), "Account doesn't exist");
        }

        FC_ASSERT(opt_account_id.valid(), "Account not found");

        return (*opt_account_id);
    }

    optional< player_id_type > get_player_id(const string &account_name_or_id) const
    {
        optional<player_id_type> opt_player_id = maybe_id<player_id_type>(account_name_or_id);
        if (opt_player_id.valid())
        {
            FC_ASSERT(!player_object::is_special_player(*opt_player_id), "Invalid player");
            FC_ASSERT(is_player_exists(_db, *opt_player_id), "Player doesn't exist");

            return *opt_player_id;
        }

        account_id_type account_id = get_account_id(account_name_or_id);

        const auto& idx = _db.get_index_type<playchain::chain::player_index>().indices().get<by_playchain_account>();
        auto it = idx.find(account_id);
        if (idx.end() == it)
            return {};

        return it->id;
    }

    optional< game_witness_id_type > get_game_witness_id(const string &account_name_or_id) const
    {
        optional<game_witness_id_type> opt_game_witness_id = maybe_id<game_witness_id_type>(account_name_or_id);
        if (opt_game_witness_id.valid())
        {
            FC_ASSERT(!game_witness_object::is_special_witness(*opt_game_witness_id), "Invalid witness");
            FC_ASSERT(is_game_witness_exists(_db, *opt_game_witness_id), "Game witness doesn't exist");

            return *opt_game_witness_id;
        }

        account_id_type account_id = get_account_id(account_name_or_id);

        const auto& idx = _db.get_index_type<playchain::chain::game_witness_index>().indices().get<by_playchain_account>();
        auto it = idx.find(account_id);
        if (idx.end() == it)
            return {};

        return it->id;
    }

    optional< account_object > get_account_by_name( const std::string &name ) const
    {
       const auto& idx = _db.get_index_type<account_index>().indices().get<by_name>();
       auto itr = idx.find(name);
       if (itr != idx.end())
          return *itr;
       return optional<account_object>();
    }

    graphene::chain::database&  _db;

    boost::signals2::scoped_connection _change_connection;
    boost::signals2::scoped_connection _new_connection;
    boost::signals2::scoped_connection _removed_connection;
    flat_set<table_id_type> _subscribe_tables_filter;
    std::function<void(const variant&)> _subscribe_tables_callback;
};

playchain_api::playchain_api(graphene::chain::database& db):
    _impl( new playchain_api_impl( db ) )
{
}
playchain_api::~playchain_api(){}

player_invitation_objects_list_with_blockchain_time playchain_api::list_player_invitations(
                                                    const string &inviter_name_or_id,
                                                    const string &last_page_uid,
                                                    const uint32_t limit) const
{
    return _impl->list_player_invitations(inviter_name_or_id, last_page_uid, limit);
}

optional< player_invitation_object_with_blockchain_time > playchain_api::get_player_invitation(
                                              const string &inviter_name_or_id,
                                              const string &uid) const

{
    return _impl->get_player_invitation(inviter_name_or_id, uid);
}

player_invitation_objects_list_with_blockchain_time playchain_api::list_all_player_invitations(
                                                    const string &last_page_id,
                                                    const uint32_t limit) const
{
    return _impl->list_all_player_invitations(last_page_id, limit);
}

vector< std::pair<player_object, account_object> > playchain_api::list_invited_players(
                                                    const string &inviter_name_or_id,
                                                    const string &last_page_id,
                                                    const uint32_t limit) const
{
    return _impl->list_invited_players(inviter_name_or_id, last_page_id, limit);
}

optional< player_object > playchain_api::get_player(const string &account_name_or_id) const
{
    return _impl->get_player(account_name_or_id);
}

vector< player_object > playchain_api::list_all_players(const string &last_page_id,
                                         const uint32_t limit) const
{
    return _impl->list_all_players(last_page_id, limit);
}

vector< room_object > playchain_api::list_rooms(const string &owner_name_or_id,
                                 const string &last_page_id,
                                 const uint32_t limit) const
{
    return _impl->list_rooms(owner_name_or_id, last_page_id, limit);
}

vector< room_object > playchain_api::list_all_rooms(
                                 const string &last_page_id,
                                 const uint32_t limit) const
{
    return _impl->list_all_rooms(last_page_id, limit);
}

vector< playchain_room_info > playchain_api::get_rooms_info_by_id(const flat_set<room_id_type>& ids) const
{
    return _impl->get_rooms_info_by_id(ids);
}

optional< playchain_room_info > playchain_api::get_room_info(const string &owner_name_or_id, const string &metadata) const
{
    return _impl->get_room_info(owner_name_or_id, metadata);
}

optional< game_witness_object > playchain_api::get_game_witness(const string &account_name_or_id) const
{
    return _impl->get_game_witness(account_name_or_id);
}

vector< game_witness_object > playchain_api::list_all_game_witnesses(
                                 const string &last_page_id,
                                 const uint32_t limit) const
{
    return _impl->list_all_game_witnesses(last_page_id, limit);
}

vector<optional<playchain_committee_member_object>> playchain_api::get_committee_members(const vector<playchain_committee_member_id_type>& committee_member_ids) const
{
    return _impl->get_committee_members(committee_member_ids);
}

optional<playchain_committee_member_object> playchain_api::get_committee_member_by_account(const string &account_name_or_id) const
{
    return _impl->get_committee_member_by_account(account_name_or_id);
}

vector< playchain_committee_member_object > playchain_api::list_all_committee_members(
                                 const string &last_page_id,
                                 const uint32_t limit) const
{
    return _impl->list_all_committee_members(last_page_id, limit);
}

vector< table_object > playchain_api::list_tables(const string &owner_name_or_room_id,
                                  const string &last_page_id,
                                  const uint32_t limit) const
{
    return _impl->list_tables(owner_name_or_room_id, last_page_id, limit);
}

vector<playchain_table_info> playchain_api::get_tables_info_by_metadata(const string &room_id,
                                                 const string &metadata,
                                                 const uint32_t limit) const
{
    return _impl->get_tables_info_by_metadata(room_id, metadata, limit);
}

vector< table_object > playchain_api::list_all_tables(
                                 const string &last_page_id,
                                 const uint32_t limit) const
{
    return _impl->list_all_tables(last_page_id, limit);
}

block_header playchain_api::get_last_irreversible_block_header() const
{
    return _impl->get_last_irreversible_block_header();
}

playchain_member_balance_info playchain_api::get_playchain_balance_info(const string &name_or_id) const
{
    return _impl->get_playchain_balance_info(name_or_id);
}

flat_map<string, account_id_type> playchain_api::get_account_id_by_name(const flat_set<string> &names) const
{
    return _impl->get_account_id_by_name(names);
}

vector<playchain_table_info_ext> playchain_api::get_tables_info_by_id(const flat_set<table_id_type>& ids) const
{
    return _impl->get_tables_info_by_id(ids);
}

void playchain_api::set_tables_subscribe_callback( std::function<void(const variant&)> cb, const flat_set<table_id_type>& ids )
{
    _impl->set_tables_subscribe_callback(cb, ids);
}

void playchain_api::cancel_tables_subscribe_callback( const flat_set<table_id_type>& ids )
{
    _impl->cancel_tables_subscribe_callback(ids);
}

void playchain_api::cancel_all_tables_subscribe_callback( )
{
    _impl->cancel_all_tables_subscribe_callback();
}

optional< playchain_table_info > playchain_api::get_table_info_for_pending_buy_in_proposal(const string &name_or_id, const string &uid) const
{
    return _impl->get_table_info_for_pending_buy_in_proposal(name_or_id, uid);
}

playchain_property_object playchain_api::get_playchain_properties() const
{
    return _impl->get_playchain_properties();
}

vector< playchain_player_table_info > playchain_api::list_tables_with_player(const string &name_or_id, const uint32_t limit) const
{
    return _impl->list_tables_with_player(name_or_id, limit);
}

optional< playchain_account_info > playchain_api::get_player_account_by_name(const string &name) const
{
    return _impl->get_player_account_by_name(name);
}

version_info playchain_api::get_version() const
{
    return _impl->get_version();
}
}}
