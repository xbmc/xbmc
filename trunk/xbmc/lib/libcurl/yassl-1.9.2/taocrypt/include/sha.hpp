/* sha.hpp                                
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

/* sha.hpp provides SHA-1 digests, see RFC 3174
*/

#ifndef TAO_CRYPT_SHA_HPP
#define TAO_CRYPT_SHA_HPP

#include "hash.hpp"


#if defined(TAOCRYPT_X86ASM_AVAILABLE) && defined(TAO_ASM)
    #define DO_SHA_ASM
#endif

namespace TaoCrypt {


// SHA-1 digest
class SHA : public HASHwithTransform {
public:
    enum { BLOCK_SIZE = 64, DIGEST_SIZE = 20, PAD_SIZE = 56,
           TAO_BYTE_ORDER = BigEndianOrder};   // in Bytes
    SHA() : HASHwithTransform(DIGEST_SIZE / sizeof(word32), BLOCK_SIZE)
                { Init(); }
    ByteOrder getByteOrder()  const { return ByteOrder(TAO_BYTE_ORDER); }
    word32    getBlockSize()  const { return BLOCK_SIZE; }
    word32    getDigestSize() const { return DIGEST_SIZE; }
    word32    getPadSize()    const { return PAD_SIZE; }

#ifdef DO_SHA_ASM
    void Update(const byte* data, word32 len);
#endif
    void Init();

    SHA(const SHA&);
    SHA& operator= (const SHA&);

    void Swap(SHA&);
private:
    void Transform();
    void AsmTransform(const byte* data, word32 times);
};


inline void swap(SHA& a, SHA& b)
{
    a.Swap(b);
}

// SHA-256 digest
class SHA256 : public HASHwithTransform {
public:
    enum { BLOCK_SIZE = 64, DIGEST_SIZE = 32, PAD_SIZE = 56,
           TAO_BYTE_ORDER = BigEndianOrder};   // in Bytes
    SHA256() : HASHwithTransform(DIGEST_SIZE / sizeof(word32), BLOCK_SIZE)
                { Init(); }
    ByteOrder getByteOrder()  const { return ByteOrder(TAO_BYTE_ORDER); }
    word32    getBlockSize()  const { return BLOCK_SIZE; }
    word32    getDigestSize() const { return DIGEST_SIZE; }
    word32    getPadSize()    const { return PAD_SIZE; }

    void Init();

    SHA256(const SHA256&);
    SHA256& operator= (const SHA256&);

    void Swap(SHA256&);
private:
    void Transform();
};


// SHA-224 digest
class SHA224 : public HASHwithTransform {
public:
    enum { BLOCK_SIZE = 64, DIGEST_SIZE = 28, PAD_SIZE = 56,
           TAO_BYTE_ORDER = BigEndianOrder};   // in Bytes
    SHA224() : HASHwithTransform(SHA256::DIGEST_SIZE /sizeof(word32),BLOCK_SIZE)
                { Init(); }
    ByteOrder getByteOrder()  const { return ByteOrder(TAO_BYTE_ORDER); }
    word32    getBlockSize()  const { return BLOCK_SIZE; }
    word32    getDigestSize() const { return DIGEST_SIZE; }
    word32    getPadSize()    const { return PAD_SIZE; }

    void Init();

    SHA224(const SHA224&);
    SHA224& operator= (const SHA224&);

    void Swap(SHA224&);
private:
    void Transform();
};


#ifdef WORD64_AVAILABLE

// SHA-512 digest
class SHA512 : public HASH64withTransform {
public:
    enum { BLOCK_SIZE = 128, DIGEST_SIZE = 64, PAD_SIZE = 112,
           TAO_BYTE_ORDER = BigEndianOrder};   // in Bytes
    SHA512() : HASH64withTransform(DIGEST_SIZE / sizeof(word64), BLOCK_SIZE)
                { Init(); }
    ByteOrder getByteOrder()  const { return ByteOrder(TAO_BYTE_ORDER); }
    word32    getBlockSize()  const { return BLOCK_SIZE; }
    word32    getDigestSize() const { return DIGEST_SIZE; }
    word32    getPadSize()    const { return PAD_SIZE; }

    void Init();

    SHA512(const SHA512&);
    SHA512& operator= (const SHA512&);

    void Swap(SHA512&);
private:
    void Transform();
};


// SHA-384 digest
class SHA384 : public HASH64withTransform {
public:
    enum { BLOCK_SIZE = 128, DIGEST_SIZE = 48, PAD_SIZE = 112,
           TAO_BYTE_ORDER = BigEndianOrder};   // in Bytes
    SHA384() : HASH64withTransform(SHA512::DIGEST_SIZE/ sizeof(word64),
                                   BLOCK_SIZE)
                { Init(); }
    ByteOrder getByteOrder()  const { return ByteOrder(TAO_BYTE_ORDER); }
    word32    getBlockSize()  const { return BLOCK_SIZE; }
    word32    getDigestSize() const { return DIGEST_SIZE; }
    word32    getPadSize()    const { return PAD_SIZE; }

    void Init();

    SHA384(const SHA384&);
    SHA384& operator= (const SHA384&);

    void Swap(SHA384&);
private:
    void Transform();
};

#endif // WORD64_AVAILABLE


} // namespace


#endif // TAO_CRYPT_SHA_HPP

