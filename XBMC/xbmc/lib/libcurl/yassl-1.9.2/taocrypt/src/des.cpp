/* des.cpp                                
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

/* C++ part based on Wei Dai's des.cpp from CryptoPP */
/* x86 asm is original */


#if defined(TAOCRYPT_KERNEL_MODE)
    #define DO_TAOCRYPT_KERNEL_MODE
#endif                                  // only some modules now support this


#include "runtime.hpp"
#include "des.hpp"
#ifdef USE_SYS_STL
    #include <algorithm>
#else
    #include "algorithm.hpp"
#endif


namespace STL = STL_NAMESPACE;



namespace TaoCrypt {


/* permuted choice table (key) */
static const byte pc1[] = {
       57, 49, 41, 33, 25, 17,  9,
        1, 58, 50, 42, 34, 26, 18,
       10,  2, 59, 51, 43, 35, 27,
       19, 11,  3, 60, 52, 44, 36,

       63, 55, 47, 39, 31, 23, 15,
        7, 62, 54, 46, 38, 30, 22,
       14,  6, 61, 53, 45, 37, 29,
       21, 13,  5, 28, 20, 12,  4
};

/* number left rotations of pc1 */
static const byte totrot[] = {
       1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28
};

/* permuted choice key (table) */
static const byte pc2[] = {
       14, 17, 11, 24,  1,  5,
        3, 28, 15,  6, 21, 10,
       23, 19, 12,  4, 26,  8,
       16,  7, 27, 20, 13,  2,
       41, 52, 31, 37, 47, 55,
       30, 40, 51, 45, 33, 48,
       44, 49, 39, 56, 34, 53,
       46, 42, 50, 36, 29, 32
};

/* End of DES-defined tables */

/* bit 0 is left-most in byte */
static const int bytebit[] = {
       0200,0100,040,020,010,04,02,01
};

const word32 Spbox[8][64] = {
{
0x01010400,0x00000000,0x00010000,0x01010404,
0x01010004,0x00010404,0x00000004,0x00010000,
0x00000400,0x01010400,0x01010404,0x00000400,
0x01000404,0x01010004,0x01000000,0x00000004,
0x00000404,0x01000400,0x01000400,0x00010400,
0x00010400,0x01010000,0x01010000,0x01000404,
0x00010004,0x01000004,0x01000004,0x00010004,
0x00000000,0x00000404,0x00010404,0x01000000,
0x00010000,0x01010404,0x00000004,0x01010000,
0x01010400,0x01000000,0x01000000,0x00000400,
0x01010004,0x00010000,0x00010400,0x01000004,
0x00000400,0x00000004,0x01000404,0x00010404,
0x01010404,0x00010004,0x01010000,0x01000404,
0x01000004,0x00000404,0x00010404,0x01010400,
0x00000404,0x01000400,0x01000400,0x00000000,
0x00010004,0x00010400,0x00000000,0x01010004},
{
0x80108020,0x80008000,0x00008000,0x00108020,
0x00100000,0x00000020,0x80100020,0x80008020,
0x80000020,0x80108020,0x80108000,0x80000000,
0x80008000,0x00100000,0x00000020,0x80100020,
0x00108000,0x00100020,0x80008020,0x00000000,
0x80000000,0x00008000,0x00108020,0x80100000,
0x00100020,0x80000020,0x00000000,0x00108000,
0x00008020,0x80108000,0x80100000,0x00008020,
0x00000000,0x00108020,0x80100020,0x00100000,
0x80008020,0x80100000,0x80108000,0x00008000,
0x80100000,0x80008000,0x00000020,0x80108020,
0x00108020,0x00000020,0x00008000,0x80000000,
0x00008020,0x80108000,0x00100000,0x80000020,
0x00100020,0x80008020,0x80000020,0x00100020,
0x00108000,0x00000000,0x80008000,0x00008020,
0x80000000,0x80100020,0x80108020,0x00108000},
{
0x00000208,0x08020200,0x00000000,0x08020008,
0x08000200,0x00000000,0x00020208,0x08000200,
0x00020008,0x08000008,0x08000008,0x00020000,
0x08020208,0x00020008,0x08020000,0x00000208,
0x08000000,0x00000008,0x08020200,0x00000200,
0x00020200,0x08020000,0x08020008,0x00020208,
0x08000208,0x00020200,0x00020000,0x08000208,
0x00000008,0x08020208,0x00000200,0x08000000,
0x08020200,0x08000000,0x00020008,0x00000208,
0x00020000,0x08020200,0x08000200,0x00000000,
0x00000200,0x00020008,0x08020208,0x08000200,
0x08000008,0x00000200,0x00000000,0x08020008,
0x08000208,0x00020000,0x08000000,0x08020208,
0x00000008,0x00020208,0x00020200,0x08000008,
0x08020000,0x08000208,0x00000208,0x08020000,
0x00020208,0x00000008,0x08020008,0x00020200},
{
0x00802001,0x00002081,0x00002081,0x00000080,
0x00802080,0x00800081,0x00800001,0x00002001,
0x00000000,0x00802000,0x00802000,0x00802081,
0x00000081,0x00000000,0x00800080,0x00800001,
0x00000001,0x00002000,0x00800000,0x00802001,
0x00000080,0x00800000,0x00002001,0x00002080,
0x00800081,0x00000001,0x00002080,0x00800080,
0x00002000,0x00802080,0x00802081,0x00000081,
0x00800080,0x00800001,0x00802000,0x00802081,
0x00000081,0x00000000,0x00000000,0x00802000,
0x00002080,0x00800080,0x00800081,0x00000001,
0x00802001,0x00002081,0x00002081,0x00000080,
0x00802081,0x00000081,0x00000001,0x00002000,
0x00800001,0x00002001,0x00802080,0x00800081,
0x00002001,0x00002080,0x00800000,0x00802001,
0x00000080,0x00800000,0x00002000,0x00802080},
{
0x00000100,0x02080100,0x02080000,0x42000100,
0x00080000,0x00000100,0x40000000,0x02080000,
0x40080100,0x00080000,0x02000100,0x40080100,
0x42000100,0x42080000,0x00080100,0x40000000,
0x02000000,0x40080000,0x40080000,0x00000000,
0x40000100,0x42080100,0x42080100,0x02000100,
0x42080000,0x40000100,0x00000000,0x42000000,
0x02080100,0x02000000,0x42000000,0x00080100,
0x00080000,0x42000100,0x00000100,0x02000000,
0x40000000,0x02080000,0x42000100,0x40080100,
0x02000100,0x40000000,0x42080000,0x02080100,
0x40080100,0x00000100,0x02000000,0x42080000,
0x42080100,0x00080100,0x42000000,0x42080100,
0x02080000,0x00000000,0x40080000,0x42000000,
0x00080100,0x02000100,0x40000100,0x00080000,
0x00000000,0x40080000,0x02080100,0x40000100},
{
0x20000010,0x20400000,0x00004000,0x20404010,
0x20400000,0x00000010,0x20404010,0x00400000,
0x20004000,0x00404010,0x00400000,0x20000010,
0x00400010,0x20004000,0x20000000,0x00004010,
0x00000000,0x00400010,0x20004010,0x00004000,
0x00404000,0x20004010,0x00000010,0x20400010,
0x20400010,0x00000000,0x00404010,0x20404000,
0x00004010,0x00404000,0x20404000,0x20000000,
0x20004000,0x00000010,0x20400010,0x00404000,
0x20404010,0x00400000,0x00004010,0x20000010,
0x00400000,0x20004000,0x20000000,0x00004010,
0x20000010,0x20404010,0x00404000,0x20400000,
0x00404010,0x20404000,0x00000000,0x20400010,
0x00000010,0x00004000,0x20400000,0x00404010,
0x00004000,0x00400010,0x20004010,0x00000000,
0x20404000,0x20000000,0x00400010,0x20004010},
{
0x00200000,0x04200002,0x04000802,0x00000000,
0x00000800,0x04000802,0x00200802,0x04200800,
0x04200802,0x00200000,0x00000000,0x04000002,
0x00000002,0x04000000,0x04200002,0x00000802,
0x04000800,0x00200802,0x00200002,0x04000800,
0x04000002,0x04200000,0x04200800,0x00200002,
0x04200000,0x00000800,0x00000802,0x04200802,
0x00200800,0x00000002,0x04000000,0x00200800,
0x04000000,0x00200800,0x00200000,0x04000802,
0x04000802,0x04200002,0x04200002,0x00000002,
0x00200002,0x04000000,0x04000800,0x00200000,
0x04200800,0x00000802,0x00200802,0x04200800,
0x00000802,0x04000002,0x04200802,0x04200000,
0x00200800,0x00000000,0x00000002,0x04200802,
0x00000000,0x00200802,0x04200000,0x00000800,
0x04000002,0x04000800,0x00000800,0x00200002},
{
0x10001040,0x00001000,0x00040000,0x10041040,
0x10000000,0x10001040,0x00000040,0x10000000,
0x00040040,0x10040000,0x10041040,0x00041000,
0x10041000,0x00041040,0x00001000,0x00000040,
0x10040000,0x10000040,0x10001000,0x00001040,
0x00041000,0x00040040,0x10040040,0x10041000,
0x00001040,0x00000000,0x00000000,0x10040040,
0x10000040,0x10001000,0x00041040,0x00040000,
0x00041040,0x00040000,0x10041000,0x00001000,
0x00000040,0x10040040,0x00001000,0x00041040,
0x10001000,0x00000040,0x10000040,0x10040000,
0x10040040,0x10000000,0x00040000,0x10001040,
0x00000000,0x10041040,0x00040040,0x10000040,
0x10040000,0x10001000,0x10001040,0x00000000,
0x10041040,0x00041000,0x00041000,0x00001040,
0x00001040,0x00040040,0x10000000,0x10041000}
};


void BasicDES::SetKey(const byte* key, word32 /*length*/, CipherDir dir)
{
    byte buffer[56+56+8];
    byte *const pc1m = buffer;                 /* place to modify pc1 into */
    byte *const pcr = pc1m + 56;               /* place to rotate pc1 into */
    byte *const ks = pcr + 56;
    register int i,j,l;
    int m;

    for (j = 0; j < 56; j++) {          /* convert pc1 to bits of key */
        l = pc1[j] - 1;                 /* integer bit location  */
        m = l & 07;                     /* find bit              */
        pc1m[j] = (key[l >> 3] &        /* find which key byte l is in */
            bytebit[m])                 /* and which bit of that byte */
            ? 1 : 0;                    /* and store 1-bit result */
    }
    for (i = 0; i < 16; i++) {          /* key chunk for each iteration */
        memset(ks, 0, 8);               /* Clear key schedule */
        for (j = 0; j < 56; j++)        /* rotate pc1 the right amount */
            pcr[j] = pc1m[(l = j + totrot[i]) < (j < 28 ? 28 : 56) ? l: l-28];
        /* rotate left and right halves independently */
        for (j = 0; j < 48; j++){   /* select bits individually */
            /* check bit that goes to ks[j] */
            if (pcr[pc2[j] - 1]){
                /* mask it in if it's there */
                l= j % 6;
                ks[j/6] |= bytebit[l] >> 2;
            }
        }
        /* Now convert to odd/even interleaved form for use in F */
        k_[2*i] = ((word32)ks[0] << 24)
            | ((word32)ks[2] << 16)
            | ((word32)ks[4] << 8)
            | ((word32)ks[6]);
        k_[2*i + 1] = ((word32)ks[1] << 24)
            | ((word32)ks[3] << 16)
            | ((word32)ks[5] << 8)
            | ((word32)ks[7]);
    }
    
    // reverse key schedule order
    if (dir == DECRYPTION)
        for (i = 0; i < 16; i += 2) {
            STL::swap(k_[i],   k_[32 - 2 - i]);
            STL::swap(k_[i+1], k_[32 - 1 - i]);
        }
   
}

static inline void IPERM(word32& left, word32& right)
{
    word32 work;

    right = rotlFixed(right, 4U);
    work = (left ^ right) & 0xf0f0f0f0;
    left ^= work;

    right = rotrFixed(right^work, 20U);
    work = (left ^ right) & 0xffff0000;
    left ^= work;

    right = rotrFixed(right^work, 18U);
    work = (left ^ right) & 0x33333333;
    left ^= work;

    right = rotrFixed(right^work, 6U);
    work = (left ^ right) & 0x00ff00ff;
    left ^= work;

    right = rotlFixed(right^work, 9U);
    work = (left ^ right) & 0xaaaaaaaa;
    left = rotlFixed(left^work, 1U);
    right ^= work;
}

static inline void FPERM(word32& left, word32& right)
{
    word32 work;

    right = rotrFixed(right, 1U);
    work = (left ^ right) & 0xaaaaaaaa;
    right ^= work;
    left = rotrFixed(left^work, 9U);
    work = (left ^ right) & 0x00ff00ff;
    right ^= work;
    left = rotlFixed(left^work, 6U);
    work = (left ^ right) & 0x33333333;
    right ^= work;
    left = rotlFixed(left^work, 18U);
    work = (left ^ right) & 0xffff0000;
    right ^= work;
    left = rotlFixed(left^work, 20U);
    work = (left ^ right) & 0xf0f0f0f0;
    right ^= work;
    left = rotrFixed(left^work, 4U);
}


void BasicDES::RawProcessBlock(word32& lIn, word32& rIn) const
{
    word32 l = lIn, r = rIn;
    const word32* kptr = k_;

    for (unsigned i=0; i<8; i++)
    {
        word32 work = rotrFixed(r, 4U) ^ kptr[4*i+0];
        l ^= Spbox[6][(work) & 0x3f]
          ^  Spbox[4][(work >> 8) & 0x3f]
          ^  Spbox[2][(work >> 16) & 0x3f]
          ^  Spbox[0][(work >> 24) & 0x3f];
        work = r ^ kptr[4*i+1];
        l ^= Spbox[7][(work) & 0x3f]
          ^  Spbox[5][(work >> 8) & 0x3f]
          ^  Spbox[3][(work >> 16) & 0x3f]
          ^  Spbox[1][(work >> 24) & 0x3f];

        work = rotrFixed(l, 4U) ^ kptr[4*i+2];
        r ^= Spbox[6][(work) & 0x3f]
          ^  Spbox[4][(work >> 8) & 0x3f]
          ^  Spbox[2][(work >> 16) & 0x3f]
          ^  Spbox[0][(work >> 24) & 0x3f];
        work = l ^ kptr[4*i+3];
        r ^= Spbox[7][(work) & 0x3f]
          ^  Spbox[5][(work >> 8) & 0x3f]
          ^  Spbox[3][(work >> 16) & 0x3f]
          ^  Spbox[1][(work >> 24) & 0x3f];
    }

    lIn = l; rIn = r;
}



typedef BlockGetAndPut<word32, BigEndian> Block;


void DES::ProcessAndXorBlock(const byte* in, const byte* xOr, byte* out) const
{
    word32 l,r;
    Block::Get(in)(l)(r);
    IPERM(l,r);

    RawProcessBlock(l, r);

    FPERM(l,r);
    Block::Put(xOr, out)(r)(l);
}


void DES_EDE2::SetKey(const byte* key, word32 sz, CipherDir dir)
{
    des1_.SetKey(key, sz, dir);
    des2_.SetKey(key + 8, sz, ReverseDir(dir));
}


void DES_EDE2::ProcessAndXorBlock(const byte* in, const byte* xOr,
                                  byte* out) const
{
    word32 l,r;
    Block::Get(in)(l)(r);
    IPERM(l,r);

    des1_.RawProcessBlock(l, r);
    des2_.RawProcessBlock(r, l);
    des1_.RawProcessBlock(l, r);

    FPERM(l,r);
    Block::Put(xOr, out)(r)(l);
}


void DES_EDE3::SetKey(const byte* key, word32 sz, CipherDir dir)
{
    des1_.SetKey(key+(dir==ENCRYPTION?0:2*8), sz, dir);
    des2_.SetKey(key+8, sz, ReverseDir(dir));
    des3_.SetKey(key+(dir==DECRYPTION?0:2*8), sz, dir);
}



#if defined(DO_DES_ASM)

// ia32 optimized version
void DES_EDE3::Process(byte* out, const byte* in, word32 sz)
{
    if (!isMMX) {
        Mode_BASE::Process(out, in, sz);
        return;
    }

    word32 blocks = sz / DES_BLOCK_SIZE;

    if (mode_ == CBC)    
        if (dir_ == ENCRYPTION)
            while (blocks--) {
                r_[0] ^= *(word32*)in;
                r_[1] ^= *(word32*)(in + 4);

                AsmProcess((byte*)r_, (byte*)r_, (void*)Spbox);
                
                memcpy(out, r_, DES_BLOCK_SIZE);

                in  += DES_BLOCK_SIZE;
                out += DES_BLOCK_SIZE;
            }
        else
            while (blocks--) {
                AsmProcess(in, out, (void*)Spbox);
               
                *(word32*)out       ^= r_[0];
                *(word32*)(out + 4) ^= r_[1];

                memcpy(r_, in, DES_BLOCK_SIZE);

                out += DES_BLOCK_SIZE;
                in  += DES_BLOCK_SIZE;
            }
    else
        while (blocks--) {
            AsmProcess(in, out, (void*)Spbox);
           
            out += DES_BLOCK_SIZE;
            in  += DES_BLOCK_SIZE;
        }
}

#endif // DO_DES_ASM


void DES_EDE3::ProcessAndXorBlock(const byte* in, const byte* xOr,
                                  byte* out) const
{
    word32 l,r;
    Block::Get(in)(l)(r);
    IPERM(l,r);

    des1_.RawProcessBlock(l, r);
    des2_.RawProcessBlock(r, l);
    des3_.RawProcessBlock(l, r);

    FPERM(l,r);
    Block::Put(xOr, out)(r)(l);
}


#if defined(DO_DES_ASM)

/* Uses IPERM algorithm from above

   left  is in eax
   right is in ebx

   uses ecx
*/
#define AsmIPERM() {\
    AS2(    rol   ebx, 4                        )   \
    AS2(    mov   ecx, eax                      )   \
    AS2(    xor   ecx, ebx                      )   \
    AS2(    and   ecx, 0xf0f0f0f0               )   \
    AS2(    xor   ebx, ecx                      )   \
    AS2(    xor   eax, ecx                      )   \
    AS2(    ror   ebx, 20                       )   \
    AS2(    mov   ecx, eax                      )   \
    AS2(    xor   ecx, ebx                      )   \
    AS2(    and   ecx, 0xffff0000               )   \
    AS2(    xor   ebx, ecx                      )   \
    AS2(    xor   eax, ecx                      )   \
    AS2(    ror   ebx, 18                       )   \
    AS2(    mov   ecx, eax                      )   \
    AS2(    xor   ecx, ebx                      )   \
    AS2(    and   ecx, 0x33333333               )   \
    AS2(    xor   ebx, ecx                      )   \
    AS2(    xor   eax, ecx                      )   \
    AS2(    ror   ebx, 6                        )   \
    AS2(    mov   ecx, eax                      )   \
    AS2(    xor   ecx, ebx                      )   \
    AS2(    and   ecx, 0x00ff00ff               )   \
    AS2(    xor   ebx, ecx                      )   \
    AS2(    xor   eax, ecx                      )   \
    AS2(    rol   ebx, 9                        )   \
    AS2(    mov   ecx, eax                      )   \
    AS2(    xor   ecx, ebx                      )   \
    AS2(    and   ecx, 0xaaaaaaaa               )   \
    AS2(    xor   eax, ecx                      )   \
    AS2(    rol   eax, 1                        )   \
    AS2(    xor   ebx, ecx                      ) }


/* Uses FPERM algorithm from above

   left  is in eax
   right is in ebx

   uses ecx
*/
#define AsmFPERM()    {\
    AS2(    ror  ebx, 1                     )    \
    AS2(    mov  ecx, eax                   )    \
    AS2(    xor  ecx, ebx                   )    \
    AS2(    and  ecx, 0xaaaaaaaa            )    \
    AS2(    xor  eax, ecx                   )    \
    AS2(    xor  ebx, ecx                   )    \
    AS2(    ror  eax, 9                     )    \
    AS2(    mov  ecx, ebx                   )    \
    AS2(    xor  ecx, eax                   )    \
    AS2(    and  ecx, 0x00ff00ff            )    \
    AS2(    xor  eax, ecx                   )    \
    AS2(    xor  ebx, ecx                   )    \
    AS2(    rol  eax, 6                     )    \
    AS2(    mov  ecx, ebx                   )    \
    AS2(    xor  ecx, eax                   )    \
    AS2(    and  ecx, 0x33333333            )    \
    AS2(    xor  eax, ecx                   )    \
    AS2(    xor  ebx, ecx                   )    \
    AS2(    rol  eax, 18                    )    \
    AS2(    mov  ecx, ebx                   )    \
    AS2(    xor  ecx, eax                   )    \
    AS2(    and  ecx, 0xffff0000            )    \
    AS2(    xor  eax, ecx                   )    \
    AS2(    xor  ebx, ecx                   )    \
    AS2(    rol  eax, 20                    )    \
    AS2(    mov  ecx, ebx                   )    \
    AS2(    xor  ecx, eax                   )    \
    AS2(    and  ecx, 0xf0f0f0f0            )    \
    AS2(    xor  eax, ecx                   )    \
    AS2(    xor  ebx, ecx                   )    \
    AS2(    ror  eax, 4                     ) }




/* DesRound implements this algorithm:

        word32 work = rotrFixed(r, 4U) ^ key[0];
        l ^= Spbox[6][(work) & 0x3f]
          ^  Spbox[4][(work >> 8) & 0x3f]
          ^  Spbox[2][(work >> 16) & 0x3f]
          ^  Spbox[0][(work >> 24) & 0x3f];
        work = r ^ key[1];
        l ^= Spbox[7][(work) & 0x3f]
          ^  Spbox[5][(work >> 8) & 0x3f]
          ^  Spbox[3][(work >> 16) & 0x3f]
          ^  Spbox[1][(work >> 24) & 0x3f];

        work = rotrFixed(l, 4U) ^ key[2];
        r ^= Spbox[6][(work) & 0x3f]
          ^  Spbox[4][(work >> 8) & 0x3f]
          ^  Spbox[2][(work >> 16) & 0x3f]
          ^  Spbox[0][(work >> 24) & 0x3f];
        work = l ^ key[3];
        r ^= Spbox[7][(work) & 0x3f]
          ^  Spbox[5][(work >> 8) & 0x3f]
          ^  Spbox[3][(work >> 16) & 0x3f]
          ^  Spbox[1][(work >> 24) & 0x3f];

   left  is in aex
   right is in ebx
   key   is in edx

   edvances key for next round

   uses ecx, esi, and edi
*/
#define DesRound() \
    AS2(    mov   ecx,  ebx                     )\
    AS2(    mov   esi,  DWORD PTR [edx]         )\
    AS2(    ror   ecx,  4                       )\
    AS2(    xor   ecx,  esi                     )\
    AS2(    and   ecx,  0x3f3f3f3f              )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   eax,  [ebp + esi*4 + 6*256]   )\
    AS2(    shr   ecx,  16                      )\
    AS2(    xor   eax,  [ebp + edi*4 + 4*256]   )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   eax,  [ebp + esi*4 + 2*256]   )\
    AS2(    mov   esi,  DWORD PTR [edx + 4]     )\
    AS2(    xor   eax,  [ebp + edi*4]           )\
    AS2(    mov   ecx,  ebx                     )\
    AS2(    xor   ecx,  esi                     )\
    AS2(    and   ecx,  0x3f3f3f3f              )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   eax,  [ebp + esi*4 + 7*256]   )\
    AS2(    shr   ecx,  16                      )\
    AS2(    xor   eax,  [ebp + edi*4 + 5*256]   )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   eax,  [ebp + esi*4 + 3*256]   )\
    AS2(    mov   esi,  DWORD PTR [edx + 8]     )\
    AS2(    xor   eax,  [ebp + edi*4 + 1*256]   )\
    AS2(    mov   ecx,  eax                     )\
    AS2(    ror   ecx,  4                       )\
    AS2(    xor   ecx,  esi                     )\
    AS2(    and   ecx,  0x3f3f3f3f              )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   ebx,  [ebp + esi*4 + 6*256]   )\
    AS2(    shr   ecx,  16                      )\
    AS2(    xor   ebx,  [ebp + edi*4 + 4*256]   )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   ebx,  [ebp + esi*4 + 2*256]   )\
    AS2(    mov   esi,  DWORD PTR [edx + 12]    )\
    AS2(    xor   ebx,  [ebp + edi*4]           )\
    AS2(    mov   ecx,  eax                     )\
    AS2(    xor   ecx,  esi                     )\
    AS2(    and   ecx,  0x3f3f3f3f              )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   ebx,  [ebp + esi*4 + 7*256]   )\
    AS2(    shr   ecx,  16                      )\
    AS2(    xor   ebx,  [ebp + edi*4 + 5*256]   )\
    AS2(    movzx esi,  cl                      )\
    AS2(    movzx edi,  ch                      )\
    AS2(    xor   ebx,  [ebp + esi*4 + 3*256]   )\
    AS2(    add   edx,  16                      )\
    AS2(    xor   ebx,  [ebp + edi*4 + 1*256]   )


#ifdef _MSC_VER
    __declspec(naked) 
#endif
void DES_EDE3::AsmProcess(const byte* in, byte* out, void* box) const
{
#ifdef __GNUC__
    #define AS1(x)    asm(#x);
    #define AS2(x, y) asm(#x ", " #y);

    asm(".intel_syntax noprefix");

    #define PROLOG()  \
        AS2(    movd  mm3, edi                      )   \
        AS2(    movd  mm4, ebx                      )   \
        AS2(    movd  mm5, esi                      )   \
        AS2(    movd  mm6, ebp                      )   \
        AS2(    mov   edx, DWORD PTR [ebp +  8]     )   \
        AS2(    mov   esi, DWORD PTR [ebp + 12]     )   \
        AS2(    mov   ebp, DWORD PTR [ebp + 20]     )

    // ebp restored at end
    #define EPILOG()    \
        AS2(    movd  edi, mm3                      )   \
        AS2(    movd  ebx, mm4                      )   \
        AS2(    movd  esi, mm5                      )   \
        AS1(    emms                                )   \
        asm(".att_syntax");

#else
    #define AS1(x)      __asm x
    #define AS2(x, y)   __asm x, y

    #define PROLOG()  \
        AS1(    push  ebp                           )   \
        AS2(    mov   ebp, esp                      )   \
        AS2(    movd  mm3, edi                      )   \
        AS2(    movd  mm4, ebx                      )   \
        AS2(    movd  mm5, esi                      )   \
        AS2(    movd  mm6, ebp                      )   \
        AS2(    mov   esi, DWORD PTR [ebp +  8]     )   \
        AS2(    mov   edx, ecx                      )   \
        AS2(    mov   ebp, DWORD PTR [ebp + 16]     )

    // ebp restored at end
    #define EPILOG() \
        AS2(    movd  edi, mm3                      )   \
        AS2(    movd  ebx, mm4                      )   \
        AS2(    movd  esi, mm5                      )   \
        AS2(    mov   esp, ebp                      )   \
        AS1(    pop   ebp                           )   \
        AS1(    emms                                )   \
        AS1(    ret 12                              )

#endif


    PROLOG()

    AS2(    movd  mm2, edx                      )

    #ifdef OLD_GCC_OFFSET
        AS2(    add   edx, 60                       )   // des1 = des1 key
    #else
        AS2(    add   edx, 56                       )   // des1 = des1 key
    #endif

    AS2(    mov   eax, DWORD PTR [esi]          )
    AS2(    mov   ebx, DWORD PTR [esi + 4]      )
    AS1(    bswap eax                           )    // left
    AS1(    bswap ebx                           )    // right

    AsmIPERM()

    DesRound() // 1
    DesRound() // 2
    DesRound() // 3
    DesRound() // 4
    DesRound() // 5
    DesRound() // 6
    DesRound() // 7
    DesRound() // 8

    // swap left and right 
    AS2(    xchg  eax, ebx                      )

    DesRound() // 1
    DesRound() // 2
    DesRound() // 3
    DesRound() // 4
    DesRound() // 5
    DesRound() // 6
    DesRound() // 7
    DesRound() // 8

    // swap left and right
    AS2(    xchg  eax, ebx                      )

    DesRound() // 1
    DesRound() // 2
    DesRound() // 3
    DesRound() // 4
    DesRound() // 5
    DesRound() // 6
    DesRound() // 7
    DesRound() // 8

    AsmFPERM()

    //end
    AS2(    movd  ebp, mm6                      )

    // swap and write out
    AS1(    bswap ebx                           )
    AS1(    bswap eax                           )

#ifdef __GNUC__
    AS2(    mov   esi, DWORD PTR [ebp +  16]    )   // outBlock
#else
    AS2(    mov   esi, DWORD PTR [ebp +  12]    )   // outBlock
#endif

    AS2(    mov   DWORD PTR [esi],     ebx      )   // right first
    AS2(    mov   DWORD PTR [esi + 4], eax      )
    

    EPILOG()
}



#endif // defined(DO_DES_ASM)


}  // namespace
