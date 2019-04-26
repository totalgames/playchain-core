#include "actor.hpp"
#include "../common/database_fixture.hpp"

Actor::Actor(const char* pszName)
    : name(pszName)
    , private_key(private_key_type::regenerate(fc::sha256::hash(name)))
    , public_key(private_key.get_public_key())
{
}

Actor::Actor(const std::string& name)
    : name(name)
    , private_key(private_key_type::regenerate(fc::sha256::hash(name)))
    , public_key(private_key.get_public_key())
{
}

Actor::Actor(const std::string& name, const private_key_type &private_key, const public_key_type &public_key)
    : name(name)
    , private_key(private_key)
    , public_key(public_key)
{
}

Actor::operator const std::string&() const
{
    return name;
}

//////////////////////////////////////////////////////////////////////////
ActorActions::ActorActions(fixture_type& f, const Actor& a)
    : _fixture(f)
    , _actor(a)
{
    create();
}

account_id_type ActorActions::create()
{
    const auto& idx = _fixture.db.get_index_type<account_index>().indices().get<by_name>();
    auto itF = idx.find(_actor);
    if(itF == idx.end())
        return _fixture.create_account(_actor, _actor.public_key, false).get_id();

    return itF->get_id();
}

ActorActions::operator account_id_type() const
{
    return _fixture.get_account(_actor).get_id();
}

int64_t ActorActions::balance()const
{
    return _fixture.get_balance(_fixture.get_account(_actor).get_id(), asset_id_type(0));
}

void ActorActions::supply(asset amount)
{
    _fixture.fund(_fixture.get_account(_actor), amount);
}

void ActorActions::transfer(const Actor& a, asset amount)
{
    _fixture.transfer(_fixture.get_account(_actor), _fixture.get_account(a), amount);
}

processed_transaction ActorActions::push_transaction(signed_transaction& tx)
{
    auto& db = _fixture.db;

    tx.validate();

    auto &&result = db.push_transaction(tx, database::skip_transaction_dupe_check);
    database_fixture::verify_asset_supplies(db);
    return result;
}

//////////////////////////////////////////////////////////////////////////
ActorActions::prepare_transaction::prepare_transaction(signed_transaction& tx, fixture_type& f)
    : _tx(tx)
    , _fixture(f) 
{
}

struct set_fee_visitor
{
    set_fee_visitor(const database &db,
                    operation &op):
        _db(db),
        _op(op){}

    typedef void result_type;

    template<typename T>
    void operator()( T& op )
    {
        if (op.fee.amount == 0)
        {
            this->_db.current_fee_schedule().set_fee(_op);
        }
    }

private:
    const database &_db;
    operation &_op;
};

ActorActions::prepare_transaction::~prepare_transaction()
{
    auto& db = _fixture.db;

    for (auto& op : _tx.operations)
    {
        auto vs = set_fee_visitor{db, op};
        op.visit(vs);
    }

    test::set_expiration(db, _tx);

    for (auto& signer : _signers)
    {
        _tx.sign(signer.private_key, _fixture.db.get_chain_id());
    }
}

