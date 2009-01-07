/* blowfish.cpp                                
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

/* C++ code based on Wei Dai's blowfish.cpp from CryptoPP */
/* x86 asm is original */


#if defined(TAOCRYPT_KERNEL_MODE)
    #define DO_TAOCRYPT_KERNEL_MODE
#endif                                  // only some modules now support this


#include "runtime.hpp"
#include "blowfish.hpp"





namespace TaoCrypt {


#if defined(DO_BLOWFISH_ASM)

// ia32 optimized version
void Blowfish::Process(byte* out, const byte* in, word32 sz)
{
    if (!isMMX) {
        Mode_BASE::Process(out, in, sz);
        return;
    }

    word32 blocks = sz / BLOCK_SIZE;

    if (mode_ == ECB)
        while (blocks--) {
            AsmProcess(in, out);
            out += BLOCK_SIZE;
            in  += BLOCK_SIZE;
        }
    else if (mode_ == CBC)
        if (dir_ == ENCRYPTION)
            while (blocks--) {
                r_[0] ^= *(word32*)in;
                r_[1] ^= *(word32*)(in + 4);

                AsmProcess((byte*)r_, (byte*)r_);
                
                memcpy(out, r_, BLOCK_SIZE);

                out += BLOCK_SIZE;
                in  += BLOCK_SIZE;
            }
        else
            while (blocks--) {
                AsmProcess(in, out);
                
                *(word32*)out       ^= r_[0];
                *(word32*)(out + 4) ^= r_[1];

                memcpy(r_, in, BLOCK_SIZE);

                out += BLOCK_SIZE;
                in  += BLOCK_SIZE;
            }
}

#endif // DO_BLOWFISH_ASM


void Blowfish::SetKey(const byte* key_string, word32 keylength, CipherDir dir)
{
	assert(keylength >= 4 && keylength <= 56);

	unsigned i, j=0, k;
	word32 data, dspace[2] = {0, 0};

	memcpy(pbox_, p_init_, sizeof(p_init_));
	memcpy(sbox_, s_init_, sizeof(s_init_));

	// Xor key string into encryption key vector
	for (i=0 ; i<ROUNDS+2 ; ++i) {
		data = 0;
		for (k=0 ; k<4 ; ++k )
			data = (data << 8) | key_string[j++ % keylength];
		pbox_[i] ^= data;
	}

	crypt_block(dspace, pbox_);

	for (i=0; i<ROUNDS; i+=2)
		crypt_block(pbox_ + i, pbox_ + i + 2);

	crypt_block(pbox_ + ROUNDS, sbox_);

	for (i=0; i < 4*256-2; i+=2)
		crypt_block(sbox_ + i, sbox_ + i + 2);

	if (dir==DECRYPTION)
		for (i=0; i<(ROUNDS+2)/2; i++)
			STL::swap(pbox_[i], pbox_[ROUNDS+1-i]);
}


#define BFBYTE_0(x) ( x     &0xFF)
#define BFBYTE_1(x) ((x>> 8)&0xFF)
#define BFBYTE_2(x) ((x>>16)&0xFF)
#define BFBYTE_3(x) ( x>>24)


#define BF_S(Put, Get, I) (\
        Put ^= p[I], \
		tmp =  p[18 + BFBYTE_3(Get)],  \
        tmp += p[274+ BFBYTE_2(Get)],  \
        tmp ^= p[530+ BFBYTE_1(Get)],  \
        tmp += p[786+ BFBYTE_0(Get)],  \
        Put ^= tmp \
    )


#define BF_ROUNDS           \
    BF_S(right, left,  1);  \
    BF_S(left,  right, 2);  \
    BF_S(right, left,  3);  \
    BF_S(left,  right, 4);  \
    BF_S(right, left,  5);  \
    BF_S(left,  right, 6);  \
    BF_S(right, left,  7);  \
    BF_S(left,  right, 8);  \
    BF_S(right, left,  9);  \
    BF_S(left,  right, 10); \
    BF_S(right, left,  11); \
    BF_S(left,  right, 12); \
    BF_S(right, left,  13); \
    BF_S(left,  right, 14); \
    BF_S(right, left,  15); \
    BF_S(left,  right, 16); 

#define BF_EXTRA_ROUNDS     \
    BF_S(right, left,  17); \
    BF_S(left,  right, 18); \
    BF_S(right, left,  19); \
    BF_S(left,  right, 20);


// Used by key setup, no byte swapping
void Blowfish::crypt_block(const word32 in[2], word32 out[2]) const
{
	word32 left  = in[0];
	word32 right = in[1];

	const word32  *const s = sbox_;
	const word32* p = pbox_;

	left ^= p[0];

    // roll back up and use s and p index instead of just p
    for (unsigned i = 0; i < ROUNDS / 2; i++) {
        right ^= (((s[GETBYTE(left,3)] + s[256+GETBYTE(left,2)])
            ^ s[2*256+GETBYTE(left,1)]) + s[3*256+GETBYTE(left,0)])
            ^ p[2*i+1];

        left ^= (((s[GETBYTE(right,3)] + s[256+GETBYTE(right,2)])
            ^ s[2*256+GETBYTE(right,1)]) + s[3*256+GETBYTE(right,0)])
            ^ p[2*i+2];
    }

	right ^= p[ROUNDS + 1];

	out[0] = right;
	out[1] = left;
}


typedef BlockGetAndPut<word32, BigEndian> gpBlock;

void Blowfish::ProcessAndXorBlock(const byte* in, const byte* xOr, byte* out)
    const
{
    word32 left, right;
	const word32  *const s = sbox_;
    const word32* p = pbox_;
    
    gpBlock::Get(in)(left)(right);
	left ^= p[0];

    // roll back up and use s and p index instead of just p
    for (unsigned i = 0; i < ROUNDS / 2; i++) {
        right ^= (((s[GETBYTE(left,3)] + s[256+GETBYTE(left,2)])
            ^ s[2*256+GETBYTE(left,1)]) + s[3*256+GETBYTE(left,0)])
            ^ p[2*i+1];

        left ^= (((s[GETBYTE(right,3)] + s[256+GETBYTE(right,2)])
            ^ s[2*256+GETBYTE(right,1)]) + s[3*256+GETBYTE(right,0)])
            ^ p[2*i+2];
    }

	right ^= p[ROUNDS + 1];

    gpBlock::Put(xOr, out)(right)(left);
}


#if defined(DO_BLOWFISH_ASM)
    #ifdef __GNUC__
        #define AS1(x)    asm(#x);
        #define AS2(x, y) asm(#x ", " #y);

        #define PROLOG()  \
            asm(".intel_syntax noprefix"); \
            AS2(    movd  mm3, edi                      )   \
            AS2(    movd  mm4, ebx                      )   \
            AS2(    movd  mm5, esi                      )   \
            AS2(    mov   ecx, DWORD PTR [ebp +  8]     )   \
            AS2(    mov   esi, DWORD PTR [ebp + 12]     )

        #define EPILOG()  \
            AS2(    movd esi, mm5                  )   \
            AS2(    movd ebx, mm4                  )   \
            AS2(    movd edi, mm3                  )   \
            AS1(    emms                           )   \
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
            AS2(    mov   esi, DWORD PTR [ebp +  8]     )

        #define EPILOG()  \
            AS2(    movd esi, mm5                       )   \
            AS2(    movd ebx, mm4                       )   \
            AS2(    movd edi, mm3                       )   \
            AS2(    mov  esp, ebp                       )   \
            AS1(    pop  ebp                            )   \
            AS1(    emms                                )   \
            AS1(    ret 8                               )
            
    #endif


#define BF_ROUND(P, G, I)   \
    /* Put ^= p[I]  */                              \
    AS2(    xor   P,   [edi + I*4]              )   \
    /* tmp =  p[18 + BFBYTE_3(Get)] */              \
    AS2(    mov   ecx, G                        )   \
    AS2(    shr   ecx, 16                       )   \
    AS2(    movzx edx, ch                       )   \
    AS2(    mov   esi, [edi + edx*4 +   72]     )   \
    /* tmp += p[274+ BFBYTE_2(Get)] */              \
    AS2(    movzx ecx, cl                       )   \
    AS2(    add   esi, [edi + ecx*4 + 1096]     )   \
    /* tmp ^= p[530+ BFBYTE_1(Get)] */              \
    AS2(    mov   ecx, G                        )   \
    AS2(    movzx edx, ch                       )   \
    AS2(    xor   esi, [edi + edx*4 + 2120]     )   \
    /* tmp += p[786+ BFBYTE_0(Get)] */              \
    AS2(    movzx ecx, cl                       )   \
    AS2(    add   esi, [edi + ecx*4 + 3144]     )   \
    /* Put ^= tmp */                                \
    AS2(    xor   P,   esi                      )


#ifdef _MSC_VER
    __declspec(naked) 
#endif
void Blowfish::AsmProcess(const byte* inBlock, byte* outBlock) const
{
    PROLOG()

    #ifdef OLD_GCC_OFFSET
        AS2(    lea   edi, [ecx + 60]                       )   // pbox
    #else
        AS2(    lea   edi, [ecx + 56]                       )   // pbox
    #endif

    AS2(    mov   eax, DWORD PTR [esi]                                  )
    AS2(    mov   edx, DWORD PTR [edi]                                  )
    AS1(    bswap eax                                                   )

    AS2(    mov   ebx, DWORD PTR [esi + 4]                              )
    AS2(    xor   eax, edx                      )   // left
    AS1(    bswap ebx                           )   // right


    BF_ROUND(ebx, eax, 1)
    BF_ROUND(eax, ebx, 2)
    BF_ROUND(ebx, eax, 3)
    BF_ROUND(eax, ebx, 4)
    BF_ROUND(ebx, eax, 5)
    BF_ROUND(eax, ebx, 6)
    BF_ROUND(ebx, eax, 7)
    BF_ROUND(eax, ebx, 8)
    BF_ROUND(ebx, eax, 9)
    BF_ROUND(eax, ebx, 10)
    BF_ROUND(ebx, eax, 11)
    BF_ROUND(eax, ebx, 12)
    BF_ROUND(ebx, eax, 13)
    BF_ROUND(eax, ebx, 14)
    BF_ROUND(ebx, eax, 15)
    BF_ROUND(eax, ebx, 16)
    #if ROUNDS == 20
        BF_ROUND(ebx, eax, 17)
        BF_ROUND(eax, ebx, 18)
        BF_ROUND(ebx, eax, 19)
        BF_ROUND(eax, ebx, 20)

        AS2(    xor   ebx, [edi + 84]           )   // 20 + 1 (x4)
    #else
        AS2(    xor   ebx, [edi + 68]           )   // 16 + 1 (x4)
    #endif

    #ifdef __GNUC__
        AS2(    mov   edi, [ebp + 16]           ) // outBlock
    #else
        AS2(    mov   edi, [ebp + 12]           ) // outBlock
    #endif

    AS1(    bswap ebx                           )
    AS1(    bswap eax                           )

    AS2(    mov   [edi]    , ebx                )
    AS2(    mov   [edi + 4], eax                )

    EPILOG()
}


#endif  // DO_BLOWFISH_ASM


} // namespace

