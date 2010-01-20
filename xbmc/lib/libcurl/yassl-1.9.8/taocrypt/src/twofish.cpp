/* twofish.cpp                                
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

/* C++ based on Wei Dai's twofish.cpp from CryptoPP */
/* x86 asm original */


#if defined(TAOCRYPT_KERNEL_MODE)
    #define DO_TAOCRYPT_KERNEL_MODE
#endif                                  // only some modules now support this

#include "runtime.hpp"
#include "twofish.hpp"



namespace TaoCrypt {


#if defined(DO_TWOFISH_ASM)

// ia32 optimized version
void Twofish::Process(byte* out, const byte* in, word32 sz)
{
    if (!isMMX) {
        Mode_BASE::Process(out, in, sz);
        return;
    }

    word32 blocks = sz / BLOCK_SIZE;

    if (mode_ == ECB)
        while (blocks--) {
            if (dir_ == ENCRYPTION)
                AsmEncrypt(in, out);
            else
                AsmDecrypt(in, out);
        
            out += BLOCK_SIZE;
            in  += BLOCK_SIZE;
        }
    else if (mode_ == CBC)
        if (dir_ == ENCRYPTION)
            while (blocks--) {
                r_[0] ^= *(word32*)in;
                r_[1] ^= *(word32*)(in +  4);
                r_[2] ^= *(word32*)(in +  8);
                r_[3] ^= *(word32*)(in + 12);

                AsmEncrypt((byte*)r_, (byte*)r_);
                memcpy(out, r_, BLOCK_SIZE);

                out += BLOCK_SIZE;
                in  += BLOCK_SIZE;
            }
        else
            while (blocks--) {
                AsmDecrypt(in, out);
               
                *(word32*)out        ^= r_[0];
                *(word32*)(out +  4) ^= r_[1];
                *(word32*)(out +  8) ^= r_[2];
                *(word32*)(out + 12) ^= r_[3];

                memcpy(r_, in, BLOCK_SIZE);

                out += BLOCK_SIZE;
                in  += BLOCK_SIZE;
            }
}

#endif // DO_TWOFISH_ASM


namespace {     // locals

// compute (c * x^4) mod (x^4 + (a + 1/a) * x^3 + a * x^2 + (a + 1/a) * x + 1)
// over GF(256)
static inline unsigned int Mod(unsigned int c)
{
	static const unsigned int modulus = 0x14d;
	unsigned int c2 = (c<<1) ^ ((c & 0x80) ? modulus : 0);
	unsigned int c1 = c2 ^ (c>>1) ^ ((c & 1) ? (modulus>>1) : 0);
	return c | (c1 << 8) | (c2 << 16) | (c1 << 24);
}

// compute RS(12,8) code with the above polynomial as generator
// this is equivalent to multiplying by the RS matrix
static word32 ReedSolomon(word32 high, word32 low)
{
	for (unsigned int i=0; i<8; i++) {
		high = Mod(high>>24) ^ (high<<8) ^ (low>>24);
		low <<= 8;
	}
	return high;
}

}  // local namespace



inline word32 Twofish::h0(word32 x, const word32* key, unsigned int kLen)
{
	x = x | (x<<8) | (x<<16) | (x<<24);
	switch(kLen)
	{
#define Q(a, b, c, d, t) q_[a][GETBYTE(t,0)] ^ (q_[b][GETBYTE(t,1)] << 8) ^  \
            (q_[c][GETBYTE(t,2)] << 16) ^ (q_[d][GETBYTE(t,3)] << 24)
	case 4: x = Q(1, 0, 0, 1, x) ^ key[6];
	case 3: x = Q(1, 1, 0, 0, x) ^ key[4];
	case 2: x = Q(0, 1, 0, 1, x) ^ key[2];
			x = Q(0, 0, 1, 1, x) ^ key[0];
	}
	return x;
}

inline word32 Twofish::h(word32 x, const word32* key, unsigned int kLen)
{
	x = h0(x, key, kLen);
	return mds_[0][GETBYTE(x,0)] ^ mds_[1][GETBYTE(x,1)] ^ 
        mds_[2][GETBYTE(x,2)] ^ mds_[3][GETBYTE(x,3)];
}


void Twofish::SetKey(const byte* userKey, word32 keylen, CipherDir /*dummy*/)
{
	assert(keylen >= 16 && keylen <= 32);

	unsigned int len = (keylen <= 16 ? 2 : (keylen <= 24 ? 3 : 4));
    word32 key[8];
	GetUserKey(LittleEndianOrder, key, len*2, userKey, keylen);

	unsigned int i;
	for (i=0; i<40; i+=2) {
		word32 a = h(i, key, len);
		word32 b = rotlFixed(h(i+1, key+1, len), 8);
		k_[i] = a+b;
		k_[i+1] = rotlFixed(a+2*b, 9);
	}

	word32 svec[8];
	for (i=0; i<len; i++)
		svec[2*(len-i-1)] = ReedSolomon(key[2*i+1], key[2*i]);

	for (i=0; i<256; i++) {
		word32 t = h0(i, svec, len);
		s_[0][i] = mds_[0][GETBYTE(t, 0)];
		s_[1][i] = mds_[1][GETBYTE(t, 1)];
		s_[2][i] = mds_[2][GETBYTE(t, 2)];
		s_[3][i] = mds_[3][GETBYTE(t, 3)];
	}
}


void Twofish::ProcessAndXorBlock(const byte* in, const byte* xOr, byte* out)
    const
{
    if (dir_ == ENCRYPTION)
        encrypt(in, xOr, out);
    else
        decrypt(in, xOr, out);
}

#define G1(x) (s_[0][GETBYTE(x,0)] ^ s_[1][GETBYTE(x,1)] ^ \
            s_[2][GETBYTE(x,2)] ^ s_[3][GETBYTE(x,3)])
#define G2(x) (s_[0][GETBYTE(x,3)] ^ s_[1][GETBYTE(x,0)] ^ \
            s_[2][GETBYTE(x,1)] ^ s_[3][GETBYTE(x,2)])

#define ENCROUND(n, a, b, c, d) \
	x = G1 (a); y = G2 (b); \
	x += y; y += x + k[2 * (n) + 1]; \
	(c) ^= x + k[2 * (n)]; \
	(c) = rotrFixed(c, 1); \
	(d) = rotlFixed(d, 1) ^ y

#define ENCCYCLE(n) \
	ENCROUND (2 * (n), a, b, c, d); \
	ENCROUND (2 * (n) + 1, c, d, a, b)

#define DECROUND(n, a, b, c, d) \
	x = G1 (a); y = G2 (b); \
	x += y; y += x; \
	(d) ^= y + k[2 * (n) + 1]; \
	(d) = rotrFixed(d, 1); \
	(c) = rotlFixed(c, 1); \
	(c) ^= (x + k[2 * (n)])

#define DECCYCLE(n) \
	DECROUND (2 * (n) + 1, c, d, a, b); \
	DECROUND (2 * (n), a, b, c, d)


typedef BlockGetAndPut<word32, LittleEndian> gpBlock;

void Twofish::encrypt(const byte* inBlock, const byte* xorBlock,
                  byte* outBlock) const
{
	word32 x, y, a, b, c, d;

	gpBlock::Get(inBlock)(a)(b)(c)(d);

	a ^= k_[0];
	b ^= k_[1];
	c ^= k_[2];
	d ^= k_[3];

	const word32 *k = k_+8;

	ENCCYCLE (0);
	ENCCYCLE (1);
	ENCCYCLE (2);
	ENCCYCLE (3);
	ENCCYCLE (4);
	ENCCYCLE (5);
	ENCCYCLE (6);
	ENCCYCLE (7);

	c ^= k_[4];
	d ^= k_[5];
	a ^= k_[6];
	b ^= k_[7]; 

	gpBlock::Put(xorBlock, outBlock)(c)(d)(a)(b);
}


void Twofish::decrypt(const byte* inBlock, const byte* xorBlock,
                  byte* outBlock) const
{
	word32 x, y, a, b, c, d;

	gpBlock::Get(inBlock)(c)(d)(a)(b);

	c ^= k_[4];
	d ^= k_[5];
	a ^= k_[6];
	b ^= k_[7];

	const word32 *k = k_+8;
	DECCYCLE (7);
	DECCYCLE (6);
	DECCYCLE (5);
	DECCYCLE (4);
	DECCYCLE (3);
	DECCYCLE (2);
	DECCYCLE (1);
	DECCYCLE (0);

	a ^= k_[0];
	b ^= k_[1];
	c ^= k_[2];
	d ^= k_[3];

	gpBlock::Put(xorBlock, outBlock)(a)(b)(c)(d);
}



#if defined(DO_TWOFISH_ASM)
    #ifdef __GNUC__
        #define AS1(x)    asm(#x);
        #define AS2(x, y) asm(#x ", " #y);

        #define PROLOG()  \
            asm(".intel_syntax noprefix"); \
            AS2(    movd  mm3, edi                      )   \
            AS2(    movd  mm4, ebx                      )   \
            AS2(    movd  mm5, esi                      )   \
            AS2(    movd  mm6, ebp                      )   \
            AS2(    mov   edi, DWORD PTR [ebp +  8]     )   \
            AS2(    mov   esi, DWORD PTR [ebp + 12]     )

        #define EPILOG()  \
            AS2(    movd esp, mm6                  )   \
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
            AS2(    movd  mm6, ebp                      )   \
            AS2(    mov   edi, ecx                      )   \
            AS2(    mov   esi, DWORD PTR [ebp +  8]     )

        /* ebp already set */
        #define EPILOG()  \
            AS2(    movd esi, mm5                   )   \
            AS2(    movd ebx, mm4                   )   \
            AS2(    movd edi, mm3                   )   \
            AS2(    mov  esp, ebp                   )   \
            AS1(    pop  ebp                        )   \
            AS1(    emms                            )   \
            AS1(    ret 8                           )    
            
    #endif




    // x = esi, y = [esp], s_ = ebp
    // edi always open for G1 and G2
    // G1 also uses edx after save and restore
    // G2 also uses eax after save and restore
    //      and ecx for tmp [esp] which Rounds also use
    //      and restore from mm7

    // x = G1(a)   bytes(0,1,2,3)
#define ASMG1(z, zl, zh) \
    AS2(    movd  mm2, edx                          )   \
    AS2(    movzx edi, zl                           )   \
    AS2(    mov   esi, DWORD PTR     [ebp + edi*4]  )   \
    AS2(    movzx edx, zh                           )   \
    AS2(    xor   esi, DWORD PTR 1024[ebp + edx*4]  )   \
                                                        \
    AS2(    mov   edx, z                            )   \
    AS2(    shr   edx, 16                           )   \
    AS2(    movzx edi, dl                           )   \
    AS2(    xor   esi, DWORD PTR 2048[ebp + edi*4]  )   \
    AS2(    movzx edx, dh                           )   \
    AS2(    xor   esi, DWORD PTR 3072[ebp + edx*4]  )   \
    AS2(    movd  edx, mm2                          )


    // y = G2(b)  bytes(3,0,1,2)  [ put y into ecx for Rounds ]
#define ASMG2(z, zl, zh)    \
    AS2(    movd  mm7, ecx                          )   \
    AS2(    movd  mm2, eax                          )   \
    AS2(    mov   edi, z                            )   \
    AS2(    shr   edi, 24                           )   \
    AS2(    mov   ecx, DWORD PTR     [ebp + edi*4]  )   \
    AS2(    movzx eax, zl                           )   \
    AS2(    xor   ecx, DWORD PTR 1024[ebp + eax*4]  )   \
                                                        \
    AS2(    mov   eax, z                            )   \
    AS2(    shr   eax, 16                           )   \
    AS2(    movzx edi, zh                           )   \
    AS2(    xor   ecx, DWORD PTR 2048[ebp + edi*4]  )   \
    AS2(    movzx eax, al                           )   \
    AS2(    xor   ecx, DWORD PTR 3072[ebp + eax*4]  )   \
    AS2(    movd  eax, mm2                          )


    // encrypt Round (n), 
    // x = esi, k = ebp, edi open
    // y is in ecx from G2, restore when done from mm7
    //      before C (which be same register!)
#define ASMENCROUND(N, A, A2, A3, B, B2, B3, C, D)      \
    /* setup s_  */                                     \
    AS2(    movd  ebp, mm1                          )   \
    ASMG1(A, A2, A3)                                    \
    ASMG2(B, B2, B3)                                    \
    /* setup k  */                                      \
    AS2(    movd  ebp, mm0                          )   \
    /* x += y   */                                      \
    AS2(    add   esi, ecx                          )   \
    AS2(    add   ebp, 32                           )   \
    /* y += x + k[2 * (n) + 1] */                       \
    AS2(    add   ecx, esi                          )   \
    AS2(    rol   D,   1                            )   \
    AS2(    add   ecx, DWORD PTR [ebp + 8 * N + 4]  )   \
	/* (d) = rotlFixed(d, 1) ^ y  */                    \
    AS2(    xor   D,   ecx                          )   \
    AS2(    movd  ecx, mm7                          )   \
	/* (c) ^= x + k[2 * (n)] */                         \
    AS2(    mov   edi, esi                          )   \
    AS2(    add   edi, DWORD PTR [ebp + 8 * N]      )   \
    AS2(    xor   C,   edi                          )   \
	/* (c) = rotrFixed(c, 1) */                         \
    AS2(    ror   C,   1                            )


    // decrypt Round (n), 
    // x = esi, k = ebp, edi open
    // y is in ecx from G2, restore ecx from mm7 when done
#define ASMDECROUND(N, A, A2, A3, B, B2, B3, C, D)      \
    /* setup s_  */                                     \
    AS2(    movd  ebp, mm1                          )   \
    ASMG1(A, A2, A3)                                    \
    ASMG2(B, B2, B3)                                    \
    /* setup k  */                                      \
    AS2(    movd  ebp, mm0                          )   \
    /* x += y   */                                      \
    AS2(    add   esi, ecx                          )   \
    AS2(    add   ebp, 32                           )   \
    /* y += x     */                                    \
    AS2(    add   ecx, esi                          )   \
	/* (d) ^= y + k[2 * (n) + 1] */                     \
    AS2(    mov   edi, DWORD PTR [ebp + 8 * N + 4]  )   \
    AS2(    add   edi, ecx                          )   \
    AS2(    movd  ecx, mm7                          )   \
    AS2(    xor   D,   edi                          )   \
	/* (d) = rotrFixed(d, 1)     */                     \
    AS2(    ror   D,   1                            )   \
	/* (c) = rotlFixed(c, 1)     */                     \
    AS2(    rol   C,   1                            )   \
	/* (c) ^= (x + k[2 * (n)])   */                     \
    AS2(    mov   edi, esi                          )   \
    AS2(    add   edi, DWORD PTR [ebp + 8 * N]      )   \
    AS2(    xor   C,   edi                          )


#ifdef _MSC_VER
    __declspec(naked) 
#endif
void Twofish::AsmEncrypt(const byte* inBlock, byte* outBlock) const
{
    PROLOG()

    #ifdef OLD_GCC_OFFSET
        AS2(    add   edi, 60                       ) // k_
    #else
        AS2(    add   edi, 56                       ) // k_
    #endif

    AS2(    mov   ebp, edi                      )

    AS2(    mov   eax, DWORD PTR [esi]          ) // a
    AS2(    movd  mm0, edi                      ) // store k_
    AS2(    mov   ebx, DWORD PTR [esi +  4]     ) // b
    AS2(    add   ebp, 160                      ) // s_[0]
    AS2(    mov   ecx, DWORD PTR [esi +  8]     ) // c
    AS2(    movd  mm1, ebp                      ) // store s_
    AS2(    mov   edx, DWORD PTR [esi + 12]     ) // d
    
    AS2(    xor   eax, DWORD PTR [edi]          ) // k_[0]
    AS2(    xor   ebx, DWORD PTR [edi +  4]     ) //   [1]
    AS2(    xor   ecx, DWORD PTR [edi +  8]     ) //   [2]
    AS2(    xor   edx, DWORD PTR [edi + 12]     ) //   [3]


    ASMENCROUND( 0, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND( 1, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMENCROUND( 2, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND( 3, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMENCROUND( 4, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND( 5, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMENCROUND( 6, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND( 7, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMENCROUND( 8, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND( 9, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMENCROUND(10, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND(11, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMENCROUND(12, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND(13, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMENCROUND(14, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMENCROUND(15, ecx, cl, ch, edx, dl, dh, eax, ebx)


    AS2(    movd  ebp, mm6                      )
    AS2(    movd  esi, mm0                      ) // k_
    #ifdef __GNUC__
        AS2(    mov   edi, [ebp + 16]           ) // outBlock
    #else
        AS2(    mov   edi, [ebp + 12]           ) // outBlock
    #endif

    AS2(    xor   ecx, DWORD PTR [esi + 16]     ) // k_[4]
    AS2(    xor   edx, DWORD PTR [esi + 20]     ) // k_[5]
    AS2(    xor   eax, DWORD PTR [esi + 24]     ) // k_[6]
    AS2(    xor   ebx, DWORD PTR [esi + 28]     ) // k_[7]

    AS2(    mov   [edi],      ecx               ) // write out
    AS2(    mov   [edi +  4], edx               ) // write out
    AS2(    mov   [edi +  8], eax               ) // write out
    AS2(    mov   [edi + 12], ebx               ) // write out


    EPILOG()
}


#ifdef _MSC_VER
    __declspec(naked) 
#endif
void Twofish::AsmDecrypt(const byte* inBlock, byte* outBlock) const
{
    PROLOG()

    #ifdef OLD_GCC_OFFSET
        AS2(    add   edi, 60                       ) // k_
    #else
        AS2(    add   edi, 56                       ) // k_
    #endif

    AS2(    mov   ebp, edi                      )

    AS2(    mov   ecx, DWORD PTR [esi]          ) // c
    AS2(    movd  mm0, edi                      ) // store k_
    AS2(    mov   edx, DWORD PTR [esi +  4]     ) // d
    AS2(    add   ebp, 160                      ) // s_[0]
    AS2(    mov   eax, DWORD PTR [esi +  8]     ) // a
    AS2(    movd  mm1, ebp                      ) // store s_
    AS2(    mov   ebx, DWORD PTR [esi + 12]     ) // b

    AS2(    xor   ecx, DWORD PTR [edi + 16]     ) // k_[4]
    AS2(    xor   edx, DWORD PTR [edi + 20]     ) //   [5]
    AS2(    xor   eax, DWORD PTR [edi + 24]     ) //   [6]
    AS2(    xor   ebx, DWORD PTR [edi + 28]     ) //   [7]


    ASMDECROUND(15, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND(14, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMDECROUND(13, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND(12, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMDECROUND(11, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND(10, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMDECROUND( 9, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND( 8, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMDECROUND( 7, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND( 6, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMDECROUND( 5, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND( 4, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMDECROUND( 3, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND( 2, eax, al, ah, ebx, bl, bh, ecx, edx)
    ASMDECROUND( 1, ecx, cl, ch, edx, dl, dh, eax, ebx)
    ASMDECROUND( 0, eax, al, ah, ebx, bl, bh, ecx, edx)


    AS2(    movd  ebp, mm6                      )
    AS2(    movd  esi, mm0                      ) // k_
    #ifdef __GNUC__
        AS2(    mov   edi, [ebp + 16]           ) // outBlock
    #else
        AS2(    mov   edi, [ebp + 12]           ) // outBlock
    #endif

    AS2(    xor   eax, DWORD PTR [esi     ]     ) // k_[0]
    AS2(    xor   ebx, DWORD PTR [esi +  4]     ) // k_[1]
    AS2(    xor   ecx, DWORD PTR [esi +  8]     ) // k_[2]
    AS2(    xor   edx, DWORD PTR [esi + 12]     ) // k_[3]

    AS2(    mov   [edi],      eax               ) // write out
    AS2(    mov   [edi +  4], ebx               ) // write out
    AS2(    mov   [edi +  8], ecx               ) // write out
    AS2(    mov   [edi + 12], edx               ) // write out


    EPILOG()
}



#endif // defined(DO_TWOFISH_ASM)





} // namespace


