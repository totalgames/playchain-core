#include "playchain_common.hpp"

#include <playchain/chain/playchain_utils.hpp>

#include <playchain/chain/schema/player_object.hpp>
#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>
#include <playchain/chain/schema/game_witness_object.hpp>
#include <playchain/chain/schema/playchain_committee_member_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/validators.hpp>

#include <algorithm>

namespace playchain_common
{

std::string get_operation_name::name_from_type(const std::string& type_name) const
{
    auto start = type_name.find_last_of(':') + 1;
    auto end = type_name.find_last_of('_');
    return type_name.substr(start, end - start);
}

playchain_fixture::playchain_fixture()
{
    phistory_api.reset(new history_api(app));
    pdatabase_api.reset(new database_api(db));
    pplaychain_api.reset(new playchain_api(db));
}

void playchain_fixture::init_fees()
{
    enable_fees();

    flat_set< fee_parameters > fees = db.get_global_properties().parameters.get_current_fees().parameters;

    transfer_operation::fee_parameters_type transfer_params;
    transfer_params.fee = 0;
    transfer_params.price_per_kbyte = 0;
    fees.erase(transfer_params);
    fees.insert(transfer_params);

    player_invitation_create_operation::fee_parameters_type player_invitation_create_params;
    player_invitation_create_params.fee = 2 * GRAPHENE_BLOCKCHAIN_PRECISION;
    player_invitation_create_params.price_per_kbyte = 0;
    fees.erase(player_invitation_create_params);
    fees.insert(player_invitation_create_params);

    player_invitation_cancel_operation::fee_parameters_type player_invitation_cancel_params;
    player_invitation_cancel_params.fee = 1 * GRAPHENE_BLOCKCHAIN_PRECISION;
    player_invitation_cancel_params.price_per_kbyte = 0;
    fees.erase(player_invitation_cancel_params);
    fees.insert(player_invitation_cancel_params);

    buy_in_table_operation::fee_parameters_type buy_in_table_params;
    buy_in_table_params.fee = 0;
    fees.erase(player_invitation_cancel_params);
    fees.insert(player_invitation_cancel_params);

    buy_out_table_operation::fee_parameters_type buy_out_table_params;
    buy_out_table_params.fee = 0;
    buy_out_table_params.price_per_kbyte = 0;
    fees.erase(buy_out_table_params);
    fees.insert(buy_out_table_params);

    player_create_by_room_owner_operation::fee_parameters_type player_create_by_room_owner_params;
    player_create_by_room_owner_params.fee = 0;
    fees.erase(player_create_by_room_owner_params);
    fees.insert(player_create_by_room_owner_params);

    room_create_operation::fee_parameters_type room_create_params;
    room_create_params.fee = 0;
    room_create_params.price_per_kbyte = 0;
    fees.erase(room_create_params);
    fees.insert(room_create_params);

    room_update_operation::fee_parameters_type room_update_params;
    room_update_params.fee = 0;
    room_update_params.price_per_kbyte = 0;
    fees.erase(room_update_params);
    fees.insert(room_update_params);

    table_create_operation::fee_parameters_type table_create_params;
    table_create_params.fee = 0;
    table_create_params.price_per_kbyte = 0;
    fees.erase(table_create_params);
    fees.insert(table_create_params);

    table_update_operation::fee_parameters_type table_update_params;
    table_update_params.fee = 0;
    table_update_params.price_per_kbyte = 0;
    fees.erase(table_update_params);
    fees.insert(table_update_params);

    committee_member_create_operation::fee_parameters_type committee_member_params;
    committee_member_params.fee = 0;
    fees.erase(committee_member_params);
    fees.insert(committee_member_params);

    witness_create_operation::fee_parameters_type graphene_witness_params;
    graphene_witness_params.fee = 0;
    fees.erase(graphene_witness_params);
    fees.insert(graphene_witness_params);

    playchain_committee_member_create_operation::fee_parameters_type playchain_committee_member_params;
    playchain_committee_member_params.fee = 0;
    fees.erase(playchain_committee_member_params);
    fees.insert(playchain_committee_member_params);

    account_update_operation::fee_parameters_type account_update_params;
    account_update_params.fee = 0;
    fees.erase(account_update_params);
    fees.insert(account_update_params);

    change_fees( fees );
}

std::string playchain_fixture::id_to_string(const Actor &a)
{
    account_id_type id = actor(a);
    return this->id_to_string(id);
}

string playchain_fixture::to_string(const asset& a)
{
    asset_object core_asset = db.get(a.asset_id);
    return core_asset.amount_to_string(a.amount);
}

string playchain_fixture::get_next_uid(const account_id_type &inviter)
{
    static uint32_t invitation_num = 0;
    ++invitation_num;

    fc::ripemd160::encoder enc;
    fc::raw::pack( enc, inviter );
    fc::raw::pack( enc, invitation_num );

    return enc.result().str();
}

void playchain_fixture::pending_buy_in_resolve_all(Actor &table_owner, const table_object &table)
{
    auto sz = table.get_pending_proposals();
    if (!sz)
        return;

    std::vector<pending_buy_in_id_type> proposals;
    proposals.reserve(sz);

    for(const auto &ps: table.pending_proposals)
    {
        proposals.emplace_back(ps.second);
    }

    for(const auto &id: proposals)
    {
        try {
            buy_in_reserving_resolve(table_owner, table.id, id);
        } FC_LOG_AND_RETHROW()
    }

    generate_block();
}

asset playchain_fixture::get_account_balance(const Actor& account)
{
    return db.get_balance(actor(account), asset_id_type());
}

fc::ecc::private_key playchain_fixture::create_private_key_from_password(const string &password, const string &salt)
{
    string pwd = salt + "active" + password;
    auto secret = fc::sha256::hash(pwd.c_str(), pwd.size());
    auto priv_key = fc::ecc::private_key::regenerate(secret);
    return priv_key;
}

void playchain_fixture::print_block_operations()
{
    auto history = db.get_applied_operations();

    ilog("--> block ${b}:", ("b", db.head_block_num()));

    for(const auto &rec: history)
    {
       if (!rec.valid())
           continue;

       playchain_common::get_operation_name get;
       ilog("${id}: ${n} -> ${op}", ("id", (object_id_type)rec->id)("n", rec->op.visit(get))("op", rec->op));
    }
}

operation_history_id_type playchain_fixture::print_last_operations(const account_id_type& who,
                                                                   const operation_history_id_type &from)
{
    auto history = phistory_api->get_account_history(id_to_string(who),
                                                from,
                                                100,
                                                operation_history_id_type());

    BOOST_REQUIRE(begin(history) != end(history));

    std::reverse(begin(history), end(history));

    for(const auto &rec: history)
    {
        playchain_common::get_operation_name get;
        ilog("${id}: ${n} -> ${op}", ("id", (object_id_type)rec.id)("n", rec.op.visit(get))("op", rec.op));
    }

    auto next_history_record = history.rbegin()->id;

    ilog("<-- last record ${id}:", ("id", (std::string)(object_id_type)next_history_record));

    return next_history_record;
}

operation_history_id_type playchain_fixture::scroll_history_to_case_start_point(const account_id_type& who)
{
    generate_blocks(2);

    //scroll history to test point
    auto history = phistory_api->get_account_history(id_to_string(who),
                                                operation_history_id_type(),
                                                100,
                                                operation_history_id_type());

    if (history.empty())
        return operation_history_id_type();

    return history.begin()->id;
}

bool playchain_fixture::check_game_event_type(const vector<operation_history_object> &history, const size_t record, const int event_id)
{
    if (history[record].op.which() != game_event_id)
        return false;

    auto &&op = history[record].op.get<game_event_operation>();
    return op.event.which() == event_id;
}

void playchain_fixture::next_maintenance()
{
    const auto& props = db.get_global_properties();

    generate_blocks(db.get_dynamic_global_properties().time +
                    fc::seconds(props.parameters.maintenance_interval));

    verify_asset_supplies(db);
}

player_invitation_create_operation playchain_fixture::create_invitation_op(const account_id_type& inviter, uint32_t lifetime_in_sec)
{
    player_invitation_create_operation op;

    op.inviter = inviter;
    op.uid = get_next_uid(op.inviter);
    op.lifetime_in_sec = lifetime_in_sec;
    op.metadata = default_invitation_metadata;

    return op;
}

player_invitation_create_operation playchain_fixture::create_invitation(const Actor& inviter, uint32_t lifetime_in_sec)
{
    auto op = create_invitation_op(inviter, lifetime_in_sec);

    actor(inviter).push_operation(op);
    return op;
}

void playchain_fixture::create_invitation(const Actor& inviter, uint32_t lifetime_in_sec, player_invitation_create_operation& op)
{
    op = create_invitation(inviter, lifetime_in_sec);
}

player_invitation_cancel_operation playchain_fixture::cancel_invitation_op(const account_id_type& inviter, const string &uid)
{
    player_invitation_cancel_operation op;

    op.inviter = inviter;
    op.uid = uid;

    return op;
}

player_invitation_cancel_operation playchain_fixture::cancel_invitation(const Actor& inviter, const string &uid)
{
    auto op = cancel_invitation_op(inviter, uid);

    actor(inviter).push_operation(op);
    return op;
}

void playchain_fixture::cancel_invitation(const Actor& inviter, const string &uid, player_invitation_cancel_operation &op)
{
    op = cancel_invitation(inviter, uid);
}

player_invitation_resolve_operation playchain_fixture::resolve_invitation_op(const account_id_type& inviter, const string &uid, const string &name)
{
    player_invitation_resolve_operation op;

    op.inviter = inviter;
    op.uid = uid;
    op.name = name;

    return op;
}

player_invitation_resolve_operation playchain_fixture::resolve_invitation(const Actor& inviter, const string &uid, const string &name)
{
    player_invitation_resolve_operation op = resolve_invitation_op(inviter, uid, name);

    account_id_type inviter_id = actor(inviter);
    auto digest = utils::create_digest_by_invitation(db.get_chain_id(), inviter_id(db).name, uid);
    op.mandat = inviter.private_key.sign_compact(digest);
    auto priv_key = create_private_key_from_password(name, name);
    public_key_type pub_key =priv_key.get_public_key();
    op.owner = authority(1, pub_key, 1);
    op.active = authority(1, pub_key, 1);

    actor(inviter).push_operation_not_sign(op);
    return op;
}

void playchain_fixture::resolve_invitation(const Actor& inviter, const string &uid, const string &name, player_invitation_resolve_operation &op)
{
    op = resolve_invitation(inviter, uid, name);
}

Actor playchain_fixture::create_new_player(const Actor& inviter, const string &name, const asset &supply_amount)
{
    player_invitation_create_operation invitation_op;

    BOOST_REQUIRE_NO_THROW(create_invitation(inviter,
                                             PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds(),
                                             invitation_op));

    player_invitation_resolve_operation op = resolve_invitation_op(inviter, invitation_op.uid, name);

    account_id_type inviter_id = actor(inviter);
    auto digest = utils::create_digest_by_invitation(db.get_chain_id(), inviter_id(db).name, invitation_op.uid);
    op.mandat = inviter.private_key.sign_compact(digest);
    auto priv_key = create_private_key_from_password(name, name);
    public_key_type pub_key = priv_key.get_public_key();
    op.owner = authority(1, pub_key, 1);
    op.active = authority(1, pub_key, 1);

    BOOST_REQUIRE_NO_THROW(actor(inviter).push_operation_not_sign(op));

    fund(get_account(name), supply_amount);

    return Actor(name, priv_key, pub_key);
}

player_id_type playchain_fixture::get_player(const Actor& player)
{
    account_id_type account_id = actor(player);
    const auto& idx = db.get_index_type<playchain::chain::player_index>().indices().get<by_playchain_account>();
    auto it = idx.find(account_id);
    BOOST_REQUIRE(idx.end() != it);
    return (*it).id;
}

game_witness_id_type playchain_fixture::get_witness(const Actor& account)
{
    account_id_type account_id = actor(account);
    const auto& idx = db.get_index_type<game_witness_index>().indices().get<by_playchain_account>();
    auto it = idx.find(account_id);
    BOOST_REQUIRE(idx.end() != it);
    return (*it).id;
}

room_create_operation playchain_fixture::create_room_op(const account_id_type& owner,
                                                        const std::string &metadata,
                                                        const std::string &protocol_version)
{
    room_create_operation op;

    op.owner = owner;
    op.server_url = default_room_server;
    op.metadata = metadata;
    if (protocol_version.empty())
        op.protocol_version = default_protocol_version;
    else
        op.protocol_version = protocol_version;

    return op;
}

room_create_operation playchain_fixture::create_room(const Actor& owner,
                                                     const std::string &metadata,
                                                     const std::string &protocol_version)
{
    auto op = create_room_op(owner, metadata, protocol_version);

    actor(owner).push_operation(op);
    return op;
}

room_update_operation playchain_fixture::update_room_op(const account_id_type& owner, const room_id_type& room,  const std::string &room_server,
                                                        const std::string &room_metadata, const std::string &protocol_version)
{
    room_update_operation op;

    op.owner = owner;
    op.room = room;
    op.server_url = room_server;
    op.metadata = room_metadata;
    if (protocol_version.empty())
        op.protocol_version = default_protocol_version;
    else
        op.protocol_version = protocol_version;

    return op;
}

room_update_operation playchain_fixture::update_room(const Actor& owner, const room_id_type& room, const std::string &room_server,
                                                     const std::string &room_metadata, const std::string &protocol_version)
{
    auto op = update_room_op(owner, room, room_server, room_metadata, protocol_version);

    actor(owner).push_operation(op);
    return op;
}

table_create_operation playchain_fixture::create_table_op(const account_id_type& owner, const room_id_type &room, const amount_type required_witnesses, const std::string &metadata)
{
    table_create_operation op;

    op.owner = owner;
    op.room = room;
    if (!metadata.empty())
        op.metadata = metadata;
    else
        op.metadata = default_table_metadata;
    op.required_witnesses = required_witnesses;
    op.min_accepted_proposal_asset = asset(1);

    return op;
}

table_create_operation playchain_fixture::create_table(const Actor& owner, const room_id_type &room, const amount_type required_witnesses, const std::string &metadata)
{
    auto op = create_table_op(owner, room, required_witnesses, metadata);

    actor(owner).push_operation(op);
    return op;
}

table_update_operation playchain_fixture::update_table_op(const account_id_type& owner, const table_id_type &table, const amount_type required_witnesses, const std::string &metadata)
{
    table_update_operation op;

    op.owner = owner;
    op.table = table;
    if (!metadata.empty())
        op.metadata = metadata;
    else
        op.metadata = default_table_metadata;
    op.required_witnesses = required_witnesses;
    op.min_accepted_proposal_asset = asset(1);

    return op;
}

table_update_operation playchain_fixture::update_table(const Actor& owner, const table_id_type &table, const amount_type required_witnesses, const std::string &metadata)
{
    auto op = update_table_op(owner, table, required_witnesses, metadata);

    actor(owner).push_operation(op);
    return op;
}

room_id_type playchain_fixture::create_new_room(const Actor& owner, const std::string &metadata, const std::string &protocol_version)
{
    BOOST_REQUIRE_NO_THROW(create_room(owner, metadata, protocol_version));

    generate_block();

    account_id_type owner_id = actor(owner);

    auto history = phistory_api->get_account_history(id_to_string(owner_id), operation_history_id_type(), 10, operation_history_id_type());

    object_id_type result;
    for(const auto &rec: history)
    {
        if (rec.op.which() == room_create_id)
        {
            BOOST_CHECK_EQUAL(rec.result.which(), object_id_type_id);

            result = rec.result.get<object_id_type>();
            break;
        }
    }

    BOOST_CHECK(result != object_id_type{});

    return result;
}

table_id_type playchain_fixture::create_new_table(const Actor& owner, const room_id_type &room, const amount_type required_witnesses, const std::string &metadata)
{
    BOOST_REQUIRE_NO_THROW(create_table(owner, room, required_witnesses, metadata));

    generate_block();

    account_id_type owner_id = actor(owner);

    auto history = phistory_api->get_account_history(id_to_string(owner_id), operation_history_id_type(), 1, operation_history_id_type());

    BOOST_CHECK_EQUAL(history.size(), 1u);

    const operation_history_object &history_record = history[0];

    BOOST_CHECK_EQUAL(history_record.op.which(), table_create_id);

    BOOST_CHECK_EQUAL(history_record.result.which(), object_id_type_id);

    return history_record.result.get<object_id_type>();
}

const game_witness_object &playchain_fixture::create_witness(const Actor &account)
{
    create_new_room(account);

    auto pret = pplaychain_api->get_game_witness(account.name);
    BOOST_REQUIRE(pret.valid());

    return db.get<game_witness_object>(pret->id);
}

buy_in_table_operation playchain_fixture::buy_in_table_op(
                                       const account_id_type& player, const account_id_type& owner,
                                       const table_id_type &table, const asset &amount)
{
    buy_in_table_operation op;

    op.player = player;
    op.table_owner = owner;
    op.table = table;
    op.amount = amount;

    return op;
}

buy_in_table_operation playchain_fixture::buy_in_table(const Actor& player, const Actor& owner,
                                    const table_id_type& table, const asset &amount)
{
    auto op = buy_in_table_op(player, owner, table, amount);

    actor(player).push_operations_with_additional_signers(op, owner);
    return op;
}

buy_out_table_operation playchain_fixture::buy_out_table_op(const account_id_type& player,
                                                            const account_id_type& owner,
                                         const table_id_type &table, const asset &amount,
                                                            const std::string &reason)
{
    buy_out_table_operation op;

    op.player = player;
    op.table_owner = owner;
    op.table = table;
    op.amount = amount;
    op.reason = reason;

    return op;
}

buy_out_table_operation playchain_fixture::buy_out_table(const Actor& player,
                                      const table_id_type& table, const asset &amount,
                                      const std::string &reason)
{
    auto op = buy_out_table_op(actor(player), table(db).room(db).owner, table, amount, reason);

    actor(player).push_operation(op);
    return op;
}

buy_out_table_operation playchain_fixture::buy_out_table(const Actor& player,
                                      const Actor& owner,
                                      const Actor& authority,
                                      const table_id_type& table,
                                      const asset &amount,
                                      const std::string &reason /*= ""*/)
{
    auto op = buy_out_table_op(actor(player), actor(owner), table, amount, reason);

    actor(authority).push_operation(op);
    return op;
}

buy_in_reserve_operation playchain_fixture::buy_in_reserve_op(const account_id_type& player, const string& uid,
                                       const asset &amount, const string &metadata,
                                       const std::string &protocol_version)
{
    buy_in_reserve_operation op;

    op.player = player;
    op.uid = uid;
    op.amount = amount;
    op.metadata = metadata;
    if (protocol_version.empty())
        op.protocol_version = default_protocol_version;
    else
        op.protocol_version = protocol_version;

    return op;
}

buy_in_reserve_operation playchain_fixture::buy_in_reserve(const Actor& player, const string& uid,
                                        const asset &amount, const string &metadata,
                                        const std::string &protocol_version)
{
    auto op = buy_in_reserve_op(actor(player), uid, amount, metadata, protocol_version);

    actor(player).push_operation(op);
    return op;
}

buy_in_reserving_cancel_operation playchain_fixture::buy_in_reserving_cancel_op(const account_id_type& player, const string& uid)
{
    buy_in_reserving_cancel_operation op;

    op.player = player;
    op.uid = uid;

    return op;
}

buy_in_reserving_cancel_operation playchain_fixture:: buy_in_reserving_cancel(const Actor& player, const string& uid)
{
    auto op = buy_in_reserving_cancel_op(actor(player), uid);

    actor(player).push_operation(op);
    return op;
}

buy_in_reserving_resolve_operation playchain_fixture::buy_in_reserving_resolve_op(const account_id_type& owner, const table_id_type& table, const pending_buy_in_id_type& id)
{
    buy_in_reserving_resolve_operation op;

    op.table = table;
    op.table_owner = owner;
    op.pending_buyin = id;

    return op;
}

buy_in_reserving_resolve_operation playchain_fixture::buy_in_reserving_resolve(const Actor& owner, const table_id_type& table, const pending_buy_in_id_type& id)
{
    auto op = buy_in_reserving_resolve_op(actor(owner), table, id);

    actor(owner).push_operation(op);
    return op;
}

buy_in_reserving_cancel_all_operation playchain_fixture::buy_in_reserving_cancel_all_op(const account_id_type& player)
{
    buy_in_reserving_cancel_all_operation op;

    op.player = player;

    return op;
}

buy_in_reserving_cancel_all_operation playchain_fixture::buy_in_reserving_cancel_all(const Actor& player)
{
    auto op = buy_in_reserving_cancel_all_op(actor(player));

    actor(player).push_operation(op);
    return op;
}

game_start_playing_check_operation playchain_fixture::game_start_playing_check_op(
                                       const account_id_type& voter,
                                       const account_id_type& owner,
                                       const table_id_type &table,
                                       const game_initial_data &initial_data)
{
    game_start_playing_check_operation op;

    op.voter = voter;
    op.table = table;
    op.table_owner = owner;
    op.initial_data = initial_data;

    return op;
}

game_start_playing_check_operation playchain_fixture::game_start_playing_check(
                                    const Actor& voter,
                                    const table_id_type &table,
                                    const game_initial_data &initial_data)
{
    auto op = game_start_playing_check_op(actor(voter), table(db).room(db).owner, table, initial_data);

    actor(voter).push_operation(op);
    return op;
}

game_result_check_operation playchain_fixture::game_result_check_op(
                                    const account_id_type& voter,
                                    const account_id_type& owner,
                                    const table_id_type &table,
                                    const game_result &result)
{
    game_result_check_operation op;

    op.voter = voter;
    op.table = table;
    op.table_owner = owner;
    op.result = result;

    return op;
}

game_result_check_operation playchain_fixture::game_result_check(
                                    const Actor& voter,
                                    const table_id_type &table,
                                    const game_result &result)
{
    auto op = game_result_check_op(actor(voter), table(db).room(db).owner, table, result);

    actor(voter).push_operation(op);
    return op;
}

game_reset_operation playchain_fixture::game_reset_op(
                                    const account_id_type& owner,
                                    const table_id_type &table,
                                    const bool rollback_table)
{
    game_reset_operation op;

    op.table = table;
    op.table_owner = owner;
    op.rollback_table = rollback_table;

    return op;
}

game_reset_operation playchain_fixture::game_reset(
                                    const Actor& voter,
                                    const table_id_type &table,
                                    const bool rollback_table)
{
    auto op = game_reset_op(table(db).room(db).owner, table, rollback_table);

    actor(voter).push_operation(op);
    return op;
}

player_create_by_room_owner_operation playchain_fixture::player_create_by_room_owner_op(
        const account_id_type& room_owner,
        const account_id_type& account)
{
    player_create_by_room_owner_operation op;

    op.account = account;
    op.room_owner = room_owner;

    return op;
}

player_create_by_room_owner_operation playchain_fixture::player_create_by_room_owner(
        const Actor& room_owner,
        const account_id_type &account)
{
    auto op = player_create_by_room_owner_op(actor(room_owner), account);

    actor(room_owner).push_operation(op);
    return op;
}

playchain_committee_member_create_operation playchain_fixture::playchain_committee_member_create_op(
                                            const account_id_type& committee_member_account,
                                            const string& url)
{
    playchain_committee_member_create_operation op;

    op.committee_member_account = committee_member_account;
    op.url = url;

    return op;
}

playchain_committee_member_create_operation playchain_fixture::playchain_committee_member_create(
                                const Actor& committee_member_account,
                                const string& url)
{
    auto op = playchain_committee_member_create_op(actor(committee_member_account), url);

    actor(committee_member_account).push_operation(op);
    return op;
}

playchain_committee_member_update_operation playchain_fixture::playchain_committee_member_update_op(
                                            const account_id_type& committee_member_account,
                                            const playchain_committee_member_id_type& committee_member,
                                            const optional<string>& purl)
{
    playchain_committee_member_update_operation op;

    op.committee_member = committee_member;
    op.committee_member_account = committee_member_account;
    op.new_url = purl;

    return op;
}

playchain_committee_member_update_operation playchain_fixture::playchain_committee_member_update(
                                const Actor& committee_member_account,
                                const playchain_committee_member_id_type& committee_member,
                                const optional<string>& purl)
{
    auto op = playchain_committee_member_update_op(actor(committee_member_account), committee_member, purl);

    actor(committee_member_account).push_operation(op);
    return op;
}

void playchain_fixture::vote_for(const vote_id_type &vote_id, const Actor &voter, bool approve)
{
    account_object voting_account_object = get_account(voter);
    if (approve)
    {
        auto insert_result = voting_account_object.options.votes.insert(vote_id);
        BOOST_CHECK(insert_result.second);
    }
    else
    {
        unsigned votes_removed = voting_account_object.options.votes.erase(vote_id);
        BOOST_CHECK(votes_removed);
    }

    auto voted_witnesses = 0;
    auto voted_committee = 0;
    auto voted_playchain_committee = 0;

    for( vote_id_type id : voting_account_object.options.votes )
    {
       if( id.type() == vote_id_type::witness )
          ++voted_witnesses;
       else if ( id.type() == vote_id_type::committee )
          ++voted_committee;
       else if ( id.type() == vote_id_type::playchain_committee )
          ++voted_playchain_committee;
    }

    voting_account_object.options.num_witness = voted_witnesses;
    voting_account_object.options.num_committee = std::max(voted_committee, voted_playchain_committee);

    account_update_operation account_update_op;
    account_update_op.account = voting_account_object.id;
    account_update_op.new_options = voting_account_object.options;

    try
    {
        actor(voter).push_operation(account_update_op);
    }FC_LOG_AND_RETHROW()
}

tables_alive_operation playchain_fixture::tables_alive_op(const account_id_type& room_owner,
                                     const std::set<table_id_type> &tables)
{
    tables_alive_operation op;

    op.owner = room_owner;
    std::copy(begin(tables), end(tables), std::inserter(op.tables, op.tables.end()));

    return op;
}

tables_alive_operation playchain_fixture::table_alive(const Actor& room_owner,
                                  const table_id_type &table)
{
    return tables_alive(room_owner, {table});
}

tables_alive_operation playchain_fixture::tables_alive(const Actor& room_owner,
                                  const std::set<table_id_type> &tables)
{
    auto op = tables_alive_op(actor(room_owner), tables);

    actor(room_owner).push_operation(op);
    return op;
}

const playchain_committee_member_object &playchain_fixture::get_playchain_committee_member(const Actor &member) const
{
    auto &&member_account_object = get_account(member);

    using namespace playchain::chain;

    BOOST_REQUIRE(is_game_witness(db, member_account_object.id));

    auto &&witness = get_game_witness(db, member_account_object.id);

    const auto& committee_members_by_witness = db.get_index_type<playchain_committee_member_index>().indices().get<by_game_witness>();
    auto it = committee_members_by_witness.find(witness.id);
    BOOST_REQUIRE(committee_members_by_witness.end() != it);

    return (*it);
}

bool playchain_fixture::is_active_playchain_committee_member(const Actor &member) const
{
    auto account_id = get_account(member).id;
    auto &&active = get_playchain_properties(db).active_games_committee_members;
    auto lmb = [this, &account_id](const game_witness_id_type &w_id)
    {
        return w_id(db).account == account_id;
    };
    return active.end() != std::find_if(begin(active), end(active), lmb);
}

void playchain_fixture::vote_for_playchain_committee_member(const Actor &member, const Actor &voter, bool approve)
{
    auto vote_id = get_playchain_committee_member(member).vote_id;
    vote_for(vote_id, voter, approve);
}

void playchain_fixture::approve_proposal(const object_id_type &proposal_id, const Actor &member, bool approve)
{
    BOOST_REQUIRE(db.find_object(proposal_id));

    proposal_update_operation prop_update_op;

    prop_update_op.fee_paying_account = get_account(member).id;
    prop_update_op.proposal = proposal_id;
    if (approve)
    {
        prop_update_op.active_approvals_to_add.insert( get_account( member ).id );
    }
    else
    {
        prop_update_op.active_approvals_to_remove.insert( get_account( member ).id );
    }

    try
    {
        actor(member).push_operation(prop_update_op);
    }FC_LOG_AND_RETHROW()
}

void playchain_fixture::elect_member(const Actor &member)
{
    vote_for_playchain_committee_member(member, member);

    next_maintenance();

    BOOST_REQUIRE(is_active_playchain_committee_member(member));

    ilog("Playchain Commitee: ${l}", ("l", get_playchain_properties(db).active_games_committee_members));
}

playchain_fixture::proposal_info playchain_fixture::propose_playchain_params_change(const Actor &member, const playchain_parameters &new_params,
                                              bool review,
                                              const fc::microseconds &ext_review)
{
    auto time = db.head_block_time();

    playchain_committee_member_update_parameters_v4_operation update_op;
    update_op.new_parameters = new_params;

    proposal_create_operation prop_op;

    const chain_parameters& current_params = db.get_global_properties().parameters;

    if (review)
    {
        prop_op.review_period_seconds = current_params.committee_proposal_review_period;
        prop_op.expiration_time = time + current_params.committee_proposal_review_period + (uint32_t)ext_review.to_seconds();
    }else
    {
        prop_op.expiration_time = time + (uint32_t)ext_review.to_seconds();
    }
    prop_op.fee_paying_account = get_account(member).id;

    prop_op.proposed_ops.emplace_back( update_op );

    object_id_type proposal_id;

    try
    {
        proposal_id = actor(member).push_operation(prop_op).operation_results.front().get<object_id_type>();
    }FC_LOG_AND_RETHROW()

    proposal_update_operation prop_update_op;

    prop_update_op.fee_paying_account = get_account(member).id;
    prop_update_op.proposal = proposal_id;
    prop_update_op.active_approvals_to_add.insert( get_account( member ).id );

    try
    {
        actor(member).push_operation(prop_update_op);
    }FC_LOG_AND_RETHROW()

    return {proposal_id, prop_op.expiration_time};
}
}
