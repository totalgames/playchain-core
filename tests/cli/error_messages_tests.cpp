#include <boost/test/unit_test.hpp>

#include "test_applications.hpp"

#include <boost/algorithm/string.hpp>

namespace error_messages_tests
{
using namespace cli;

BOOST_AUTO_TEST_SUITE( error_messages )
////////////////
// check error messages returned by api
////////////////
BOOST_AUTO_TEST_CASE( error_message_on_invalid_login )
{
    using namespace graphene::chain;
    using namespace graphene::app;
    std::shared_ptr<graphene::app::application> app1;

    try
    {
        fc::temp_directory app_dir(graphene::utilities::temp_directory_path());

        int server_port_number = 0;
        app1 = start_application(app_dir, server_port_number);

        // connect to the server
        client_connection con(app1, app_dir, server_port_number);

        //get non-existed account
        con.api()->get_account("alice");

        BOOST_ERROR("Must not be here");
    }
    catch (fc::exception& e)
    {
        std::string my_input(e.to_detail_string());
        std::vector<std::string> results;
        boost::algorithm::split(results, my_input, boost::algorithm::is_any_of("\n"));

        BOOST_REQUIRE_EQUAL(results[1], "Error code 20180001: Account 'alice' not found.");
    }

    app1->shutdown();
}

BOOST_AUTO_TEST_CASE(error_message_on_invalid_password)
{
    using namespace graphene::chain;
    using namespace graphene::app;
    std::shared_ptr<graphene::app::application> app1;

    try
    {
        fc::temp_directory app_dir(graphene::utilities::temp_directory_path());

        int server_port_number = 0;
        app1 = start_application(app_dir, server_port_number);

        // connect to the server
        client_connection con(app1, app_dir, server_port_number);

        BOOST_TEST_MESSAGE("Setting wallet password");
        con.api()->set_password("supersecret");
        con.api()->unlock("supersecret");

        //feed not valid key
        con.api()->login_with_pubkey("nathan", "ABBB3568368");

        BOOST_ERROR("Must not be here");
    }
    catch (fc::exception& e)
    {
        std::string my_input(e.to_detail_string());
        std::vector<std::string> results;
        boost::algorithm::split(results, my_input, boost::algorithm::is_any_of("\n"));

        BOOST_REQUIRE_EQUAL(results[1], "Error code 20180003: Invalid credentials for account 'nathan'.");
    }

    app1->shutdown();
}

////////////////
// check error messages returned by api
////////////////
BOOST_AUTO_TEST_CASE(error_message_on_creation_existed_account)
{
    using namespace graphene::chain;
    using namespace graphene::app;
    std::shared_ptr<graphene::app::application> app1;

    try
    {
        fc::temp_directory app_dir(graphene::utilities::temp_directory_path());

        int server_port_number = 0;
        app1 = start_application(app_dir, server_port_number);

        // connect to the server
        client_connection con(app1, app_dir, server_port_number);

        BOOST_TEST_MESSAGE("Setting wallet password");
        con.api()->set_password("supersecret");
        con.api()->unlock("supersecret");

        // import Nathan account
        BOOST_TEST_MESSAGE("Importing registrator key");

        auto registrator_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init")));
        BOOST_CHECK(con.api()->import_key("init1", key_to_wif(registrator_key)));

        //try to create existed account
        con.api()->create_account_with_brain_key("ABBB3568368", "nathan", "init1", "init1", true, false);

        BOOST_ERROR("Must not be here");
    }
    catch (fc::exception& e)
    {
        std::string my_input(e.to_detail_string());
        std::vector<std::string> results;
        boost::algorithm::split(results, my_input, boost::algorithm::is_any_of("\n"));

        BOOST_REQUIRE_EQUAL(results[1], "Invalid Argument: Account 'nathan' already exists. Client Code = 20180000");
    }

    app1->shutdown();
}

////////////////
// check error messages returned by api
////////////////
BOOST_AUTO_TEST_CASE(error_message_on_invalid_name_account_creation)
{
    using namespace graphene::chain;
    using namespace graphene::app;
    std::shared_ptr<graphene::app::application> app1;

    try
    {
        fc::temp_directory app_dir(graphene::utilities::temp_directory_path());

        int server_port_number = 0;
        app1 = start_application(app_dir, server_port_number);

        // connect to the server
        client_connection con(app1, app_dir, server_port_number);

        BOOST_TEST_MESSAGE("Setting wallet password");
        con.api()->set_password("supersecret");
        con.api()->unlock("supersecret");

        // import Nathan account
        BOOST_TEST_MESSAGE("Importing registrator key");

        auto registrator_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init")));
        BOOST_CHECK(con.api()->import_key("init1", key_to_wif(registrator_key)));

        //try to create existed account
        con.api()->create_account_with_brain_key("ABBB3568368", "---", "init1", "init1", true, false);

        BOOST_ERROR("Must not be here");
    }
    catch (fc::exception& e)
    {
        std::string my_input(e.to_detail_string());
        std::vector<std::string> results;
        boost::algorithm::split(results, my_input, boost::algorithm::is_any_of("\n"));

        BOOST_REQUIRE_EQUAL(results[1], "Error code 20180004: Account name '---' is invalid.");
    }

    app1->shutdown();
}

BOOST_AUTO_TEST_SUITE_END()
}
