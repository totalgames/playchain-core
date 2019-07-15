#include "playchain_common.hpp"

namespace donate_tests
{
struct donate_fixture: public playchain_common::playchain_fixture
{
    const int64_t poor_player_init_balance = 2 * GRAPHENE_BLOCKCHAIN_PRECISION;
    const int64_t normal_player_init_balance = 10 * poor_player_init_balance;
    const int64_t minimum_donate = normal_player_init_balance / 2;

    DECLARE_ACTOR(playerwithoutbalance)
    DECLARE_ACTOR(poorplayer)
    DECLARE_ACTOR(normalplayer)

    donate_fixture()
    {
        actor(playerwithoutbalance);
        actor(poorplayer).supply(asset(poor_player_init_balance));
        actor(normalplayer).supply(asset(normal_player_init_balance));

        init_fees();
    }

    void init_fees()
    {
        playchain_common::playchain_fixture::init_fees();

        flat_set< fee_parameters > fees = db.get_global_properties().parameters.get_current_fees().parameters;

        donate_to_playchain_operation::fee_parameters_type donate_params;
        donate_params.fee = minimum_donate;
        fees.erase(donate_params);
        fees.insert(donate_params);

        change_fees( fees );
    }

    donate_to_playchain_operation donate_op(const account_id_type& donator, const asset &amount)
    {
        donate_to_playchain_operation op;

        op.donator = donator;
        op.fee = amount;

        return op;
    }

    donate_to_playchain_operation donate_op(const Actor& donator, const asset &amount)
    {
        return donate_op(actor(donator), amount);
    }

    donate_to_playchain_operation donate(const Actor& donator, const asset &amount)
    {
        auto op = donate_op(donator, amount);

        actor(donator).push_operation(op);
        return op;
    }

    asset get_account_balance(const Actor& account)
    {
        return db.get_balance(actor(account), asset_id_type());
    }

};

BOOST_FIXTURE_TEST_SUITE( donate_tests, donate_fixture)

PLAYCHAIN_TEST_CASE(check_negative_donate_operation)
{
    BOOST_CHECK_THROW(donate_op(normalplayer, asset{0}).validate(), fc::exception);
    BOOST_CHECK_THROW(donate_op(GRAPHENE_COMMITTEE_ACCOUNT, asset{normal_player_init_balance}).validate(), fc::exception);
}

PLAYCHAIN_TEST_CASE(check_normal_donate_operation)
{
    auto balance = get_account_balance(normalplayer);

    auto amount = asset{ balance.amount / 2, balance.asset_id };

    try
    {
        donate(normalplayer, amount);
    }FC_LOG_AND_RETHROW()

    generate_block();

    BOOST_CHECK_EQUAL(to_string(get_account_balance(normalplayer)),
                      to_string(balance - amount) );
}

PLAYCHAIN_TEST_CASE(check_negative_donate_operation_insufficient_funds)
{
    auto poorbalance = get_account_balance(poorplayer);

    BOOST_CHECK_THROW(donate(poorplayer, poorbalance), fc::exception);

    auto balance = get_account_balance(normalplayer);

    BOOST_CHECK_NO_THROW(donate(normalplayer, balance));

    BOOST_CHECK_THROW(donate(normalplayer, poorbalance), fc::exception);

    BOOST_CHECK_THROW(donate(normalplayer, poorbalance), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
