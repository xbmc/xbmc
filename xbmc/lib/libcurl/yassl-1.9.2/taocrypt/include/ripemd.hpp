/* ripemd.hpp                                
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

/* ripemd.hpp provides RIPEMD digest support
*/

#ifndef TAO_CRYPT_RIPEMD_HPP
#define TAO_CRYPT_RIPEMD_HPP

#include "hash.hpp"


#if defined(TAOCRYPT_X86ASM_AVAILABLE) && defined(TAO_ASM)
    #define DO_RIPEMD_ASM
#endif

namespace TaoCrypt {


// RIPEMD160 digest
class RIPEMD160 : public HASHwithTransform {
public:
    enum { BLOCK_SIZE = 64, DIGEST_SIZE = 20, PAD_SIZE = 56,
           TAO_BYTE_ORDER = LittleEndianOrder };   // in Bytes
    RIPEMD160() : HASHwithTransform(DIGEST_SIZE / sizeof(word32), BLOCK_SIZE)
                { Init(); }
    ByteOrder getByteOrder()  const { return ByteOrder(TAO_BYTE_ORDER); }
    word32    getBlockSize()  const { return BLOCK_SIZE; }
    word32    getDigestSize() const { return DIGEST_SIZE; }
    word32    getPadSize()    const { return PAD_SIZE; }

    RIPEMD160(const RIPEMD160&);
    RIPEMD160& operator= (const RIPEMD160&);

#ifdef DO_RIPEMD_ASM
    void Update(const byte*, word32);
#endif
    void Init();
    void Swap(RIPEMD160&);
private:
    void Transform();
    void AsmTransform(const byte* data, word32 times);
};

inline void swap(RIPEMD160& a, RIPEMD160& b)
{
    a.Swap(b);
}


} // namespace

#endif // TAO_CRYPT_RIPEMD_HPP

