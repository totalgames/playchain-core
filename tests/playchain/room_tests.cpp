#include "playchain_common.hpp"

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/validators.hpp>

namespace room_tests
{
struct room_fixture: public  playchain_common::playchain_fixture
{
    const int64_t rich_registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;
    const int64_t poor_registrator_init_balance = 13*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(richregistrator)
    DECLARE_ACTOR(poorregistrator)
    DECLARE_ACTOR(emptyregistrator)

    room_fixture()
    {
        actor(richregistrator).supply(asset(rich_registrator_init_balance));
        actor(poorregistrator).supply(asset(poor_registrator_init_balance));
        actor(emptyregistrator);

        enable_fees();
    }
};

BOOST_FIXTURE_TEST_SUITE( room_tests, room_fixture)

PLAYCHAIN_TEST_CASE(check_negative_create_room_operation)
{
    BOOST_CHECK_THROW(create_room_op(GRAPHENE_COMMITTEE_ACCOUNT).validate(), fc::exception);
    BOOST_CHECK_THROW(create_room_op(GRAPHENE_WITNESS_ACCOUNT).validate(), fc::exception);
    BOOST_CHECK_THROW(create_room_op(GRAPHENE_RELAXED_COMMITTEE_ACCOUNT).validate(), fc::exception);
    BOOST_CHECK_THROW(create_room_op(GRAPHENE_NULL_ACCOUNT).validate(), fc::exception);
    BOOST_CHECK_THROW(create_room_op(GRAPHENE_TEMP_ACCOUNT).validate(), fc::exception);
    BOOST_CHECK_THROW(create_room_op(GRAPHENE_PROXY_TO_SELF_ACCOUNT).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_negative_update_room_operation)
{
    BOOST_CHECK_THROW(update_room_op(GRAPHENE_COMMITTEE_ACCOUNT, room_id_type(), default_room_server, default_room_metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(update_room_op(GRAPHENE_WITNESS_ACCOUNT, room_id_type(), default_room_server, default_room_metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(update_room_op(GRAPHENE_RELAXED_COMMITTEE_ACCOUNT, room_id_type(), default_room_server, default_room_metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(update_room_op(GRAPHENE_NULL_ACCOUNT, room_id_type(), default_room_server, default_room_metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(update_room_op(GRAPHENE_TEMP_ACCOUNT, room_id_type(), default_room_server, default_room_metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(update_room_op(GRAPHENE_PROXY_TO_SELF_ACCOUNT, room_id_type(), default_room_server, default_room_metadata).validate(), fc::exception);
    BOOST_CHECK_THROW(update_room_op(actor(richregistrator), room_id_type(), "", default_room_metadata).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_create_room_operation)
{
    BOOST_CHECK_THROW(create_room(emptyregistrator), fc::exception);
    BOOST_CHECK_THROW(create_room(poorregistrator), fc::exception);

    auto next_history_record = scroll_history_to_case_start_point(actor(richregistrator));

    account_id_type owner_id = actor(richregistrator);
    room_id_type room_id = create_new_room(richregistrator);

    BOOST_REQUIRE_NO_THROW(room_id(db));

    BOOST_CHECK_EQUAL((std::string)(object_id_type)(room_id(db).owner), (std::string)(object_id_type)owner_id);

    generate_blocks(2);

    auto history = phistory_api->get_account_history(richregistrator.name,
                                                next_history_record,
                                                100,
                                                operation_history_id_type());

    std::reverse(begin(history), end(history));

    auto record_offset = 0u;

    BOOST_CHECK_EQUAL(history[record_offset].op.which(), room_create_id);
    BOOST_CHECK_EQUAL(history[++record_offset].op.which(), game_witness_create_id);
}

PLAYCHAIN_TEST_CASE(check_update_room_operation)
{
    room_id_type room_id = create_new_room(richregistrator);

    BOOST_REQUIRE_EQUAL(room_id(db).metadata, default_room_metadata);

    const std::string new_room_meta = R"json({"info": "Cool Room!""})json";

    BOOST_REQUIRE_NO_THROW(update_room(richregistrator, room_id, "cool.poker.org:9999", new_room_meta));

    generate_block();

    account_id_type owner_id = actor(richregistrator);

    auto history = phistory_api->get_account_history(id_to_string(owner_id), operation_history_id_type(), 1, operation_history_id_type());

    BOOST_CHECK_EQUAL(history.size(), 1u);

    BOOST_CHECK_EQUAL(history[0].op.which(), room_update_id);

    BOOST_CHECK_EQUAL(room_id(db).metadata, new_room_meta);
}

PLAYCHAIN_TEST_CASE(check_negative_create_table_operation)
{
    room_id_type room_id = create_new_room(richregistrator);

    BOOST_REQUIRE_THROW(create_table_op(GRAPHENE_COMMITTEE_ACCOUNT, room_id).validate(), fc::exception);
    BOOST_REQUIRE_THROW(create_table_op(GRAPHENE_WITNESS_ACCOUNT, room_id).validate(), fc::exception);
    BOOST_REQUIRE_THROW(create_table_op(GRAPHENE_RELAXED_COMMITTEE_ACCOUNT, room_id).validate(), fc::exception);
    BOOST_REQUIRE_THROW(create_table_op(GRAPHENE_NULL_ACCOUNT, room_id).validate(), fc::exception);
    BOOST_REQUIRE_THROW(create_table_op(GRAPHENE_TEMP_ACCOUNT, room_id).validate(), fc::exception);
    BOOST_REQUIRE_THROW(create_table_op(GRAPHENE_PROXY_TO_SELF_ACCOUNT, room_id).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_create_table_operation)
{
    auto room_id = create_new_room(richregistrator);
    auto table_id = create_new_table(richregistrator, room_id);

    BOOST_REQUIRE_NO_THROW(table_id(db));

    BOOST_CHECK_EQUAL((std::string)(object_id_type)(table_id(db).room), (std::string)(object_id_type)room_id);
    BOOST_CHECK_EQUAL(table_id(db).metadata, default_table_metadata);
}

PLAYCHAIN_TEST_CASE(check_create_special_objects)
{
    if (PLAYCHAIN_VERSION == version(0, 0, 0))
    {
        BOOST_REQUIRE(!is_room_exists(db, PLAYCHAIN_NULL_ROOM));
        BOOST_REQUIRE(!is_game_witness_exists(db, PLAYCHAIN_NULL_GAME_WITNESS));
        BOOST_REQUIRE(!is_table_exists(db, PLAYCHAIN_NULL_TABLE));

        auto room_id = create_new_room(richregistrator);

        BOOST_CHECK(is_room_exists(db, PLAYCHAIN_NULL_ROOM));
        BOOST_CHECK(is_game_witness_exists(db, PLAYCHAIN_NULL_GAME_WITNESS));
        BOOST_CHECK(!is_table_exists(db, PLAYCHAIN_NULL_TABLE));

        create_new_table(richregistrator, room_id);
    }

    BOOST_CHECK(is_room_exists(db, PLAYCHAIN_NULL_ROOM));
    BOOST_CHECK(is_game_witness_exists(db, PLAYCHAIN_NULL_GAME_WITNESS));
    BOOST_CHECK(is_table_exists(db, PLAYCHAIN_NULL_TABLE));
}

PLAYCHAIN_TEST_CASE(check_room_with_version_creation)
{
    BOOST_REQUIRE_NO_THROW(create_room(richregistrator, "test1", "1.0.0+"));
    BOOST_REQUIRE_NO_THROW(create_room(richregistrator, "test2", "1.2.0+20190501"));
    BOOST_CHECK_THROW(create_room(richregistrator, "test3", "120+20190501"), fc::exception);
    BOOST_CHECK_THROW(create_room(richregistrator, "test4", "+20190501"), fc::exception);
    BOOST_CHECK_THROW(create_room(richregistrator, "test5", "20190501"), fc::exception);
    BOOST_CHECK_THROW(create_room(richregistrator, "test6", "..1"), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
