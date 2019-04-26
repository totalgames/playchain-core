#pragma once

#include <map>
#include <string>

#include <graphene/chain/genesis_state.hpp>
#include <fc/static_variant.hpp>

namespace playchain {
namespace util {

using graphene::chain::genesis_state_type;
using account_type = genesis_state_type::initial_account_type;
using balance_type = genesis_state_type::initial_balance_type;

using graphene::chain::void_t;
using graphene::chain::asset;
using graphene::chain::share_type;
using graphene::chain::public_key_type;
using graphene::chain::address;

using genesis_account_info_item_type = fc::static_variant<void_t, account_type, balance_type>;

class genesis_mapper
{
public:
    genesis_mapper();

    void reset(const genesis_state_type&);

    void update(const genesis_account_info_item_type&);

    void update(const std::string& name,
                const std::string& owner_key,
                const std::string& active_key,
                const share_type& balance);

    void save(genesis_state_type&);

private:
    using genesis_account_info_item_map_by_type = std::map<int, genesis_account_info_item_type>;
    using genesis_account_info_items_type = std::map<address, genesis_account_info_item_map_by_type>;
    genesis_account_info_items_type _uniq_items;
    std::map<std::string, account_type> _uniq_account_items;
    asset _accounts_supply = asset(0);
};
}
}
