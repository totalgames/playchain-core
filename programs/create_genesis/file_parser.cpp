#include "file_parser.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <sstream>
#include <string>

#include <graphene/chain/genesis_state.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>

#include <fc/io/json.hpp>

using graphene::chain::public_key_type;
using graphene::chain::share_type;

namespace playchain {
namespace util {

struct file_format_type
{
    struct user
    {
        std::string name;
        std::string key;
        std::string owner_key;
        share_type balance;
    };

    std::vector<user> users;
};
}
}

FC_REFLECT(playchain::util::file_format_type::user, (name)(key)(owner_key)(balance))
FC_REFLECT(playchain::util::file_format_type, (users))

namespace playchain {
namespace util {

file_parser::file_parser(const boost::filesystem::path& path)
    : _path(path)
{
    FC_ASSERT(boost::filesystem::exists(_path), "Path ${p} does not exists.", ("p", _path.string()));

    _path.normalize();
}

void file_parser::update(genesis_state_type& result)
{
    _mapper.reset(result);

    ilog("Loading ${file}.", ("file", _path.string()));

    boost::filesystem::ifstream fl;
    fl.open(_path.string(), std::ios::in);

    FC_ASSERT((bool)fl, "Can't read file ${p}.", ("p", _path.string()));

    std::stringstream ss;

    ss << fl.rdbuf();

    fl.close();

    file_format_type input = fc::json::from_string(ss.str()).as<file_format_type>(20);

    for (const auto& user : input.users)
    {
        _mapper.update(user.name, user.owner_key.empty()?user.key:user.owner_key, user.key, user.balance);
    }

    _mapper.save(result);
}
}
}
