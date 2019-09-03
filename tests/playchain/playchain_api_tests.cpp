#include "playchain_common.hpp"

#include <fc/variant.hpp>
#include <vector>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/evaluators/validators.hpp>
#include <playchain/chain/playchain_utils.hpp>

#include <graphene/chain/hardfork.hpp>

namespace playchain_api_tests {
struct playchain_api_fixture : public playchain_common::playchain_fixture {
  const int64_t init_balance = 1000 * GRAPHENE_BLOCKCHAIN_PRECISION;
  const int64_t big_init_balance = 100000 * GRAPHENE_BLOCKCHAIN_PRECISION;

  uint32_t default_invitation_lifetime =
      PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds();

  DECLARE_ACTOR(registrator)
  DECLARE_ACTOR(alice)

  DECLARE_ACTOR(gwitness1)
  DECLARE_ACTOR(gwitness2)
  DECLARE_ACTOR(gwitness3)

  playchain_api_fixture() {
    actor(registrator).supply(asset(big_init_balance));

    actor(gwitness1).supply(asset(big_init_balance));
    actor(gwitness2).supply(asset(big_init_balance));
    actor(gwitness3).supply(asset(big_init_balance));

    init_fees();

    alice = create_new_player(registrator, alice, asset(big_init_balance));

    create_account("mike");

    //test only with latest voting algorithm!!!
    generate_blocks(HARDFORK_PLAYCHAIN_10_TIME);
  }

  bool is_account_exists(const string &name) const {
    const auto &idx =
        db.get_index_type<account_index>().indices().get<by_name>();
    const auto itr = idx.find(name);
    return (itr != idx.end());
  }

  void play_the_game(const Actor &table_owner, const table_id_type &table,
                     const Actor &witness1, const Actor &witness2,
                     const Actor &player1, const Actor &player2) {
    auto &&table_obj = table(db);

    BOOST_REQUIRE(table_obj.is_free());

    auto stake = asset(big_init_balance / 2);

    BOOST_REQUIRE_NO_THROW(buy_in_table(player1, table_owner, table, stake));
    BOOST_REQUIRE_NO_THROW(buy_in_table(player2, table_owner, table, stake));

    game_initial_data initial;
    initial.cash[actor(player1)] = stake;
    initial.cash[actor(player2)] = stake;
    initial.info = "**** is diller";

    BOOST_REQUIRE_NO_THROW(
        game_start_playing_check(table_owner, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness1, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness2, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(player1, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(player2, table, initial));

    generate_block();

    BOOST_REQUIRE(table_obj.is_playing());

    game_result result;
    auto win = asset(stake.amount / 2);
    auto win_rake = asset(win.amount / 20);
    auto &winner_result = result.cash[actor(player2)];
    winner_result.cash = stake + win - win_rake;
    winner_result.rake = win_rake;
    auto &last_result = result.cash[actor(player1)];
    last_result.cash = stake - win;
    result.log = "**** has A 4";

    BOOST_REQUIRE_NO_THROW(game_result_check(table_owner, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(player1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(witness1, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(witness2, table, result));

    generate_block();

    BOOST_REQUIRE(table_obj.is_free());
  }

  using playchain_table_info = playchain::app::playchain_table_info;

  auto get_tables_notify_callback(std::vector<playchain_table_info> &table_infos)
  {
      auto callback = [&table_infos](const fc::variant &v) {
          auto vs = v.as<std::vector<fc::variant>>(GRAPHENE_MAX_NESTED_OBJECTS);

          BOOST_REQUIRE(!vs.empty());

          table_infos.clear();

          for(auto &&v: vs)
          {
              table_infos.emplace_back(v.as<playchain_table_info>(GRAPHENE_MAX_NESTED_OBJECTS));
          }

          ilog("CALLBACK ${ti}", ("ti", table_infos));
      };
      return callback;
  }

  auto get_tables_notify_callback(playchain_table_info &table_info)
  {
      auto callback = [&table_info](const fc::variant &v) {
          auto vs = v.as<std::vector<fc::variant>>(GRAPHENE_MAX_NESTED_OBJECTS);

          BOOST_REQUIRE(!vs.empty());
          BOOST_REQUIRE(vs.size() == 1u);

          for(auto &&v: vs)
          {
              table_info = v.as<playchain_table_info>(GRAPHENE_MAX_NESTED_OBJECTS);
              break;
          }

          ilog("CALLBACK ${ti}", ("ti", table_info));
      };
      return callback;
  }
};

BOOST_FIXTURE_TEST_SUITE(playchain_api_tests, playchain_api_fixture)

PLAYCHAIN_TEST_CASE(check_negative_list_player_invitations) {
  BOOST_CHECK_THROW(pplaychain_api->list_player_invitations(
                        id_to_string(registrator), "",
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_player_invitations(
                        id_to_string(PLAYCHAIN_NULL_PLAYER), "",
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_player_invitations(
                        id_to_string(GRAPHENE_COMMITTEE_ACCOUNT), "",
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_player_invitations(
                        "", "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK_THROW(
      pplaychain_api->list_player_invitations(
          invalid_account_name, "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_player_invitations_for_single_inviter) {
  std::set<string> invitation_uids;

  const size_t INVITATIONS = 4;

  auto invitations =
      pplaychain_api->list_player_invitations(id_to_string(registrator), "", 10)
          .first;

  BOOST_REQUIRE(invitations.empty());

  for (size_t ci = 1; ci <= INVITATIONS; ++ci) {
    player_invitation_create_operation op;

    BOOST_REQUIRE_NO_THROW(create_invitation(
        registrator, default_invitation_lifetime + fc::minutes(ci).to_seconds(),
        op));
    invitation_uids.emplace(op.uid);
  }

  invitations =
      pplaychain_api->list_player_invitations(id_to_string(registrator), "", 10)
          .first;

  BOOST_REQUIRE_EQUAL(invitations.size(), invitation_uids.size());

  invitations =
      pplaychain_api->list_player_invitations(id_to_string(registrator), "", 1)
          .first;

  BOOST_REQUIRE_EQUAL(invitations.size(), 1u);

  invitation_uids.erase(invitations.begin()->uid);

  string last_uid = invitations.rbegin()->uid;

  invitations =
      pplaychain_api
          ->list_player_invitations(id_to_string(registrator), last_uid, 2)
          .first;

  BOOST_REQUIRE_EQUAL(invitations.size(), 2u);

  last_uid = invitations.rbegin()->uid;

  std::for_each(begin(invitations), end(invitations),
                [&](const player_invitation_object &obj) {
                  invitation_uids.erase(obj.uid);
                });

  BOOST_REQUIRE(PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST);

  invitations =
      pplaychain_api
          ->list_player_invitations(id_to_string(registrator), last_uid,
                                    PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST / 2)
          .first;

  BOOST_REQUIRE_EQUAL(invitations.size(), INVITATIONS - 3u);

  std::for_each(begin(invitations), end(invitations),
                [&](const player_invitation_object &obj) {
                  invitation_uids.erase(obj.uid);
                });

  BOOST_REQUIRE(invitation_uids.empty());
}

PLAYCHAIN_TEST_CASE(check_list_player_invitations_for_multi_inviters) {
  for (size_t ci = 1; ci <= 3; ++ci) {
    BOOST_REQUIRE_NO_THROW(
        create_invitation(registrator, default_invitation_lifetime +
                                           fc::minutes(ci).to_seconds()));
    BOOST_REQUIRE_NO_THROW(create_invitation(
        alice, default_invitation_lifetime + fc::minutes(ci).to_seconds()));
  }

  auto invitations =
      pplaychain_api->list_player_invitations(id_to_string(registrator), "", 2)
          .first;

  BOOST_REQUIRE_EQUAL(invitations.size(), 2u);
  BOOST_CHECK_EQUAL(invitations[0].inviter(db).name, registrator.name);
  BOOST_CHECK_EQUAL(invitations[1].inviter(db).name, registrator.name);

  auto next_registrator_uid = invitations.rbegin()->uid;

  invitations =
      pplaychain_api->list_player_invitations(id_to_string(alice), "", 2).first;

  BOOST_REQUIRE_EQUAL(invitations.size(), 2u);
  BOOST_CHECK_EQUAL(invitations[0].inviter(db).name, alice.name);
  BOOST_CHECK_EQUAL(invitations[1].inviter(db).name, alice.name);

  auto next_alice_uid = invitations.rbegin()->uid;

  invitations = pplaychain_api
                    ->list_player_invitations(id_to_string(registrator),
                                              next_registrator_uid, 2)
                    .first;

  BOOST_REQUIRE_EQUAL(invitations.size(), 1u);
  BOOST_CHECK_EQUAL(invitations[0].inviter(db).name, registrator.name);

  invitations =
      pplaychain_api
          ->list_player_invitations(id_to_string(alice), next_alice_uid, 2)
          .first;

  BOOST_REQUIRE_EQUAL(invitations.size(), 1u);
  BOOST_CHECK_EQUAL(invitations[0].inviter(db).name, alice.name);
}

PLAYCHAIN_TEST_CASE(check_list_player_invitations_detect_id) {
  BOOST_REQUIRE_NO_THROW(create_invitation(
      alice, default_invitation_lifetime + fc::minutes(1).to_seconds()));

  BOOST_CHECK_EQUAL(
      pplaychain_api->list_player_invitations(id_to_string(alice), "", 1)
          .first.size(),
      1u);

  BOOST_CHECK_EQUAL(
      pplaychain_api
          ->list_player_invitations(id_to_string(get_player(alice)), "", 1)
          .first.size(),
      1u);

  BOOST_CHECK_EQUAL(
      pplaychain_api->list_player_invitations(alice.name, "", 1).first.size(),
      1u);
}

PLAYCHAIN_TEST_CASE(check_get_player_invitation) {
  player_invitation_create_operation op;
  BOOST_REQUIRE_NO_THROW(create_invitation(
      alice, default_invitation_lifetime + fc::minutes(1).to_seconds(), op));

  BOOST_CHECK(pplaychain_api->get_player_invitation(id_to_string(alice), op.uid)
                  .valid());

  BOOST_CHECK(
      pplaychain_api
          ->get_player_invitation(id_to_string(get_player(alice)), op.uid)
          .valid());

  BOOST_CHECK(
      pplaychain_api->get_player_invitation(alice.name, op.uid).valid());
}

PLAYCHAIN_TEST_CASE(check_negative_list_all_player_invitations) {
  BOOST_CHECK_THROW(pplaychain_api->list_all_player_invitations(
                        "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_all_player_invitations(
                        id_to_string(PLAYCHAIN_NULL_PLAYER),
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_all_player_invitations(
                        id_to_string(GRAPHENE_COMMITTEE_ACCOUNT),
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  // create two invitations
  BOOST_REQUIRE_NO_THROW(create_invitation(
      alice, default_invitation_lifetime + fc::minutes(1).to_seconds()));
  BOOST_REQUIRE_NO_THROW(create_invitation(
      alice, default_invitation_lifetime + fc::minutes(2).to_seconds()));

  // it is not accept account id
  BOOST_CHECK_THROW(
      pplaychain_api->list_all_player_invitations(
          id_to_string(alice), PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_all_player_invitations) {
  const size_t INVITATIONS = 4;

  auto invitations =
      pplaychain_api->list_player_invitations(id_to_string(alice), "", 10)
          .first;

  BOOST_REQUIRE(invitations.empty());

  for (size_t ci = 1; ci <= INVITATIONS; ++ci) {
    BOOST_REQUIRE_NO_THROW(
        create_invitation(registrator, default_invitation_lifetime +
                                           fc::minutes(ci).to_seconds()));
  }

  auto ivitations = pplaychain_api->list_all_player_invitations("", 2).first;

  BOOST_CHECK_EQUAL(ivitations.size(), 2u);

  auto next_id = ivitations.rbegin()->id;

  ivitations = pplaychain_api
                   ->list_all_player_invitations(
                       id_to_string(player_invitation_id_type{next_id}), 2)
                   .first;

  BOOST_CHECK_EQUAL(ivitations.size(), 2u);

  BOOST_CHECK(ivitations.begin()->id != next_id);
  BOOST_CHECK(ivitations.rbegin()->id != next_id);
}

PLAYCHAIN_TEST_CASE(check_negative_get_player) {
  BOOST_CHECK_THROW(
      pplaychain_api->get_player(id_to_string(PLAYCHAIN_NULL_PLAYER)),
      fc::exception);

  BOOST_CHECK_THROW(
      pplaychain_api->get_player(id_to_string(GRAPHENE_COMMITTEE_ACCOUNT)),
      fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->get_player(""), fc::exception);

  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK_THROW(pplaychain_api->get_player(invalid_account_name),
                    fc::exception);
}

PLAYCHAIN_TEST_CASE(check_get_player) {
  BOOST_REQUIRE_NO_THROW(pplaychain_api->get_player(id_to_string(alice)));

  // alice is player
  auto player = pplaychain_api->get_player(id_to_string(alice));

  BOOST_CHECK(player.valid());

  BOOST_CHECK_EQUAL(player->account(db).name, alice.name);

  // registrator is not player
  player = pplaychain_api->get_player(id_to_string(registrator));

  BOOST_CHECK(!player.valid());
}

PLAYCHAIN_TEST_CASE(check_negative_list_all_players) {
  BOOST_CHECK_THROW(pplaychain_api->list_all_players(
                        "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                    fc::exception);

  // player_id_type instead account_type_id expected
  BOOST_CHECK_THROW(
      pplaychain_api->list_all_players(id_to_string(alice),
                                       PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_all_players) {
  Actor player1 =
      create_new_player(registrator, "player1", asset(init_balance));
  Actor player2 =
      create_new_player(registrator, "player2", asset(init_balance));
  Actor player3 =
      create_new_player(registrator, "player3", asset(init_balance));
  Actor player4 =
      create_new_player(registrator, "player4", asset(init_balance));

  BOOST_REQUIRE_NO_THROW(pplaychain_api->list_all_players("", 1));

  auto players = pplaychain_api->list_all_players("", 3);

  BOOST_REQUIRE_EQUAL(players.size(), 3u);
  BOOST_CHECK_EQUAL(players.begin()->account(db).name, "alice");
  BOOST_CHECK_EQUAL(players[1].account(db).name, "player1");

  auto next_id = players.rbegin()->id;

  players = pplaychain_api->list_all_players(
      id_to_string(player_id_type{next_id}), 2);

  BOOST_REQUIRE_EQUAL(players.size(), 2u);
  BOOST_CHECK_EQUAL(players.rbegin()->account(db).name, "player4");

  next_id = players.rbegin()->id;

  players = pplaychain_api->list_all_players(
      id_to_string(player_id_type{next_id}), 2);

  BOOST_CHECK(players.empty());
}

PLAYCHAIN_TEST_CASE(check_negative_list_rooms) {
  BOOST_CHECK_THROW(
      pplaychain_api->list_rooms(id_to_string(PLAYCHAIN_NULL_PLAYER), "",
                                 PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);

  BOOST_CHECK_THROW(
      pplaychain_api->list_rooms(id_to_string(GRAPHENE_COMMITTEE_ACCOUNT), "",
                                 PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_rooms(
                        "", "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK_THROW(
      pplaychain_api->list_rooms(invalid_account_name, "",
                                 PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);

  // room_id_type instead player_id_type expected
  BOOST_CHECK_THROW(
      pplaychain_api->list_rooms(id_to_string(registrator),
                                 id_to_string(player_id_type{}),
                                 PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_rooms_for_one_owner) {
  create_room(registrator);
  create_room(alice);
  create_room(registrator, "2");
  create_room(registrator, "3");

  BOOST_REQUIRE_NO_THROW(
      pplaychain_api->list_rooms(id_to_string(registrator), "", 1));

  auto rooms = pplaychain_api->list_rooms(id_to_string(registrator), "", 2);

  //    for(const auto &rec: rooms)
  //    {
  //        ilog("${o}", ("o", rec));
  //    }

  BOOST_REQUIRE_EQUAL(rooms.size(), 2u);

  auto next_id = rooms.rbegin()->id;

  rooms = pplaychain_api->list_rooms(id_to_string(registrator),
                                     id_to_string(room_id_type{next_id}), 2);

  BOOST_REQUIRE_EQUAL(rooms.size(), 1u);
}

PLAYCHAIN_TEST_CASE(check_negative_list_all_rooms) {
  BOOST_CHECK_THROW(pplaychain_api->list_all_rooms(
                        id_to_string(room_id_type{}),
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                    fc::exception);

  // room_id_type instead player_id_type expected
  BOOST_CHECK_THROW(
      pplaychain_api->list_all_rooms(id_to_string(player_id_type{}),
                                     PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_all_rooms) {
  create_room(registrator);
  create_room(registrator, "next");
  create_room(alice);

  BOOST_REQUIRE_NO_THROW(pplaychain_api->list_all_rooms("", 1));

  auto rooms = pplaychain_api->list_all_rooms("", 2);

  BOOST_REQUIRE_EQUAL(rooms.size(), 2u);
  BOOST_CHECK_EQUAL(rooms[0].owner(db).name, registrator.name);
  BOOST_CHECK_EQUAL(rooms[1].owner(db).name, registrator.name);

  auto next_id = rooms.rbegin()->id;

  rooms =
      pplaychain_api->list_all_rooms(id_to_string(room_id_type{next_id}), 2);

  BOOST_REQUIRE_EQUAL(rooms.size(), 1u);
  BOOST_CHECK_EQUAL(rooms[0].owner(db).name, alice.name);
}

PLAYCHAIN_TEST_CASE(check_negative_get_game_witness) {
  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK_THROW(pplaychain_api->get_game_witness(invalid_account_name),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->get_game_witness(""), fc::exception);

  auto room_id = create_new_room(registrator);

  // get_game_witness accept only: game witness ID, player ID, account ID,
  // account name
  BOOST_CHECK_THROW(pplaychain_api->get_game_witness(id_to_string(room_id)),
                    fc::exception);
}

PLAYCHAIN_TEST_CASE(check_get_game_witness) {
  create_witness(registrator);

  BOOST_REQUIRE_NO_THROW(
      pplaychain_api->get_game_witness(id_to_string(registrator)));

  auto witness = pplaychain_api->get_game_witness(id_to_string(registrator));

  BOOST_CHECK(witness.valid());

  BOOST_CHECK_EQUAL(witness->account(db).name, registrator.name);

  // alice is not game witness
  witness = pplaychain_api->get_game_witness(id_to_string(alice));

  BOOST_CHECK(!witness.valid());
}

PLAYCHAIN_TEST_CASE(check_negative_list_all_game_witnesses) {
  BOOST_CHECK_THROW(pplaychain_api->list_all_game_witnesses(
                        id_to_string(game_witness_id_type{}),
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                    fc::exception);

  // game_witness_id_type instead player_id_type expected
  BOOST_CHECK_THROW(pplaychain_api->list_all_game_witnesses(
                        id_to_string(player_id_type{}),
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_all_game_witnesses) {
  Actor witness1 =
      create_new_player(registrator, "witness1", asset(big_init_balance));
  Actor witness2 =
      create_new_player(registrator, "witness2", asset(big_init_balance));

  create_witness(witness1);
  create_witness(witness2);
  create_witness(registrator);

  BOOST_REQUIRE_NO_THROW(pplaychain_api->list_all_game_witnesses("", 1));

  auto witnesses = pplaychain_api->list_all_game_witnesses("", 2);

  BOOST_REQUIRE_EQUAL(witnesses.size(), 2u);
  BOOST_CHECK_EQUAL(witnesses[0].account(db).name, witness1.name);
  BOOST_CHECK_EQUAL(witnesses[1].account(db).name, witness2.name);

  auto next_id = witnesses.rbegin()->id;

  witnesses = pplaychain_api->list_all_game_witnesses(
      id_to_string(game_witness_id_type{next_id}), 2);

  BOOST_REQUIRE_EQUAL(witnesses.size(), 1u);
  BOOST_CHECK_EQUAL(witnesses[0].account(db).name, registrator.name);
}

PLAYCHAIN_TEST_CASE(check_negative_list_all_tables) {
  BOOST_CHECK_THROW(pplaychain_api->list_all_tables(
                        "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                    fc::exception);

  // table_id_type instead player_id_type expected
  BOOST_CHECK_THROW(
      pplaychain_api->list_all_tables(id_to_string(player_id_type{}),
                                      PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_all_tables) {
  auto room = create_new_room(registrator);
  create_table(registrator, room);
  create_table(registrator, room);
  room = create_new_room(alice);
  create_table(alice, room);

  BOOST_REQUIRE_NO_THROW(pplaychain_api->list_all_tables("", 1));

  auto tables = pplaychain_api->list_all_tables("", 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 2u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, registrator.name);
  BOOST_CHECK_EQUAL(tables[1].room(db).owner(db).name, registrator.name);

  auto next_id = tables.rbegin()->id;

  tables =
      pplaychain_api->list_all_tables(id_to_string(table_id_type{next_id}), 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 1u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, alice.name);
}

PLAYCHAIN_TEST_CASE(check_negative_list_tables) {
  BOOST_CHECK_THROW(
      pplaychain_api->list_tables(id_to_string(PLAYCHAIN_NULL_PLAYER), "",
                                  PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);

  BOOST_CHECK_THROW(
      pplaychain_api->list_tables(id_to_string(GRAPHENE_COMMITTEE_ACCOUNT), "",
                                  PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_tables(
                        "", "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK_THROW(
      pplaychain_api->list_tables(invalid_account_name, "",
                                  PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);

  // table_id_type instead player_id_type expected
  BOOST_CHECK_THROW(
      pplaychain_api->list_tables(id_to_string(registrator),
                                  id_to_string(player_id_type{}),
                                  PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_list_tables_by_room) {
  auto registrator_room = create_new_room(registrator);
  auto alice_room = create_new_room(alice);

  create_table(registrator, registrator_room);
  create_table(alice, alice_room);
  create_table(registrator, registrator_room);
  create_table(alice, alice_room);
  create_table(registrator, registrator_room);

  BOOST_REQUIRE_NO_THROW(
      pplaychain_api->list_tables(id_to_string(registrator_room), "", 1));
  BOOST_REQUIRE_NO_THROW(
      pplaychain_api->list_tables(id_to_string(alice_room), "", 1));

  auto tables =
      pplaychain_api->list_tables(id_to_string(registrator_room), "", 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 2u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, registrator.name);
  BOOST_CHECK_EQUAL(tables[1].room(db).owner(db).name, registrator.name);

  auto next_registrator_id = tables.rbegin()->id;

  tables = pplaychain_api->list_tables(id_to_string(alice_room), "", 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 2u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, alice.name);
  BOOST_CHECK_EQUAL(tables[1].room(db).owner(db).name, alice.name);

  auto next_alice_id = tables.rbegin()->id;

  tables = pplaychain_api->list_tables(
      id_to_string(registrator_room),
      id_to_string(table_id_type{next_registrator_id}), 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 1u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, registrator.name);

  tables = pplaychain_api->list_tables(
      id_to_string(alice_room), id_to_string(table_id_type{next_alice_id}), 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 0u);
}

PLAYCHAIN_TEST_CASE(check_list_tables_by_owner) {
  auto registrator_room = create_new_room(registrator);
  auto alice_room = create_new_room(alice);

  create_table(registrator, registrator_room);
  create_table(alice, alice_room);
  create_table(registrator, registrator_room);
  create_table(alice, alice_room);
  create_table(registrator, registrator_room);

  try{
      pplaychain_api->list_tables(id_to_string(registrator), "", 1);
  } FC_LOG_AND_RETHROW()

  BOOST_REQUIRE_NO_THROW(
      pplaychain_api->list_tables(id_to_string(alice), "", 1));

  auto tables = pplaychain_api->list_tables(id_to_string(registrator), "", 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 2u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, registrator.name);
  BOOST_CHECK_EQUAL(tables[1].room(db).owner(db).name, registrator.name);

  auto next_registrator_id = tables.rbegin()->id;

  tables = pplaychain_api->list_tables(id_to_string(alice), "", 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 2u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, alice.name);
  BOOST_CHECK_EQUAL(tables[1].room(db).owner(db).name, alice.name);

  auto next_alice_id = tables.rbegin()->id;

  tables = pplaychain_api->list_tables(
      id_to_string(registrator),
      id_to_string(table_id_type{next_registrator_id}), 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 1u);
  BOOST_CHECK_EQUAL(tables[0].room(db).owner(db).name, registrator.name);

  tables = pplaychain_api->list_tables(
      id_to_string(alice), id_to_string(table_id_type{next_alice_id}), 2);

  BOOST_REQUIRE_EQUAL(tables.size(), 0u);
}

PLAYCHAIN_TEST_CASE(check_list_tables_by_owner_and_room) {
  auto registrator_room = create_new_room(registrator);
  auto alice_room = create_new_room(alice);

  create_table(registrator, registrator_room);
  create_table(alice, alice_room);
  create_table(alice, alice_room);
  create_table(alice, alice_room);
  create_table(registrator, registrator_room);
  create_table(registrator, registrator_room);
  create_table(registrator, registrator_room);
  create_table(alice, alice_room);
  create_table(registrator, registrator_room);

  std::set<table_id_type> table_ids;

  BOOST_REQUIRE_GE(PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST, 10);

  auto tables = pplaychain_api->list_tables(
      id_to_string(registrator), "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST);

  std::for_each(
      begin(tables), end(tables),
      [&table_ids](const table_object &table) { table_ids.insert(table.id); });

  tables = pplaychain_api->list_tables(id_to_string(alice), "",
                                       PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST);

  std::for_each(
      begin(tables), end(tables),
      [&table_ids](const table_object &table) { table_ids.insert(table.id); });

  tables = pplaychain_api->list_tables(id_to_string(registrator), "", 3);

  BOOST_REQUIRE_EQUAL(tables.size(), 3u);

  std::for_each(
      begin(tables), end(tables),
      [&table_ids](const table_object &table) { table_ids.erase(table.id); });

  auto next_registrator_id = tables.rbegin()->id;

  tables = pplaychain_api->list_tables(id_to_string(alice_room), "", 3);

  BOOST_REQUIRE_EQUAL(tables.size(), 3u);

  std::for_each(
      begin(tables), end(tables),
      [&table_ids](const table_object &table) { table_ids.erase(table.id); });

  auto next_alice_id = tables.rbegin()->id;

  tables = pplaychain_api->list_tables(
      id_to_string(registrator_room),
      id_to_string(table_id_type{next_registrator_id}), 3);

  BOOST_REQUIRE_EQUAL(tables.size(), 2u);

  std::for_each(
      begin(tables), end(tables),
      [&table_ids](const table_object &table) { table_ids.erase(table.id); });

  tables = pplaychain_api->list_tables(
      id_to_string(alice), id_to_string(table_id_type{next_alice_id}), 3);

  BOOST_REQUIRE_EQUAL(tables.size(), 1u);

  std::for_each(
      begin(tables), end(tables),
      [&table_ids](const table_object &table) { table_ids.erase(table.id); });

  BOOST_CHECK(table_ids.empty());
}

PLAYCHAIN_TEST_CASE(check_negative_get_playchain_balance_info) {
  BOOST_CHECK_THROW(pplaychain_api->get_playchain_balance_info(
                        id_to_string(PLAYCHAIN_NULL_PLAYER)),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->get_playchain_balance_info(
                        id_to_string(GRAPHENE_COMMITTEE_ACCOUNT)),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->get_playchain_balance_info(""),
                    fc::exception);

  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK_THROW(
      pplaychain_api->get_playchain_balance_info(invalid_account_name),
      fc::exception);
}

PLAYCHAIN_TEST_CASE(check_get_playchain_balance_info) {
  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 2u);

  Actor witness1 =
      create_new_player(registrator, "witness1", asset(big_init_balance));
  Actor witness2 =
      create_new_player(registrator, "witness2", asset(big_init_balance));
  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  create_witness(witness1);
  create_witness(witness2);

  string last_uid;
  {
    player_invitation_create_operation op;

    BOOST_REQUIRE_NO_THROW(create_invitation(
        alice,
        PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds() +
            fc::minutes(1).to_seconds(),
        op));
    last_uid = op.uid;
  }

  Actor sam{"sam"};

  BOOST_REQUIRE_NO_THROW(resolve_invitation(alice, last_uid, sam));

  generate_block();

  BOOST_REQUIRE(is_account_exists(sam.name));

  actor(sam).supply(asset(big_init_balance));

  sam.private_key = create_private_key_from_password(sam.name, sam.name);

  play_the_game(registrator, table, witness1, witness2, bob, sam);

  next_maintenance();

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(db.get_global_properties()
                                  .parameters.cashback_vesting_period_seconds));

  auto &&alice_balance =
      pplaychain_api->get_playchain_balance_info(id_to_string(alice));

  BOOST_CHECK(alice_balance.account_balance > asset{0});
  BOOST_CHECK(alice_balance.referral_balance > asset{0});
  BOOST_CHECK(alice_balance.referral_balance_id.valid());
  BOOST_CHECK(alice_balance.rake_balance == asset{0});
  BOOST_CHECK(!alice_balance.rake_balance_id.valid());
  BOOST_CHECK(alice_balance.witness_balance == asset{0});
  BOOST_CHECK(!alice_balance.witness_balance_id.valid());

  auto &&bob_balance =
      pplaychain_api->get_playchain_balance_info(id_to_string(bob));

  BOOST_CHECK(bob_balance.account_balance > asset{0});
  BOOST_CHECK(bob_balance.referral_balance == asset{0});
  BOOST_CHECK(bob_balance.rake_balance == asset{0});
  BOOST_CHECK(bob_balance.witness_balance == asset{0});

  auto &&sam_balance =
      pplaychain_api->get_playchain_balance_info(id_to_string(sam));

  BOOST_CHECK(sam_balance.account_balance > asset{0});
  BOOST_CHECK(sam_balance.referral_balance == asset{0});
  BOOST_CHECK(sam_balance.rake_balance == asset{0});
  BOOST_CHECK(sam_balance.witness_balance == asset{0});

  auto &&registrator_balance =
      pplaychain_api->get_playchain_balance_info(id_to_string(registrator));

  BOOST_CHECK(registrator_balance.referral_balance == asset{0});
  BOOST_CHECK(!registrator_balance.referral_balance_id.valid());
  BOOST_CHECK(registrator_balance.rake_balance > asset{0});
  BOOST_CHECK(registrator_balance.rake_balance_id.valid());
  BOOST_CHECK(registrator_balance.witness_balance == asset{0});
  BOOST_CHECK(!registrator_balance.witness_balance_id.valid());

  auto &&witness1_balance =
      pplaychain_api->get_playchain_balance_info(id_to_string(witness1));

  BOOST_CHECK(witness1_balance.referral_balance == asset{0});
  BOOST_CHECK(witness1_balance.rake_balance == asset{0});
  BOOST_CHECK(witness1_balance.witness_balance > asset{0});

  auto &&witness2_balance =
      pplaychain_api->get_playchain_balance_info(id_to_string(witness2));

  BOOST_CHECK(witness2_balance.referral_balance == asset{0});
  BOOST_CHECK(!witness2_balance.referral_balance_id.valid());
  BOOST_CHECK(witness2_balance.rake_balance == asset{0});
  BOOST_CHECK(!witness2_balance.rake_balance_id.valid());
  BOOST_CHECK(witness2_balance.witness_balance > asset{0});
  BOOST_CHECK(witness2_balance.witness_balance_id.valid());
}

PLAYCHAIN_TEST_CASE(check_negative_get_account_id_by_name) {
  BOOST_CHECK(pplaychain_api->get_account_id_by_name({""}).empty());

  flat_set<string> fake_accounts;
  for (size_t ci = 1; ci <= PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1; ++ci) {
    fake_accounts.insert("player" + std::to_string(ci));
  }

  BOOST_CHECK_THROW(pplaychain_api->get_account_id_by_name(fake_accounts),
                    fc::exception);

  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK(
      pplaychain_api->get_account_id_by_name({invalid_account_name}).empty());
}

PLAYCHAIN_TEST_CASE(check_get_account_id_by_name) {
  BOOST_CHECK(pplaychain_api->get_account_id_by_name({alice.name}).size() ==
              1u);

  BOOST_CHECK(
      actor(alice) ==
      pplaychain_api->get_account_id_by_name({alice.name}).at(alice.name));
}

PLAYCHAIN_TEST_CASE(check_negative_get_tables_info_by_id) {
  flat_set<table_id_type> fake_tabls;
  for (size_t ci = 1; ci <= PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1; ++ci) {
    fake_tabls.insert(table_id_type{ci});
  }

  BOOST_CHECK_THROW(pplaychain_api->get_tables_info_by_id(fake_tabls),
                    fc::exception);
}

PLAYCHAIN_TEST_CASE(check_get_tables_info_by_id) {
  using namespace playchain::app;

  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 2u);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  BOOST_CHECK_NO_THROW(pplaychain_api->get_tables_info_by_id({table}));

  auto &&infos = pplaychain_api->get_tables_info_by_id({table});

  BOOST_REQUIRE_EQUAL(infos.size(), 1u);

  BOOST_CHECK(infos[0].id == table);
  BOOST_CHECK(infos[0].metadata == default_table_metadata);
  BOOST_CHECK(infos[0].required_witnesses == 2u);
  BOOST_CHECK(infos[0].state == playchain_table_state::free);

  Actor witness1 =
      create_new_player(registrator, "witness1", asset(big_init_balance));
  Actor witness2 =
      create_new_player(registrator, "witness2", asset(big_init_balance));

  create_witness(witness1);
  create_witness(witness2);

  auto &&table_obj = table(db);

  BOOST_REQUIRE(table_obj.is_free());

  auto stake = asset(big_init_balance / 2);

  BOOST_REQUIRE_NO_THROW(buy_in_table(alice, registrator, table, stake));
  BOOST_REQUIRE_NO_THROW(buy_in_table(bob, registrator, table, stake));

  game_initial_data initial;
  initial.cash[actor(alice)] = stake;
  initial.cash[actor(bob)] = stake;
  initial.info = "**** is diller";

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::voting_for_playing);

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness1, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(witness2, table, initial));

  generate_block();

  BOOST_REQUIRE(table_obj.is_playing());

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::playing);

  game_result result;
  auto win = asset(stake.amount / 2);
  auto win_rake = asset(win.amount / 20);
  auto &winner_result = result.cash[actor(alice)];
  winner_result.cash = stake + win - win_rake;
  winner_result.rake = win_rake;
  auto &last_result = result.cash[actor(bob)];
  last_result.cash = stake - win;
  result.log = "**** has A 4";

  BOOST_REQUIRE_NO_THROW(game_result_check(registrator, table, result));

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::voting_for_results);

  BOOST_REQUIRE_NO_THROW(game_result_check(alice, table, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(bob, table, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(witness1, table, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(witness2, table, result));

  generate_block();

  BOOST_REQUIRE(table_obj.is_free());

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::free);
}

PLAYCHAIN_TEST_CASE(check_negative_list_invited_players) {
  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  BOOST_CHECK_THROW(pplaychain_api->list_invited_players(
                        id_to_string(registrator), "",
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST + 1),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_invited_players(
                        id_to_string(PLAYCHAIN_NULL_PLAYER), "",
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_invited_players(
                        id_to_string(GRAPHENE_COMMITTEE_ACCOUNT), "",
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_invited_players(
                        "", "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  const std::string invalid_account_name = "abracadabra";
  BOOST_REQUIRE(is_valid_name(invalid_account_name) &&
                !is_account_exists(invalid_account_name));

  BOOST_CHECK_THROW(
      pplaychain_api->list_invited_players(
          invalid_account_name, "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
      fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_invited_players(
                        id_to_string(registrator),
                        id_to_string(room_id_type(1)),
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  BOOST_CHECK_THROW(pplaychain_api->list_invited_players(
                        id_to_string(registrator),
                        id_to_string(table_id_type(1)),
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  // registrator is not player
  BOOST_CHECK_THROW(pplaychain_api->list_invited_players(
                        id_to_string(registrator), "",
                        PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST),
                    fc::exception);

  BOOST_CHECK_NO_THROW(pplaychain_api->list_invited_players(
      id_to_string(alice), "", PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST));

  BOOST_CHECK_NO_THROW(pplaychain_api->list_invited_players(
      id_to_string(alice), id_to_string(bob),
      PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST));
}

PLAYCHAIN_TEST_CASE(check_list_invited_players) {
  BOOST_CHECK(pplaychain_api
                  ->list_invited_players(id_to_string(alice), "",
                                         PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST)
                  .empty());

  string last_uid;
  {
    player_invitation_create_operation op;

    BOOST_REQUIRE_NO_THROW(create_invitation(
        alice,
        PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds() +
            fc::minutes(1).to_seconds(),
        op));
    last_uid = op.uid;
  }

  BOOST_CHECK(pplaychain_api
                  ->list_invited_players(id_to_string(alice), "",
                                         PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST)
                  .empty());

  Actor sam1("sam"), sam2("sam2");

  BOOST_REQUIRE_NO_THROW(resolve_invitation(alice, last_uid, sam1));

  generate_block();

  BOOST_REQUIRE(is_account_exists(sam1));

  BOOST_CHECK_EQUAL(
      pplaychain_api
          ->list_invited_players(id_to_string(alice), "",
                                 PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST)
          .size(),
      1u);

  {
    player_invitation_create_operation op;

    BOOST_REQUIRE_NO_THROW(create_invitation(
        alice,
        PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds() +
            fc::minutes(1).to_seconds(),
        op));
    last_uid = op.uid;
  }

  BOOST_REQUIRE_NO_THROW(resolve_invitation(alice, last_uid, sam2));

  generate_block();

  BOOST_REQUIRE(is_account_exists(sam2));

  BOOST_CHECK_EQUAL(
      pplaychain_api
          ->list_invited_players(id_to_string(alice), "",
                                 PLAYCHAIN_MAX_SIZE_FOR_RESULT_API_LIST)
          .size(),
      2u);

  auto &&invited =
      pplaychain_api->list_invited_players(id_to_string(alice), "", 1u);

  BOOST_REQUIRE(!invited.empty());

  BOOST_CHECK(account_id_type{invited[0].second.id} == actor(sam1));

  invited = pplaychain_api->list_invited_players(
      id_to_string(alice), id_to_string(player_id_type{invited[0].first.id}),
      1u);

  BOOST_REQUIRE(!invited.empty());

  BOOST_CHECK(account_id_type{invited[0].second.id} == actor(sam2));
}

BOOST_AUTO_TEST_CASE(set_tables_subscribe_callback_check) {
  using namespace playchain::app;

  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 0u);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  playchain_table_info table_info;

  auto callback = get_tables_notify_callback(table_info);

  pplaychain_api->set_tables_subscribe_callback(callback, {table});

  auto stake = asset(player_init_balance / 2);

  BOOST_CHECK_NO_THROW(buy_in_table(alice, registrator, table, stake));
  BOOST_CHECK_NO_THROW(buy_in_table(bob, registrator, table, stake));

  game_initial_data initial;
  initial.cash[actor(alice)] = stake;
  initial.cash[actor(bob)] = stake;
  initial.info = "**** is diller";

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::free);

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::voting_for_playing);

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::playing);

  game_result result;
  auto win = asset(stake.amount / 2);
  auto win_rake = asset(win.amount / 20);
  auto &winner_result = result.cash[actor(alice)];
  winner_result.cash = stake + win - win_rake;
  winner_result.rake = win_rake;
  auto &last_result = result.cash[actor(bob)];
  last_result.cash = stake - win;
  result.log = "**** has A 4";

  BOOST_REQUIRE_NO_THROW(game_result_check(registrator, table, result));

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::voting_for_results);

  BOOST_REQUIRE_NO_THROW(game_result_check(alice, table, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(bob, table, result));

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::free);
}

BOOST_AUTO_TEST_CASE(
    set_tables_subscribe_callback_check_for_start_game_expiration) {
  using namespace playchain::app;

  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 0u);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  auto stake = asset(player_init_balance / 2);

  BOOST_CHECK_NO_THROW(buy_in_table(alice, registrator, table, stake));
  BOOST_CHECK_NO_THROW(buy_in_table(bob, registrator, table, stake));

  playchain_table_info table_info;

  auto callback = get_tables_notify_callback(table_info);

  pplaychain_api->set_tables_subscribe_callback(callback, {table});

  game_initial_data initial;
  initial.cash[actor(alice)] = stake;
  initial.cash[actor(bob)] = stake;
  initial.info = "**** is diller";

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::free);

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::voting_for_playing);

  const auto &params = get_playchain_parameters(db);

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(params.voting_for_playing_expiration_seconds));
  fc::usleep(fc::milliseconds(200));
  idump((table_info));

  BOOST_CHECK(table_info.state == playchain_table_state::free);
}

BOOST_AUTO_TEST_CASE(
    update_tables_subscribe_callback_when_buy_in_reserve_check) {
  using namespace playchain::app;

  static const std::string table_meta = "test";

  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 0u, table_meta);

  BOOST_REQUIRE_NO_THROW(table_alive(registrator, table));

  playchain_table_info_ext table_info;

  auto callback = get_tables_notify_callback(table_info);

  pplaychain_api->set_tables_subscribe_callback(callback, {table});

  auto stake = asset(player_init_balance / 2);

  try {
    buy_in_reserve(alice, "1", stake, table_meta);
  }
  FC_LOG_AND_RETHROW()

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_info));
}

PLAYCHAIN_TEST_CASE(check_get_rooms_info_by_id) {
  using namespace playchain::app;

  room_id_type room0 = create_new_room(registrator);
  room_id_type room1 = create_new_room(registrator, "next1");
  room_id_type room2 = create_new_room(registrator, "next2");
  room_id_type invalid_room = room_id_type{room2.instance.value + 1};

  generate_block();

  BOOST_CHECK(pplaychain_api->get_rooms_info_by_id({room0})[0].metadata ==
              default_room_metadata);
  BOOST_CHECK(pplaychain_api->get_rooms_info_by_id({room1})[0].metadata ==
              "next1");
  BOOST_CHECK(pplaychain_api->get_rooms_info_by_id({room2})[0].metadata ==
              "next2");

  generate_block();

  BOOST_CHECK(
      pplaychain_api->get_rooms_info_by_id({room0, room1, room2, invalid_room})
          .size() == 3u);

  table_id_type table = create_new_table(registrator, room1, 2u);

  Actor witness1 =
      create_new_player(registrator, "witness1", asset(big_init_balance));
  Actor witness2 =
      create_new_player(registrator, "witness2", asset(big_init_balance));
  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));
  Actor sam = create_new_player(registrator, "sam", asset(big_init_balance));

  create_witness(witness1);
  create_witness(witness2);

  play_the_game(registrator, table, witness1, witness2, bob, sam);

  next_maintenance();

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(db.get_global_properties()
                                  .parameters.cashback_vesting_period_seconds));

  playchain_room_info room1_info =
      pplaychain_api->get_rooms_info_by_id({room1})[0];

  BOOST_CHECK(room1_info.rake_balance_id.valid());
  BOOST_CHECK(room1_info.rake_balance.amount > 0);

  playchain_room_info room2_info =
      pplaychain_api->get_rooms_info_by_id({room2})[0];

  BOOST_CHECK(!room2_info.rake_balance_id.valid());
  BOOST_CHECK(room2_info.rake_balance.amount == 0);
}

PLAYCHAIN_TEST_CASE(check_get_room_info) {
  using namespace playchain::app;

  create_new_room(registrator);
  create_new_room(registrator, "next1");

  BOOST_CHECK(pplaychain_api->get_room_info(registrator, default_room_metadata)
                  .valid());
  BOOST_CHECK(pplaychain_api->get_room_info(registrator, "next1").valid());
  BOOST_CHECK(
      !pplaychain_api->get_room_info(registrator, "abracadabra").valid());
}

PLAYCHAIN_TEST_CASE(check_get_tables_info_by_metadata) {
  using namespace playchain::app;

  room_id_type room = create_new_room(registrator);

  for (size_t ci = 0; ci < 5; ++ci) {
    create_new_table(registrator, room, 0u, "meta1");
    generate_block();
    std::stringstream other_meta;
    other_meta << "meta-" << ci;
    create_new_table(registrator, room, 0u, other_meta.str());
    create_new_table(registrator, room, 0u, "meta2");
    generate_block();
  }

  auto &&tables_with_meta1 = pplaychain_api->get_tables_info_by_metadata(
      id_to_string(room), "meta1", 3);
  BOOST_REQUIRE_EQUAL(tables_with_meta1.size(), 3u);
  for (const playchain_table_info &info : tables_with_meta1) {
    BOOST_CHECK_EQUAL(info.metadata, "meta1");
  }
  auto &&tables_with_meta2 = pplaychain_api->get_tables_info_by_metadata(
      id_to_string(room), "meta2", 3);
  BOOST_REQUIRE_EQUAL(tables_with_meta2.size(), 3u);
  for (const playchain_table_info &info : tables_with_meta2) {
    BOOST_CHECK_EQUAL(info.metadata, "meta2");
  }
}

PLAYCHAIN_TEST_CASE(check_list_tables_with_player) {
  using namespace playchain::app;

  static const std::string table_with_game_meta = "Game";
  static const std::string table_with_alice_meta = "Pending";

  room_id_type room = create_new_room(registrator);

  table_id_type table_with_game =
      create_new_table(registrator, room, 0u, table_with_game_meta);
  table_id_type table_with_alice =
      create_new_table(registrator, room, 0u, table_with_alice_meta);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));
  Actor sam = create_new_player(registrator, "sam", asset(big_init_balance));

  auto stake = asset(big_init_balance / 2);

  BOOST_REQUIRE_NO_THROW(table_alive(registrator, table_with_game));
  BOOST_REQUIRE_NO_THROW(table_alive(registrator, table_with_alice));

  BOOST_REQUIRE_NO_THROW(buy_in_reserve(alice, get_next_uid(actor(alice)),
                                        stake, table_with_alice_meta));

  generate_block();

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(alice, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(alice, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::pending);
    BOOST_CHECK(info.table.id == table_with_alice);
  }

  const auto &params = get_playchain_parameters(db);

  generate_blocks(
      db.get_dynamic_global_properties().time +
      fc::seconds(params.pending_buyin_proposal_lifetime_limit_in_seconds));

  const table_object &table_with_game_obj = table_with_game(db);

  BOOST_REQUIRE(table_with_game_obj.is_free());

  BOOST_REQUIRE_NO_THROW(
      buy_in_table(bob, registrator, table_with_game, stake));
  BOOST_REQUIRE_NO_THROW(
      buy_in_table(sam, registrator, table_with_game, stake));

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(bob, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(bob, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::attable);
    BOOST_CHECK(info.table.id == table_with_game);
  }
  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(sam, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(sam, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::attable);
    BOOST_CHECK(info.table.id == table_with_game);
  }

  game_initial_data initial;
  initial.cash[actor(bob)] = stake;
  initial.cash[actor(sam)] = stake;
  initial.info = "**** is diller";

  BOOST_REQUIRE_NO_THROW(
      game_start_playing_check(registrator, table_with_game, initial));
  BOOST_REQUIRE_NO_THROW(
      game_start_playing_check(bob, table_with_game, initial));
  BOOST_REQUIRE_NO_THROW(
      game_start_playing_check(sam, table_with_game, initial));

  generate_block();

  BOOST_REQUIRE(table_with_game_obj.is_playing());

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(bob, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(bob, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
    BOOST_CHECK(info.table.id == table_with_game);
  }
  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(sam, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(sam, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
    BOOST_CHECK(info.table.id == table_with_game);
  }

  generate_block();

  game_result result;
  auto win = asset(stake.amount / 2);
  auto win_rake = asset(win.amount / 20);
  auto &winner_result = result.cash[actor(sam)];
  winner_result.cash = stake + win - win_rake;
  asset sam_new_cash = winner_result.cash;
  winner_result.rake = win_rake;
  auto &last_result = result.cash[actor(bob)];
  last_result.cash = stake - win;
  asset bob_new_cash = last_result.cash;
  result.log = "**** has A 4";

  BOOST_REQUIRE_NO_THROW(
      game_result_check(registrator, table_with_game, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(bob, table_with_game, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(sam, table_with_game, result));

  generate_block();

  BOOST_REQUIRE(table_with_game_obj.is_free());

  BOOST_REQUIRE_NO_THROW(buy_out_table(sam, table_with_game, sam_new_cash));
  BOOST_REQUIRE_NO_THROW(buy_out_table(bob, table_with_game, bob_new_cash));

  BOOST_CHECK(pplaychain_api->list_tables_with_player(alice, 10).empty());
  BOOST_CHECK(pplaychain_api->list_tables_with_player(bob, 10).empty());
  BOOST_CHECK(pplaychain_api->list_tables_with_player(sam, 10).empty());
}

PLAYCHAIN_TEST_CASE(check_list_tables_with_player_and_buyouting) {
  using namespace playchain::app;

  static const std::string game = "Game";

  room_id_type room = create_new_room(registrator);

  table_id_type table = create_new_table(registrator, room, 0u, game);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));
  Actor sam = create_new_player(registrator, "sam", asset(big_init_balance));

  auto stake = asset(big_init_balance / 2);

  BOOST_REQUIRE_NO_THROW(buy_in_table(bob, registrator, table, stake));
  BOOST_REQUIRE_NO_THROW(buy_in_table(sam, registrator, table, stake));

  generate_block();

  const table_object &table_obj = table(db);

  BOOST_REQUIRE(table_obj.is_free());

  game_initial_data initial;
  initial.cash[actor(bob)] = stake;
  initial.cash[actor(sam)] = stake;
  initial.info = "**** is diller";

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(sam, table, initial));

  generate_block();

  BOOST_REQUIRE(table_obj.is_playing());

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(bob, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(bob, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
    BOOST_CHECK(info.table.id == table);
    BOOST_CHECK_EQUAL(to_string(info.balance), to_string(stake));
    BOOST_CHECK_EQUAL(to_string(info.buyouting_balance), to_string(asset{}));
  }

  BOOST_REQUIRE_NO_THROW(
      buy_out_table(sam, table, asset(big_init_balance / 2)));

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(sam, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(sam, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
    BOOST_CHECK(info.table.id == table);
    BOOST_CHECK_EQUAL(to_string(info.balance), to_string(stake));
    BOOST_CHECK_EQUAL(to_string(info.buyouting_balance),
                      to_string(asset{big_init_balance / 2}));
  }
}

PLAYCHAIN_TEST_CASE(check_version_ext_conversions) {
  static const char *VER_EXP = "1.2.3+20190501";
  std::string ver_str{VER_EXP};
  fc::variant ver_var{ver_str};
  version_ext ver;
  fc::from_variant(ver_var, ver);

  BOOST_CHECK(ver.base.major_v() == 1);
  BOOST_CHECK(ver.base.minor_v() == 2);
  BOOST_CHECK(ver.base.patch() == 3);
  BOOST_CHECK(ver.metadata == "20190501");

  ver_str = ver;

  BOOST_CHECK(ver_str == VER_EXP);

  ver_var.clear();

  fc::to_variant(ver, ver_var);

  BOOST_CHECK(ver_var.as_string() == VER_EXP);
}

PLAYCHAIN_TEST_CASE(check_playchain_login) {
  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  generate_block();

  BOOST_CHECK(!pplaychain_api->get_player_account_by_name("mike").valid());
  BOOST_CHECK(pplaychain_api->get_player_account_by_name("bob").valid());

  playchain::app::playchain_account_info info =
      (*pplaychain_api->get_player_account_by_name("bob"));

  BOOST_CHECK(playchain::chain::is_account_exists(db, info.account));
  BOOST_CHECK(playchain::chain::is_player_exists(db, info.player));
  BOOST_CHECK(info.login_key == bob.public_key);
}

PLAYCHAIN_TEST_CASE(check_get_tables_info_by_id_with_missed_votes) {
  using namespace playchain::app;

  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 0u);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  BOOST_CHECK_NO_THROW(pplaychain_api->get_tables_info_by_id({table}));

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::free);
    BOOST_CHECK(infos[0].cash.size() == 0);
    BOOST_CHECK(infos[0].playing_cash.size() == 0);
    BOOST_CHECK(infos[0].missed_voters.size() == 0);
  }

  auto &&table_obj = table(db);

  BOOST_REQUIRE(table_obj.is_free());

  auto stake = asset(big_init_balance / 2);

  BOOST_REQUIRE_NO_THROW(buy_in_table(alice, registrator, table, stake));
  BOOST_REQUIRE_NO_THROW(buy_in_table(bob, registrator, table, stake));

  game_initial_data initial;
  initial.cash[actor(alice)] = stake;
  initial.cash[actor(bob)] = stake;
  initial.info = "**** is diller";

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::voting_for_playing);

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));

  const auto &params = get_playchain_parameters(db);

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(params.voting_for_playing_expiration_seconds));

  BOOST_REQUIRE(table_obj.is_free());

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::free);
    BOOST_CHECK(infos[0].cash.size() == 2);
    BOOST_CHECK(infos[0].playing_cash.size() == 0);
    BOOST_CHECK(infos[0].missed_voters.size() == 1);
    auto info = (*infos[0].missed_voters.begin());
    BOOST_CHECK(info.second == bob.name);
  }

  generate_block();

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));

  generate_block();

  BOOST_REQUIRE(table_obj.is_playing());

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::playing);
    BOOST_CHECK(infos[0].cash.size() == 0);
    BOOST_CHECK(infos[0].playing_cash.size() == 2);
    BOOST_CHECK(infos[0].missed_voters.size() == 0);
  }

  game_result result;
  auto win = asset(stake.amount / 2);
  auto win_rake = asset(win.amount / 20);
  auto &winner_result = result.cash[actor(alice)];
  winner_result.cash = stake + win - win_rake;
  winner_result.rake = win_rake;
  auto &last_result = result.cash[actor(bob)];
  last_result.cash = stake - win;
  result.log = "**** has A 4";

  BOOST_REQUIRE_NO_THROW(game_result_check(registrator, table, result));

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::voting_for_results);

  BOOST_REQUIRE_NO_THROW(game_result_check(bob, table, result));

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(params.voting_for_results_expiration_seconds));

  BOOST_REQUIRE(table_obj.is_free());

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::free);
    BOOST_CHECK(infos[0].cash.size() == 2);
    BOOST_CHECK(infos[0].playing_cash.size() == 0);
    BOOST_CHECK(infos[0].missed_voters.size() == 1);
    auto info = (*infos[0].missed_voters.begin());
    BOOST_CHECK(info.second == alice.name);
  }
}

PLAYCHAIN_TEST_CASE(check_get_tables_info_by_id_with_votes_stat) {
  using namespace playchain::app;

  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 0u);

  create_witness(gwitness1);
  create_witness(gwitness2);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  BOOST_CHECK_NO_THROW(pplaychain_api->get_tables_info_by_id({table}));

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::free);
    BOOST_CHECK(infos[0].cash.size() == 0);
    BOOST_CHECK(infos[0].playing_cash.size() == 0);
    BOOST_CHECK(infos[0].voters.size() == 0);
    BOOST_CHECK(infos[0].missed_voters.size() == 0);
  }

  auto &&table_obj = table(db);

  BOOST_REQUIRE(table_obj.is_free());

  auto stake = asset(big_init_balance / 2);

  BOOST_REQUIRE_NO_THROW(buy_in_table(alice, registrator, table, stake));
  BOOST_REQUIRE_NO_THROW(buy_in_table(bob, registrator, table, stake));

  game_initial_data initial;
  initial.cash[actor(alice)] = stake;
  initial.cash[actor(bob)] = stake;
  initial.info = "**** is diller";

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::voting_for_playing);

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));

  const auto &params = get_playchain_parameters(db);

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(params.voting_for_playing_expiration_seconds));

  BOOST_REQUIRE(table_obj.is_free());

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::free);
    BOOST_CHECK(infos[0].cash.size() == 2);
    BOOST_CHECK(infos[0].playing_cash.size() == 0);
    BOOST_CHECK(infos[0].voters.size() == 2);
    BOOST_CHECK(infos[0].missed_voters.size() == 1);
    auto info = (*infos[0].missed_voters.begin());
    BOOST_CHECK(info.second == bob.name);
  }

  generate_block();

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(gwitness1, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(gwitness2, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));

  generate_block();

  BOOST_REQUIRE(table_obj.is_playing());

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::playing);
    BOOST_CHECK(infos[0].cash.size() == 0);
    BOOST_CHECK(infos[0].playing_cash.size() == 2);
    BOOST_CHECK(infos[0].voters.size() == 5);
    BOOST_CHECK(infos[0].missed_voters.size() == 0);
  }

  game_result result;
  auto win = asset(stake.amount / 2);
  auto win_rake = asset(win.amount / 20);
  auto &winner_result = result.cash[actor(alice)];
  winner_result.cash = stake + win - win_rake;
  winner_result.rake = win_rake;
  auto &last_result = result.cash[actor(bob)];
  last_result.cash = stake - win;
  result.log = "**** has A 4";

  BOOST_REQUIRE_NO_THROW(game_result_check(registrator, table, result));

  BOOST_CHECK(pplaychain_api->get_tables_info_by_id({table})[0].state ==
              playchain_table_state::voting_for_results);

  BOOST_REQUIRE_NO_THROW(game_result_check(bob, table, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(gwitness1, table, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(gwitness2, table, result));

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(params.voting_for_results_expiration_seconds));

  BOOST_REQUIRE(table_obj.is_free());

  {
    auto &&infos = pplaychain_api->get_tables_info_by_id({table});

    BOOST_REQUIRE_EQUAL(infos.size(), 1u);

    BOOST_CHECK(infos[0].id == table);
    BOOST_CHECK(infos[0].state == playchain_table_state::free);
    BOOST_CHECK(infos[0].cash.size() == 2);
    BOOST_CHECK(infos[0].playing_cash.size() == 0);
    BOOST_CHECK(infos[0].voters.size() == 4);
    BOOST_CHECK(infos[0].missed_voters.size() == 1);
    auto info = (*infos[0].missed_voters.begin());
    BOOST_CHECK(info.second == alice.name);
  }
}

PLAYCHAIN_TEST_CASE(check_list_tables_with_player_when_game_is_finished) {
  using namespace playchain::app;

  static const std::string meta = "Game";

  room_id_type room = create_new_room(registrator);

  table_id_type table = create_new_table(registrator, room, 0u, meta);

  const auto &params = get_playchain_parameters(db);

  Actor b1 = create_new_player(registrator, "b1", asset(big_init_balance));
  Actor b2 = create_new_player(registrator, "b2", asset(big_init_balance));
  Actor b3 = create_new_player(registrator, "b3", asset(big_init_balance));

  BOOST_REQUIRE_NO_THROW(table_alive(registrator, table));

  auto next_history_record =
      scroll_history_to_case_start_point(actor(registrator));

  auto stake = asset(big_init_balance / 10);

  BOOST_REQUIRE_NO_THROW(
      buy_in_reserve(b1, get_next_uid(actor(b1)), stake, meta));
  BOOST_REQUIRE_NO_THROW(
      buy_in_reserve(b2, get_next_uid(actor(b2)), stake, meta));
  BOOST_REQUIRE_NO_THROW(
      buy_in_reserve(b3, get_next_uid(actor(b3)), stake, meta));

  generate_block();

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b1, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b1, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::pending);
  }

  const table_object &table_obj = table(db);

  pending_buy_in_resolve_all(registrator, table_obj);

  generate_block();

  BOOST_REQUIRE(table_obj.is_free());

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b1, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b1, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::attable);
  }

  {
    game_initial_data initial;
    initial.cash[actor(b1)] = stake;
    initial.cash[actor(b2)] = stake;
    initial.cash[actor(b3)] = stake;
    initial.info = "**** is diller";

    BOOST_REQUIRE_NO_THROW(
        game_start_playing_check(registrator, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b1, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b2, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b3, table, initial));
  }

  generate_block();

  BOOST_REQUIRE(table_obj.is_playing());

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b1, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b1, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
  }
  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b2, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b2, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
  }
  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b3, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b3, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
  }

  generate_block();

  BOOST_CHECK_NO_THROW(buy_out_table(b1, table, stake));

  {
    game_result result;
    result.cash[actor(b1)].cash = stake;
    result.cash[actor(b2)].cash = stake;
    result.cash[actor(b3)].cash = stake;
    result.log = "initial";

    BOOST_REQUIRE_NO_THROW(
        buy_in_reserve(b1, get_next_uid(actor(b1)), stake, meta));

    generate_block();

    pending_buy_in_resolve_all(registrator, table_obj);

    BOOST_REQUIRE_NO_THROW(game_result_check(registrator, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b2, table, result));
    BOOST_REQUIRE_NO_THROW(game_result_check(b3, table, result));
  }

  generate_blocks(db.get_dynamic_global_properties().time +
                  fc::seconds(params.voting_for_results_expiration_seconds));

  BOOST_CHECK(table_obj.pending_proposals.count(get_player(b1)));
  BOOST_CHECK(!table_obj.cash.count(get_player(b1)));
  BOOST_CHECK(!table_obj.playing_cash.count(get_player(b1)));

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b1, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b1, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::pending);
  }

  pending_buy_in_resolve_all(registrator, table_obj);

  auto buy_in_time = db.get_dynamic_global_properties().time;

  generate_blocks(2);

  BOOST_CHECK(!table_obj.pending_proposals.count(get_player(b1)));
  BOOST_CHECK(table_obj.cash.count(get_player(b1)));
  BOOST_CHECK(!table_obj.playing_cash.count(get_player(b1)));

  BOOST_REQUIRE(table_obj.is_free());

  {
    game_initial_data initial;
    initial.cash[actor(b2)] = stake;
    initial.cash[actor(b3)] = stake;
    initial.info = "**** is diller";

    BOOST_REQUIRE_NO_THROW(
        game_start_playing_check(registrator, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b2, table, initial));
    BOOST_REQUIRE_NO_THROW(game_start_playing_check(b3, table, initial));
  }

  generate_block();

  BOOST_REQUIRE(table_obj.is_playing());

  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b1, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b1, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::attable);
  }
  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b2, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b2, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
  }
  BOOST_REQUIRE_EQUAL(pplaychain_api->list_tables_with_player(b3, 10).size(),
                      1u);
  {
    auto info = pplaychain_api->list_tables_with_player(b3, 10)[0];

    BOOST_CHECK_EQUAL((int)info.state,
                      (int)playchain_player_table_state::playing);
  }

  generate_blocks(buy_in_time + fc::seconds(params.buy_in_expiration_seconds));

  BOOST_CHECK(pplaychain_api->list_tables_with_player(b1, 10).empty());

  generate_blocks(2);

  print_last_operations(actor(registrator), next_history_record);
}

BOOST_AUTO_TEST_CASE(set_tables_subscribe_callback_inblock_check) {
  using namespace playchain::app;

  room_id_type room = create_new_room(registrator);
  table_id_type table = create_new_table(registrator, room, 0u);

  Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

  std::vector<playchain_table_info> table_infos;

  auto callback = get_tables_notify_callback(table_infos);

  pplaychain_api->set_tables_subscribe_callback(callback, {table});

  auto stake = asset(player_init_balance / 2);

  BOOST_CHECK_NO_THROW(buy_in_table(alice, registrator, table, stake));
  BOOST_CHECK_NO_THROW(buy_in_table(bob, registrator, table, stake));

  game_initial_data initial;
  initial.cash[actor(alice)] = stake;
  initial.cash[actor(bob)] = stake;
  initial.info = "**** is diller";

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_infos));

  BOOST_REQUIRE_EQUAL(table_infos.size(), 1u);
  BOOST_CHECK(table_infos.rbegin()->state == playchain_table_state::free);

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));

  BOOST_REQUIRE_NO_THROW(game_start_playing_check(alice, table, initial));
  BOOST_REQUIRE_NO_THROW(game_start_playing_check(bob, table, initial));

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_infos));

  BOOST_REQUIRE_EQUAL(table_infos.size(), 1u);
  BOOST_CHECK(table_infos.rbegin()->state == playchain_table_state::playing);

  game_result result;
  auto win = asset(stake.amount / 2);
  auto win_rake = asset(win.amount / 20);
  auto &winner_result = result.cash[actor(alice)];
  winner_result.cash = stake + win - win_rake;
  winner_result.rake = win_rake;
  auto &last_result = result.cash[actor(bob)];
  last_result.cash = stake - win;
  result.log = "**** has A 4";

  BOOST_REQUIRE_NO_THROW(game_result_check(registrator, table, result));

  BOOST_REQUIRE_NO_THROW(game_result_check(alice, table, result));
  BOOST_REQUIRE_NO_THROW(game_result_check(bob, table, result));

  generate_block();
  fc::usleep(fc::milliseconds(200));
  idump((table_infos));

  BOOST_REQUIRE_EQUAL(table_infos.size(), 1u);
  BOOST_CHECK(table_infos.rbegin()->state == playchain_table_state::free);
}

BOOST_AUTO_TEST_CASE(check_version_info)
{
    auto version_info =  pplaychain_api->get_version();

    idump((version_info));

    BOOST_CHECK(!version_info.playchain_ver.empty());
    BOOST_CHECK(!version_info.bitshares_ver.empty());
    BOOST_CHECK(!version_info.git_revision_description.empty());
    BOOST_CHECK(!version_info.git_revision_sha.empty());
    BOOST_CHECK(!version_info.git_revision_unix_timestamp.empty());
    BOOST_CHECK(!version_info.openssl_ver.empty());
    BOOST_CHECK(!version_info.boost_ver.empty());
    BOOST_CHECK(!version_info.websocket_ver.empty());
}

BOOST_AUTO_TEST_CASE(check_tables_subscribe_callback_reset_starting_game_check) {
    using namespace playchain::app;

    room_id_type room = create_new_room(registrator);
    table_id_type table = create_new_table(registrator, room, 0u);

    Actor bob = create_new_player(registrator, "bob", asset(big_init_balance));

    std::vector<playchain_table_info> table_infos;

    auto callback = get_tables_notify_callback(table_infos);

    pplaychain_api->set_tables_subscribe_callback(callback, {table});

    auto stake = asset(player_init_balance / 2);

    BOOST_CHECK_NO_THROW(buy_in_table(alice, registrator, table, stake));
    BOOST_CHECK_NO_THROW(buy_in_table(bob, registrator, table, stake));

    game_initial_data initial;
    initial.cash[actor(alice)] = stake;
    initial.cash[actor(bob)] = stake;
    initial.info = "**** is diller";

    generate_block();
    fc::usleep(fc::milliseconds(200));
    idump((table_infos));

    BOOST_REQUIRE_EQUAL(table_infos.size(), 1u);
    BOOST_CHECK(table_infos.rbegin()->state == playchain_table_state::free);

    BOOST_REQUIRE_NO_THROW(game_start_playing_check(registrator, table, initial));

    generate_block();
    fc::usleep(fc::milliseconds(200));
    idump((table_infos));

    BOOST_REQUIRE(is_table_voting(db, table));

    BOOST_REQUIRE_EQUAL(table_infos.size(), 1u);
    BOOST_CHECK(table_infos.rbegin()->state == playchain_table_state::voting_for_playing);

    BOOST_REQUIRE_NO_THROW(game_reset(registrator, table, false));

    BOOST_REQUIRE(!is_table_voting(db, table));

    generate_block();
    fc::usleep(fc::milliseconds(200));
    idump((table_infos));

    BOOST_REQUIRE_EQUAL(table_infos.size(), 1u);
    BOOST_CHECK(table_infos.rbegin()->state == playchain_table_state::free);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace playchain_api_tests
