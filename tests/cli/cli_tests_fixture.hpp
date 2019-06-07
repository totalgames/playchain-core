#pragma once

#include "test_applications.hpp"

#include <boost/algorithm/string.hpp>

#include <playchain/chain/playchain_config.hpp>
#include <playchain/chain/playchain_utils.hpp>
#include <graphene/utilities/key_conversion.hpp>

namespace cli
{
    class cli_tests_fixture
    {
    public:
        cli_tests_fixture();

        virtual ~cli_tests_fixture();

        fc::ecc::private_key create_private_key_from_password(const string &password, const string &salt);
        void create_new_account(const std::string &new_account = "alice");

        const std::string owner = "nathan";
        fc::ecc::private_key owner_key;

        std::shared_ptr<graphene::app::application> cli_app;
        std::unique_ptr<client_connection> connection;

        fc::temp_directory app_dir;
    };
}
