#include "parsers.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <sstream>
#include <iostream>
#include <vector>

#include <graphene/chain/genesis_state.hpp>

#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/io/json.hpp>

namespace playchain {
namespace util {

using graphene::chain::public_key_type;
using graphene::chain::share_type;
using graphene::chain::address;

using graphene::chain::genesis_state_type;
using account_type = genesis_state_type::initial_account_type;
using balance_type = genesis_state_type::initial_balance_type;

struct user_info
{
    public_key_type key;
    bool single_key = true;
    share_type balance;
};
}
}

FC_REFLECT(playchain::util::user_info, (key)(single_key)(balance))

namespace playchain {
namespace util {

using graphene::chain::chain_id_type;

void load(const std::string& path, genesis_state_type& genesis)
{
    boost::filesystem::path path_to_load(path);
    FC_ASSERT(boost::filesystem::exists(path_to_load), "Path ${p} does not exists.", ("p", path_to_load.string()));

    path_to_load.normalize();

    ilog("Loading ${file}.", ("file", path_to_load.string()));

    boost::filesystem::ifstream fl;
    fl.open(path_to_load.string(), std::ios::in);

    FC_ASSERT((bool)fl, "Can't read file ${p}.", ("p", path_to_load.string()));

    std::stringstream ss;

    ss << fl.rdbuf();

    fl.close();

    genesis = fc::json::from_string(ss.str()).as<genesis_state_type>(20);
}

void check_users(const genesis_state_type& genesis, const std::vector<std::string>& users)
{
    ilog("Checking users '${users}'.", ("users", users));
    std::map<std::string, user_info> checked_users;
    for (const auto& user : users)
    {
        account_type initial_account;
        for (const auto& account : genesis.initial_accounts)
        {
            if (account.name == user)
            {
                initial_account = account;
                break;
            }
        }

        if (!initial_account.name.empty())
        {
            for (const auto& balance : genesis.initial_balances)
            {
                if (balance.owner == address{initial_account.active_key})
                {
                    auto &info = checked_users[initial_account.name];
                    info.balance = balance.amount;
                    info.key = initial_account.active_key;
                    info.single_key = (initial_account.active_key == initial_account.owner_key);
                    break;
                }
            }
        }
    }
    for (const auto& info : checked_users)
    {
        ilog("${user}: ${info}", ("user", info.first)("info", info.second));
    }
    FC_ASSERT(checked_users.size() == users.size(), "Not all users from list exist in genesis");
}

void save_to_string(genesis_state_type& genesis, std::string& output_json, bool pretty_print)
{
    fc::variant vo;
    fc::to_variant(genesis, vo, 20);

    if (pretty_print)
    {
        output_json = fc::json::to_pretty_string(vo);
        output_json.append("\n");
    }
    else
    {
        output_json = fc::json::to_string(vo);
    }
}

void save_to_file(genesis_state_type& genesis, const std::string& path, bool pretty_print)
{
    std::string output_json;
    save_to_string(genesis, output_json, pretty_print);

    boost::filesystem::path path_to_save(path);

    path_to_save.normalize();

    ilog("Saving ${file}.", ("file", path_to_save.string()));

    boost::filesystem::ofstream fl;
    fl.open(path_to_save);

    FC_ASSERT((bool)fl, "Can't write to file ${p}.", ("p", path_to_save.string()));

    fl << output_json;
    fl.close();
}

void print(genesis_state_type& genesis, bool pretty_print)
{
    std::string output_json;
    save_to_string(genesis, output_json, pretty_print);
    std::cout << output_json << std::endl;
}
}
}
