#pragma once

#include <memory>

#include <graphene/app/application.hpp>
#include <graphene/utilities/tempdir.hpp>
#include <graphene/wallet/wallet.hpp>

namespace cli
{
    std::shared_ptr<graphene::app::application> start_application(fc::temp_directory& app_dir, int& server_port_number);

    bool generate_block(std::shared_ptr<graphene::app::application> app);

    bool generate_maintenance_block(std::shared_ptr<graphene::app::application> app);

    struct client_connection_impl;

    class client_connection
    {
    public:
       client_connection(std::shared_ptr<graphene::app::application> app, const fc::temp_directory& data_dir, const int server_port_number);
       ~client_connection();

       using wallet_api = graphene::wallet::wallet_api;

       wallet_api * api();
       const std::string &wallet_filename() const;

    private:
       std::unique_ptr<client_connection_impl> _impl;
    };
}
