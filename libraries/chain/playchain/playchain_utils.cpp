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

#include <playchain/chain/playchain_utils.hpp>

#include <graphene/chain/database.hpp>

#include <graphene/chain/transaction_object.hpp>

#include <fc/crypto/ripemd160.hpp>

namespace playchain { namespace chain { namespace utils {

digest_type create_digest_by_invitation(const chain_id_type& chain_id, const string &inviter, const string &invitation_uid)
{
    digest_type::encoder enc;
    fc::raw::pack( enc, chain_id );
    fc::raw::pack( enc, inviter );
    fc::raw::pack( enc, invitation_uid );
    return enc.result();
}

public_key_type get_signature_key(const signature_type &signature, const digest_type &digest)
{
    public_key_type result = fc::ecc::public_key(signature, digest);
    FC_ASSERT(public_key_type() != result);
    return result;
}

int64_t get_database_entropy(const database &d)
{
    fc::ripemd160 h;
    const auto &props = d.get_dynamic_global_properties();
    auto block_num = props.last_irreversible_block_num;
    if (block_num < 1)
        block_num = props.head_block_number;
    auto pblock = d.fetch_block_by_number(block_num);
    if (pblock.valid())
        h = pblock->id();
    else
        h = fc::ripemd160::hash(d.get_chain_id());

    fc::array<char, 6> buff;

    size_t ci = 0, cj = 0;
    char *phdata = h.data();
    for(; ci < h.data_size() && cj < buff.size(); ++cj, ++phdata)
    {
        if (*phdata)
        {
            buff.at(ci++) = *phdata;
        }
    }

    auto &&trx_index = d.get_index_type<transaction_index>().indices().get<by_trx_id>();

    auto it = trx_index.begin();
    while (it != trx_index.end() && ci < buff.size())
    {
        for (auto &&sig: it->trx.signatures)
        {
            buff.at(ci) ^= sig.data[ci % sig.size()] % 0xff;
            if (++ci >= buff.size())
                break;
        }
        ++it;
    }

    h = fc::ripemd160::hash((char *)buff.data, buff.size());

    int64_t result = 0;
    result = (int64_t)h._hash[0];
    result <<= 32;
    result |= h._hash[1];

    return result;
}

size_t get_pseudo_random(const int64_t entropy, const size_t pos /*= 0xff*/, const size_t min /*= 0*/, const size_t max /*= 0xffff*/)
{
    auto now_hi = uint64_t(entropy) << 32;

    /// High performance random generator
    /// http://xorshift.di.unimi.it/
    uint64_t k = now_hi + uint64_t(pos)*2685821657736338717ULL;
    k ^= (k >> 12);
    k ^= (k << 25);
    k ^= (k >> 27);
    k *= 2685821657736338717ULL;

    return min + (size_t)(k % max);
}

}}}
