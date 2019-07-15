#include "test_applications.hpp"

#include <graphene/app/plugin.hpp>

#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/witness/witness.hpp>
#include <graphene/market_history/market_history_plugin.hpp>
#include <graphene/egenesis/egenesis.hpp>
#include <graphene/wallet/wallet.hpp>

#include <fc/thread/thread.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/rpc/cli.hpp>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/types.h>

#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/test/unit_test.hpp>

/*********************
 * Helper Methods
 *********************/

/////////
/// @brief forward declaration, using as a hack to generate a genesis.json file
/// for testing
/////////
namespace graphene { namespace app { namespace detail {
   graphene::chain::genesis_state_type create_example_genesis();
} } } // graphene::app::detail

namespace cli
{
namespace
{
    /////////
    /// @brief create a genesis_json file
    /// @param directory the directory to place the file "genesis.json"
    /// @returns the full path to the file
    ////////
    boost::filesystem::path create_genesis_file(fc::temp_directory& directory) {
       boost::filesystem::path genesis_path = boost::filesystem::path{directory.path().generic_string()} / "genesis.json";
       fc::path genesis_out = genesis_path;
       graphene::chain::genesis_state_type genesis_state = graphene::app::detail::create_example_genesis();

       fc::json::save_to_file(genesis_state, genesis_out);
       return genesis_path;
    }

    //////
    /// @brief attempt to find an available port on localhost
    /// @returns an available port number, or -1 on error
    /////
    int get_available_port()
    {
        int max_loop = 5;
        int result = 0;
        int socket_fd = -1;
        while (result < 1024 && --max_loop > 0)
        {
            struct sockaddr_in sin;
            socket_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (socket_fd == -1)
               return -1;
            sin.sin_family = AF_INET;
            sin.sin_port = 0;
            sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            if (bind(socket_fd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)) == -1)
               break;
            socklen_t len = sizeof(sin);
            if (getsockname(socket_fd, (struct sockaddr *)&sin, &len) == -1)
               break;
            result = sin.sin_port;
            close(socket_fd);
            socket_fd = -1;
        }

        if (socket_fd > 0)
            close(socket_fd);
        if (result < 1024)
            return 1999;
        return result;
    }
}

///////////
/// @brief Start the application
/// @param app_dir the temporary directory to use
/// @param server_port_number to be filled with the rpc endpoint port number
/// @returns the application object
//////////
std::shared_ptr<graphene::app::application> start_application(fc::temp_directory& app_dir, int& server_port_number) {
   std::shared_ptr<graphene::app::application> app1(new graphene::app::application{});

   app1->register_plugin<graphene::account_history::account_history_plugin>(true);
   app1->register_plugin< graphene::market_history::market_history_plugin >(true);
   app1->register_plugin< graphene::witness_plugin::witness_plugin >(true);
   app1->register_plugin< graphene::grouped_orders::grouped_orders_plugin>(true);
   boost::program_options::variables_map cfg;
   server_port_number = get_available_port();
   cfg.emplace("rpc-endpoint", boost::program_options::variable_value(string("127.0.0.1:" + std::to_string(server_port_number)), false));
   cfg.emplace("genesis-json", boost::program_options::variable_value(create_genesis_file(app_dir), false));
   cfg.emplace("seed-nodes", boost::program_options::variable_value(string("[]"), false));

   app1->initialize(app_dir.path(), cfg);
   app1->initialize_plugins(cfg);

   app1->startup();
   app1->startup_plugins();

   fc::usleep(fc::milliseconds(500));
    return app1;
}

///////////
/// Send a block to the db
/// @param app the application
/// @returns true on success
///////////
bool generate_block(std::shared_ptr<graphene::app::application> app) {
    graphene::chain::signed_block block;
    return generate_block(app, block);
}

bool generate_block(std::shared_ptr<graphene::app::application> app, graphene::chain::signed_block &block)
{
    try {
       fc::ecc::private_key committee_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init")));
        auto db = app->chain_database();
        block = db->generate_block(
              db->get_slot_time(1),
              db->get_scheduled_witness(1),
              committee_key,
              database::skip_nothing);
        return true;
    } catch (exception &) {
       return false;
    }
}

///////////
/// @brief Skip intermediate blocks, and generate a maintenance block
/// @param app the application
/// @returns true on success
///////////
bool generate_maintenance_block(std::shared_ptr<graphene::app::application> app) {
   try {
      fc::ecc::private_key committee_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("init")));
      uint32_t skip = ~0;
      auto db = app->chain_database();
      auto maint_time = db->get_dynamic_global_properties().next_maintenance_time;
      auto slots_to_miss = db->get_slot_at_time(maint_time);
      db->generate_block(db->get_slot_time(slots_to_miss),
            db->get_scheduled_witness(slots_to_miss),
            committee_key,
            skip);
      return true;
   } catch (exception& e)
   {
      return false;
   }
}

struct client_connection_impl
{
    client_connection_impl(std::shared_ptr<graphene::app::application> app, const fc::temp_directory& data_dir, const int server_port_number)
    {
        wallet_data.chain_id = app->chain_database()->get_chain_id();
        wallet_data.ws_server = "ws://127.0.0.1:" + std::to_string(server_port_number);
        wallet_data.ws_user = "";
        wallet_data.ws_password = "";
        websocket_connection  = websocket_client.connect( wallet_data.ws_server );

        api_connection = std::make_shared<fc::rpc::websocket_api_connection>(*websocket_connection, GRAPHENE_MAX_NESTED_OBJECTS);

        remote_login_api = api_connection->get_remote_api< graphene::app::login_api >(1);
        BOOST_CHECK(remote_login_api->login( wallet_data.ws_user, wallet_data.ws_password ) );

        wallet_api_ptr = std::make_shared<graphene::wallet::wallet_api>(wallet_data, remote_login_api);
        wallet_filename = data_dir.path().generic_string() + "/wallet.json";
        wallet_api_ptr->set_wallet_filename(wallet_filename);

        wallet_api = fc::api<graphene::wallet::wallet_api>(wallet_api_ptr);

        wallet_cli = std::make_shared<fc::rpc::cli>(GRAPHENE_MAX_NESTED_OBJECTS);
        for( auto& name_formatter : wallet_api_ptr->get_result_formatters() )
           wallet_cli->format_result( name_formatter.first, name_formatter.second );

        boost::signals2::scoped_connection closed_connection(websocket_connection->closed.connect([=]{
           cerr << "Server has disconnected us.\n";
           wallet_cli->stop();
        }));
        (void)(closed_connection);
    }

    fc::http::websocket_client websocket_client;
    graphene::wallet::wallet_data wallet_data;
    fc::http::websocket_connection_ptr websocket_connection;
    std::shared_ptr<fc::rpc::websocket_api_connection> api_connection;
    fc::api<login_api> remote_login_api;
    std::shared_ptr<graphene::wallet::wallet_api> wallet_api_ptr;
    fc::api<graphene::wallet::wallet_api> wallet_api;
    std::shared_ptr<fc::rpc::cli> wallet_cli;
    std::string wallet_filename;
};

client_connection::client_connection(std::shared_ptr<graphene::app::application> app,
                                     const fc::temp_directory& data_dir,
                                     const int server_port_number)
{
    _impl.reset(new client_connection_impl(app, data_dir, server_port_number));
}
client_connection::~client_connection(){}

client_connection::wallet_api *client_connection::api()
{
    return _impl->wallet_api_ptr.get();
}

const std::string &client_connection::wallet_filename() const
{
    return _impl->wallet_filename;
}
}
