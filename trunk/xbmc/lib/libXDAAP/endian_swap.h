/* endian swaps
 *
 * Copyright (c) 2004 David Hammerton
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef ENDIAN_SWAP_H_
#define ENDIAN_SWAP_H_

#include "portability.h"

#ifdef WORDS_BIGENDIAN

#define __Swap16(v) (v)
#define __Swap32(v) (v)
#define __Swap64(v) (v)
#define __SwapPtr64(v)

#else /* WORDS_BIGENDIAN */

#define __Swap16(v) ((((v) & 0x00FF) << 0x08) | \
                     (((v) & 0xFF00) >> 0x08)) 
#define __Swap32(v) ((((v) & 0x000000FF) << 0x18) | \
                     (((v) & 0x0000FF00) << 0x08) | \
                     (((v) & 0x00FF0000) >> 0x08) | \
                     (((v) & 0xFF000000) >> 0x18))
/* Microsft Visual C++ doesn't like this */
/*
#define __Swap64(v) ((((v) & 0x00000000000000FFLL) >> 0x38) | \
                     (((v) & 0x000000000000FF00LL) >> 0x28) | \
                     (((v) & 0x0000000000FF0000LL) >> 0x18) | \
                     (((v) & 0x00000000FF000000LL) >> 0x08) | \
                     (((v) & 0x000000FF00000000LL) << 0x08) | \
                     (((v) & 0x0000FF0000000000LL) << 0x18) | \
                     (((v) & 0x00FF000000000000LL) << 0x28) | \
                     (((v) & 0xFF00000000000000LL) << 0x38))*/
static void __SwapPtr64(int64_t *v)
{
    unsigned char b, *bv;

    bv = (unsigned char*)v;

    b = bv[0];
    bv[0] = bv[7];
    bv[7] = b;

    b = bv[1];
    bv[1] = bv[6];
    bv[6] = b;

    b = bv[2];
    bv[2] = bv[5];
    bv[5] = b;

    b = bv[3];
    bv[3] = bv[4];
    bv[4] = b;
}

#endif /* WORDS_BIGENDIAN */


#endif /* ENDIAN_SWAP_H */

