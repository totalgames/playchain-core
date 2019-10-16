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

#include "room_rating.hpp"

#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

#include <playchain/chain/schema/room_object.hpp>
#include <playchain/chain/schema/room_rating_object.hpp>
#include <playchain/chain/schema/table_object.hpp>

#include <playchain/chain/evaluators/db_helpers.hpp>
#include <playchain/chain/playchain_utils.hpp>
#include <playchain/chain/evaluators/validators.hpp>

#include <chrono>
#include <algorithm>

namespace playchain { namespace chain {

namespace
{
    template <class T>
    T adjust_precision(const T& val)
    {
        return ROOM_RATING_PRECISION * val;
    }

    uint64_t time_factor(uint64_t x)
    {
        if (x <= TIME_FACTOR_FADE_START)
        {
            return adjust_precision(x);
        }
        else
        {
            return adjust_precision(TIME_FACTOR_C1 * std::exp(TIME_FACTOR_C2 * x) + TIME_FACTOR_C3);
        }
    }

    uint64_t quantity_factor(uint64_t N)
    {
        if (N <= QUANTITY_FACTOR_FADE_START)
        {
            return adjust_precision(QUANTITY_FACTOR_C1 + N * QUANTITY_FACTOR_C2);
        }
        else
        {
            return adjust_precision(std::log(N)/ QUANTITY_FACTOR_C3 + QUANTITY_FACTOR_C4);
        }
    }

    uint64_t stat_correction()
    {
        return adjust_precision(STAT_CORRECTION_M);
    }
}

namespace
{
    class randomizing_context
    {
    public:
        explicit randomizing_context(const database &d);

        uint16_t next() const;

    private:

        const size_t _MIN = 1;
        const size_t _MAX = 0xfff;

        int64_t _entropy = 0;
        mutable size_t _pos = 0;
    };

    randomizing_context::randomizing_context(const database &d)
    {
        _entropy = utils::get_database_entropy(d);
    }

    uint16_t randomizing_context::next() const
    {
        return (uint16_t)utils::get_pseudo_random(_entropy, _pos++, _MIN, _MAX);
    }

    void recalculate_rating_simple_impl(database &d, const randomizing_context &rnd, const room_object &room)
    {
        const auto& dprops = d.get_dynamic_global_properties();
        auto old_rating = room.rating;

        assert(sizeof(old_rating) == 4);

        auto new_rating = old_rating;

        //(1) randomize to make chance for different rooms with same rating
        new_rating  &= (~0xffff);
        new_rating |= rnd.next();

        // TODO: (2) calculate fraud statistic to decrease rating
        // TODO: (3) use votes from players to increase rating

        d.modify(room, [&](room_object &obj){
            obj.prev_rating = old_rating;
            obj.rating = new_rating;
            obj.last_rating_update = dprops.time;
        });
    }

    template<typename MeasurementsIndex>
    void remove_expired_room_rating_measurement(database &d)
    {
        auto& measurements_by_expiration = d.get_index_type<MeasurementsIndex>().indices().template get<by_expiration>();

        while (measurements_by_expiration.begin() != measurements_by_expiration.end()
            && measurements_by_expiration.begin()->expiration <= d.head_block_time())
        {
#if defined(LOG_RATING)
            if (d.head_block_time() >= fc::time_point_sec( LOG_RATING_BLOCK_TIMESTUMP_FROM ))
            {
                const auto &m = *measurements_by_expiration.begin();
                ilog("${t} >> remove_expired_room_rating_measurement: room = ${r}, m = ${id}", ("t", d.head_block_time())("r", m.room)("id", m.id));
            }
#endif
            // remove the db object
            d.remove(*measurements_by_expiration.begin());
        }
    }

    void remove_expired_room_rating_measurements(database &d)
    {
        remove_expired_room_rating_measurement<room_rating_kpi_measurement_index>(d);
        remove_expired_room_rating_measurement<room_rating_standby_measurement_index>(d);
    }

    struct measurement_waiting_resolve_visitor
    {
        measurement_waiting_resolve_visitor() = default;

        bool operator()(const room_rating_kpi_measurement_object &obj)
        {
            return obj.waiting_resolve;
        }

        bool operator()(const room_rating_standby_measurement_object &)
        {
            return false;
        }
    };

    struct measurement_since_seconds_visitor
    {
        measurement_since_seconds_visitor() = default;

        fc::time_point_sec operator()(const room_rating_kpi_measurement_object &obj)
        {
            return obj.created;
        }

        fc::time_point_sec operator()(const room_rating_standby_measurement_object &obj)
        {
            return obj.updated;
        }
    };

    template<typename MeasurementsIndex>
    std::tuple<uint64_t, uint64_t, uint32_t> calculate_measurements(database &d, const room_object &room)
    {
        measurement_waiting_resolve_visitor waiting_resolve;
        measurement_since_seconds_visitor since_seconds;

        uint64_t weight_sum_by_time_factor = 0;
        uint64_t measurement_sum_by_time_factor = 0;

        auto& measurements_by_room = d.get_index_type<MeasurementsIndex>().indices().template get<by_room>();

        auto range = measurements_by_room.equal_range(room.id);
        uint32_t measurement_quantity = 0;
        for (auto it =range.first; it!=range.second; ++it)
        {
            auto& measurement = *it;

            if(waiting_resolve(measurement))
                continue;

            auto since = since_seconds(measurement);
            std::chrono::seconds elapsed_secconds(d.head_block_time().sec_since_epoch() - since.sec_since_epoch());
            auto minutes_from_measurement_till_now = std::chrono::duration_cast<std::chrono::minutes> (elapsed_secconds);

            auto tf = time_factor(minutes_from_measurement_till_now.count());
            weight_sum_by_time_factor += measurement.weight * tf;
            measurement_sum_by_time_factor += tf;

            ++measurement_quantity;
        }

        return std::make_tuple(weight_sum_by_time_factor, measurement_sum_by_time_factor, measurement_quantity);
    }

    void recalculate_room_rating_factors(database &d, const room_object &room)
    {
        auto kpi_measurements = calculate_measurements<room_rating_kpi_measurement_index>(d, room);
        auto standby_measurements = calculate_measurements<room_rating_standby_measurement_index>(d, room);

        d.modify(room, [&](room_object &obj) {
            obj.weight_sum_by_time_factor = std::get<0>(kpi_measurements) + std::get<0>(standby_measurements);
            obj.measurement_sum_by_time_factor = std::get<1>(kpi_measurements) + std::get<1>(standby_measurements);
            obj.measurement_quantity = std::get<2>(kpi_measurements) + std::get<2>(standby_measurements);
        });
    }

    void recalculate_room_rating(database &d, const room_object& room, uint64_t constC_weight_sum_by_time_factor, uint64_t constC_measurement_sum_by_time_factor)
    {
        const auto& dprops = d.get_dynamic_global_properties();

        constC_measurement_sum_by_time_factor = std::max(constC_measurement_sum_by_time_factor, (uint64_t)1);


        auto new_rating = (fc::uint128_t(room.weight_sum_by_time_factor) * constC_measurement_sum_by_time_factor + constC_weight_sum_by_time_factor * stat_correction())
                                * 25 * quantity_factor(room.measurement_quantity)
                                / (constC_measurement_sum_by_time_factor * (room.measurement_sum_by_time_factor + stat_correction()));

        d.modify(room, [&](room_object &obj) {
            obj.prev_rating = obj.rating;
            obj.rating = new_rating.to_integer();
            obj.last_rating_update = dprops.time;
        });
    }
}

void update_room_rating_simple_impl(database &d)
{
    auto& rooms_by_last_update = d.get_index_type<room_index>().indices().get<by_last_rating_update>();

    if (rooms_by_last_update.size() == 1) //only special room
        return;

    randomizing_context rnd{d};

    const auto& parameters = get_playchain_parameters(d);
    auto itr = rooms_by_last_update.begin();
    size_t ci = 0;
    while( itr != rooms_by_last_update.end() &&
           ci++ < parameters.rooms_rating_recalculations_per_maintenance)
    {
        const room_object &room = *itr++;
        if (room_object::is_special_room(room.id))
            continue;

        if (room.last_updated_table != table_id_type{})
        {
            --ci;
            continue;
        }
        recalculate_rating_simple_impl(d, rnd, room);
    }
}

void update_room_rating(database &d)
{
    if (d.head_block_time() < HARDFORK_PLAYCHAIN_5_TIME)
    {
        return update_room_rating_simple_impl(d);
    }

    auto& rooms_by_last_update = d.get_index_type<room_index>().indices().get<by_last_rating_update>();

    if (rooms_by_last_update.size() == 1) //only special room
        return;

    uint64_t constC_weight_sum_by_time_factor = 0;
    uint64_t constC_measurement_sum_by_time_factor = 0;

    using cref_type = std::reference_wrapper<const room_object>;
    std::vector<cref_type> updated_rooms;

    const auto& parameters = get_playchain_parameters(d);
    auto itr = rooms_by_last_update.begin();
    size_t ci = 0;
    while (itr != rooms_by_last_update.end() &&
        ci++ < parameters.rooms_rating_recalculations_per_maintenance)
    {
        const room_object &room = *itr++;
        if (room_object::is_special_room(room.id))
            continue;

        if (room.last_updated_table != table_id_type{})
        {
            --ci;
            continue;
        }

        recalculate_room_rating_factors(d, room);

        constC_weight_sum_by_time_factor += room.weight_sum_by_time_factor;
        constC_measurement_sum_by_time_factor += room.measurement_sum_by_time_factor;

        updated_rooms.emplace_back(std::cref(room));
    }

    uint64_t average_room_rating = 0;
    for (const room_object& room: updated_rooms)
    {
        recalculate_room_rating(d, room, constC_weight_sum_by_time_factor, constC_measurement_sum_by_time_factor);

#if defined(LOG_RATING)
        if (d.head_block_time() >= fc::time_point_sec( LOG_RATING_BLOCK_TIMESTUMP_FROM ))
        {
            ilog("${t} >> rating: ${id} - ${r}", ("t", d.head_block_time())("id", room.id)("r", room.rating));
            idump((room));
        }
#endif
        average_room_rating += room.rating;
    }
    average_room_rating = average_room_rating / updated_rooms.size();

#if defined(LOG_RATING)
    if (d.head_block_time() >= fc::time_point_sec( LOG_RATING_BLOCK_TIMESTUMP_FROM ))
    {
        ilog("${t} >> average_room_rating = ${r}", ("t", d.head_block_time())("r", average_room_rating));
    }
#endif

    d.modify(get_dynamic_playchain_properties(d), [&](dynamic_playchain_property_object &obj) {
        obj.average_room_rating = average_room_rating;
    });
}


void update_table_weight(database &d)
{
    const auto& parameters = get_playchain_parameters(d);
    const auto& idx = d.get_index_type<room_index>().indices().get<by_room_rating>();
    using cref_type = std::reference_wrapper<const room_object>;
    std::vector<cref_type> rooms;
    for( const room_object& room : idx )
    {
        if (room.is_not_changed_rating())
            continue;

        rooms.emplace_back(std::cref(room));
    }

    if (!rooms.empty())
    {
        auto& tables_by_last_update = d.get_index_type<table_index>().indices().get<by_room>();
        for( const room_object& room : rooms )
        {
            auto last_updated_table = room.last_updated_table;
            auto itr = tables_by_last_update.lower_bound(std::make_tuple(room.id, last_updated_table));
            size_t ci = 0;
            while( itr != tables_by_last_update.end() &&
                   ci++ < parameters.tables_weight_recalculations_per_maintenance)
            {
                const table_object &table = *itr++;
                if (table_object::is_special_table(table.id))
                    continue;

                table.set_weight(d);
                last_updated_table = table.id;
            }
            d.modify(room, [&](room_object &obj){
                if (ci >= parameters.tables_weight_recalculations_per_maintenance)
                {
                    obj.last_updated_table = last_updated_table;
                }
                else
                {
                    obj.last_updated_table = table_id_type{};
                    obj.prev_rating = obj.rating;
                }
            });
        }
    }
}

void clenup_room_rating(database &d)
{
    remove_expired_room_rating_measurements(d);
}

}}

