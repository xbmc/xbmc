/* sha.cpp                                
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

/* based on Wei Dai's sha.cpp from CryptoPP */

#include "runtime.hpp"
#include <string.h>
#include "sha.hpp"
#ifdef USE_SYS_STL
    #include <algorithm>
#else
    #include "algorithm.hpp"
#endif


namespace STL = STL_NAMESPACE;



namespace TaoCrypt {

#define blk0(i) (W[i] = buffer_[i])
#define blk1(i) (W[i&15] = \
                 rotlFixed(W[(i+13)&15]^W[(i+8)&15]^W[(i+2)&15]^W[i&15],1))

#define f1(x,y,z) (z^(x &(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)

// (R0+R1), R2, R3, R4 are the different operations used in SHA1
#define R0(v,w,x,y,z,i) z+= f1(w,x,y) + blk0(i) + 0x5A827999+ \
                        rotlFixed(v,5); w = rotlFixed(w,30);
#define R1(v,w,x,y,z,i) z+= f1(w,x,y) + blk1(i) + 0x5A827999+ \
                        rotlFixed(v,5); w = rotlFixed(w,30);
#define R2(v,w,x,y,z,i) z+= f2(w,x,y) + blk1(i) + 0x6ED9EBA1+ \
                        rotlFixed(v,5); w = rotlFixed(w,30);
#define R3(v,w,x,y,z,i) z+= f3(w,x,y) + blk1(i) + 0x8F1BBCDC+ \
                        rotlFixed(v,5); w = rotlFixed(w,30);
#define R4(v,w,x,y,z,i) z+= f4(w,x,y) + blk1(i) + 0xCA62C1D6+ \
                        rotlFixed(v,5); w = rotlFixed(w,30);


void SHA::Init()
{
    digest_[0] = 0x67452301L;
    digest_[1] = 0xEFCDAB89L;
    digest_[2] = 0x98BADCFEL;
    digest_[3] = 0x10325476L;
    digest_[4] = 0xC3D2E1F0L;

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}

void SHA256::Init()
{
    digest_[0] = 0x6A09E667L;
    digest_[1] = 0xBB67AE85L;
    digest_[2] = 0x3C6EF372L;
    digest_[3] = 0xA54FF53AL;
    digest_[4] = 0x510E527FL;
    digest_[5] = 0x9B05688CL;
    digest_[6] = 0x1F83D9ABL;
    digest_[7] = 0x5BE0CD19L;

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}


void SHA224::Init()
{
    digest_[0] = 0xc1059ed8;
    digest_[1] = 0x367cd507;
    digest_[2] = 0x3070dd17;
    digest_[3] = 0xf70e5939;
    digest_[4] = 0xffc00b31;
    digest_[5] = 0x68581511;
    digest_[6] = 0x64f98fa7;
    digest_[7] = 0xbefa4fa4;

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}


#ifdef WORD64_AVAILABLE

void SHA512::Init()
{
    digest_[0] = W64LIT(0x6a09e667f3bcc908);
    digest_[1] = W64LIT(0xbb67ae8584caa73b);
    digest_[2] = W64LIT(0x3c6ef372fe94f82b);
    digest_[3] = W64LIT(0xa54ff53a5f1d36f1);
    digest_[4] = W64LIT(0x510e527fade682d1);
    digest_[5] = W64LIT(0x9b05688c2b3e6c1f);
    digest_[6] = W64LIT(0x1f83d9abfb41bd6b);
    digest_[7] = W64LIT(0x5be0cd19137e2179);

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}


void SHA384::Init()
{
    digest_[0] = W64LIT(0xcbbb9d5dc1059ed8);
    digest_[1] = W64LIT(0x629a292a367cd507);
    digest_[2] = W64LIT(0x9159015a3070dd17);
    digest_[3] = W64LIT(0x152fecd8f70e5939);
    digest_[4] = W64LIT(0x67332667ffc00b31);
    digest_[5] = W64LIT(0x8eb44a8768581511);
    digest_[6] = W64LIT(0xdb0c2e0d64f98fa7);
    digest_[7] = W64LIT(0x47b5481dbefa4fa4);

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}

#endif // WORD64_AVAILABLE


SHA::SHA(const SHA& that) : HASHwithTransform(DIGEST_SIZE / sizeof(word32),
                                              BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_   = that.loLen_;
    hiLen_   = that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}


SHA256::SHA256(const SHA256& that) : HASHwithTransform(DIGEST_SIZE /
                                       sizeof(word32), BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_   = that.loLen_;
    hiLen_   = that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}


SHA224::SHA224(const SHA224& that) : HASHwithTransform(SHA256::DIGEST_SIZE /
                                       sizeof(word32), BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_   = that.loLen_;
    hiLen_   = that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}


#ifdef WORD64_AVAILABLE 

SHA512::SHA512(const SHA512& that) : HASH64withTransform(DIGEST_SIZE /
                                       sizeof(word64), BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_   = that.loLen_;
    hiLen_   = that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}


SHA384::SHA384(const SHA384& that) : HASH64withTransform(SHA512::DIGEST_SIZE /
                                       sizeof(word64), BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_   = that.loLen_;
    hiLen_   = that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}

#endif // WORD64_AVAILABLE


SHA& SHA::operator= (const SHA& that)
{
    SHA tmp(that);
    Swap(tmp);

    return *this;
}


SHA256& SHA256::operator= (const SHA256& that)
{
    SHA256 tmp(that);
    Swap(tmp);

    return *this;
}


SHA224& SHA224::operator= (const SHA224& that)
{
    SHA224 tmp(that);
    Swap(tmp);

    return *this;
}


#ifdef WORD64_AVAILABLE

SHA512& SHA512::operator= (const SHA512& that)
{
    SHA512 tmp(that);
    Swap(tmp);

    return *this;
}


SHA384& SHA384::operator= (const SHA384& that)
{
    SHA384 tmp(that);
    Swap(tmp);

    return *this;
}

#endif // WORD64_AVAILABLE


void SHA::Swap(SHA& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}


void SHA256::Swap(SHA256& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}


void SHA224::Swap(SHA224& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}


#ifdef WORD64_AVAILABLE

void SHA512::Swap(SHA512& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}


void SHA384::Swap(SHA384& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}

#endif // WORD64_AVIALABLE


#ifdef DO_SHA_ASM

// Update digest with data of size len
void SHA::Update(const byte* data, word32 len)
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
            ByteReverse(local, local, BLOCK_SIZE);
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

#endif // DO_SHA_ASM


void SHA::Transform()
{
    word32 W[BLOCK_SIZE / sizeof(word32)];

    // Copy context->state[] to working vars 
    word32 a = digest_[0];
    word32 b = digest_[1];
    word32 c = digest_[2];
    word32 d = digest_[3];
    word32 e = digest_[4];

    // 4 rounds of 20 operations each. Loop unrolled. 
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);

    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);

    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);

    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);

    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

    // Add the working vars back into digest state[]
    digest_[0] += a;
    digest_[1] += b;
    digest_[2] += c;
    digest_[3] += d;
    digest_[4] += e;

    // Wipe variables
    a = b = c = d = e = 0;
    memset(W, 0, sizeof(W));
}


#define blk2(i) (W[i&15]+=s1(W[(i-2)&15])+W[(i-7)&15]+s0(W[(i-15)&15]))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) ((x&y)|(z&(x|y)))

#define a(i) T[(0-i)&7]
#define b(i) T[(1-i)&7]
#define c(i) T[(2-i)&7]
#define d(i) T[(3-i)&7]
#define e(i) T[(4-i)&7]
#define f(i) T[(5-i)&7]
#define g(i) T[(6-i)&7]
#define h(i) T[(7-i)&7]

#define R(i) h(i)+=S1(e(i))+Ch(e(i),f(i),g(i))+K[i+j]+(j?blk2(i):blk0(i));\
	d(i)+=h(i);h(i)+=S0(a(i))+Maj(a(i),b(i),c(i))

// for SHA256
#define S0(x) (rotrFixed(x,2)^rotrFixed(x,13)^rotrFixed(x,22))
#define S1(x) (rotrFixed(x,6)^rotrFixed(x,11)^rotrFixed(x,25))
#define s0(x) (rotrFixed(x,7)^rotrFixed(x,18)^(x>>3))
#define s1(x) (rotrFixed(x,17)^rotrFixed(x,19)^(x>>10))


static const word32 K256[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


static void Transform256(word32* digest_, word32* buffer_)
{
    const  word32* K = K256;

    word32 W[16];
    word32 T[8];

    // Copy digest to working vars
    memcpy(T, digest_, sizeof(T));

    // 64 operations, partially loop unrolled
    for (unsigned int j = 0; j < 64; j += 16) {
        R( 0); R( 1); R( 2); R( 3);
        R( 4); R( 5); R( 6); R( 7);
        R( 8); R( 9); R(10); R(11);
        R(12); R(13); R(14); R(15);
    }

    // Add the working vars back into digest
    digest_[0] += a(0);
    digest_[1] += b(0);
    digest_[2] += c(0);
    digest_[3] += d(0);
    digest_[4] += e(0);
    digest_[5] += f(0);
    digest_[6] += g(0);
    digest_[7] += h(0);

    // Wipe variables
    memset(W, 0, sizeof(W));
    memset(T, 0, sizeof(T));
}


// undef for 256
#undef S0
#undef S1
#undef s0
#undef s1


void SHA256::Transform()
{
    Transform256(digest_, buffer_);
}


void SHA224::Transform()
{
    Transform256(digest_, buffer_);
}


#ifdef WORD64_AVAILABLE

static const word64 K512[80] = {
	W64LIT(0x428a2f98d728ae22), W64LIT(0x7137449123ef65cd),
	W64LIT(0xb5c0fbcfec4d3b2f), W64LIT(0xe9b5dba58189dbbc),
	W64LIT(0x3956c25bf348b538), W64LIT(0x59f111f1b605d019),
	W64LIT(0x923f82a4af194f9b), W64LIT(0xab1c5ed5da6d8118),
	W64LIT(0xd807aa98a3030242), W64LIT(0x12835b0145706fbe),
	W64LIT(0x243185be4ee4b28c), W64LIT(0x550c7dc3d5ffb4e2),
	W64LIT(0x72be5d74f27b896f), W64LIT(0x80deb1fe3b1696b1),
	W64LIT(0x9bdc06a725c71235), W64LIT(0xc19bf174cf692694),
	W64LIT(0xe49b69c19ef14ad2), W64LIT(0xefbe4786384f25e3),
	W64LIT(0x0fc19dc68b8cd5b5), W64LIT(0x240ca1cc77ac9c65),
	W64LIT(0x2de92c6f592b0275), W64LIT(0x4a7484aa6ea6e483),
	W64LIT(0x5cb0a9dcbd41fbd4), W64LIT(0x76f988da831153b5),
	W64LIT(0x983e5152ee66dfab), W64LIT(0xa831c66d2db43210),
	W64LIT(0xb00327c898fb213f), W64LIT(0xbf597fc7beef0ee4),
	W64LIT(0xc6e00bf33da88fc2), W64LIT(0xd5a79147930aa725),
	W64LIT(0x06ca6351e003826f), W64LIT(0x142929670a0e6e70),
	W64LIT(0x27b70a8546d22ffc), W64LIT(0x2e1b21385c26c926),
	W64LIT(0x4d2c6dfc5ac42aed), W64LIT(0x53380d139d95b3df),
	W64LIT(0x650a73548baf63de), W64LIT(0x766a0abb3c77b2a8),
	W64LIT(0x81c2c92e47edaee6), W64LIT(0x92722c851482353b),
	W64LIT(0xa2bfe8a14cf10364), W64LIT(0xa81a664bbc423001),
	W64LIT(0xc24b8b70d0f89791), W64LIT(0xc76c51a30654be30),
	W64LIT(0xd192e819d6ef5218), W64LIT(0xd69906245565a910),
	W64LIT(0xf40e35855771202a), W64LIT(0x106aa07032bbd1b8),
	W64LIT(0x19a4c116b8d2d0c8), W64LIT(0x1e376c085141ab53),
	W64LIT(0x2748774cdf8eeb99), W64LIT(0x34b0bcb5e19b48a8),
	W64LIT(0x391c0cb3c5c95a63), W64LIT(0x4ed8aa4ae3418acb),
	W64LIT(0x5b9cca4f7763e373), W64LIT(0x682e6ff3d6b2b8a3),
	W64LIT(0x748f82ee5defb2fc), W64LIT(0x78a5636f43172f60),
	W64LIT(0x84c87814a1f0ab72), W64LIT(0x8cc702081a6439ec),
	W64LIT(0x90befffa23631e28), W64LIT(0xa4506cebde82bde9),
	W64LIT(0xbef9a3f7b2c67915), W64LIT(0xc67178f2e372532b),
	W64LIT(0xca273eceea26619c), W64LIT(0xd186b8c721c0c207),
	W64LIT(0xeada7dd6cde0eb1e), W64LIT(0xf57d4f7fee6ed178),
	W64LIT(0x06f067aa72176fba), W64LIT(0x0a637dc5a2c898a6),
	W64LIT(0x113f9804bef90dae), W64LIT(0x1b710b35131c471b),
	W64LIT(0x28db77f523047d84), W64LIT(0x32caab7b40c72493),
	W64LIT(0x3c9ebe0a15c9bebc), W64LIT(0x431d67c49c100d4c),
	W64LIT(0x4cc5d4becb3e42b6), W64LIT(0x597f299cfc657e2a),
	W64LIT(0x5fcb6fab3ad6faec), W64LIT(0x6c44198c4a475817)
};


// for SHA512
#define S0(x) (rotrFixed(x,28)^rotrFixed(x,34)^rotrFixed(x,39))
#define S1(x) (rotrFixed(x,14)^rotrFixed(x,18)^rotrFixed(x,41))
#define s0(x) (rotrFixed(x,1)^rotrFixed(x,8)^(x>>7))
#define s1(x) (rotrFixed(x,19)^rotrFixed(x,61)^(x>>6))


static void Transform512(word64* digest_, word64* buffer_)
{
    const word64* K = K512;

    word64 W[16];
    word64 T[8];

    // Copy digest to working vars
    memcpy(T, digest_, sizeof(T));

    // 64 operations, partially loop unrolled
    for (unsigned int j = 0; j < 80; j += 16) {
        R( 0); R( 1); R( 2); R( 3);
        R( 4); R( 5); R( 6); R( 7);
        R( 8); R( 9); R(10); R(11);
        R(12); R(13); R(14); R(15);
    }

    // Add the working vars back into digest 

    digest_[0] += a(0);
    digest_[1] += b(0);
    digest_[2] += c(0);
    digest_[3] += d(0);
    digest_[4] += e(0);
    digest_[5] += f(0);
    digest_[6] += g(0);
    digest_[7] += h(0);

    // Wipe variables
    memset(W, 0, sizeof(W));
    memset(T, 0, sizeof(T));
}


void SHA512::Transform()
{
    Transform512(digest_, buffer_);
}


void SHA384::Transform()
{
    Transform512(digest_, buffer_);
}

#endif // WORD64_AVIALABLE


#ifdef DO_SHA_ASM

// f1(x,y,z) (z^(x &(y^z)))
// place in esi
#define ASMf1(x,y,z)   \
    AS2(    mov   esi, y    )   \
    AS2(    xor   esi, z    )   \
    AS2(    and   esi, x    )   \
    AS2(    xor   esi, z    )


// R0(v,w,x,y,z,i) =
//      z+= f1(w,x,y) + W[i] + 0x5A827999 + rotlFixed(v,5);
//      w = rotlFixed(w,30);

//      use esi for f
//      use edi as tmp


#define ASMR0(v,w,x,y,z,i) \
    AS2(    mov   esi, x                        )   \
    AS2(    mov   edi, [esp + i * 4]            )   \
    AS2(    xor   esi, y                        )   \
    AS2(    and   esi, w                        )   \
    AS2(    lea     z, [edi + z + 0x5A827999]   )   \
    AS2(    mov   edi, v                        )   \
    AS2(    xor   esi, y                        )   \
    AS2(    rol   edi, 5                        )   \
    AS2(    add     z, esi                      )   \
    AS2(    rol     w, 30                       )   \
    AS2(    add     z, edi                      )


/*  Some macro stuff, but older gas ( < 2,16 ) can't process &, so do by hand
    % won't work on gas at all

#define xstr(s) str(s)
#define  str(s) #s

#define WOFF1(a) ( a       & 15)
#define WOFF2(a) ((a +  2) & 15)
#define WOFF3(a) ((a +  8) & 15)
#define WOFF4(a) ((a + 13) & 15)

#ifdef __GNUC__
    #define WGET1(i) asm("mov esp, [edi - "xstr(WOFF1(i))" * 4] ");
    #define WGET2(i) asm("xor esp, [edi - "xstr(WOFF2(i))" * 4] ");
    #define WGET3(i) asm("xor esp, [edi - "xstr(WOFF3(i))" * 4] ");
    #define WGET4(i) asm("xor esp, [edi - "xstr(WOFF4(i))" * 4] ");
    #define WPUT1(i) asm("mov [edi - "xstr(WOFF1(i))" * 4], esp ");
#else
    #define WGET1(i) AS2( mov   esp, [edi - WOFF1(i) * 4]   )
    #define WGET2(i) AS2( xor   esp, [edi - WOFF2(i) * 4]   )
    #define WGET3(i) AS2( xor   esp, [edi - WOFF3(i) * 4]   )
    #define WGET4(i) AS2( xor   esp, [edi - WOFF4(i) * 4]   )
    #define WPUT1(i) AS2( mov   [edi - WOFF1(i) * 4], esp   )
#endif
*/

// ASMR1 = ASMR0 but use esp for W calcs

#define ASMR1(v,w,x,y,z,i,W1,W2,W3,W4) \
    AS2(    mov   edi, [esp + W1 * 4]           )   \
    AS2(    mov   esi, x                        )   \
    AS2(    xor   edi, [esp + W2 * 4]           )   \
    AS2(    xor   esi, y                        )   \
    AS2(    xor   edi, [esp + W3 * 4]           )   \
    AS2(    and   esi, w                        )   \
    AS2(    xor   edi, [esp + W4 * 4]           )   \
    AS2(    rol   edi, 1                        )   \
    AS2(    xor   esi, y                        )   \
    AS2(    mov   [esp + W1 * 4], edi           )   \
    AS2(    lea     z, [edi + z + 0x5A827999]   )   \
    AS2(    mov   edi, v                        )   \
    AS2(    rol   edi, 5                        )   \
    AS2(    add     z, esi                      )   \
    AS2(    rol     w, 30                       )   \
    AS2(    add     z, edi                      )


// ASMR2 = ASMR1 but f is xor, xor instead

#define ASMR2(v,w,x,y,z,i,W1,W2,W3,W4) \
    AS2(    mov   edi, [esp + W1 * 4]           )   \
    AS2(    mov   esi, x                        )   \
    AS2(    xor   edi, [esp + W2 * 4]           )   \
    AS2(    xor   esi, y                        )   \
    AS2(    xor   edi, [esp + W3 * 4]           )   \
    AS2(    xor   esi, w                        )   \
    AS2(    xor   edi, [esp + W4 * 4]           )   \
    AS2(    rol   edi, 1                        )   \
    AS2(    add     z, esi                      )   \
    AS2(    mov   [esp + W1 * 4], edi           )   \
    AS2(    lea     z, [edi + z + 0x6ED9EBA1]   )   \
    AS2(    mov   edi, v                        )   \
    AS2(    rol   edi, 5                        )   \
    AS2(    rol     w, 30                       )   \
    AS2(    add     z, edi                      )


// ASMR3 = ASMR2 but f is (x&y)|(z&(x|y))
//               which is (w&x)|(y&(w|x))

#define ASMR3(v,w,x,y,z,i,W1,W2,W3,W4) \
    AS2(    mov   edi, [esp + W1 * 4]           )   \
    AS2(    mov   esi, x                        )   \
    AS2(    xor   edi, [esp + W2 * 4]           )   \
    AS2(     or   esi, w                        )   \
    AS2(    xor   edi, [esp + W3 * 4]           )   \
    AS2(    and   esi, y                        )   \
    AS2(    xor   edi, [esp + W4 * 4]           )   \
    AS2(    movd  mm0, esi                      )   \
    AS2(    rol   edi, 1                        )   \
    AS2(    mov   esi, x                        )   \
    AS2(    mov   [esp + W1 * 4], edi           )   \
    AS2(    and   esi, w                        )   \
    AS2(    lea     z, [edi + z + 0x8F1BBCDC]   )   \
    AS2(    movd  edi, mm0                      )   \
    AS2(     or   esi, edi                      )   \
    AS2(    mov   edi, v                        )   \
    AS2(    rol   edi, 5                        )   \
    AS2(    add     z, esi                      )   \
    AS2(    rol     w, 30                       )   \
    AS2(    add     z, edi                      )


// ASMR4 = ASMR2 but different constant

#define ASMR4(v,w,x,y,z,i,W1,W2,W3,W4) \
    AS2(    mov   edi, [esp + W1 * 4]           )   \
    AS2(    mov   esi, x                        )   \
    AS2(    xor   edi, [esp + W2 * 4]           )   \
    AS2(    xor   esi, y                        )   \
    AS2(    xor   edi, [esp + W3 * 4]           )   \
    AS2(    xor   esi, w                        )   \
    AS2(    xor   edi, [esp + W4 * 4]           )   \
    AS2(    rol   edi, 1                        )   \
    AS2(    add     z, esi                      )   \
    AS2(    mov   [esp + W1 * 4], edi           )   \
    AS2(    lea     z, [edi + z + 0xCA62C1D6]   )   \
    AS2(    mov   edi, v                        )   \
    AS2(    rol   edi, 5                        )   \
    AS2(    rol     w, 30                       )   \
    AS2(    add     z, edi                      )


#ifdef _MSC_VER
    __declspec(naked) 
#endif
void SHA::AsmTransform(const byte* data, word32 times)
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
        AS2(    mov   eax, DWORD PTR [ebp + 16]     )

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
        AS1(    push  ebp                           )   \
        AS2(    mov   ebp, esp                      )   \
        AS2(    movd  mm3, edi                      )   \
        AS2(    movd  mm4, ebx                      )   \
        AS2(    movd  mm5, esi                      )   \
        AS2(    movd  mm6, ebp                      )   \
        AS2(    mov   edi, data                     )   \
        AS2(    mov   eax, times                    )

    #define EPILOG() \
        AS2(    movd  ebp, mm6                  )   \
        AS2(    movd  esi, mm5                  )   \
        AS2(    movd  ebx, mm4                  )   \
        AS2(    movd  edi, mm3                  )   \
        AS2(    mov   esp, ebp                  )   \
        AS1(    pop   ebp                       )   \
        AS1(    emms   )                            \
        AS1(    ret 8  )   
#endif

    PROLOG()

    AS2(    mov   esi, ecx              )

    #ifdef OLD_GCC_OFFSET
        AS2(    add   esi, 20               )   // digest_[0]
    #else
        AS2(    add   esi, 16               )   // digest_[0]
    #endif

    AS2(    movd  mm2, eax              )   // store times_
    AS2(    movd  mm1, esi              )   // store digest_

    AS2(    sub   esp, 68               )   // make room on stack

AS1( loopStart: )

    // byte reverse 16 words of input, 4 at a time, put on stack for W[]

    // part 1
    AS2(    mov   eax, [edi]        )
    AS2(    mov   ebx, [edi +  4]   )
    AS2(    mov   ecx, [edi +  8]   )
    AS2(    mov   edx, [edi + 12]   )

    AS1(    bswap eax   )
    AS1(    bswap ebx   )
    AS1(    bswap ecx   )
    AS1(    bswap edx   )

    AS2(    mov   [esp],      eax   )
    AS2(    mov   [esp +  4], ebx   )
    AS2(    mov   [esp +  8], ecx   )
    AS2(    mov   [esp + 12], edx   )

    // part 2
    AS2(    mov   eax, [edi + 16]   )
    AS2(    mov   ebx, [edi + 20]   )
    AS2(    mov   ecx, [edi + 24]   )
    AS2(    mov   edx, [edi + 28]   )

    AS1(    bswap eax   )
    AS1(    bswap ebx   )
    AS1(    bswap ecx   )
    AS1(    bswap edx   )

    AS2(    mov   [esp + 16], eax   )
    AS2(    mov   [esp + 20], ebx   )
    AS2(    mov   [esp + 24], ecx   )
    AS2(    mov   [esp + 28], edx   )


    // part 3
    AS2(    mov   eax, [edi + 32]   )
    AS2(    mov   ebx, [edi + 36]   )
    AS2(    mov   ecx, [edi + 40]   )
    AS2(    mov   edx, [edi + 44]   )

    AS1(    bswap eax   )
    AS1(    bswap ebx   )
    AS1(    bswap ecx   )
    AS1(    bswap edx   )

    AS2(    mov   [esp + 32], eax   )
    AS2(    mov   [esp + 36], ebx   )
    AS2(    mov   [esp + 40], ecx   )
    AS2(    mov   [esp + 44], edx   )


    // part 4
    AS2(    mov   eax, [edi + 48]   )
    AS2(    mov   ebx, [edi + 52]   )
    AS2(    mov   ecx, [edi + 56]   )
    AS2(    mov   edx, [edi + 60]   )

    AS1(    bswap eax   )
    AS1(    bswap ebx   )
    AS1(    bswap ecx   )
    AS1(    bswap edx   )

    AS2(    mov   [esp + 48], eax   )
    AS2(    mov   [esp + 52], ebx   )
    AS2(    mov   [esp + 56], ecx   )
    AS2(    mov   [esp + 60], edx   )

    AS2(    mov   [esp + 64], edi   )   // store edi for end

    // read from digest_
    AS2(    mov   eax, [esi]            )   // a1
    AS2(    mov   ebx, [esi +  4]       )   // b1
    AS2(    mov   ecx, [esi +  8]       )   // c1
    AS2(    mov   edx, [esi + 12]       )   // d1
    AS2(    mov   ebp, [esi + 16]       )   // e1


    ASMR0(eax, ebx, ecx, edx, ebp,  0)
    ASMR0(ebp, eax, ebx, ecx, edx,  1)
    ASMR0(edx, ebp, eax, ebx, ecx,  2)
    ASMR0(ecx, edx, ebp, eax, ebx,  3)
    ASMR0(ebx, ecx, edx, ebp, eax,  4)
    ASMR0(eax, ebx, ecx, edx, ebp,  5)
    ASMR0(ebp, eax, ebx, ecx, edx,  6)
    ASMR0(edx, ebp, eax, ebx, ecx,  7)
    ASMR0(ecx, edx, ebp, eax, ebx,  8)
    ASMR0(ebx, ecx, edx, ebp, eax,  9)
    ASMR0(eax, ebx, ecx, edx, ebp, 10)
    ASMR0(ebp, eax, ebx, ecx, edx, 11)
    ASMR0(edx, ebp, eax, ebx, ecx, 12)
    ASMR0(ecx, edx, ebp, eax, ebx, 13)
    ASMR0(ebx, ecx, edx, ebp, eax, 14)
    ASMR0(eax, ebx, ecx, edx, ebp, 15)

    ASMR1(ebp, eax, ebx, ecx, edx, 16,  0,  2,  8, 13)
    ASMR1(edx, ebp, eax, ebx, ecx, 17,  1,  3,  9, 14)
    ASMR1(ecx, edx, ebp, eax, ebx, 18,  2,  4, 10, 15)
    ASMR1(ebx, ecx, edx, ebp, eax, 19,  3,  5, 11,  0)

    ASMR2(eax, ebx, ecx, edx, ebp, 20,  4,  6, 12,  1)
    ASMR2(ebp, eax, ebx, ecx, edx, 21,  5,  7, 13,  2)
    ASMR2(edx, ebp, eax, ebx, ecx, 22,  6,  8, 14,  3)
    ASMR2(ecx, edx, ebp, eax, ebx, 23,  7,  9, 15,  4)
    ASMR2(ebx, ecx, edx, ebp, eax, 24,  8, 10,  0,  5)
    ASMR2(eax, ebx, ecx, edx, ebp, 25,  9, 11,  1,  6)
    ASMR2(ebp, eax, ebx, ecx, edx, 26, 10, 12,  2,  7)
    ASMR2(edx, ebp, eax, ebx, ecx, 27, 11, 13,  3,  8)
    ASMR2(ecx, edx, ebp, eax, ebx, 28, 12, 14,  4,  9)
    ASMR2(ebx, ecx, edx, ebp, eax, 29, 13, 15,  5, 10)
    ASMR2(eax, ebx, ecx, edx, ebp, 30, 14,  0,  6, 11)
    ASMR2(ebp, eax, ebx, ecx, edx, 31, 15,  1,  7, 12)
    ASMR2(edx, ebp, eax, ebx, ecx, 32,  0,  2,  8, 13)
    ASMR2(ecx, edx, ebp, eax, ebx, 33,  1,  3,  9, 14)
    ASMR2(ebx, ecx, edx, ebp, eax, 34,  2,  4, 10, 15)
    ASMR2(eax, ebx, ecx, edx, ebp, 35,  3,  5, 11,  0)
    ASMR2(ebp, eax, ebx, ecx, edx, 36,  4,  6, 12,  1)
    ASMR2(edx, ebp, eax, ebx, ecx, 37,  5,  7, 13,  2)
    ASMR2(ecx, edx, ebp, eax, ebx, 38,  6,  8, 14,  3)
    ASMR2(ebx, ecx, edx, ebp, eax, 39,  7,  9, 15,  4)


    ASMR3(eax, ebx, ecx, edx, ebp, 40,  8, 10,  0,  5)
    ASMR3(ebp, eax, ebx, ecx, edx, 41,  9, 11,  1,  6)
    ASMR3(edx, ebp, eax, ebx, ecx, 42, 10, 12,  2,  7)
    ASMR3(ecx, edx, ebp, eax, ebx, 43, 11, 13,  3,  8)
    ASMR3(ebx, ecx, edx, ebp, eax, 44, 12, 14,  4,  9)
    ASMR3(eax, ebx, ecx, edx, ebp, 45, 13, 15,  5, 10)
    ASMR3(ebp, eax, ebx, ecx, edx, 46, 14,  0,  6, 11)
    ASMR3(edx, ebp, eax, ebx, ecx, 47, 15,  1,  7, 12)
    ASMR3(ecx, edx, ebp, eax, ebx, 48,  0,  2,  8, 13)
    ASMR3(ebx, ecx, edx, ebp, eax, 49,  1,  3,  9, 14)
    ASMR3(eax, ebx, ecx, edx, ebp, 50,  2,  4, 10, 15)
    ASMR3(ebp, eax, ebx, ecx, edx, 51,  3,  5, 11,  0)
    ASMR3(edx, ebp, eax, ebx, ecx, 52,  4,  6, 12,  1)
    ASMR3(ecx, edx, ebp, eax, ebx, 53,  5,  7, 13,  2)
    ASMR3(ebx, ecx, edx, ebp, eax, 54,  6,  8, 14,  3)
    ASMR3(eax, ebx, ecx, edx, ebp, 55,  7,  9, 15,  4)
    ASMR3(ebp, eax, ebx, ecx, edx, 56,  8, 10,  0,  5)
    ASMR3(edx, ebp, eax, ebx, ecx, 57,  9, 11,  1,  6)
    ASMR3(ecx, edx, ebp, eax, ebx, 58, 10, 12,  2,  7)
    ASMR3(ebx, ecx, edx, ebp, eax, 59, 11, 13,  3,  8)

    ASMR4(eax, ebx, ecx, edx, ebp, 60, 12, 14,  4,  9)
    ASMR4(ebp, eax, ebx, ecx, edx, 61, 13, 15,  5, 10)
    ASMR4(edx, ebp, eax, ebx, ecx, 62, 14,  0,  6, 11)
    ASMR4(ecx, edx, ebp, eax, ebx, 63, 15,  1,  7, 12)
    ASMR4(ebx, ecx, edx, ebp, eax, 64,  0,  2,  8, 13)
    ASMR4(eax, ebx, ecx, edx, ebp, 65,  1,  3,  9, 14)
    ASMR4(ebp, eax, ebx, ecx, edx, 66,  2,  4, 10, 15)
    ASMR4(edx, ebp, eax, ebx, ecx, 67,  3,  5, 11,  0)
    ASMR4(ecx, edx, ebp, eax, ebx, 68,  4,  6, 12,  1)
    ASMR4(ebx, ecx, edx, ebp, eax, 69,  5,  7, 13,  2)
    ASMR4(eax, ebx, ecx, edx, ebp, 70,  6,  8, 14,  3)
    ASMR4(ebp, eax, ebx, ecx, edx, 71,  7,  9, 15,  4)
    ASMR4(edx, ebp, eax, ebx, ecx, 72,  8, 10,  0,  5)
    ASMR4(ecx, edx, ebp, eax, ebx, 73,  9, 11,  1,  6)
    ASMR4(ebx, ecx, edx, ebp, eax, 74, 10, 12,  2,  7)
    ASMR4(eax, ebx, ecx, edx, ebp, 75, 11, 13,  3,  8)
    ASMR4(ebp, eax, ebx, ecx, edx, 76, 12, 14,  4,  9)
    ASMR4(edx, ebp, eax, ebx, ecx, 77, 13, 15,  5, 10)
    ASMR4(ecx, edx, ebp, eax, ebx, 78, 14,  0,  6, 11)
    ASMR4(ebx, ecx, edx, ebp, eax, 79, 15,  1,  7, 12)


    AS2(    movd  esi, mm1              )   // digest_

    AS2(    add   [esi],      eax       )   // write out
    AS2(    add   [esi +  4], ebx       )
    AS2(    add   [esi +  8], ecx       )
    AS2(    add   [esi + 12], edx       )
    AS2(    add   [esi + 16], ebp       )

    // setup next round
    AS2(    movd  ebp, mm2              )   // times
 
    AS2(    mov   edi, DWORD PTR [esp + 64] )   // data
    
    AS2(    add   edi, 64               )   // next round of data
    AS2(    mov   [esp + 64], edi       )   // restore
    
    AS1(    dec   ebp                   )
    AS2(    movd  mm2, ebp              )
    AS1(    jnz   loopStart             )


    EPILOG()
}


#endif // DO_SHA_ASM

} // namespace
