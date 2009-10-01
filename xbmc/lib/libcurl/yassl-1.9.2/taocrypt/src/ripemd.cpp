/* ripemd.cpp                                
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


/* based on Wei Dai's ripemd.cpp from CryptoPP */

#include "runtime.hpp"
#include "ripemd.hpp"
#ifdef USE_SYS_STL
    #include <algorithm>
#else
    #include "algorithm.hpp"
#endif


namespace STL = STL_NAMESPACE;



namespace TaoCrypt {

void RIPEMD160::Init()
{
    digest_[0] = 0x67452301L;
    digest_[1] = 0xefcdab89L;
    digest_[2] = 0x98badcfeL;
    digest_[3] = 0x10325476L;
    digest_[4] = 0xc3d2e1f0L;

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}


RIPEMD160::RIPEMD160(const RIPEMD160& that)
    : HASHwithTransform(DIGEST_SIZE / sizeof(word32), BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_   = that.loLen_;
    hiLen_   = that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}


RIPEMD160& RIPEMD160::operator= (const RIPEMD160& that)
{
    RIPEMD160 tmp(that);
    Swap(tmp);

    return *this;
}


void RIPEMD160::Swap(RIPEMD160& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}


#ifdef DO_RIPEMD_ASM

// Update digest with data of size len
void RIPEMD160::Update(const byte* data, word32 len)
{
    if (!isMMX) {
        HASHwithTransform::Update(data, len);
        return;
    }

    byte* local = reinterpret_cast<byte*>(buffer_);

    // remove buffered data if possible
    if (buffLen_)  {   
        word32 add = min(len, BLOCK_SIZE - buffLen_);
        memcpy(&local[buffLen_], data, add);

        buffLen_ += add;
        data     += add;
        len      -= add;

        if (buffLen_ == BLOCK_SIZE) {
            Transform();
            AddLength(BLOCK_SIZE);
            buffLen_ = 0;
        }
    }

    // all at once for asm
    if (buffLen_ == 0) {
        word32 times = len / BLOCK_SIZE;
        if (times) {
            AsmTransform(data, times);
            const word32 add = BLOCK_SIZE * times;
            AddLength(add);
            len  -= add;
            data += add;
        }
    }

    // cache any data left
    if (len) {
        memcpy(&local[buffLen_], data, len);
        buffLen_ += len;
    }
}

#endif // DO_RIPEMD_ASM


// for all
#define F(x, y, z)    (x ^ y ^ z) 
#define G(x, y, z)    (z ^ (x & (y^z)))
#define H(x, y, z)    (z ^ (x | ~y))
#define I(x, y, z)    (y ^ (z & (x^y)))
#define J(x, y, z)    (x ^ (y | ~z))

#define k0 0
#define k1 0x5a827999
#define k2 0x6ed9eba1
#define k3 0x8f1bbcdc
#define k4 0xa953fd4e
#define k5 0x50a28be6
#define k6 0x5c4dd124
#define k7 0x6d703ef3
#define k8 0x7a6d76e9
#define k9 0

// for 160 and 320
#define Subround(f, a, b, c, d, e, x, s, k) \
    a += f(b, c, d) + x + k;\
    a = rotlFixed((word32)a, s) + e;\
    c = rotlFixed((word32)c, 10U)


void RIPEMD160::Transform()
{
    unsigned long a1, b1, c1, d1, e1, a2, b2, c2, d2, e2;
    a1 = a2 = digest_[0];
    b1 = b2 = digest_[1];
    c1 = c2 = digest_[2];
    d1 = d2 = digest_[3];
    e1 = e2 = digest_[4];

    Subround(F, a1, b1, c1, d1, e1, buffer_[ 0], 11, k0);
    Subround(F, e1, a1, b1, c1, d1, buffer_[ 1], 14, k0);
    Subround(F, d1, e1, a1, b1, c1, buffer_[ 2], 15, k0);
    Subround(F, c1, d1, e1, a1, b1, buffer_[ 3], 12, k0);
    Subround(F, b1, c1, d1, e1, a1, buffer_[ 4],  5, k0);
    Subround(F, a1, b1, c1, d1, e1, buffer_[ 5],  8, k0);
    Subround(F, e1, a1, b1, c1, d1, buffer_[ 6],  7, k0);
    Subround(F, d1, e1, a1, b1, c1, buffer_[ 7],  9, k0);
    Subround(F, c1, d1, e1, a1, b1, buffer_[ 8], 11, k0);
    Subround(F, b1, c1, d1, e1, a1, buffer_[ 9], 13, k0);
    Subround(F, a1, b1, c1, d1, e1, buffer_[10], 14, k0);
    Subround(F, e1, a1, b1, c1, d1, buffer_[11], 15, k0);
    Subround(F, d1, e1, a1, b1, c1, buffer_[12],  6, k0);
    Subround(F, c1, d1, e1, a1, b1, buffer_[13],  7, k0);
    Subround(F, b1, c1, d1, e1, a1, buffer_[14],  9, k0);
    Subround(F, a1, b1, c1, d1, e1, buffer_[15],  8, k0);

    Subround(G, e1, a1, b1, c1, d1, buffer_[ 7],  7, k1);
    Subround(G, d1, e1, a1, b1, c1, buffer_[ 4],  6, k1);
    Subround(G, c1, d1, e1, a1, b1, buffer_[13],  8, k1);
    Subround(G, b1, c1, d1, e1, a1, buffer_[ 1], 13, k1);
    Subround(G, a1, b1, c1, d1, e1, buffer_[10], 11, k1);
    Subround(G, e1, a1, b1, c1, d1, buffer_[ 6],  9, k1);
    Subround(G, d1, e1, a1, b1, c1, buffer_[15],  7, k1);
    Subround(G, c1, d1, e1, a1, b1, buffer_[ 3], 15, k1);
    Subround(G, b1, c1, d1, e1, a1, buffer_[12],  7, k1);
    Subround(G, a1, b1, c1, d1, e1, buffer_[ 0], 12, k1);
    Subround(G, e1, a1, b1, c1, d1, buffer_[ 9], 15, k1);
    Subround(G, d1, e1, a1, b1, c1, buffer_[ 5],  9, k1);
    Subround(G, c1, d1, e1, a1, b1, buffer_[ 2], 11, k1);
    Subround(G, b1, c1, d1, e1, a1, buffer_[14],  7, k1);
    Subround(G, a1, b1, c1, d1, e1, buffer_[11], 13, k1);
    Subround(G, e1, a1, b1, c1, d1, buffer_[ 8], 12, k1);

    Subround(H, d1, e1, a1, b1, c1, buffer_[ 3], 11, k2);
    Subround(H, c1, d1, e1, a1, b1, buffer_[10], 13, k2);
    Subround(H, b1, c1, d1, e1, a1, buffer_[14],  6, k2);
    Subround(H, a1, b1, c1, d1, e1, buffer_[ 4],  7, k2);
    Subround(H, e1, a1, b1, c1, d1, buffer_[ 9], 14, k2);
    Subround(H, d1, e1, a1, b1, c1, buffer_[15],  9, k2);
    Subround(H, c1, d1, e1, a1, b1, buffer_[ 8], 13, k2);
    Subround(H, b1, c1, d1, e1, a1, buffer_[ 1], 15, k2);
    Subround(H, a1, b1, c1, d1, e1, buffer_[ 2], 14, k2);
    Subround(H, e1, a1, b1, c1, d1, buffer_[ 7],  8, k2);
    Subround(H, d1, e1, a1, b1, c1, buffer_[ 0], 13, k2);
    Subround(H, c1, d1, e1, a1, b1, buffer_[ 6],  6, k2);
    Subround(H, b1, c1, d1, e1, a1, buffer_[13],  5, k2);
    Subround(H, a1, b1, c1, d1, e1, buffer_[11], 12, k2);
    Subround(H, e1, a1, b1, c1, d1, buffer_[ 5],  7, k2);
    Subround(H, d1, e1, a1, b1, c1, buffer_[12],  5, k2);

    Subround(I, c1, d1, e1, a1, b1, buffer_[ 1], 11, k3);
    Subround(I, b1, c1, d1, e1, a1, buffer_[ 9], 12, k3);
    Subround(I, a1, b1, c1, d1, e1, buffer_[11], 14, k3);
    Subround(I, e1, a1, b1, c1, d1, buffer_[10], 15, k3);
    Subround(I, d1, e1, a1, b1, c1, buffer_[ 0], 14, k3);
    Subround(I, c1, d1, e1, a1, b1, buffer_[ 8], 15, k3);
    Subround(I, b1, c1, d1, e1, a1, buffer_[12],  9, k3);
    Subround(I, a1, b1, c1, d1, e1, buffer_[ 4],  8, k3);
    Subround(I, e1, a1, b1, c1, d1, buffer_[13],  9, k3);
    Subround(I, d1, e1, a1, b1, c1, buffer_[ 3], 14, k3);
    Subround(I, c1, d1, e1, a1, b1, buffer_[ 7],  5, k3);
    Subround(I, b1, c1, d1, e1, a1, buffer_[15],  6, k3);
    Subround(I, a1, b1, c1, d1, e1, buffer_[14],  8, k3);
    Subround(I, e1, a1, b1, c1, d1, buffer_[ 5],  6, k3);
    Subround(I, d1, e1, a1, b1, c1, buffer_[ 6],  5, k3);
    Subround(I, c1, d1, e1, a1, b1, buffer_[ 2], 12, k3);

    Subround(J, b1, c1, d1, e1, a1, buffer_[ 4],  9, k4);
    Subround(J, a1, b1, c1, d1, e1, buffer_[ 0], 15, k4);
    Subround(J, e1, a1, b1, c1, d1, buffer_[ 5],  5, k4);
    Subround(J, d1, e1, a1, b1, c1, buffer_[ 9], 11, k4);
    Subround(J, c1, d1, e1, a1, b1, buffer_[ 7],  6, k4);
    Subround(J, b1, c1, d1, e1, a1, buffer_[12],  8, k4);
    Subround(J, a1, b1, c1, d1, e1, buffer_[ 2], 13, k4);
    Subround(J, e1, a1, b1, c1, d1, buffer_[10], 12, k4);
    Subround(J, d1, e1, a1, b1, c1, buffer_[14],  5, k4);
    Subround(J, c1, d1, e1, a1, b1, buffer_[ 1], 12, k4);
    Subround(J, b1, c1, d1, e1, a1, buffer_[ 3], 13, k4);
    Subround(J, a1, b1, c1, d1, e1, buffer_[ 8], 14, k4);
    Subround(J, e1, a1, b1, c1, d1, buffer_[11], 11, k4);
    Subround(J, d1, e1, a1, b1, c1, buffer_[ 6],  8, k4);
    Subround(J, c1, d1, e1, a1, b1, buffer_[15],  5, k4);
    Subround(J, b1, c1, d1, e1, a1, buffer_[13],  6, k4);

    Subround(J, a2, b2, c2, d2, e2, buffer_[ 5],  8, k5);
    Subround(J, e2, a2, b2, c2, d2, buffer_[14],  9, k5);
    Subround(J, d2, e2, a2, b2, c2, buffer_[ 7],  9, k5);
    Subround(J, c2, d2, e2, a2, b2, buffer_[ 0], 11, k5);
    Subround(J, b2, c2, d2, e2, a2, buffer_[ 9], 13, k5);
    Subround(J, a2, b2, c2, d2, e2, buffer_[ 2], 15, k5);
    Subround(J, e2, a2, b2, c2, d2, buffer_[11], 15, k5);
    Subround(J, d2, e2, a2, b2, c2, buffer_[ 4],  5, k5);
    Subround(J, c2, d2, e2, a2, b2, buffer_[13],  7, k5);
    Subround(J, b2, c2, d2, e2, a2, buffer_[ 6],  7, k5);
    Subround(J, a2, b2, c2, d2, e2, buffer_[15],  8, k5);
    Subround(J, e2, a2, b2, c2, d2, buffer_[ 8], 11, k5);
    Subround(J, d2, e2, a2, b2, c2, buffer_[ 1], 14, k5);
    Subround(J, c2, d2, e2, a2, b2, buffer_[10], 14, k5);
    Subround(J, b2, c2, d2, e2, a2, buffer_[ 3], 12, k5);
    Subround(J, a2, b2, c2, d2, e2, buffer_[12],  6, k5);

    Subround(I, e2, a2, b2, c2, d2, buffer_[ 6],  9, k6); 
    Subround(I, d2, e2, a2, b2, c2, buffer_[11], 13, k6);
    Subround(I, c2, d2, e2, a2, b2, buffer_[ 3], 15, k6);
    Subround(I, b2, c2, d2, e2, a2, buffer_[ 7],  7, k6);
    Subround(I, a2, b2, c2, d2, e2, buffer_[ 0], 12, k6);
    Subround(I, e2, a2, b2, c2, d2, buffer_[13],  8, k6);
    Subround(I, d2, e2, a2, b2, c2, buffer_[ 5],  9, k6);
    Subround(I, c2, d2, e2, a2, b2, buffer_[10], 11, k6);
    Subround(I, b2, c2, d2, e2, a2, buffer_[14],  7, k6);
    Subround(I, a2, b2, c2, d2, e2, buffer_[15],  7, k6);
    Subround(I, e2, a2, b2, c2, d2, buffer_[ 8], 12, k6);
    Subround(I, d2, e2, a2, b2, c2, buffer_[12],  7, k6);
    Subround(I, c2, d2, e2, a2, b2, buffer_[ 4],  6, k6);
    Subround(I, b2, c2, d2, e2, a2, buffer_[ 9], 15, k6);
    Subround(I, a2, b2, c2, d2, e2, buffer_[ 1], 13, k6);
    Subround(I, e2, a2, b2, c2, d2, buffer_[ 2], 11, k6);

    Subround(H, d2, e2, a2, b2, c2, buffer_[15],  9, k7);
    Subround(H, c2, d2, e2, a2, b2, buffer_[ 5],  7, k7);
    Subround(H, b2, c2, d2, e2, a2, buffer_[ 1], 15, k7);
    Subround(H, a2, b2, c2, d2, e2, buffer_[ 3], 11, k7);
    Subround(H, e2, a2, b2, c2, d2, buffer_[ 7],  8, k7);
    Subround(H, d2, e2, a2, b2, c2, buffer_[14],  6, k7);
    Subround(H, c2, d2, e2, a2, b2, buffer_[ 6],  6, k7);
    Subround(H, b2, c2, d2, e2, a2, buffer_[ 9], 14, k7);
    Subround(H, a2, b2, c2, d2, e2, buffer_[11], 12, k7);
    Subround(H, e2, a2, b2, c2, d2, buffer_[ 8], 13, k7);
    Subround(H, d2, e2, a2, b2, c2, buffer_[12],  5, k7);
    Subround(H, c2, d2, e2, a2, b2, buffer_[ 2], 14, k7);
    Subround(H, b2, c2, d2, e2, a2, buffer_[10], 13, k7);
    Subround(H, a2, b2, c2, d2, e2, buffer_[ 0], 13, k7);
    Subround(H, e2, a2, b2, c2, d2, buffer_[ 4],  7, k7);
    Subround(H, d2, e2, a2, b2, c2, buffer_[13],  5, k7);

    Subround(G, c2, d2, e2, a2, b2, buffer_[ 8], 15, k8);
    Subround(G, b2, c2, d2, e2, a2, buffer_[ 6],  5, k8);
    Subround(G, a2, b2, c2, d2, e2, buffer_[ 4],  8, k8);
    Subround(G, e2, a2, b2, c2, d2, buffer_[ 1], 11, k8);
    Subround(G, d2, e2, a2, b2, c2, buffer_[ 3], 14, k8);
    Subround(G, c2, d2, e2, a2, b2, buffer_[11], 14, k8);
    Subround(G, b2, c2, d2, e2, a2, buffer_[15],  6, k8);
    Subround(G, a2, b2, c2, d2, e2, buffer_[ 0], 14, k8);
    Subround(G, e2, a2, b2, c2, d2, buffer_[ 5],  6, k8);
    Subround(G, d2, e2, a2, b2, c2, buffer_[12],  9, k8);
    Subround(G, c2, d2, e2, a2, b2, buffer_[ 2], 12, k8);
    Subround(G, b2, c2, d2, e2, a2, buffer_[13],  9, k8);
    Subround(G, a2, b2, c2, d2, e2, buffer_[ 9], 12, k8);
    Subround(G, e2, a2, b2, c2, d2, buffer_[ 7],  5, k8);
    Subround(G, d2, e2, a2, b2, c2, buffer_[10], 15, k8);
    Subround(G, c2, d2, e2, a2, b2, buffer_[14],  8, k8);

    Subround(F, b2, c2, d2, e2, a2, buffer_[12],  8, k9);
    Subround(F, a2, b2, c2, d2, e2, buffer_[15],  5, k9);
    Subround(F, e2, a2, b2, c2, d2, buffer_[10], 12, k9);
    Subround(F, d2, e2, a2, b2, c2, buffer_[ 4],  9, k9);
    Subround(F, c2, d2, e2, a2, b2, buffer_[ 1], 12, k9);
    Subround(F, b2, c2, d2, e2, a2, buffer_[ 5],  5, k9);
    Subround(F, a2, b2, c2, d2, e2, buffer_[ 8], 14, k9);
    Subround(F, e2, a2, b2, c2, d2, buffer_[ 7],  6, k9);
    Subround(F, d2, e2, a2, b2, c2, buffer_[ 6],  8, k9);
    Subround(F, c2, d2, e2, a2, b2, buffer_[ 2], 13, k9);
    Subround(F, b2, c2, d2, e2, a2, buffer_[13],  6, k9);
    Subround(F, a2, b2, c2, d2, e2, buffer_[14],  5, k9);
    Subround(F, e2, a2, b2, c2, d2, buffer_[ 0], 15, k9);
    Subround(F, d2, e2, a2, b2, c2, buffer_[ 3], 13, k9);
    Subround(F, c2, d2, e2, a2, b2, buffer_[ 9], 11, k9);
    Subround(F, b2, c2, d2, e2, a2, buffer_[11], 11, k9);

    c1         = digest_[1] + c1 + d2;
    digest_[1] = digest_[2] + d1 + e2;
    digest_[2] = digest_[3] + e1 + a2;
    digest_[3] = digest_[4] + a1 + b2;
    digest_[4] = digest_[0] + b1 + c2;
    digest_[0] = c1;
}


#ifdef DO_RIPEMD_ASM

/*
    // F(x ^ y ^ z)
    // place in esi
#define ASMF(x, y, z)  \
    AS2(    mov   esi, x                )   \
    AS2(    xor   esi, y                )   \
    AS2(    xor   esi, z                )


    // G(z ^ (x & (y^z)))
    // place in esi
#define ASMG(x, y, z)  \
    AS2(    mov   esi, z                )   \
    AS2(    xor   esi, y                )   \
    AS2(    and   esi, x                )   \
    AS2(    xor   esi, z                )

    
    // H(z ^ (x | ~y))
    // place in esi
#define ASMH(x, y, z) \
    AS2(    mov   esi, y                )   \
    AS1(    not   esi                   )   \
    AS2(     or   esi, x                )   \
    AS2(    xor   esi, z                )


    // I(y ^ (z & (x^y)))
    // place in esi
#define ASMI(x, y, z)  \
    AS2(    mov   esi, y                )   \
    AS2(    xor   esi, x                )   \
    AS2(    and   esi, z                )   \
    AS2(    xor   esi, y                )


    // J(x ^ (y | ~z)))
    // place in esi
#define ASMJ(x, y, z)   \
    AS2(    mov   esi, z                )   \
    AS1(    not   esi                   )   \
    AS2(     or   esi, y                )   \
    AS2(    xor   esi, x                )


// for 160 and 320
// #define ASMSubround(f, a, b, c, d, e, i, s, k) 
//    a += f(b, c, d) + data[i] + k;
//    a = rotlFixed((word32)a, s) + e;
//    c = rotlFixed((word32)c, 10U)

#define ASMSubround(f, a, b, c, d, e, index, s, k) \
    // a += f(b, c, d) + data[i] + k                    \
    AS2(    mov   esp, [edi + index * 4]            )   \
    f(b, c, d)                                          \
    AS2(    add   esi, k                            )   \
    AS2(    add   esi, esp                          )   \
    AS2(    add     a, esi                          )   \
    // a = rotlFixed((word32)a, s) + e                  \
    AS2(    rol     a, s                            )   \
    AS2(    rol     c, 10                           )   \
    // c = rotlFixed((word32)c, 10U)                    \
    AS2(    add     a, e                            )
*/


// combine F into subround w/ setup
// esi already has c, setup for next round when done
// esp already has edi[index], setup for next round when done

#define ASMSubroundF(a, b, c, d, e, index, s) \
    /* a += (b ^ c ^ d) + data[i] + k  */               \
    AS2(    xor   esi, b                            )   \
    AS2(    add     a, [edi + index * 4]            )   \
    AS2(    xor   esi, d                            )   \
    AS2(    add     a, esi                          )   \
    /* a = rotlFixed((word32)a, s) + e */               \
    AS2(    mov   esi, b                            )   \
    AS2(    rol     a, s                            )   \
    /* c = rotlFixed((word32)c, 10U) */                 \
    AS2(    rol     c, 10                           )   \
    AS2(    add     a, e                            )


// combine G into subround w/ setup
// esi already has c, setup for next round when done
// esp already has edi[index], setup for next round when done

#define ASMSubroundG(a, b, c, d, e, index, s, k) \
    /* a += (d ^ (b & (c^d))) + data[i] + k  */         \
    AS2(    xor   esi, d                            )   \
    AS2(    and   esi, b                            )   \
    AS2(    add     a, [edi + index * 4]            )   \
    AS2(    xor   esi, d                            )   \
    AS2(    lea     a, [esi + a + k]                )   \
    /* a = rotlFixed((word32)a, s) + e */               \
    AS2(    mov   esi, b                            )   \
    AS2(    rol     a, s                            )   \
    /* c = rotlFixed((word32)c, 10U) */                 \
    AS2(    rol     c, 10                           )   \
    AS2(    add     a, e                            )


// combine H into subround w/ setup
// esi already has c, setup for next round when done
// esp already has edi[index], setup for next round when done

#define ASMSubroundH(a, b, c, d, e, index, s, k) \
    /* a += (d ^ (b | ~c)) + data[i] + k  */            \
    AS1(    not   esi                               )   \
    AS2(     or   esi, b                            )   \
    AS2(    add     a, [edi + index * 4]            )   \
    AS2(    xor   esi, d                            )   \
    AS2(    lea     a, [esi + a + k]                )   \
    /* a = rotlFixed((word32)a, s) + e */               \
    AS2(    mov   esi, b                            )   \
    AS2(    rol     a, s                            )   \
    /* c = rotlFixed((word32)c, 10U) */                 \
    AS2(    rol     c, 10                           )   \
    AS2(    add     a, e                            )


// combine I into subround w/ setup
// esi already has c, setup for next round when done
// esp already has edi[index], setup for next round when done

#define ASMSubroundI(a, b, c, d, e, index, s, k) \
    /* a += (c ^ (d & (b^c))) + data[i] + k  */         \
    AS2(    xor   esi, b                            )   \
    AS2(    and   esi, d                            )   \
    AS2(    add     a, [edi + index * 4]            )   \
    AS2(    xor   esi, c                            )   \
    AS2(    lea     a, [esi + a + k]                )   \
    /* a = rotlFixed((word32)a, s) + e */               \
    AS2(    mov   esi, b                            )   \
    AS2(    rol     a, s                            )   \
    /* c = rotlFixed((word32)c, 10U) */                 \
    AS2(    rol     c, 10                           )   \
    AS2(    add     a, e                            )


// combine J into subround w/ setup
// esi already has d, setup for next round when done
// esp already has edi[index], setup for next round when done

#define ASMSubroundJ(a, b, c, d, e, index, s, k) \
    /* a += (b ^ (c | ~d))) + data[i] + k  */           \
    AS1(    not   esi                               )   \
    AS2(     or   esi, c                            )   \
    /* c = rotlFixed((word32)c, 10U) */                 \
    AS2(    add     a, [edi + index * 4]            )   \
    AS2(    xor   esi, b                            )   \
    AS2(    rol     c, 10                           )   \
    AS2(    lea     a, [esi + a + k]                )   \
    /* a = rotlFixed((word32)a, s) + e */               \
    AS2(    rol     a, s                            )   \
    AS2(    mov   esi, c                            )   \
    AS2(    add     a, e                            )


#ifdef _MSC_VER
    __declspec(naked) 
#endif
void RIPEMD160::AsmTransform(const byte* data, word32 times)
{
#ifdef __GNUC__
    #define AS1(x)    asm(#x);
    #define AS2(x, y) asm(#x ", " #y);

    #define PROLOG()  \
        asm(".intel_syntax noprefix"); \
        AS2(    movd  mm3, edi                      )   \
        AS2(    movd  mm4, ebx                      )   \
        AS2(    movd  mm5, esi                      )   \
        AS2(    movd  mm6, ebp                      )   \
        AS2(    mov   ecx, DWORD PTR [ebp +  8]     )   \
        AS2(    mov   edi, DWORD PTR [ebp + 12]     )   \
        AS2(    mov   edx, DWORD PTR [ebp + 16]     )

    #define EPILOG()  \
        AS2(    movd  ebp, mm6                  )   \
        AS2(    movd  esi, mm5                  )   \
        AS2(    movd  ebx, mm4                  )   \
        AS2(    mov   esp, ebp                  )   \
        AS2(    movd  edi, mm3                  )   \
        AS1(    emms                            )   \
        asm(".att_syntax");
#else
    #define AS1(x)    __asm x
    #define AS2(x, y) __asm x, y

    #define PROLOG() \
        AS1(    push  ebp                       )   \
        AS2(    mov   ebp, esp                  )   \
        AS2(    movd  mm3, edi                  )   \
        AS2(    movd  mm4, ebx                  )   \
        AS2(    movd  mm5, esi                  )   \
        AS2(    movd  mm6, ebp                  )   \
        AS2(    mov   edi, DWORD PTR [ebp +  8] )   \
        AS2(    mov   edx, DWORD PTR [ebp + 12] )

    #define EPILOG() \
        AS2(    movd  ebp, mm6                  )   \
        AS2(    movd  esi, mm5                  )   \
        AS2(    movd  ebx, mm4                  )   \
        AS2(    movd  edi, mm3                  )   \
        AS2(    mov   esp, ebp                  )   \
        AS1(    pop   ebp                       )   \
        AS1(    emms                            )   \
        AS1(    ret   8                         )
        
#endif

    PROLOG()

    #ifdef OLD_GCC_OFFSET
        AS2(    lea   esi, [ecx + 20]               )   // digest_[0]
    #else
        AS2(    lea   esi, [ecx + 16]               )   // digest_[0]
    #endif

    AS2(    sub   esp, 24               )   // make room for tmp a1 - e1
    AS2(    movd  mm1, esi              )   // store digest_
    
AS1( loopStart: )

    AS2(    movd  mm2, edx              )   // store times_

    AS2(    mov   eax, [esi]            )   // a1
    AS2(    mov   ebx, [esi +  4]       )   // b1
    AS2(    mov   ecx, [esi +  8]       )   // c1
    AS2(    mov   edx, [esi + 12]       )   // d1
    AS2(    mov   ebp, [esi + 16]       )   // e1

    // setup 
    AS2(    mov   esi, ecx      )

    ASMSubroundF( eax, ebx, ecx, edx, ebp,  0, 11)
    ASMSubroundF( ebp, eax, ebx, ecx, edx,  1, 14)
    ASMSubroundF( edx, ebp, eax, ebx, ecx,  2, 15)
    ASMSubroundF( ecx, edx, ebp, eax, ebx,  3, 12)
    ASMSubroundF( ebx, ecx, edx, ebp, eax,  4,  5)
    ASMSubroundF( eax, ebx, ecx, edx, ebp,  5,  8)
    ASMSubroundF( ebp, eax, ebx, ecx, edx,  6,  7)
    ASMSubroundF( edx, ebp, eax, ebx, ecx,  7,  9)
    ASMSubroundF( ecx, edx, ebp, eax, ebx,  8, 11)
    ASMSubroundF( ebx, ecx, edx, ebp, eax,  9, 13)
    ASMSubroundF( eax, ebx, ecx, edx, ebp, 10, 14)
    ASMSubroundF( ebp, eax, ebx, ecx, edx, 11, 15)
    ASMSubroundF( edx, ebp, eax, ebx, ecx, 12,  6)
    ASMSubroundF( ecx, edx, ebp, eax, ebx, 13,  7)
    ASMSubroundF( ebx, ecx, edx, ebp, eax, 14,  9)
    ASMSubroundF( eax, ebx, ecx, edx, ebp, 15,  8)

    ASMSubroundG( ebp, eax, ebx, ecx, edx,  7,  7, k1)
    ASMSubroundG( edx, ebp, eax, ebx, ecx,  4,  6, k1)
    ASMSubroundG( ecx, edx, ebp, eax, ebx, 13,  8, k1)
    ASMSubroundG( ebx, ecx, edx, ebp, eax,  1, 13, k1)
    ASMSubroundG( eax, ebx, ecx, edx, ebp, 10, 11, k1)
    ASMSubroundG( ebp, eax, ebx, ecx, edx,  6,  9, k1)
    ASMSubroundG( edx, ebp, eax, ebx, ecx, 15,  7, k1)
    ASMSubroundG( ecx, edx, ebp, eax, ebx,  3, 15, k1)
    ASMSubroundG( ebx, ecx, edx, ebp, eax, 12,  7, k1)
    ASMSubroundG( eax, ebx, ecx, edx, ebp,  0, 12, k1)
    ASMSubroundG( ebp, eax, ebx, ecx, edx,  9, 15, k1)
    ASMSubroundG( edx, ebp, eax, ebx, ecx,  5,  9, k1)
    ASMSubroundG( ecx, edx, ebp, eax, ebx,  2, 11, k1)
    ASMSubroundG( ebx, ecx, edx, ebp, eax, 14,  7, k1)
    ASMSubroundG( eax, ebx, ecx, edx, ebp, 11, 13, k1)
    ASMSubroundG( ebp, eax, ebx, ecx, edx,  8, 12, k1)

    ASMSubroundH( edx, ebp, eax, ebx, ecx,  3, 11, k2)
    ASMSubroundH( ecx, edx, ebp, eax, ebx, 10, 13, k2)
    ASMSubroundH( ebx, ecx, edx, ebp, eax, 14,  6, k2)
    ASMSubroundH( eax, ebx, ecx, edx, ebp,  4,  7, k2)
    ASMSubroundH( ebp, eax, ebx, ecx, edx,  9, 14, k2)
    ASMSubroundH( edx, ebp, eax, ebx, ecx, 15,  9, k2)
    ASMSubroundH( ecx, edx, ebp, eax, ebx,  8, 13, k2)
    ASMSubroundH( ebx, ecx, edx, ebp, eax,  1, 15, k2)
    ASMSubroundH( eax, ebx, ecx, edx, ebp,  2, 14, k2)
    ASMSubroundH( ebp, eax, ebx, ecx, edx,  7,  8, k2)
    ASMSubroundH( edx, ebp, eax, ebx, ecx,  0, 13, k2)
    ASMSubroundH( ecx, edx, ebp, eax, ebx,  6,  6, k2)
    ASMSubroundH( ebx, ecx, edx, ebp, eax, 13,  5, k2)
    ASMSubroundH( eax, ebx, ecx, edx, ebp, 11, 12, k2)
    ASMSubroundH( ebp, eax, ebx, ecx, edx,  5,  7, k2)
    ASMSubroundH( edx, ebp, eax, ebx, ecx, 12,  5, k2)

    ASMSubroundI( ecx, edx, ebp, eax, ebx,  1, 11, k3)
    ASMSubroundI( ebx, ecx, edx, ebp, eax,  9, 12, k3)
    ASMSubroundI( eax, ebx, ecx, edx, ebp, 11, 14, k3)
    ASMSubroundI( ebp, eax, ebx, ecx, edx, 10, 15, k3)
    ASMSubroundI( edx, ebp, eax, ebx, ecx,  0, 14, k3)
    ASMSubroundI( ecx, edx, ebp, eax, ebx,  8, 15, k3)
    ASMSubroundI( ebx, ecx, edx, ebp, eax, 12,  9, k3)
    ASMSubroundI( eax, ebx, ecx, edx, ebp,  4,  8, k3)
    ASMSubroundI( ebp, eax, ebx, ecx, edx, 13,  9, k3)
    ASMSubroundI( edx, ebp, eax, ebx, ecx,  3, 14, k3)
    ASMSubroundI( ecx, edx, ebp, eax, ebx,  7,  5, k3)
    ASMSubroundI( ebx, ecx, edx, ebp, eax, 15,  6, k3)
    ASMSubroundI( eax, ebx, ecx, edx, ebp, 14,  8, k3)
    ASMSubroundI( ebp, eax, ebx, ecx, edx,  5,  6, k3)
    ASMSubroundI( edx, ebp, eax, ebx, ecx,  6,  5, k3)
    ASMSubroundI( ecx, edx, ebp, eax, ebx,  2, 12, k3)

    // setup
    AS2(    mov   esi, ebp      )

    ASMSubroundJ( ebx, ecx, edx, ebp, eax,  4,  9, k4)
    ASMSubroundJ( eax, ebx, ecx, edx, ebp,  0, 15, k4)
    ASMSubroundJ( ebp, eax, ebx, ecx, edx,  5,  5, k4)
    ASMSubroundJ( edx, ebp, eax, ebx, ecx,  9, 11, k4)
    ASMSubroundJ( ecx, edx, ebp, eax, ebx,  7,  6, k4)
    ASMSubroundJ( ebx, ecx, edx, ebp, eax, 12,  8, k4)
    ASMSubroundJ( eax, ebx, ecx, edx, ebp,  2, 13, k4)
    ASMSubroundJ( ebp, eax, ebx, ecx, edx, 10, 12, k4)
    ASMSubroundJ( edx, ebp, eax, ebx, ecx, 14,  5, k4)
    ASMSubroundJ( ecx, edx, ebp, eax, ebx,  1, 12, k4)
    ASMSubroundJ( ebx, ecx, edx, ebp, eax,  3, 13, k4)
    ASMSubroundJ( eax, ebx, ecx, edx, ebp,  8, 14, k4)
    ASMSubroundJ( ebp, eax, ebx, ecx, edx, 11, 11, k4)
    ASMSubroundJ( edx, ebp, eax, ebx, ecx,  6,  8, k4)
    ASMSubroundJ( ecx, edx, ebp, eax, ebx, 15,  5, k4)
    ASMSubroundJ( ebx, ecx, edx, ebp, eax, 13,  6, k4)

    // store a1 - e1 on stack
    AS2(    movd  esi, mm1              )   // digest_

    AS2(    mov   [esp],      eax       )
    AS2(    mov   [esp +  4], ebx       )
    AS2(    mov   [esp +  8], ecx       )
    AS2(    mov   [esp + 12], edx       )
    AS2(    mov   [esp + 16], ebp       )

    AS2(    mov   eax, [esi]            )   // a2
    AS2(    mov   ebx, [esi +  4]       )   // b2
    AS2(    mov   ecx, [esi +  8]       )   // c2
    AS2(    mov   edx, [esi + 12]       )   // d2
    AS2(    mov   ebp, [esi + 16]       )   // e2


    // setup
    AS2(    mov   esi, edx      )

    ASMSubroundJ( eax, ebx, ecx, edx, ebp,  5,  8, k5)
    ASMSubroundJ( ebp, eax, ebx, ecx, edx, 14,  9, k5)
    ASMSubroundJ( edx, ebp, eax, ebx, ecx,  7,  9, k5)
    ASMSubroundJ( ecx, edx, ebp, eax, ebx,  0, 11, k5)
    ASMSubroundJ( ebx, ecx, edx, ebp, eax,  9, 13, k5)
    ASMSubroundJ( eax, ebx, ecx, edx, ebp,  2, 15, k5)
    ASMSubroundJ( ebp, eax, ebx, ecx, edx, 11, 15, k5)
    ASMSubroundJ( edx, ebp, eax, ebx, ecx,  4,  5, k5)
    ASMSubroundJ( ecx, edx, ebp, eax, ebx, 13,  7, k5)
    ASMSubroundJ( ebx, ecx, edx, ebp, eax,  6,  7, k5)
    ASMSubroundJ( eax, ebx, ecx, edx, ebp, 15,  8, k5)
    ASMSubroundJ( ebp, eax, ebx, ecx, edx,  8, 11, k5)
    ASMSubroundJ( edx, ebp, eax, ebx, ecx,  1, 14, k5)
    ASMSubroundJ( ecx, edx, ebp, eax, ebx, 10, 14, k5)
    ASMSubroundJ( ebx, ecx, edx, ebp, eax,  3, 12, k5)
    ASMSubroundJ( eax, ebx, ecx, edx, ebp, 12,  6, k5)

    // setup
    AS2(    mov   esi, ebx      )

    ASMSubroundI( ebp, eax, ebx, ecx, edx,  6,  9, k6) 
    ASMSubroundI( edx, ebp, eax, ebx, ecx, 11, 13, k6)
    ASMSubroundI( ecx, edx, ebp, eax, ebx,  3, 15, k6)
    ASMSubroundI( ebx, ecx, edx, ebp, eax,  7,  7, k6)
    ASMSubroundI( eax, ebx, ecx, edx, ebp,  0, 12, k6)
    ASMSubroundI( ebp, eax, ebx, ecx, edx, 13,  8, k6)
    ASMSubroundI( edx, ebp, eax, ebx, ecx,  5,  9, k6)
    ASMSubroundI( ecx, edx, ebp, eax, ebx, 10, 11, k6)
    ASMSubroundI( ebx, ecx, edx, ebp, eax, 14,  7, k6)
    ASMSubroundI( eax, ebx, ecx, edx, ebp, 15,  7, k6)
    ASMSubroundI( ebp, eax, ebx, ecx, edx,  8, 12, k6)
    ASMSubroundI( edx, ebp, eax, ebx, ecx, 12,  7, k6)
    ASMSubroundI( ecx, edx, ebp, eax, ebx,  4,  6, k6)
    ASMSubroundI( ebx, ecx, edx, ebp, eax,  9, 15, k6)
    ASMSubroundI( eax, ebx, ecx, edx, ebp,  1, 13, k6)
    ASMSubroundI( ebp, eax, ebx, ecx, edx,  2, 11, k6)

    ASMSubroundH( edx, ebp, eax, ebx, ecx, 15,  9, k7)
    ASMSubroundH( ecx, edx, ebp, eax, ebx,  5,  7, k7)
    ASMSubroundH( ebx, ecx, edx, ebp, eax,  1, 15, k7)
    ASMSubroundH( eax, ebx, ecx, edx, ebp,  3, 11, k7)
    ASMSubroundH( ebp, eax, ebx, ecx, edx,  7,  8, k7)
    ASMSubroundH( edx, ebp, eax, ebx, ecx, 14,  6, k7)
    ASMSubroundH( ecx, edx, ebp, eax, ebx,  6,  6, k7)
    ASMSubroundH( ebx, ecx, edx, ebp, eax,  9, 14, k7)
    ASMSubroundH( eax, ebx, ecx, edx, ebp, 11, 12, k7)
    ASMSubroundH( ebp, eax, ebx, ecx, edx,  8, 13, k7)
    ASMSubroundH( edx, ebp, eax, ebx, ecx, 12,  5, k7)
    ASMSubroundH( ecx, edx, ebp, eax, ebx,  2, 14, k7)
    ASMSubroundH( ebx, ecx, edx, ebp, eax, 10, 13, k7)
    ASMSubroundH( eax, ebx, ecx, edx, ebp,  0, 13, k7)
    ASMSubroundH( ebp, eax, ebx, ecx, edx,  4,  7, k7)
    ASMSubroundH( edx, ebp, eax, ebx, ecx, 13,  5, k7)

    ASMSubroundG( ecx, edx, ebp, eax, ebx,  8, 15, k8)
    ASMSubroundG( ebx, ecx, edx, ebp, eax,  6,  5, k8)
    ASMSubroundG( eax, ebx, ecx, edx, ebp,  4,  8, k8)
    ASMSubroundG( ebp, eax, ebx, ecx, edx,  1, 11, k8)
    ASMSubroundG( edx, ebp, eax, ebx, ecx,  3, 14, k8)
    ASMSubroundG( ecx, edx, ebp, eax, ebx, 11, 14, k8)
    ASMSubroundG( ebx, ecx, edx, ebp, eax, 15,  6, k8)
    ASMSubroundG( eax, ebx, ecx, edx, ebp,  0, 14, k8)
    ASMSubroundG( ebp, eax, ebx, ecx, edx,  5,  6, k8)
    ASMSubroundG( edx, ebp, eax, ebx, ecx, 12,  9, k8)
    ASMSubroundG( ecx, edx, ebp, eax, ebx,  2, 12, k8)
    ASMSubroundG( ebx, ecx, edx, ebp, eax, 13,  9, k8)
    ASMSubroundG( eax, ebx, ecx, edx, ebp,  9, 12, k8)
    ASMSubroundG( ebp, eax, ebx, ecx, edx,  7,  5, k8)
    ASMSubroundG( edx, ebp, eax, ebx, ecx, 10, 15, k8)
    ASMSubroundG( ecx, edx, ebp, eax, ebx, 14,  8, k8)

    ASMSubroundF( ebx, ecx, edx, ebp, eax, 12,  8)
    ASMSubroundF( eax, ebx, ecx, edx, ebp, 15,  5)
    ASMSubroundF( ebp, eax, ebx, ecx, edx, 10, 12)
    ASMSubroundF( edx, ebp, eax, ebx, ecx,  4,  9)
    ASMSubroundF( ecx, edx, ebp, eax, ebx,  1, 12)
    ASMSubroundF( ebx, ecx, edx, ebp, eax,  5,  5)
    ASMSubroundF( eax, ebx, ecx, edx, ebp,  8, 14)
    ASMSubroundF( ebp, eax, ebx, ecx, edx,  7,  6)
    ASMSubroundF( edx, ebp, eax, ebx, ecx,  6,  8)
    ASMSubroundF( ecx, edx, ebp, eax, ebx,  2, 13)
    ASMSubroundF( ebx, ecx, edx, ebp, eax, 13,  6)
    ASMSubroundF( eax, ebx, ecx, edx, ebp, 14,  5)
    ASMSubroundF( ebp, eax, ebx, ecx, edx,  0, 15)
    ASMSubroundF( edx, ebp, eax, ebx, ecx,  3, 13)
    ASMSubroundF( ecx, edx, ebp, eax, ebx,  9, 11)
    ASMSubroundF( ebx, ecx, edx, ebp, eax, 11, 11)

    // advance data and store for next round
    AS2(    add   edi, 64                       )
    AS2(    movd  esi, mm1                      )   // digest_
    AS2(    movd  mm0, edi                      )   // store

    // now edi as tmp

    // c1         = digest_[1] + c1 + d2;
    AS2(    add   [esp +  8], edx               )   // + d2
    AS2(    mov   edi, [esi + 4]                )   // digest_[1]
    AS2(    add   [esp +  8], edi               )

    // digest_[1] = digest_[2] + d1 + e2;
    AS2(    mov   [esi + 4], ebp                )   // e2
    AS2(    mov   edi, [esp + 12]               )   // d1
    AS2(    add   edi, [esi + 8]                )   // digest_[2]
    AS2(    add   [esi + 4], edi                )

    // digest_[2] = digest_[3] + e1 + a2;
    AS2(    mov   [esi + 8], eax                )   // a2
    AS2(    mov   edi, [esp + 16]               )   // e1
    AS2(    add   edi, [esi + 12]               )   // digest_[3]
    AS2(    add   [esi + 8], edi                )

    // digest_[3] = digest_[4] + a1 + b2;
    AS2(    mov   [esi + 12], ebx               )   // b2
    AS2(    mov   edi, [esp]                    )   // a1
    AS2(    add   edi, [esi + 16]               )   // digest_[4]
    AS2(    add   [esi + 12], edi               )

    // digest_[4] = digest_[0] + b1 + c2;
    AS2(    mov   [esi + 16], ecx               )   // c2
    AS2(    mov   edi, [esp +  4]               )   // b1
    AS2(    add   edi, [esi]                    )   // digest_[0]
    AS2(    add   [esi + 16], edi               )

    // digest_[0] = c1;
    AS2(    mov   edi, [esp +  8]               )   // c1
    AS2(    mov   [esi], edi                    )

    // setup for loop back
    AS2(    movd  edx, mm2              )   // times
    AS2(    movd  edi, mm0              )   // data, already advanced
    AS1(    dec   edx                   )
    AS1(    jnz   loopStart             )


    EPILOG()
}


#endif // DO_RIPEMD_ASM


} // namespace TaoCrypt
