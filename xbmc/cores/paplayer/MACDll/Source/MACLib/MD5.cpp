/*****************************************************************************
*
* "derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm".
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
*****************************************************************************/

#include "All.h"
#include <string.h>
#include "MD5.h"


#if __BYTE_ORDER == __BIG_ENDIAN
/*
 *  Block copy and convert byte order to little-endian.
 *  dst must be 32bit aligned.
 *  Length is the number of 32bit words 
 */   
static void 
CopyToLittleEndian ( uint32_t*       dst, 
                     const uint8_t*  src, 
             size_t          length ) 
{
    for ( ; length--; src += 4; dst++  ) {
    *dst = (( (uint32_t) src [3] ) << 24) |
           (( (uint32_t) src [2] ) << 16) |
           (( (uint32_t) src [1] ) <<  8) |
           (( (uint32_t) src [0] ) <<  0);
       
       
    }
}
#endif


/*
   Assembler versions of __MD5Transform, MD5Init and MD5Update
   currently exist for x86 and little-endian ARM.
   For other targets, we need to use the C versions below.
*/

//#if !(defined (__i386__) || ((defined (__arm__) && (__BYTE_ORDER == __LITTLE_ENDIAN))))

/*
   Initialise the MD5 context.
*/
void 
MD5Init ( MD5_CTX* context ) 
{
    context -> count [0] = 0;
    context -> count [1] = 0;

    context -> state [0] = 0x67452301;              /* Load magic constants. */
    context -> state [1] = 0xefcdab89;
    context -> state [2] = 0x98badcfe;
    context -> state [3] = 0x10325476;
}

#define ROTATE_LEFT(x, n)         ((x << n) | (x >> (32-n)))

#define F(x, y, z)             (z ^ (x & (y ^ z)))
#define G(x, y, z)             (y ^ (z & (x ^ y)))
#define H(x, y, z)             (x ^ y ^ z)
#define I(x, y, z)             (y ^ (x | ~z))

#define FF(a, b, c, d, x, s, ac)     { (a) += F (b, c, d) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT (a, s); (a) += (b); }
#define GG(a, b, c, d, x, s, ac)     { (a) += G (b, c, d) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT (a, s); (a) += (b); }
#define HH(a, b, c, d, x, s, ac)     { (a) += H (b, c, d) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT (a, s); (a) += (b); }
#define II(a, b, c, d, x, s, ac)     { (a) += I (b, c, d) + (x) + (uint32_t)(ac); (a) = ROTATE_LEFT (a, s); (a) += (b); }

static void 
__MD5Transform ( uint32_t        state [4], 
         const uint8_t*  in, 
         int             repeat ) 
{
    const uint32_t*  x;
    uint32_t         a = state [0];
    uint32_t         b = state [1];
    uint32_t         c = state [2];
    uint32_t         d = state [3];

    for ( ; repeat; repeat-- ) {
    uint32_t tempBuffer [16];
#if __BYTE_ORDER == __BIG_ENDIAN

    CopyToLittleEndian (tempBuffer, in, 16);
    x = tempBuffer;
#else
    if ( (unsigned long)in & 3 ) {
        memcpy ( tempBuffer, in, 64 );
        x = tempBuffer;
    } 
    else {
        x = (const uint32_t*) in;
    }
#endif

    FF (a, b, c, d, x[ 0],  7, 0xd76aa478); /*  1 */     /* Round 1 */
    FF (d, a, b, c, x[ 1], 12, 0xe8c7b756); /*  2 */
    FF (c, d, a, b, x[ 2], 17, 0x242070db); /*  3 */
    FF (b, c, d, a, x[ 3], 22, 0xc1bdceee); /*  4 */
    FF (a, b, c, d, x[ 4],  7, 0xf57c0faf); /*  5 */
    FF (d, a, b, c, x[ 5], 12, 0x4787c62a); /*  6 */
    FF (c, d, a, b, x[ 6], 17, 0xa8304613); /*  7 */
    FF (b, c, d, a, x[ 7], 22, 0xfd469501); /*  8 */
    FF (a, b, c, d, x[ 8],  7, 0x698098d8); /*  9 */
    FF (d, a, b, c, x[ 9], 12, 0x8b44f7af); /* 10 */
    FF (c, d, a, b, x[10], 17, 0xffff5bb1); /* 11 */
    FF (b, c, d, a, x[11], 22, 0x895cd7be); /* 12 */
    FF (a, b, c, d, x[12],  7, 0x6b901122); /* 13 */
    FF (d, a, b, c, x[13], 12, 0xfd987193); /* 14 */
    FF (c, d, a, b, x[14], 17, 0xa679438e); /* 15 */
    FF (b, c, d, a, x[15], 22, 0x49b40821); /* 16 */
    
    GG (a, b, c, d, x[ 1],  5, 0xf61e2562); /* 17 */     /* Round 2 */
    GG (d, a, b, c, x[ 6],  9, 0xc040b340); /* 18 */
    GG (c, d, a, b, x[11], 14, 0x265e5a51); /* 19 */
    GG (b, c, d, a, x[ 0], 20, 0xe9b6c7aa); /* 20 */
    GG (a, b, c, d, x[ 5],  5, 0xd62f105d); /* 21 */
    GG (d, a, b, c, x[10],  9, 0x02441453); /* 22 */
    GG (c, d, a, b, x[15], 14, 0xd8a1e681); /* 23 */
    GG (b, c, d, a, x[ 4], 20, 0xe7d3fbc8); /* 24 */
    GG (a, b, c, d, x[ 9],  5, 0x21e1cde6); /* 25 */
    GG (d, a, b, c, x[14],  9, 0xc33707d6); /* 26 */
    GG (c, d, a, b, x[ 3], 14, 0xf4d50d87); /* 27 */
    GG (b, c, d, a, x[ 8], 20, 0x455a14ed); /* 28 */
    GG (a, b, c, d, x[13],  5, 0xa9e3e905); /* 29 */
    GG (d, a, b, c, x[ 2],  9, 0xfcefa3f8); /* 30 */
    GG (c, d, a, b, x[ 7], 14, 0x676f02d9); /* 31 */
    GG (b, c, d, a, x[12], 20, 0x8d2a4c8a); /* 32 */
    
    HH (a, b, c, d, x[ 5],  4, 0xfffa3942); /* 33 */     /* Round 3 */
    HH (d, a, b, c, x[ 8], 11, 0x8771f681); /* 34 */
    HH (c, d, a, b, x[11], 16, 0x6d9d6122); /* 35 */
    HH (b, c, d, a, x[14], 23, 0xfde5380c); /* 36 */
    HH (a, b, c, d, x[ 1],  4, 0xa4beea44); /* 37 */
    HH (d, a, b, c, x[ 4], 11, 0x4bdecfa9); /* 38 */
    HH (c, d, a, b, x[ 7], 16, 0xf6bb4b60); /* 39 */
    HH (b, c, d, a, x[10], 23, 0xbebfbc70); /* 40 */
    HH (a, b, c, d, x[13],  4, 0x289b7ec6); /* 41 */
    HH (d, a, b, c, x[ 0], 11, 0xeaa127fa); /* 42 */
    HH (c, d, a, b, x[ 3], 16, 0xd4ef3085); /* 43 */
    HH (b, c, d, a, x[ 6], 23, 0x04881d05); /* 44 */
    HH (a, b, c, d, x[ 9],  4, 0xd9d4d039); /* 45 */
    HH (d, a, b, c, x[12], 11, 0xe6db99e5); /* 46 */
    HH (c, d, a, b, x[15], 16, 0x1fa27cf8); /* 47 */
    HH (b, c, d, a, x[ 2], 23, 0xc4ac5665); /* 48 */
    
    II (a, b, c, d, x[ 0],  6, 0xf4292244); /* 49 */     /* Round 4 */
    II (d, a, b, c, x[ 7], 10, 0x432aff97); /* 50 */
    II (c, d, a, b, x[14], 15, 0xab9423a7); /* 51 */
    II (b, c, d, a, x[ 5], 21, 0xfc93a039); /* 52 */
    II (a, b, c, d, x[12],  6, 0x655b59c3); /* 53 */
    II (d, a, b, c, x[ 3], 10, 0x8f0ccc92); /* 54 */
    II (c, d, a, b, x[10], 15, 0xffeff47d); /* 55 */
    II (b, c, d, a, x[ 1], 21, 0x85845dd1); /* 56 */
    II (a, b, c, d, x[ 8],  6, 0x6fa87e4f); /* 57 */
    II (d, a, b, c, x[15], 10, 0xfe2ce6e0); /* 58 */
    II (c, d, a, b, x[ 6], 15, 0xa3014314); /* 59 */
    II (b, c, d, a, x[13], 21, 0x4e0811a1); /* 60 */
    II (a, b, c, d, x[ 4],  6, 0xf7537e82); /* 61 */
    II (d, a, b, c, x[11], 10, 0xbd3af235); /* 62 */
    II (c, d, a, b, x[ 2], 15, 0x2ad7d2bb); /* 63 */
    II (b, c, d, a, x[ 9], 21, 0xeb86d391); /* 64 */
    
    state [0] = a = a + state [0];
    state [1] = b = b + state [1];
    state [2] = c = c + state [2];
    state [3] = d = d + state [3];
    
    in += 64;
    }
}


/*
   MD5 block update operation:
   Process another sub-string of the message and update the context.
*/

void 
MD5Update ( MD5_CTX*        context, 
            const uint8_t*  input, 
        size_t          inputBytes ) 
{
    int           byteIndex;
    unsigned int  partLen;
    int           len;
    int           i;
    
    /* Compute number of bytes mod 64 */
    byteIndex = (context -> count[0] >> 3) & 0x3F;
    
    /* Update number of bits: count += 8 * inputBytes */
    if ( (context -> count [0] += inputBytes << 3) < (inputBytes << 3) )
    context -> count [1]++;
    context -> count [1] += inputBytes >> (32 - 3);
    
    partLen = (64 - byteIndex);

    /* Transform as many times as possible. */
    if ( inputBytes >= partLen ) {
    memcpy ( context -> buffer + byteIndex, input, partLen );
    __MD5Transform ( context -> state, (const uint8_t*) context -> buffer, 1 );
    len       = ( inputBytes - partLen ) >> 6;
    __MD5Transform ( context -> state, input + partLen, len );
    i         = partLen + (len << 6);
    byteIndex = 0;
    } 
    else {
    i         = 0;
    }
    
    /* Buffer remaining input */
    memcpy ( (context -> buffer) + byteIndex, input + i, inputBytes - i );
}

//#endif


void 
MD5Final ( uint8_t   digest [16], 
       MD5_CTX*  context ) 
{
    static uint8_t  finalBlock [64];
    uint32_t        bits        [2];
    int             byteIndex;
    int             finalBlockLength;
    
    byteIndex        = (context -> count[0] >> 3) & 0x3F;
    finalBlockLength = (byteIndex < 56  ?  56  :  120) - byteIndex;
    finalBlock[0]    = 0x80;
    
#if __BYTE_ORDER == __BIG_ENDIAN
    CopyToLittleEndian ( bits, (const uint8_t*) context -> count, 2 );
#else
    memcpy ( bits, context->count, 8 );
#endif
    
    MD5Update ( context, (const uint8_t*)finalBlock, finalBlockLength );
    MD5Update ( context, (const uint8_t*)bits, 8 );
    
#if __BYTE_ORDER == __BIG_ENDIAN
    CopyToLittleEndian ( (uint32_t*) digest, (const uint8_t*) context -> state, 4 );
#else
    memcpy ( digest, context -> state, 16 );
#endif

    memset ( context, 0, sizeof (*context) );
}

