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

#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {
    class database;
}}

namespace playchain { namespace chain { namespace utils {

using namespace graphene::chain;

/**
 * Creates invitation digest that should be used to create MANDAT for resolve operation
 *
 * @param chain_id
 * @param inviter the name of inviter (not id)
 * @param invitation_uid the invitation UID
 * @returns the digest
 */
digest_type create_digest_by_invitation(const chain_id_type& chain_id, const string &inviter, const string &invitation_uid);

public_key_type get_signature_key(const signature_type &signature, const digest_type &);

//it is not recommend use in speed required functions
int64_t get_database_entropy(const database &d);

size_t get_pseudo_random(const int64_t entropy, const size_t pos = 0xff, const size_t min = 0, const size_t max = 0xffff);

}}}
