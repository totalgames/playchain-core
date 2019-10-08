/*
 * Copyright (c) 2018 Total Games LLC and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <playchain/chain/playchain_config.hpp>

#include <boost/make_unique.hpp>

#include <fc/exception/exception.hpp>

namespace playchain {
namespace protocol {
namespace detail {

std::unique_ptr<config> config::instance = boost::make_unique<config>();

namespace
{
    template<typename T>
    bool is_continuous_bit_sequence_set(const T value)
    {
        auto test_bits = value;
        auto ci = sizeof(test_bits);
        while(test_bits && ci-- > 0)
        {
            char bt = (char)test_bits;
            if (bt != (char)0xff)
                return false;
            test_bits >>= 8;
        }
        return true;
    }
}

config::config() /// production config
    : blockid_pool_size(GRAPHENE_BLOCKID_POOL_SIZE)
    , maximum_inviters_depth(10)
    , voting_for_playing_expiration_seconds((uint32_t)fc::seconds(9).to_seconds())
    , voting_for_results_expiration_seconds((uint32_t)fc::seconds(9).to_seconds())
    , game_lifetime_limit_in_seconds((uint32_t)fc::days(1).to_seconds())
    , pending_buyin_proposal_lifetime_limit_in_seconds((uint32_t)fc::seconds(15).to_seconds())
    , amount_reserving_places_per_user(1)
    , min_votes_for_playing(2)
    , min_votes_for_results(1)
    , minimum_desired_number_of_players_for_tables_allocation(1)
    , maximum_desired_number_of_players_for_tables_allocation(5)
    , buy_in_expiration_seconds((uint32_t)fc::minutes(3).to_seconds())
    , table_alive_expiration_seconds((uint32_t)fc::minutes(10).to_seconds())
    , room_rating_measurements_alive_periods((uint32_t)10)
    , kpi_weight_per_measurement((uint16_t)1)
    , standby_weight_per_measurement((uint16_t)1)
{
}

config::config(test_mode) /// test config
    : blockid_pool_size(0xff)
    , maximum_inviters_depth(3)
    , voting_for_playing_expiration_seconds((uint32_t)fc::seconds(30).to_seconds())
    , voting_for_results_expiration_seconds((uint32_t)fc::minutes(1).to_seconds())
    , game_lifetime_limit_in_seconds((uint32_t)fc::minutes(20).to_seconds())
    , pending_buyin_proposal_lifetime_limit_in_seconds((uint32_t)fc::minutes(2).to_seconds())
    , amount_reserving_places_per_user(2)
    , min_votes_for_playing(2)
    , min_votes_for_results(2)
    , minimum_desired_number_of_players_for_tables_allocation(1)
    , maximum_desired_number_of_players_for_tables_allocation(5)
    , buy_in_expiration_seconds((uint32_t)fc::minutes(5).to_seconds())
    , table_alive_expiration_seconds((uint32_t)fc::minutes(10).to_seconds())
    , room_rating_measurements_alive_periods((uint32_t)2)
    , kpi_weight_per_measurement((uint16_t)1)
    , standby_weight_per_measurement((uint16_t)1)
{
    FC_ASSERT(is_continuous_bit_sequence_set(blockid_pool_size),
              "blockid_pool_size must have all bits set");
}

const config& get_config()
{
    return *config::instance;
}

void override_config(std::unique_ptr<config> new_config)
{
    config::instance = std::move(new_config);
}
}
}
}
