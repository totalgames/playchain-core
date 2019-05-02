#include <boost/test/unit_test.hpp>

#include "cli_tests_fixture.hpp"

#include <fc/crypto/hex.hpp>

namespace account_create_tests
{
using namespace cli;

BOOST_FIXTURE_TEST_SUITE( account_create_tests, cli_tests_fixture )

BOOST_AUTO_TEST_CASE(create_account_with_pubkey_test)
{
    const string registrar = owner;
    const string referrer = registrar;

    auto key_info = connection->api()->suggest_brain_key();
    public_key_type pub_key = key_info.pub_key;

    BOOST_REQUIRE_NO_THROW(connection->api()->create_account_with_brain_key((std::string)pub_key, "alice", registrar, referrer, true, false));

    auto alice = connection->api()->get_account("alice");

    BOOST_CHECK_EQUAL(alice.name, "alice");

    key_info = connection->api()->suggest_brain_key();
    pub_key = key_info.pub_key;

    auto pub_key_hex = fc::to_hex(pub_key.key_data.begin(), pub_key.key_data.size());

    BOOST_REQUIRE_NO_THROW(connection->api()->create_account_with_brain_key(pub_key_hex, "bob", registrar, referrer, true, false));

    auto bob = connection->api()->get_account("bob");

    BOOST_CHECK_EQUAL(bob.name, "bob");
}

BOOST_AUTO_TEST_SUITE_END()
}
