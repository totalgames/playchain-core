#pragma once

#include <boost/filesystem.hpp>

#include <string>

namespace graphene {
namespace chain {
class genesis_state_type;
}
}

namespace playchain {

namespace util {

using graphene::chain::genesis_state_type;

void load(const std::string&, genesis_state_type&);

struct parser_i
{
    virtual ~parser_i() = default;
    virtual void update(genesis_state_type&) = 0;
};

void check_users(const genesis_state_type& genesis, const std::vector<std::string>& users);

void save_to_string(genesis_state_type&, std::string&, bool pretty_print);

void save_to_file(genesis_state_type&, const std::string& path, bool pretty_print);

void print(genesis_state_type&, bool pretty_print);
}
}
