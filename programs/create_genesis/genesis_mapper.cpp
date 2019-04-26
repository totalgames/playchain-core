#include "genesis_mapper.hpp"

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/address.hpp>
#include <graphene/chain/account_object.hpp>

namespace playchain {
namespace util {

struct get_address_visitor
{
    using result_type = address;

    address operator()(const void_t&) const
    {
        return {};
    }

    address operator()(const account_type& item) const
    {
        return address{ item.active_key };
    }

    address operator()(const balance_type& item) const
    {
        return item.owner;
    }
};

struct init_empty_item_visitor
{
    using result_type = genesis_account_info_item_type;

    genesis_account_info_item_type operator()(const void_t&) const
    {
        return void_t{};
    }

    genesis_account_info_item_type operator()(const account_type&) const
    {
        return account_type{};
    }

    genesis_account_info_item_type operator()(const balance_type&) const
    {
        return balance_type{};
    }
};

class update_visitor
{
public:
    update_visitor(genesis_account_info_item_type& item, asset& accounts_supply)
        : _item(item)
        , _accounts_supply(accounts_supply)
    {
    }

    using result_type = void;

    void operator()(const void_t&) const
    {
    }

    void operator()(const account_type& item) const
    {
        account_type& update_item = _item.get<account_type>();

        bool first = (update_item.name.empty());
        if (first)
        {
            update_item.name = item.name;
            update_item.active_key = item.active_key;
            update_item.owner_key = item.owner_key;
            update_item.is_lifetime_member = item.is_lifetime_member;
        }else if (update_item.name != item.name)
        {
            wlog("Conflict account address for '${old}' and '${new}'. First has been chosen", ("old", update_item.name)("new", item.name));
        }
    }

    void operator()(const balance_type& item) const
    {
        balance_type& update_item = _item.get<balance_type>();

        update_item.owner = item.owner;
        update_item.asset_symbol = item.asset_symbol;
        _accounts_supply -= asset{update_item.amount};
        update_item.amount = item.amount;
        _accounts_supply += asset{update_item.amount};
    }

private:
    genesis_account_info_item_type& _item;
    asset& _accounts_supply;
};

class save_visitor
{
public:
    explicit save_visitor(genesis_state_type& genesis)
        : _genesis(genesis)
    {
    }

    using result_type = void;

    void operator()(const void_t&) const
    {
    }

    void operator()(const account_type& item) const
    {
        return _genesis.initial_accounts.push_back(item);
    }

    void operator()(const balance_type& item) const
    {
        return _genesis.initial_balances.push_back(item);
    }

private:
    genesis_state_type& _genesis;
};

//

genesis_mapper::genesis_mapper()
{
}

void genesis_mapper::reset(const genesis_state_type& genesis)
{
    _uniq_items.clear();
    _uniq_account_items.clear();
    _accounts_supply.amount = 0;

    for (const auto& item : genesis.initial_accounts)
    {
        auto it = _uniq_account_items.find(item.name);
        if (_uniq_account_items.end() == it)
        {
            _uniq_account_items.emplace(std::make_pair(item.name, item));
            update(item);
        }
    }

    for (const auto& item : genesis.initial_balances)
    {
        update(item);
    }
}

void genesis_mapper::update(const genesis_account_info_item_type& item)
{
    genesis_account_info_item_type& genesis_item = _uniq_items[item.visit(get_address_visitor())][item.which()];
    if (!genesis_item.which())
    {
        genesis_item = item.visit(init_empty_item_visitor());
    }

    item.visit(update_visitor(genesis_item, _accounts_supply));
}

void genesis_mapper::update(const std::string& name,
                            const std::string& owner_key,
                            const std::string& active_key,
                            const share_type& balance)
{
    // sanitizing
    FC_ASSERT(name == "X4fWLuj9khEvh" || graphene::chain::is_valid_name(name), "Invalid user name '${n}'", ("n", name));

    auto it = _uniq_account_items.find(name);
    if (_uniq_account_items.end() == it)
    {
        auto pr = _uniq_account_items.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                              std::forward_as_tuple(name, public_key_type{owner_key}, public_key_type{active_key}, true));
        it = pr.first;
    }

    const account_type &account = it->second;
    update(account);

    if (balance > 0)
    {
        update(balance_type{ get_address_visitor{}(account), GRAPHENE_ADDRESS_PREFIX, balance });
    }
}

void genesis_mapper::save(genesis_state_type& genesis)
{
    genesis.initial_accounts.clear();
    genesis.initial_balances.clear();

    for (auto&& item : _uniq_items)
    {
        for (auto&& genesis_item : item.second)
        {
            genesis_item.second.visit(save_visitor(genesis));
        }
    }

    ilog("MAX_CORE_SUPPLY - ACCOUNTS_SUPPLY = ${d}", ("d", genesis.max_core_supply - _accounts_supply.amount));

    FC_ASSERT(asset{genesis.max_core_supply} >= _accounts_supply,
              "Invalid actual accounts supply. Received '${as}', but required '${rs}'",
              ("as", _accounts_supply)("rs", asset{genesis.max_core_supply}));
}

}
}
