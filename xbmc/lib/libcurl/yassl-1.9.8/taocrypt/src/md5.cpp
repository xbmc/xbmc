/* md5.cpp                                
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


/* based on Wei Dai's md5.cpp from CryptoPP */

#include "runtime.hpp"
#include "md5.hpp"
#ifdef USE_SYS_STL
    #include <algorithm>
#else
    #include "algorithm.hpp"
#endif


namespace STL = STL_NAMESPACE;



namespace TaoCrypt {

void MD5::Init()
{
    digest_[0] = 0x67452301L;
    digest_[1] = 0xefcdab89L;
    digest_[2] = 0x98badcfeL;
    digest_[3] = 0x10325476L;

    buffLen_ = 0;
    loLen_  = 0;
    hiLen_  = 0;
}


MD5::MD5(const MD5& that) : HASHwithTransform(DIGEST_SIZE / sizeof(word32),
                                              BLOCK_SIZE) 
{ 
    buffLen_ = that.buffLen_;
    loLen_  =  that.loLen_;
    hiLen_  =  that.hiLen_;

    memcpy(digest_, that.digest_, DIGEST_SIZE);
    memcpy(buffer_, that.buffer_, BLOCK_SIZE);
}

MD5& MD5::operator= (const MD5& that)
{
    MD5 tmp(that);
    Swap(tmp);

    return *this;
}


void MD5::Swap(MD5& other)
{
    STL::swap(loLen_,   other.loLen_);
    STL::swap(hiLen_,   other.hiLen_);
    STL::swap(buffLen_, other.buffLen_);

    memcpy(digest_, other.digest_, DIGEST_SIZE);
    memcpy(buffer_, other.buffer_, BLOCK_SIZE);
}


#ifdef DO_MD5_ASM

// Update digest with data of size len
void MD5::Update(const byte* data, word32 len)
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

    // at once for asm
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




/*
    // w = rotlFixed(w + f(x, y, z) + index[edi] + data, s) + x
#define ASMMD5STEP(f, w, x, y, z, index, data, s)       \
    f(x, y, z)                                          \
    AS2(    mov   ebp, [edi + index * 4]            )   \
    AS2(    lea     w, [esi + w + data]             )   \
    AS2(    add     w, ebp                          )   \
    AS2(    rol     w, s                            )   \
    AS2(    add     w, x                            )


    // F1(x, y, z) (z ^ (x & (y ^ z)))
    // place in esi
#define ASMF1(x, y, z) \
    AS2(    mov   esi, y                )   \
    AS2(    xor   esi, z                )   \
    AS2(    and   esi, x                )   \
    AS2(    xor   esi, z                )


#define ASMF2(x, y, z) ASMF1(z, x, y)


    // F3(x ^ y ^ z)
    // place in esi
#define ASMF3(x, y, z)  \
    AS2(    mov   esi, x                )   \
    AS2(    xor   esi, y                )   \
    AS2(    xor   esi, z                )



    // F4(x, y, z) (y ^ (x | ~z))
    // place in esi
#define ASMF4(x, y, z)  \
    AS2(    mov   esi, z                )   \
    AS1(    not   esi                   )   \
    AS2(     or   esi, x                )   \
    AS2(    xor   esi, y                )
*/


    // combine above ASMMD5STEP(f w/ each f ASMF1 - F4

    // esi already set up, after using set for next round
    // ebp already set up, set up using next round index
    
#define MD5STEP1(w, x, y, z, index, data, s)    \
    AS2(    xor   esi, z                    )   \
    AS2(    and   esi, x                    )   \
    AS2(    lea     w, [ebp + w + data]     )   \
    AS2(    xor   esi, z                    )   \
    AS2(    add     w, esi                  )   \
    AS2(    mov   esi, x                    )   \
    AS2(    rol     w, s                    )   \
    AS2(    mov   ebp, [edi + index * 4]    )   \
    AS2(    add     w, x                    )

#define MD5STEP2(w, x, y, z, index, data, s)    \
    AS2(    xor   esi, x                    )   \
    AS2(    and   esi, z                    )   \
    AS2(    lea     w, [ebp + w + data]     )   \
    AS2(    xor   esi, y                    )   \
    AS2(    add     w, esi                  )   \
    AS2(    mov   esi, x                    )   \
    AS2(    rol     w, s                    )   \
    AS2(    mov   ebp, [edi + index * 4]    )   \
    AS2(    add     w, x                    )


#define MD5STEP3(w, x, y, z, index, data, s)    \
    AS2(    xor   esi, z                    )   \
    AS2(    lea     w, [ebp + w + data]     )   \
    AS2(    xor   esi, x                    )   \
    AS2(    add     w, esi                  )   \
    AS2(    mov   esi, x                    )   \
    AS2(    rol     w, s                    )   \
    AS2(    mov   ebp, [edi + index * 4]    )   \
    AS2(    add     w, x                    )


#define MD5STEP4(w, x, y, z, index, data, s)    \
    AS2(     or   esi, x                    )   \
    AS2(    lea     w, [ebp + w + data]     )   \
    AS2(    xor   esi, y                    )   \
    AS2(    add     w, esi                  )   \
    AS2(    mov   esi, y                    )   \
    AS2(    rol     w, s                    )   \
    AS1(    not   esi                       )   \
    AS2(    mov   ebp, [edi + index * 4]    )   \
    AS2(    add     w, x                    )



#ifdef _MSC_VER
    __declspec(naked) 
#endif
void MD5::AsmTransform(const byte* data, word32 times)
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
        AS1(    push  ebp                       )   \
        AS2(    mov   ebp, esp                  )   \
        AS2(    movd  mm3, edi                  )   \
        AS2(    movd  mm4, ebx                  )   \
        AS2(    movd  mm5, esi                  )   \
        AS2(    movd  mm6, ebp                  )   \
        AS2(    mov   edi, DWORD PTR [ebp +  8] )   \
        AS2(    mov   eax, DWORD PTR [ebp + 12] )

    #define EPILOG() \
        AS2(    movd  ebp, mm6                  )   \
        AS2(    movd  esi, mm5                  )   \
        AS2(    movd  ebx, mm4                  )   \
        AS2(    movd  edi, mm3                  )   \
        AS2(    mov   esp, ebp                  )   \
        AS1(    pop   ebp                       )   \
        AS1(    emms                            )   \
        AS1(    ret  8                          )
        
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
    
    AS2(    mov   eax, [esi]            )   // a
    AS2(    mov   ebx, [esi +  4]       )   // b
    AS2(    mov   ecx, [esi +  8]       )   // c
    AS2(    mov   edx, [esi + 12]       )   // d
  
AS1(loopStart:)

    // set up
    AS2(    mov   esi, ecx      )
    AS2(    mov   ebp, [edi]    )

    MD5STEP1( eax, ebx, ecx, edx, 1,   0xd76aa478,  7)
    MD5STEP1( edx, eax, ebx, ecx, 2,   0xe8c7b756, 12)
    MD5STEP1( ecx, edx, eax, ebx, 3,   0x242070db, 17)
    MD5STEP1( ebx, ecx, edx, eax, 4,   0xc1bdceee, 22)
    MD5STEP1( eax, ebx, ecx, edx, 5,   0xf57c0faf,  7)
    MD5STEP1( edx, eax, ebx, ecx, 6,   0x4787c62a, 12)
    MD5STEP1( ecx, edx, eax, ebx, 7,   0xa8304613, 17)
    MD5STEP1( ebx, ecx, edx, eax, 8,   0xfd469501, 22)
    MD5STEP1( eax, ebx, ecx, edx, 9,   0x698098d8,  7)
    MD5STEP1( edx, eax, ebx, ecx, 10,  0x8b44f7af, 12)
    MD5STEP1( ecx, edx, eax, ebx, 11,  0xffff5bb1, 17)
    MD5STEP1( ebx, ecx, edx, eax, 12,  0x895cd7be, 22)
    MD5STEP1( eax, ebx, ecx, edx, 13,  0x6b901122,  7)
    MD5STEP1( edx, eax, ebx, ecx, 14,  0xfd987193, 12)
    MD5STEP1( ecx, edx, eax, ebx, 15,  0xa679438e, 17)
    MD5STEP1( ebx, ecx, edx, eax, 1,   0x49b40821, 22)

    MD5STEP2( eax, ebx, ecx, edx, 6,  0xf61e2562,  5)
    MD5STEP2( edx, eax, ebx, ecx, 11, 0xc040b340,  9)
    MD5STEP2( ecx, edx, eax, ebx, 0,  0x265e5a51, 14)
    MD5STEP2( ebx, ecx, edx, eax, 5,  0xe9b6c7aa, 20)
    MD5STEP2( eax, ebx, ecx, edx, 10, 0xd62f105d,  5)
    MD5STEP2( edx, eax, ebx, ecx, 15, 0x02441453,  9)
    MD5STEP2( ecx, edx, eax, ebx, 4,  0xd8a1e681, 14)
    MD5STEP2( ebx, ecx, edx, eax, 9,  0xe7d3fbc8, 20)
    MD5STEP2( eax, ebx, ecx, edx, 14, 0x21e1cde6,  5)
    MD5STEP2( edx, eax, ebx, ecx, 3,  0xc33707d6,  9)
    MD5STEP2( ecx, edx, eax, ebx, 8,  0xf4d50d87, 14)
    MD5STEP2( ebx, ecx, edx, eax, 13, 0x455a14ed, 20)
    MD5STEP2( eax, ebx, ecx, edx, 2,  0xa9e3e905,  5)
    MD5STEP2( edx, eax, ebx, ecx, 7,  0xfcefa3f8,  9)
    MD5STEP2( ecx, edx, eax, ebx, 12, 0x676f02d9, 14)
    MD5STEP2( ebx, ecx, edx, eax, 5,  0x8d2a4c8a, 20)

    MD5STEP3(  eax, ebx, ecx, edx, 8,   0xfffa3942,  4)
    MD5STEP3(  edx, eax, ebx, ecx, 11,  0x8771f681, 11)
    MD5STEP3(  ecx, edx, eax, ebx, 14,  0x6d9d6122, 16)
    MD5STEP3(  ebx, ecx, edx, eax, 1,   0xfde5380c, 23)
    MD5STEP3(  eax, ebx, ecx, edx, 4,   0xa4beea44,  4)
    MD5STEP3(  edx, eax, ebx, ecx, 7,   0x4bdecfa9, 11)
    MD5STEP3(  ecx, edx, eax, ebx, 10,  0xf6bb4b60, 16)
    MD5STEP3(  ebx, ecx, edx, eax, 13,  0xbebfbc70, 23)
    MD5STEP3(  eax, ebx, ecx, edx, 0,   0x289b7ec6,  4)
    MD5STEP3(  edx, eax, ebx, ecx, 3,   0xeaa127fa, 11)
    MD5STEP3(  ecx, edx, eax, ebx, 6,   0xd4ef3085, 16)
    MD5STEP3(  ebx, ecx, edx, eax, 9,   0x04881d05, 23)
    MD5STEP3(  eax, ebx, ecx, edx, 12,  0xd9d4d039,  4)
    MD5STEP3(  edx, eax, ebx, ecx, 15,  0xe6db99e5, 11)
    MD5STEP3(  ecx, edx, eax, ebx, 2,   0x1fa27cf8, 16)
    MD5STEP3(  ebx, ecx, edx, eax, 0,   0xc4ac5665, 23)

    // setup
    AS2(    mov   esi, edx      )
    AS1(    not   esi           )

    MD5STEP4(  eax, ebx, ecx, edx, 7,   0xf4292244,  6)
    MD5STEP4(  edx, eax, ebx, ecx, 14,  0x432aff97, 10)
    MD5STEP4(  ecx, edx, eax, ebx, 5,   0xab9423a7, 15)
    MD5STEP4(  ebx, ecx, edx, eax, 12,  0xfc93a039, 21)
    MD5STEP4(  eax, ebx, ecx, edx, 3,   0x655b59c3,  6)
    MD5STEP4(  edx, eax, ebx, ecx, 10,  0x8f0ccc92, 10)
    MD5STEP4(  ecx, edx, eax, ebx, 1,   0xffeff47d, 15)
    MD5STEP4(  ebx, ecx, edx, eax, 8,   0x85845dd1, 21)
    MD5STEP4(  eax, ebx, ecx, edx, 15,  0x6fa87e4f,  6)
    MD5STEP4(  edx, eax, ebx, ecx, 6,   0xfe2ce6e0, 10)
    MD5STEP4(  ecx, edx, eax, ebx, 13,  0xa3014314, 15)
    MD5STEP4(  ebx, ecx, edx, eax, 4,   0x4e0811a1, 21)
    MD5STEP4(  eax, ebx, ecx, edx, 11,  0xf7537e82,  6)
    MD5STEP4(  edx, eax, ebx, ecx, 2,   0xbd3af235, 10)
    MD5STEP4(  ecx, edx, eax, ebx, 9,   0x2ad7d2bb, 15)
    MD5STEP4(  ebx, ecx, edx, eax, 9,   0xeb86d391, 21)
    
    AS2(    movd  esi, mm1              )   // digest_

    AS2(    add   [esi],      eax       )   // write out
    AS2(    add   [esi +  4], ebx       )
    AS2(    add   [esi +  8], ecx       )
    AS2(    add   [esi + 12], edx       )

    AS2(    add   edi, 64               )

    AS2(    mov   eax, [esi]            )
    AS2(    mov   ebx, [esi +  4]       )
    AS2(    mov   ecx, [esi +  8]       )
    AS2(    mov   edx, [esi + 12]       )

    AS2(    movd  ebp, mm2              )   // times
    AS1(    dec   ebp                   )
    AS2(    movd  mm2, ebp              )
    AS1(    jnz   loopStart             )


    EPILOG()
}


#endif // DO_MD5_ASM


void MD5::Transform()
{
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
    w = rotlFixed(w + f(x, y, z) + data, s) + x

    // Copy context->state[] to working vars 
    word32 a = digest_[0];
    word32 b = digest_[1];
    word32 c = digest_[2];
    word32 d = digest_[3];

    MD5STEP(F1, a, b, c, d, buffer_[0]  + 0xd76aa478,  7);
    MD5STEP(F1, d, a, b, c, buffer_[1]  + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, buffer_[2]  + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, buffer_[3]  + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, buffer_[4]  + 0xf57c0faf,  7);
    MD5STEP(F1, d, a, b, c, buffer_[5]  + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, buffer_[6]  + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, buffer_[7]  + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, buffer_[8]  + 0x698098d8,  7);
    MD5STEP(F1, d, a, b, c, buffer_[9]  + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, buffer_[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, buffer_[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, buffer_[12] + 0x6b901122,  7);
    MD5STEP(F1, d, a, b, c, buffer_[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, buffer_[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, buffer_[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, buffer_[1]  + 0xf61e2562,  5);
    MD5STEP(F2, d, a, b, c, buffer_[6]  + 0xc040b340,  9);
    MD5STEP(F2, c, d, a, b, buffer_[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, buffer_[0]  + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, buffer_[5]  + 0xd62f105d,  5);
    MD5STEP(F2, d, a, b, c, buffer_[10] + 0x02441453,  9);
    MD5STEP(F2, c, d, a, b, buffer_[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, buffer_[4]  + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, buffer_[9]  + 0x21e1cde6,  5);
    MD5STEP(F2, d, a, b, c, buffer_[14] + 0xc33707d6,  9);
    MD5STEP(F2, c, d, a, b, buffer_[3]  + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, buffer_[8]  + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, buffer_[13] + 0xa9e3e905,  5);
    MD5STEP(F2, d, a, b, c, buffer_[2]  + 0xfcefa3f8,  9);
    MD5STEP(F2, c, d, a, b, buffer_[7]  + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, buffer_[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, buffer_[5]  + 0xfffa3942,  4);
    MD5STEP(F3, d, a, b, c, buffer_[8]  + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, buffer_[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, buffer_[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, buffer_[1]  + 0xa4beea44,  4);
    MD5STEP(F3, d, a, b, c, buffer_[4]  + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, buffer_[7]  + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, buffer_[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, buffer_[13] + 0x289b7ec6,  4);
    MD5STEP(F3, d, a, b, c, buffer_[0]  + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, buffer_[3]  + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, buffer_[6]  + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, buffer_[9]  + 0xd9d4d039,  4);
    MD5STEP(F3, d, a, b, c, buffer_[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, buffer_[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, buffer_[2]  + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, buffer_[0]  + 0xf4292244,  6);
    MD5STEP(F4, d, a, b, c, buffer_[7]  + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, buffer_[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, buffer_[5]  + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, buffer_[12] + 0x655b59c3,  6);
    MD5STEP(F4, d, a, b, c, buffer_[3]  + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, buffer_[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, buffer_[1]  + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, buffer_[8]  + 0x6fa87e4f,  6);
    MD5STEP(F4, d, a, b, c, buffer_[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, buffer_[6]  + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, buffer_[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, buffer_[4]  + 0xf7537e82,  6);
    MD5STEP(F4, d, a, b, c, buffer_[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, buffer_[2]  + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, buffer_[9]  + 0xeb86d391, 21);
    
    // Add the working vars back into digest state[]
    digest_[0] += a;
    digest_[1] += b;
    digest_[2] += c;
    digest_[3] += d;

    // Wipe variables
    a = b = c = d = 0;
}


} // namespace

