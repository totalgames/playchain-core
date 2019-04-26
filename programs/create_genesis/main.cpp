#include <iostream>

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <graphene/app/api.hpp>
#include <graphene/app/application.hpp>
#include <graphene/chain/genesis_state.hpp>
#include <fc/exception/exception.hpp>

#include "file_parser.hpp"

#include "genesis_tester.hpp"

#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/smart_ref_impl.hpp>

using graphene::chain::genesis_state_type;

int main(int argc, char** argv)
{
    try
    {
        namespace bpo = boost::program_options;

        bpo::options_description opts;
        // clang-format off
        opts.add_options()
                ("help,h", "Print this help message and exit.")
                ("version,v", "Print version number and exit.")
                ("import-json,i",     bpo::value<std::string>(), "Path for Json data file to parse.")
                ("input-genesis-json,g",     bpo::value<std::string>(), "Path for Json genesis file to concatenate with result.")
                ("test-resut-genesis,t", "Test opening sandbox database by resulted genesis.")
                ("suppress-output-json,s", "Do not print result Json genesis.")
                ("pretty-print,p", "Human readable format for output Json.")
                ("check-users,u", bpo::value< std::vector<std::string> >()-> multitoken()->composing(), "Users list that are checked in result genesis.")
                ("output-genesis-json,o", bpo::value<std::string>(), "Path for result Json genesis file.");
        // clang-format on

        bpo::variables_map options;

        bpo::store(bpo::parse_command_line(argc, argv, opts), options);

        if (options.count("version"))
        {
            graphene::app::options_helper::print_application_version(std::cout);
            return 0;
        }

        if (options.count("help"))
        {
            std::cout << opts << std::endl;
            return 0;
        }

        genesis_state_type genesis;

        if (options.count("input-genesis-json"))
        {
            playchain::util::load(options.at("input-genesis-json").as<std::string>(), genesis);
        }

        if (options.count("import-json"))
        {
            playchain::util::file_parser fl(options.at("import-json").as<std::string>());

            fl.update(genesis);
        }
        else
        {
            std::string connection_uri;
            if (options.count("mongodb-endpoint"))
            {
                connection_uri = options.at("mongodb-endpoint").as<std::string>();
            }

            FC_ASSERT(options.count("input-genesis-json"));
        }

        if (options.count("test-resut-genesis"))
        {
            playchain::util::test_database(genesis);
        }
        else
        {
            FC_ASSERT(!options.count("suppress-output-json"));
        }

        if (options.count("check-users") > 0)
        {
            std::vector<std::string> users;

            for (const auto& user : options.at("check-users").as<std::vector<std::string>>())
            {
                std::vector<std::string> next_users;
                boost::split(next_users, user, boost::is_any_of(" \t,"));
                for (const auto& next_user : next_users)
                {
                    if (!next_user.empty())
                        users.push_back(next_user);
                }
            }

            playchain::util::check_users(genesis, users);
        }

        if (options.count("output-genesis-json"))
        {
            playchain::util::save_to_file(genesis, options.at("output-genesis-json").as<std::string>(),
                                       options.count("pretty-print"));
        }
        else if (!options.count("suppress-output-json"))
        {
            playchain::util::print(genesis, options.count("pretty-print"));
        }

        return 0;
    }
    FC_CAPTURE_AND_LOG((0))

    return 1;
}
