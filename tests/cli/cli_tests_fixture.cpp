#include "cli_tests_fixture.hpp"

namespace cli
{
cli_tests_fixture::cli_tests_fixture()
{
    using namespace graphene::chain;
    using namespace graphene::app;

    owner_key = fc::ecc::private_key::regenerate(fc::sha256::hash(owner));

    try
    {
        fc::temp_directory app_dir(graphene::utilities::temp_directory_path());

        int server_port_number = 0;
        cli_app = start_application(app_dir, server_port_number);

        // connect to the server
        connection.reset(new  client_connection(cli_app, app_dir, server_port_number));

        connection->api()->set_password("supersecret");
        connection->api()->unlock("supersecret");

        //genesis created 'nathan' funds is separate object balance_object.
        //We must deposit funds from balance object to account_balance object before any paying
        connection->api()->import_balance(owner, {graphene::utilities::key_to_wif(owner_key)}, true);

        connection->api()->list_account_balances(owner);

        //to sign transaction in wallet
        connection->api()->import_key(owner, graphene::utilities::key_to_wif(owner_key));

        //to make owner lifetime account (to be register or witness)
        connection->api()->upgrade_account(owner, true);
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        connection.reset();
        cli_app.reset();
        throw;
    }
}

cli_tests_fixture::~cli_tests_fixture()
{
    if (cli_app)
        cli_app->shutdown();
}

fc::ecc::private_key cli_tests_fixture::create_private_key_from_password(const string &password, const string &salt)
{
    string pwd = salt + "active" + password;
    auto secret = fc::sha256::hash(pwd.c_str(), pwd.size());
    auto priv_key = fc::ecc::private_key::regenerate(secret);
    return priv_key;
}

}
