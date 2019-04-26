#include "playchain_common.hpp"

#include <graphene/chain/witness_object.hpp>

namespace graphene_witness_tests
{
struct graphene_witness_fixture: public playchain_common::playchain_fixture
{
    const int64_t rich_registrator_init_balance = 3000*GRAPHENE_BLOCKCHAIN_PRECISION;

    DECLARE_ACTOR(witness1)
    DECLARE_ACTOR(witness2)
    DECLARE_ACTOR(player1)
    DECLARE_ACTOR(player2)
    DECLARE_ACTOR(player3)

    graphene_witness_fixture()
    {
        actor(witness1).supply(asset(rich_registrator_init_balance));
        actor(witness2).supply(asset(rich_registrator_init_balance));

        init_fees();

        CREATE_PLAYER(witness1, player1);
        CREATE_PLAYER(witness1, player2);
        CREATE_PLAYER(witness1, player3);
    }

    const witness_object &get_witness(const Actor &witness) const
    {
        auto &&witness_account_object = get_account(witness);

        const auto& witness_by_account = db.get_index_type<witness_index>().indices().get<by_account>();
        auto it = witness_by_account.find(witness_account_object.id);
        BOOST_REQUIRE(witness_by_account.end() != it);

        return (*it);
    }

    bool is_active_witness(const Actor &member) const
    {
        auto account_id = get_account(member).id;
        auto &&active = db.get_global_properties().active_witnesses;
        auto lmb = [this, &account_id](const witness_id_type &w_id)
        {
            return w_id(db).witness_account == account_id;
        };
        return active.end() != std::find_if(begin(active), end(active), lmb);
    }

    void vote_for_witness(const Actor &member, const Actor &voter, bool approve = true)
    {
        auto vote_id = get_witness(member).vote_id;
        vote_for(vote_id, voter, approve);
    }
};

BOOST_FIXTURE_TEST_SUITE( graphene_witness_tests, graphene_witness_fixture)

PLAYCHAIN_TEST_CASE(check_add_new_witness)
{
    ilog(">>> Graphene Witnesses (0): ${l}", ("l", db.get_global_properties().active_witnesses));

    fc::ecc::private_key witness_key = create_private_key_from_password("for signing blocks", witness1);
    witness_id_type new_witness_id = database_fixture::create_witness(get_account(witness1), witness_key).id;

    vote_for_witness(witness1, witness1);
    for(const auto &wit: db.get_global_properties().active_witnesses)
    {
        vote_for(wit(db).vote_id, witness1);
    }

    next_maintenance();

    ilog(">>> Graphene Witnesses (1): ${l}", ("l", db.get_global_properties().active_witnesses));

    BOOST_CHECK(is_active_witness(witness1));
}

BOOST_AUTO_TEST_SUITE_END()
}
