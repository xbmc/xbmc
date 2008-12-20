/* md5.hpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* md2.hpp provides MD2 digest support, see RFC 1319
*/

#ifndef TAO_CRYPT_MD2_HPP
#define TAO_CRYPT_MD2_HPP


#include "hash.hpp"
#include "block.hpp"


namespace TaoCrypt {


// MD2 digest
class MD2 : public HASH {
public:
    enum { BLOCK_SIZE = 16, DIGEST_SIZE = 16, PAD_SIZE = 16, X_SIZE = 48 };
    MD2();

    word32 getBlockSize()  const { return BLOCK_SIZE; }
    word32 getDigestSize() const { return DIGEST_SIZE; }

    void Update(const byte*, word32);
    void Final(byte*);

    void Init();
    void Swap(MD2&);
private:
    ByteBlock X_, C_, buffer_;
    word32    count_;           // bytes % PAD_SIZE

    MD2(const MD2&);
    MD2& operator=(const MD2&);
};

inline void swap(MD2& a, MD2& b)
{
    a.Swap(b);
}


} // namespace

#endif // TAO_CRYPT_MD2_HPP

