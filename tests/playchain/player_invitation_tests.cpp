#include "playchain_common.hpp"

#include <playchain/chain/sign_utils.hpp>
#include <playchain/chain/schema/player_invitation_object.hpp>
#include <playchain/chain/schema/player_object.hpp>

#include <playchain/chain/playchain_config.hpp>

#include <playchain/chain/evaluators/validators.hpp>

#include <graphene/utilities/key_conversion.hpp>

namespace player_invitation_tests
{
struct player_invitation_fixture : public  playchain_common::playchain_fixture
{
    const int64_t rich_registrator_init_balance = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
    const int64_t poor_registrator_init_balance = 2 * GRAPHENE_BLOCKCHAIN_PRECISION;

    uint32_t default_lifetime = PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds();

    DECLARE_ACTOR(richregistrator)
    DECLARE_ACTOR(poorregistrator)
    DECLARE_ACTOR(emptyregistrator)

    player_invitation_fixture()
    {
        actor(richregistrator).supply(asset(rich_registrator_init_balance));
        actor(poorregistrator).supply(asset(poor_registrator_init_balance));
        actor(emptyregistrator);

        init_fees();
    }

    vector<player_invitation_object > list_next_n_invitations(const Actor& invitor,
                                                               const string &uid_from,
                                                               uint32_t n)
    {
        return pplaychain_api->list_player_invitations(invitor.name, uid_from, n).first;
    }
};

BOOST_FIXTURE_TEST_SUITE( player_invitation_tests, player_invitation_fixture)

//PLAYCHAIN_TEST_CASE(check_create_mandat)
//{
//    using namespace playchain::chain::utils;

//    auto digest = create_digest_by_invitation(
//                chain_id_type{"f209c0e084bde4b456eebdebbcee2a52d7bd8b49d7d2f9200303241820821fbb"},
//                                "player7", "1551185993.1075966.0");

//    idump((digest));

//    auto pkey = graphene::utilities::wif_to_key("5JuHZfB5RHzn6PSDHQ6YD1eycpCqheq8ZarDriB7gzhfDY13sPw");
//    BOOST_REQUIRE(pkey.valid());
//    auto signature = pkey->sign_compact(digest);

//    idump((signature));

//    std::string mandat{"1f1bb30972f546fef427339a2b0c6a8d965d620d71a74b622c8fdaad94d1865e75101f786472ac76bc537f1418cedda7ca4e8a2d053aba8cdc5b6b82251cac250e"};
//    signature_type external_signature;
//    fc::from_variant(fc::variant{mandat}, external_signature);

//    digest_type external_digest{"ffd28ee616c7ecb5731bda512e4f5e3fbaa508c63587491365b0c5fc7799d4df"};

//    public_key_type pk = get_signature_key(external_signature, external_digest);

//    idump((pk));
//}

PLAYCHAIN_TEST_CASE(check_negative_create_invitation_operation)
{
    BOOST_CHECK_THROW(create_invitation_op(richregistrator, 0).validate(), fc::exception);

    BOOST_CHECK_THROW(create_invitation_op(richregistrator, fc::time_point_sec::maximum().sec_since_epoch()).validate(), fc::exception);

    player_invitation_create_operation op = create_invitation_op(richregistrator, default_lifetime);

    op.uid.clear();

    BOOST_CHECK_THROW(op.validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_resolve_invitation_operation)
{
    BOOST_CHECK_THROW(cancel_invitation_op(richregistrator, "").validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_cancel_invitation_operation)
{
    BOOST_CHECK_THROW(resolve_invitation_op(richregistrator, get_next_uid(actor(richregistrator)), "").validate(), fc::exception);

    BOOST_CHECK_THROW(resolve_invitation_op(richregistrator, "", "alice").validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_create_invitation)
{
    player_invitation_create_operation op;

    BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator, default_lifetime, op));

    auto invitations = pplaychain_api->list_player_invitations(richregistrator.name, "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST).first;

    BOOST_REQUIRE_EQUAL(invitations.size(), 1u);

    const player_invitation_object &invitation = (*invitations.begin());
    account_id_type invitor_id = actor(richregistrator);

    BOOST_REQUIRE(invitation.inviter == invitor_id);
    BOOST_CHECK_EQUAL(invitation.created.sec_since_epoch(), db.head_block_time().sec_since_epoch());
    BOOST_CHECK_EQUAL((invitation.created + fc::seconds(op.lifetime_in_sec)).sec_since_epoch(), invitation.expiration.sec_since_epoch());
    BOOST_CHECK_EQUAL(invitation.metadata, default_invitation_metadata);
}

PLAYCHAIN_TEST_CASE(check_negative_create_invitation)
{
    string last_uid;
    {
        player_invitation_create_operation op;

        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(1).to_seconds(), op));
        last_uid = op.uid;
    }

    {
        player_invitation_create_operation op = create_invitation_op(
                    richregistrator, default_lifetime + fc::minutes(2).to_seconds());
        op.uid = last_uid;

        //invitation with same UID already exists

        BOOST_REQUIRE_THROW(actor(richregistrator).push_operation(op), fc::exception);
    }
}

PLAYCHAIN_TEST_CASE(check_cancel_invitations)
{
    std::set<string> invitation_uids;

    const size_t INVITATIONS = 3;

    for(size_t ci = 1; ci <= INVITATIONS; ++ci)
    {
        player_invitation_create_operation op;

        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(ci).to_seconds(), op));
        invitation_uids.emplace(op.uid);
    }

    BOOST_REQUIRE_EQUAL(list_next_n_invitations(richregistrator, "", 10).size(), invitation_uids.size());

    std::for_each(begin(invitation_uids), end(invitation_uids), [&](const string &uid)
    {
        player_invitation_cancel_operation op;

        BOOST_REQUIRE_NO_THROW(cancel_invitation(richregistrator, uid, op));
    });

    BOOST_REQUIRE(list_next_n_invitations(richregistrator, "", 10).empty());
}

PLAYCHAIN_TEST_CASE(check_negative_cancel_invitation)
{
    string last_uid;
    {
        player_invitation_create_operation op;

        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(1).to_seconds(), op));
        last_uid = op.uid;
    }

    {
        player_invitation_cancel_operation op;

        BOOST_REQUIRE_THROW(cancel_invitation(poorregistrator, last_uid, op), fc::exception);
    }
}

PLAYCHAIN_TEST_CASE(check_expiration_invitations)
{
    const size_t INVITATIONS = 2;

    for(size_t ci = 1; ci <= INVITATIONS; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(ci).to_seconds()));
    }

    auto created_time = db.head_block_time();

    BOOST_REQUIRE_EQUAL(list_next_n_invitations(richregistrator, "", 10).size(), INVITATIONS);

    generate_blocks(created_time + fc::seconds(default_lifetime) + fc::minutes(1));

    BOOST_REQUIRE_EQUAL(list_next_n_invitations(richregistrator, "", 10).size(), INVITATIONS - 1);

    generate_blocks(created_time + fc::seconds(default_lifetime) + fc::minutes(INVITATIONS));

    BOOST_REQUIRE(list_next_n_invitations(richregistrator, "", 10).empty());
}

PLAYCHAIN_TEST_CASE(check_resolve_invitations)
{
    std::set<string> invitation_uids;

    const size_t INVITATIONS = 3;

    string last_uid;

    for(size_t ci = 1; ci <= INVITATIONS; ++ci)
    {
        player_invitation_create_operation op;

        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(ci).to_seconds(), op));
        last_uid = op.uid;
        invitation_uids.emplace(last_uid);
    }

    BOOST_REQUIRE_EQUAL(list_next_n_invitations(richregistrator, "", 10).size(), INVITATIONS);

    const string new_user_name = "alice";

    BOOST_REQUIRE(!pdatabase_api->get_account_by_name(new_user_name).valid());

    BOOST_REQUIRE_NO_THROW(resolve_invitation(richregistrator, last_uid, new_user_name));

    BOOST_REQUIRE(pdatabase_api->get_account_by_name(new_user_name).valid());

    auto invitations = list_next_n_invitations(richregistrator, "", 10);

    BOOST_REQUIRE_EQUAL(invitations.size(), INVITATIONS - 1);

    BOOST_REQUIRE(std::find_if(begin(invitations), end(invitations), [&](const player_invitation_object & obj)
    {
        return obj.uid == last_uid;
    }) == invitations.end());
}

PLAYCHAIN_TEST_CASE(check_negative_resolve_invitations)
{
    string last_uid;
    {
        player_invitation_create_operation op;

        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(1).to_seconds(), op));
        last_uid = op.uid;
    }

    BOOST_REQUIRE_THROW(resolve_invitation(poorregistrator, last_uid, "wrong"), fc::exception);

    {
        player_invitation_resolve_operation op = resolve_invitation_op(richregistrator, last_uid, "codekiddy");

        //wrong signature for valid data. 'poor registrator' owns all data except private key

        account_id_type invitor_id = actor(poorregistrator);
        auto digest = utils::create_digest_by_invitation(db.get_chain_id(), invitor_id(db).name, last_uid);
        op.mandat = poorregistrator.private_key.sign_compact(digest);
        auto priv_key = create_private_key_from_password(op.name, op.name);
        public_key_type pub_key =priv_key.get_public_key();
        op.owner = authority(1, pub_key, 1);
        op.active = authority(1, pub_key, 1);

        BOOST_REQUIRE_THROW(actor(poorregistrator).push_operation_not_sign(op), fc::exception);
    }

    {
        account_id_type valid_account_id = actor(richregistrator);
        player_invitation_resolve_operation op = resolve_invitation_op(account_id_type{object_id_type{valid_account_id.space_id, valid_account_id.type_id, 999}}, last_uid, "codekiddy");

        //wrong inviter name only

        account_id_type invitor_id = actor(richregistrator);
        auto digest = utils::create_digest_by_invitation(db.get_chain_id(), invitor_id(db).name, last_uid);
        op.mandat = richregistrator.private_key.sign_compact(digest);
        auto priv_key = create_private_key_from_password(op.name, op.name);
        public_key_type pub_key =priv_key.get_public_key();
        op.owner = authority(1, pub_key, 1);
        op.active = authority(1, pub_key, 1);

        BOOST_REQUIRE_THROW(actor(poorregistrator).push_operation_not_sign(op), fc::exception);
    }

    {
        player_invitation_resolve_operation op = resolve_invitation_op(richregistrator, last_uid, "ok");

        account_id_type invitor_id = actor(richregistrator);
        auto digest = utils::create_digest_by_invitation(db.get_chain_id(), invitor_id(db).name, last_uid);
        op.mandat = richregistrator.private_key.sign_compact(digest);
        auto priv_key = create_private_key_from_password(op.name, op.name);
        public_key_type pub_key =priv_key.get_public_key();
        op.owner = authority(1, pub_key, 1);
        op.active = authority(1, pub_key, 1);

        //any user send valid request
        BOOST_REQUIRE_NO_THROW(actor(poorregistrator).push_operation_not_sign(op));
    }
}

PLAYCHAIN_TEST_CASE(check_history_while_invitations_go)
{
    std::set<string> invitation_uids;

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator));

    const size_t INVITATIONS = 3;

    string last_uid;

    for(size_t ci = 1; ci <= INVITATIONS; ++ci)
    {
        player_invitation_create_operation op;

        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(ci).to_seconds(), op));
        last_uid = op.uid;
        invitation_uids.emplace(last_uid);
    }

    auto created_time = db.head_block_time();

    generate_block();

    {
        player_invitation_cancel_operation op;

        BOOST_REQUIRE_NO_THROW(cancel_invitation(richregistrator, last_uid, op));

        invitation_uids.erase(last_uid);

        last_uid = (*invitation_uids.begin());
    }

    generate_block();

    BOOST_REQUIRE_NO_THROW(resolve_invitation(richregistrator, last_uid, "andrew"));

    generate_blocks(created_time + fc::seconds(default_lifetime) + fc::minutes(INVITATIONS));

    auto history = phistory_api->get_account_history(actor(richregistrator),
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_GE(history.size(), 9u);

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), player_invitation_create_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), account_upgrade_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), player_invitation_create_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), player_invitation_create_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), player_invitation_cancel_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), player_invitation_resolve_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), account_create_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), player_create_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), player_invitation_expire_id);
}

PLAYCHAIN_TEST_CASE(check_not_enough_money_to_create_invitation)
{
    BOOST_REQUIRE_THROW(create_invitation(emptyregistrator, default_lifetime), fc::exception);

    BOOST_REQUIRE_NO_THROW(create_invitation(poorregistrator, default_lifetime));

    BOOST_REQUIRE(!(pplaychain_api->list_player_invitations(poorregistrator.name, "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST)).first.empty());

    BOOST_REQUIRE_THROW(create_invitation(poorregistrator, default_lifetime), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_not_enough_money_to_cancel_invitation)
{
    player_invitation_create_operation op;

    BOOST_REQUIRE_NO_THROW(create_invitation(poorregistrator, default_lifetime, op));

    string last_uid = op.uid;

    BOOST_REQUIRE_THROW(cancel_invitation(poorregistrator, last_uid), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_player_create_by_room_owner)
{
    create_new_room(richregistrator);

    BOOST_CHECK_THROW(player_create_by_room_owner_op(GRAPHENE_COMMITTEE_ACCOUNT, actor(poorregistrator)).validate(), fc::exception);
    BOOST_CHECK_THROW(player_create_by_room_owner_op(actor(richregistrator), GRAPHENE_COMMITTEE_ACCOUNT).validate(), fc::exception);

    account_id_type invalid_account{999};
    BOOST_REQUIRE(!is_account_exists(db, invalid_account));

    BOOST_CHECK_THROW(player_create_by_room_owner(richregistrator, invalid_account), fc::exception);
    //poorregistrator doesn't own any room
    BOOST_CHECK_THROW(player_create_by_room_owner(poorregistrator, actor(emptyregistrator)), fc::exception);

    Actor alice = create_new_player(richregistrator, "alice", asset(poor_registrator_init_balance));

    BOOST_CHECK_THROW(player_create_by_room_owner(richregistrator, actor(alice)), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_player_create_by_room_owner)
{
    create_new_room(richregistrator);

    Actor alice("alice");

    actor(alice).supply(asset(poor_registrator_init_balance));

    try
    {
        player_create_by_room_owner(richregistrator, actor(alice));
    }FC_LOG_AND_RETHROW()
}

PLAYCHAIN_TEST_CASE(check_create_special_player_by_invitation)
{
    if (PLAYCHAIN_VERSION == version(0, 0, 0))
    {
        BOOST_REQUIRE(!is_player_exists(db, PLAYCHAIN_NULL_PLAYER));

        player_invitation_create_operation op;

        BOOST_REQUIRE_NO_THROW(create_invitation(richregistrator,
                                                 default_lifetime + fc::minutes(1).to_seconds(), op));

        BOOST_REQUIRE(!is_player_exists(db, PLAYCHAIN_NULL_PLAYER));

        BOOST_REQUIRE_NO_THROW(resolve_invitation(richregistrator, op.uid, "alice"));
    }

    BOOST_CHECK(is_player_exists(db, PLAYCHAIN_NULL_PLAYER));
}

PLAYCHAIN_TEST_CASE(check_create_special_player_by_room_owner)
{
    if (PLAYCHAIN_VERSION == version(0, 0, 0))
    {
        BOOST_REQUIRE(!is_player_exists(db, PLAYCHAIN_NULL_PLAYER));

        create_new_room(richregistrator);

        Actor alice("alice");

        actor(alice).supply(asset(poor_registrator_init_balance));

        BOOST_REQUIRE_NO_THROW(player_create_by_room_owner(richregistrator, actor(alice)));
    }

    BOOST_CHECK(is_player_exists(db, PLAYCHAIN_NULL_PLAYER));
}

BOOST_AUTO_TEST_SUITE_END()
}
