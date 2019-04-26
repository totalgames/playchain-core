#pragma once

#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/chain/protocol/transaction.hpp>

#include <iostream>

namespace graphene {
    namespace chain {
        class database_fixture;
    }
}

using namespace graphene::chain;
using fixture_type = graphene::chain::database_fixture;

namespace tuple_utils
{
    namespace
    {
        template<std::size_t Index, class TCallback, class ...TParams>
        struct _foreach_
        {
            static void tupleForeach_(TCallback&& callback, const std::tuple<TParams...>& tuple)
            {
                const std::size_t idx = sizeof...(TParams) - Index;
                callback.template operator() < idx > (std::get<idx>(tuple));
                _foreach_<Index - 1, TCallback, TParams...>::tupleForeach_(std::forward<TCallback>(callback), tuple);
            }
        };

        template<class TCallback, class ...TParams>
        struct _foreach_<0, TCallback, TParams...>
        {
            static void tupleForeach_(TCallback&& /*callback*/, const std::tuple<TParams...>& /*tuple*/) {}
        };

    } //
    template<class TCallback, class ...TParams>
    void tupleForeach(TCallback&& callback, const std::tuple<TParams...>& tuple)
    {
        _foreach_<sizeof...(TParams), TCallback, TParams...>::tupleForeach_(std::forward<TCallback>(callback), tuple);
    }

} // tuple_utils

#define DECLARE_ACTOR(actor) Actor actor = Actor(BOOST_PP_STRINGIZE(actor)); 

class Actor
{
public:
    Actor(const char* pszName);

    Actor(const std::string& name);

    Actor(const std::string& name, const private_key_type &, const public_key_type &);

    operator const std::string&() const;

    std::string name;
    private_key_type private_key;
    public_key_type public_key;
};


class ActorActions
{
    struct prepare_transaction
    {
        signed_transaction& _tx;
        fixture_type& _fixture;
        vector<Actor> _signers;

        prepare_transaction(signed_transaction& tx, fixture_type& f);
        ~prepare_transaction();

        template<std::size_t>
        void operator()(const Actor& element)
        {
            _signers.push_back(std::move(element));
        }

        template<std::size_t, class T>
        void operator()(const T& element)
        {
            _tx.operations.push_back(element);
        }
    };

public:
    ActorActions(fixture_type& f, const Actor& a);

    operator account_id_type() const;

    void supply(asset amount);
    void transfer(const Actor& a, asset amount);
    int64_t balance() const;

    template <typename... Ts> void push_operations_with_additional_signers(Ts&&... ops_or_additional_signers)
    {
        auto tuple = std::make_tuple(_actor, ops_or_additional_signers...);

        signed_transaction tx;
        tuple_utils::tupleForeach(prepare_transaction(tx, _fixture), tuple);

        push_transaction(tx);
    }

    template <typename T> processed_transaction push_operation(T&& op)
    {
        signed_transaction tx;
        tuple_utils::tupleForeach(prepare_transaction(tx, _fixture),  std::make_tuple(_actor, op));

        return push_transaction(tx);
    }

    template <typename T> void push_operation_not_sign(T&& op)
    {
        signed_transaction tx;

        tx.operations.push_back(op);
        prepare_transaction(tx, _fixture);

        push_transaction(tx);
    }

private:
    account_id_type create();
    processed_transaction push_transaction(signed_transaction& tx);

private:
    fixture_type& _fixture;
    const Actor& _actor;
};
