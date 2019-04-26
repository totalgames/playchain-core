#include <boost/test/unit_test.hpp>

#include "cli_tests_fixture.hpp"

namespace player_invitation_cli_tests
{
using namespace cli;

BOOST_FIXTURE_TEST_SUITE( player_invitation_cli_tests, cli_tests_fixture )

BOOST_AUTO_TEST_CASE(check_make_create_invitation_trx)
{
    using namespace graphene::chain;
    using namespace graphene::app;

    try
    {
        const string inviter = owner;
        const string invitation_uid = "1";

        auto chain_id = cli_app->chain_database()->get_chain_id();

        const std::string new_inviter_name = "player123";
        private_key_type new_inviter_priv_key;

        {
            auto account = connection->api()->get_account(inviter);
            auto account_balances = connection->api()->list_account_balances(inviter);

            BOOST_REQUIRE(!account_balances.empty());
            BOOST_REQUIRE(account_balances[0].amount.value > 0);

            auto trx = connection->api()->make_player_invitation_create_trx(
                        inviter, invitation_uid, PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds(), "{}");

            auto etalon_digest = playchain::chain::utils::create_digest_by_invitation(
                        chain_id, inviter, invitation_uid);

            BOOST_REQUIRE_EQUAL(trx.additional_digest, etalon_digest);

            auto invitations = connection->api()->list_player_invitations(inviter, "");

            BOOST_REQUIRE(invitations.empty());

            signed_transaction signed_trx(trx.trx);

            signed_trx.sign(owner_key, chain_id);

            BOOST_REQUIRE_NO_THROW(connection->api()->broadcast_transaction(signed_trx));

            invitations = connection->api()->list_player_invitations(inviter, "");

            BOOST_REQUIRE(!invitations.empty());

            auto mandat = owner_key.sign_compact(trx.additional_digest);

            const std::string new_account_name = new_inviter_name;
            new_inviter_priv_key = create_private_key_from_password("123", new_account_name);
            graphene::chain::public_key_type new_pub_key = new_inviter_priv_key.get_public_key();

            BOOST_REQUIRE_NO_THROW(connection->api()->create_player_from_invitation(
                                       inviter, invitation_uid, mandat, (std::string)new_pub_key, new_account_name, true, false));

            BOOST_REQUIRE_NO_THROW(connection->api()->transfer(
                                       inviter, new_account_name, "10", "PLC", "", true));
        }

        {
            auto account = connection->api()->get_account(new_inviter_name);
            auto account_balances = connection->api()->list_account_balances(new_inviter_name);

            BOOST_REQUIRE(!account_balances.empty());
            BOOST_REQUIRE(account_balances[0].amount.value > 0);

            auto trx = connection->api()->make_player_invitation_create_trx(
                        new_inviter_name, invitation_uid, PLAYCHAIN_MINIMAL_INVITATION_EXPIRATION_PERIOD.to_seconds(), "{}");

            auto etalon_digest = playchain::chain::utils::create_digest_by_invitation(
                        chain_id, new_inviter_name, invitation_uid);

            BOOST_REQUIRE_EQUAL(trx.additional_digest, etalon_digest);

            auto invitations = connection->api()->list_player_invitations(new_inviter_name, "");

            BOOST_REQUIRE(invitations.empty());

            signed_transaction signed_trx(trx.trx);

            signed_trx.sign(new_inviter_priv_key, chain_id);

            BOOST_REQUIRE_NO_THROW(connection->api()->broadcast_transaction(signed_trx));

            invitations = connection->api()->list_player_invitations(new_inviter_name, "");

            BOOST_REQUIRE(!invitations.empty());

            auto mandat = new_inviter_priv_key.sign_compact(trx.additional_digest);

            const std::string new_account_name = "player123child";
            private_key_type new_account_priv_key = create_private_key_from_password("123", new_account_name);
            graphene::chain::public_key_type new_pub_key = new_account_priv_key.get_public_key();

            BOOST_REQUIRE_NO_THROW(connection->api()->create_player_from_invitation(
                                       new_inviter_name, invitation_uid, mandat, (std::string)new_pub_key, new_account_name, true, false));

            auto &&result = connection->api()->list_invited_players(new_inviter_name, "");

            BOOST_CHECK_EQUAL(result[0].name, new_account_name);
            BOOST_CHECK(result[0].account_id.instance.value > 0);
            BOOST_CHECK(result[0].player_id.instance.value > 0);
        }
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
