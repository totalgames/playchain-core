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

#include <playchain/chain/sign_utils.hpp>

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

}}}
