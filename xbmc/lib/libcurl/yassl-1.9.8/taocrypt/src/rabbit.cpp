/* rabbit.cpp                                
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


#include "runtime.hpp"
#include "rabbit.hpp"



namespace TaoCrypt {


#define U32V(x)  (word32)(x)


#ifdef BIG_ENDIAN_ORDER
    #define LITTLE32(x) ByteReverse((word32)x)
#else
    #define LITTLE32(x) (x)
#endif


// local
namespace {


/* Square a 32-bit unsigned integer to obtain the 64-bit result and return */
/* the upper 32 bits XOR the lower 32 bits */
word32 RABBIT_g_func(word32 x)
{
    /* Temporary variables */
    word32 a, b, h, l;

    /* Construct high and low argument for squaring */
    a = x&0xFFFF;
    b = x>>16;

    /* Calculate high and low result of squaring */
    h = (((U32V(a*a)>>17) + U32V(a*b))>>15) + b*b;
    l = x*x;

    /* Return high XOR low */
    return U32V(h^l);
}


} // namespace local


/* Calculate the next internal state */
void Rabbit::NextState(RabbitCtx which)
{
    /* Temporary variables */
    word32 g[8], c_old[8], i;

    Ctx* ctx;

    if (which == Master)
        ctx = &masterCtx_;
    else
        ctx = &workCtx_;

    /* Save old counter values */
    for (i=0; i<8; i++)
        c_old[i] = ctx->c[i];

    /* Calculate new counter values */
    ctx->c[0] = U32V(ctx->c[0] + 0x4D34D34D + ctx->carry);
    ctx->c[1] = U32V(ctx->c[1] + 0xD34D34D3 + (ctx->c[0] < c_old[0]));
    ctx->c[2] = U32V(ctx->c[2] + 0x34D34D34 + (ctx->c[1] < c_old[1]));
    ctx->c[3] = U32V(ctx->c[3] + 0x4D34D34D + (ctx->c[2] < c_old[2]));
    ctx->c[4] = U32V(ctx->c[4] + 0xD34D34D3 + (ctx->c[3] < c_old[3]));
    ctx->c[5] = U32V(ctx->c[5] + 0x34D34D34 + (ctx->c[4] < c_old[4]));
    ctx->c[6] = U32V(ctx->c[6] + 0x4D34D34D + (ctx->c[5] < c_old[5]));
    ctx->c[7] = U32V(ctx->c[7] + 0xD34D34D3 + (ctx->c[6] < c_old[6]));
    ctx->carry = (ctx->c[7] < c_old[7]);
   
    /* Calculate the g-values */
    for (i=0;i<8;i++)
        g[i] = RABBIT_g_func(U32V(ctx->x[i] + ctx->c[i]));

    /* Calculate new state values */
    ctx->x[0] = U32V(g[0] + rotlFixed(g[7],16) + rotlFixed(g[6], 16));
    ctx->x[1] = U32V(g[1] + rotlFixed(g[0], 8) + g[7]);
    ctx->x[2] = U32V(g[2] + rotlFixed(g[1],16) + rotlFixed(g[0], 16));
    ctx->x[3] = U32V(g[3] + rotlFixed(g[2], 8) + g[1]);
    ctx->x[4] = U32V(g[4] + rotlFixed(g[3],16) + rotlFixed(g[2], 16));
    ctx->x[5] = U32V(g[5] + rotlFixed(g[4], 8) + g[3]);
    ctx->x[6] = U32V(g[6] + rotlFixed(g[5],16) + rotlFixed(g[4], 16));
    ctx->x[7] = U32V(g[7] + rotlFixed(g[6], 8) + g[5]);
}


/* IV setup */
void Rabbit::SetIV(const byte* iv)
{
    /* Temporary variables */
    word32 i0, i1, i2, i3, i;
      
    /* Generate four subvectors */
    i0 = LITTLE32(*(word32*)(iv+0));
    i2 = LITTLE32(*(word32*)(iv+4));
    i1 = (i0>>16) | (i2&0xFFFF0000);
    i3 = (i2<<16) | (i0&0x0000FFFF);

    /* Modify counter values */
    workCtx_.c[0] = masterCtx_.c[0] ^ i0;
    workCtx_.c[1] = masterCtx_.c[1] ^ i1;
    workCtx_.c[2] = masterCtx_.c[2] ^ i2;
    workCtx_.c[3] = masterCtx_.c[3] ^ i3;
    workCtx_.c[4] = masterCtx_.c[4] ^ i0;
    workCtx_.c[5] = masterCtx_.c[5] ^ i1;
    workCtx_.c[6] = masterCtx_.c[6] ^ i2;
    workCtx_.c[7] = masterCtx_.c[7] ^ i3;

    /* Copy state variables */
    for (i=0; i<8; i++)
        workCtx_.x[i] = masterCtx_.x[i];
    workCtx_.carry = masterCtx_.carry;

    /* Iterate the system four times */
    for (i=0; i<4; i++)
        NextState(Work);
}


/* Key setup */
void Rabbit::SetKey(const byte* key, const byte* iv)
{
    /* Temporary variables */
    word32 k0, k1, k2, k3, i;

    /* Generate four subkeys */
    k0 = LITTLE32(*(word32*)(key+ 0));
    k1 = LITTLE32(*(word32*)(key+ 4));
    k2 = LITTLE32(*(word32*)(key+ 8));
    k3 = LITTLE32(*(word32*)(key+12));

    /* Generate initial state variables */
    masterCtx_.x[0] = k0;
    masterCtx_.x[2] = k1;
    masterCtx_.x[4] = k2;
    masterCtx_.x[6] = k3;
    masterCtx_.x[1] = U32V(k3<<16) | (k2>>16);
    masterCtx_.x[3] = U32V(k0<<16) | (k3>>16);
    masterCtx_.x[5] = U32V(k1<<16) | (k0>>16);
    masterCtx_.x[7] = U32V(k2<<16) | (k1>>16);

    /* Generate initial counter values */
    masterCtx_.c[0] = rotlFixed(k2, 16);
    masterCtx_.c[2] = rotlFixed(k3, 16);
    masterCtx_.c[4] = rotlFixed(k0, 16);
    masterCtx_.c[6] = rotlFixed(k1, 16);
    masterCtx_.c[1] = (k0&0xFFFF0000) | (k1&0xFFFF);
    masterCtx_.c[3] = (k1&0xFFFF0000) | (k2&0xFFFF);
    masterCtx_.c[5] = (k2&0xFFFF0000) | (k3&0xFFFF);
    masterCtx_.c[7] = (k3&0xFFFF0000) | (k0&0xFFFF);

    /* Clear carry bit */
    masterCtx_.carry = 0;

    /* Iterate the system four times */
    for (i=0; i<4; i++)
        NextState(Master);

    /* Modify the counters */
    for (i=0; i<8; i++)
        masterCtx_.c[i] ^= masterCtx_.x[(i+4)&0x7];

    /* Copy master instance to work instance */
    for (i=0; i<8; i++) {
        workCtx_.x[i] = masterCtx_.x[i];
        workCtx_.c[i] = masterCtx_.c[i];
    }
    workCtx_.carry = masterCtx_.carry;

    if (iv) SetIV(iv);    
}


/* Encrypt/decrypt a message of any size */
void Rabbit::Process(byte* output, const byte* input, word32 msglen)
{
    /* Temporary variables */
    word32 i;
    byte buffer[16];

    /* Encrypt/decrypt all full blocks */
    while (msglen >= 16) {
        /* Iterate the system */
        NextState(Work);

        /* Encrypt/decrypt 16 bytes of data */
        *(word32*)(output+ 0) = *(word32*)(input+ 0) ^
                   LITTLE32(workCtx_.x[0] ^ (workCtx_.x[5]>>16) ^
                   U32V(workCtx_.x[3]<<16));
        *(word32*)(output+ 4) = *(word32*)(input+ 4) ^
                   LITTLE32(workCtx_.x[2] ^ (workCtx_.x[7]>>16) ^
                   U32V(workCtx_.x[5]<<16));
        *(word32*)(output+ 8) = *(word32*)(input+ 8) ^
                   LITTLE32(workCtx_.x[4] ^ (workCtx_.x[1]>>16) ^
                   U32V(workCtx_.x[7]<<16));
        *(word32*)(output+12) = *(word32*)(input+12) ^
                   LITTLE32(workCtx_.x[6] ^ (workCtx_.x[3]>>16) ^
                   U32V(workCtx_.x[1]<<16));

        /* Increment pointers and decrement length */
        input  += 16;
        output += 16;
        msglen -= 16;
    }

    /* Encrypt/decrypt remaining data */
    if (msglen) {
        /* Iterate the system */
        NextState(Work);

        /* Generate 16 bytes of pseudo-random data */
        *(word32*)(buffer+ 0) = LITTLE32(workCtx_.x[0] ^
                  (workCtx_.x[5]>>16) ^ U32V(workCtx_.x[3]<<16));
        *(word32*)(buffer+ 4) = LITTLE32(workCtx_.x[2] ^ 
                  (workCtx_.x[7]>>16) ^ U32V(workCtx_.x[5]<<16));
        *(word32*)(buffer+ 8) = LITTLE32(workCtx_.x[4] ^ 
                  (workCtx_.x[1]>>16) ^ U32V(workCtx_.x[7]<<16));
        *(word32*)(buffer+12) = LITTLE32(workCtx_.x[6] ^ 
                  (workCtx_.x[3]>>16) ^ U32V(workCtx_.x[1]<<16));

        /* Encrypt/decrypt the data */
        for (i=0; i<msglen; i++)
            output[i] = input[i] ^ buffer[i];
    }
}


}  // namespace
