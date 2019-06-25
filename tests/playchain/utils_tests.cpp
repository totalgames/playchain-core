#include "playchain_common.hpp"

#include <playchain/chain/playchain_utils.hpp>

#include <vector>
#include <algorithm>

namespace playchain_utils_tests
{

struct playchain_utils_fixture: public playchain_common::playchain_fixture
{
    DECLARE_ACTOR(alice)
    DECLARE_ACTOR(bob)

    playchain_utils_fixture()
    {
        actor(alice).supply(asset(player_init_balance));
        actor(bob).supply(asset(player_init_balance));

        init_fees();
    }

    void set_expiration( transaction& tx )
    {
       const chain_parameters& params = db.get_global_properties().parameters;
       tx.set_reference_block(db.head_block_id());
       tx.set_expiration( db.head_block_time() + fc::seconds( params.block_interval * (params.maintenance_skip_slots + 1) * 3 ) );
       return;
    }

    void transfer(
       Actor from,
       Actor to,
       const asset& amount,
       bool sign
       )
    {
        try
        {
            signed_transaction trx;
            account_id_type from_id = actor(from);
            account_id_type to_id = actor(to);
            set_expiration( trx );
            transfer_operation trans;
            trans.from = from_id;
            trans.to = to_id;
            trans.amount = amount;
            trx.operations.push_back(trans);
            trx.validate();
            if (sign)
                trx.sign(from.private_key, db.get_chain_id());
            PUSH_TX(db, trx, ~0 & ~database::skip_transaction_dupe_check);
        } FC_CAPTURE_AND_RETHROW( (from.name)(to.name)(amount) )
    }

};

using namespace playchain::chain::utils;

BOOST_FIXTURE_TEST_SUITE(utils_tests, playchain_utils_fixture)

PLAYCHAIN_TEST_CASE(check_database_entropy)
{
    auto new_entr = get_database_entropy(db);

    std::vector<decltype (new_entr)> entropies;

    BOOST_CHECK_NE(new_entr, 0);

    entropies.emplace_back(new_entr);
    auto last_entr = new_entr;

    BOOST_REQUIRE_NO_THROW(transfer(alice, bob, asset(1), true));

    new_entr = get_database_entropy(db);

    BOOST_CHECK_NE(new_entr, last_entr);

    entropies.emplace_back(new_entr);
    last_entr = new_entr;

    BOOST_REQUIRE_NO_THROW(transfer(bob, alice, asset(1), true));

    new_entr = get_database_entropy(db);

    BOOST_CHECK_NE(new_entr, last_entr);

    entropies.emplace_back(new_entr);
    last_entr = new_entr;

    //clear trx index
    generate_blocks(10);

    new_entr = get_database_entropy(db);

    BOOST_CHECK_NE(new_entr, last_entr);

    entropies.emplace_back(new_entr);
    last_entr = new_entr;

    //it creates entropy by signatures if current block is not changed
    BOOST_REQUIRE_NO_THROW(transfer(bob, alice, asset(1), false));

    new_entr = get_database_entropy(db);

    BOOST_CHECK_EQUAL(new_entr, last_entr);

    entropies.emplace_back(new_entr);
    last_entr = new_entr;

    generate_blocks(10);

    new_entr = get_database_entropy(db);

    BOOST_CHECK_NE(new_entr, last_entr);
    entropies.emplace_back(new_entr);

    idump((entropies))
}

PLAYCHAIN_TEST_CASE(check_pseudo_random)
{
    const size_t min = 1;
    const size_t max = 2019;

    auto entropy = 0;
    int pos = 0;

    BOOST_CHECK_EQUAL(get_pseudo_random(entropy, pos, min, max), (uint64_t)min);

    auto new_rnd = get_pseudo_random(entropy, pos++, min, max);

    BOOST_CHECK(new_rnd >= min && new_rnd <= max);

    auto last_rnd = new_rnd;

    new_rnd = get_pseudo_random(entropy, pos++, min, max);

    BOOST_CHECK(new_rnd >= min && new_rnd <= max);
    BOOST_CHECK_NE(new_rnd, last_rnd);

    last_rnd = new_rnd;

    new_rnd = get_pseudo_random(entropy, --pos, min, max);

    BOOST_CHECK_EQUAL(new_rnd, last_rnd);

    last_rnd = new_rnd;

    new_rnd = get_pseudo_random(entropy + 1, pos, min, max);

    BOOST_CHECK(new_rnd >= min && new_rnd <= max);
    BOOST_CHECK_NE(new_rnd, last_rnd);

    std::vector<int64_t> entropies{2184183036263686198, 8178022012374262838, -5510499060151348253, -1939016983238769590, -1939016983238769590, -1024446712921177826};
    std::vector<size_t> rnds;
    rnds.reserve(entropies.size() * 2);
    for(const auto e: entropies)
    {
        rnds.emplace_back(get_pseudo_random(e, 0, min, max));
        rnds.emplace_back(get_pseudo_random(e, 1, min, max));
    }

    decltype(rnds) rnds_etalon{1697, 562, 2012, 79, 41, 18, 1344, 513, 1344, 513, 910, 1827};

    BOOST_CHECK(std::equal(begin(rnds), end(rnds), begin(rnds)));
}

BOOST_AUTO_TEST_SUITE_END()
}
