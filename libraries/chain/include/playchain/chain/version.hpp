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

#pragma once

#include <fc/string.hpp>
#include <fc/time.hpp>

namespace playchain {
namespace chain {

/*
 * This class represents the basic versioning scheme of the PlayChain.
 * All versions are a triple consisting of a major version, hardfork version, and release version.
 * It allows easy comparison between versions. A version is a read only object.
 */
struct version
{
    version()
    {
    }
    version(uint8_t m, uint8_t h, uint16_t r);
    virtual ~version()
    {
    }

    bool operator==(const version& o) const
    {
        return v_num == o.v_num;
    }
    bool operator!=(const version& o) const
    {
        return v_num != o.v_num;
    }
    bool operator<(const version& o) const
    {
        return v_num < o.v_num;
    }
    bool operator<=(const version& o) const
    {
        return v_num <= o.v_num;
    }
    bool operator>(const version& o) const
    {
        return v_num > o.v_num;
    }
    bool operator>=(const version& o) const
    {
        return v_num >= o.v_num;
    }

    uint8_t major() const;
    uint8_t minor() const;
    uint16_t patch() const;

    operator fc::string() const;

    uint32_t v_num = 0;
};

struct version_ext
{
    version base;
    fc::string metadata;

    operator fc::string() const;

    friend bool operator==(const version_ext& a, const version_ext& b)
    {
        return a.base.major() == b.base.major() &&
               a.base.minor() == b.base.minor();
    }
    friend bool operator!=(const version_ext& a, const version_ext& b)
    {
        return !(a == b);
    }
};

struct hardfork_version : version
{
    hardfork_version()
        : version()
    {
    }
    hardfork_version(uint8_t m, uint8_t h)
        : version(m, h, 0)
    {
    }
    hardfork_version(version v)
    {
        v_num = v.v_num & 0xFFFF0000;
    }
    ~hardfork_version()
    {
    }

    void operator=(const version& o)
    {
        v_num = o.v_num & 0xFFFF0000;
    }
    void operator=(const hardfork_version& o)
    {
        v_num = o.v_num & 0xFFFF0000;
    }

    bool operator==(const hardfork_version& o) const
    {
        return v_num == o.v_num;
    }
    bool operator!=(const hardfork_version& o) const
    {
        return v_num != o.v_num;
    }
    bool operator<(const hardfork_version& o) const
    {
        return v_num < o.v_num;
    }
    bool operator<=(const hardfork_version& o) const
    {
        return v_num <= o.v_num;
    }
    bool operator>(const hardfork_version& o) const
    {
        return v_num > o.v_num;
    }
    bool operator>=(const hardfork_version& o) const
    {
        return v_num >= o.v_num;
    }

    bool operator==(const version& o) const
    {
        return v_num == (o.v_num & 0xFFFF0000);
    }
    bool operator!=(const version& o) const
    {
        return v_num != (o.v_num & 0xFFFF0000);
    }
    bool operator<(const version& o) const
    {
        return v_num < (o.v_num & 0xFFFF0000);
    }
    bool operator<=(const version& o) const
    {
        return v_num <= (o.v_num & 0xFFFF0000);
    }
    bool operator>(const version& o) const
    {
        return v_num > (o.v_num & 0xFFFF0000);
    }
    bool operator>=(const version& o) const
    {
        return v_num >= (o.v_num & 0xFFFF0000);
    }
};

struct hardfork_version_vote
{
    hardfork_version_vote()
    {
    }
    hardfork_version_vote(hardfork_version v, fc::time_point_sec t)
        : hf_version(v)
        , hf_time(t)
    {
    }

    hardfork_version hf_version;
    fc::time_point_sec hf_time;
};
}
} // playchain::chain

namespace fc {
class variant;
void to_variant(const playchain::chain::version& v, variant& var);
void from_variant(const variant& var, playchain::chain::version& v);

void to_variant(const playchain::chain::version_ext& v, variant& var);
void from_variant(const variant& var, playchain::chain::version_ext& v);

void to_variant(const playchain::chain::hardfork_version& hv, variant& var);
void from_variant(const variant& var, playchain::chain::hardfork_version& hv);
} // fc

#include <fc/reflect/reflect.hpp>
FC_REFLECT(playchain::chain::version, (v_num))
FC_REFLECT(playchain::chain::version_ext, (base)(metadata))
FC_REFLECT_DERIVED(playchain::chain::hardfork_version, (playchain::chain::version), )

FC_REFLECT(playchain::chain::hardfork_version_vote, (hf_version)(hf_time))
