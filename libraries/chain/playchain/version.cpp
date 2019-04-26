/*
 * Copyright (c) 2018 Total Games LLC, and contributors.
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

#include <playchain/chain/version.hpp>

#include <fc/exception/exception.hpp>
#include <fc/variant.hpp>

#include <sstream>

namespace playchain {
namespace chain {

/* Quick conversion utilities from http://joelverhagen.com/blog/2010/11/convert-an-int-to-a-string-and-vice-versa-in-c/
 */
inline int string_to_int(fc::string input)
{
    std::stringstream s(input);
    int i;
    s >> i;
    return i;
}

inline fc::string int_to_string(int input)
{
    std::stringstream s;
    s << input;
    return s.str();
}

version::version(uint8_t m, uint8_t h, uint16_t r)
{
    v_num = (0 | m) << 8;
    v_num = (v_num | h) << 16;
    v_num = v_num | r;
}

uint8_t version::major() const
{
    return ((v_num >> 24) & 0x000000FF);
}

uint8_t version::minor() const
{
    return ((v_num >> 16) & 0x000000FF);
}

uint16_t version::patch() const
{
    return ((v_num & 0x0000FFFF));
}

version::operator fc::string() const
{
    std::stringstream s;
    s << (int)major() << '.' << (int)minor() << '.' << patch();

    return s.str();
}

version_ext::operator fc::string() const
{
    std::string result = base;
    if (!metadata.empty())
    {
        result += '+';
        result += metadata;
    }

    return result;
}
}
} // playchain::chain

namespace fc {
void to_variant(const playchain::chain::version& v, variant& var)
{
    var = fc::string(v);
}

void from_variant(const variant& var, playchain::chain::version& v)
{
    uint32_t major = 0, hardfork = 0, revision = 0;
    char dot_a = 0, dot_b = 0;

    std::stringstream s(var.as_string());
    s >> major >> dot_a >> hardfork >> dot_b >> revision;

    // We'll accept either m.h.v or m_h_v as canonical version strings
    FC_ASSERT((dot_a == '.' || dot_a == '_') && dot_a == dot_b,
              "Variant does not contain proper dotted decimal format");
    FC_ASSERT(major <= 0xFF, "Major version is out of range");
    FC_ASSERT(hardfork <= 0xFF, "Minor/Hardfork version is out of range");
    FC_ASSERT(revision <= 0xFFFF, "Patch/Revision version is out of range");
    FC_ASSERT(s.eof(), "Extra information at end of version string");

    v.v_num = 0 | (major << 24) | (hardfork << 16) | revision;
}

void to_variant(const playchain::chain::version_ext& v, variant& var)
{
    var = fc::string(v);
}

void from_variant(const variant& var, playchain::chain::version_ext& v)
{
    auto input_str = var.as_string();

    auto it = input_str.begin();
    while (it != input_str.end())
    {
        char ch = *it;
        if (ch == '+')
        {
            break;
        }
        ++it;
    }

    fc::string base{input_str.begin(), it};
    from_variant(base, v.base);
    if (it != input_str.end())
    {
        v.metadata = fc::string{++it, input_str.end()};
    }
}

void to_variant(const playchain::chain::hardfork_version& hv, variant& var)
{
    to_variant((const playchain::chain::version&)hv, var);
}

void from_variant(const variant& var, playchain::chain::hardfork_version& hv)
{
    playchain::chain::version ver;
    from_variant(var, ver);
    hv.v_num = ver.v_num & 0xffff0000;
}
}
