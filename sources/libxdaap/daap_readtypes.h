/* endian reads
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


#ifndef DAAP_READTYPES
#define DAAP_READTYPES

#include "endian_swap.h"

/* Solaris (at least) requires integers to be
 * properly aligned. This stuff will copy the required data of
 * the buffer into a properly aligned temporary buffer, if the compiler
 * supports alignment, and the buffer isn't properly aligned
 */
#ifdef HAVE___ALIGNOF__
# define FIXUP_ALIGNMENT(type) \
    type abuf; \
    if (size != sizeof(type)) { \
        FIXME("funny sized\n"); \
    } \
    if (((int)buf % __alignof__(type)) != 0) { \
        memcpy(&abuf, buf, \
               size <= sizeof(type) ? size : sizeof(type)); \
        buf = (const void*)&abuf; \
    }
#else
# define FIXUP_ALIGNMENT(type) \
    if (size != sizeof(type)) { \
        FIXME("funny sized\n"); \
    }
#endif

static DMAP_INT8 readBigEndian_INT8(const void *buf, size_t size)
{
    /* really int8 will never need alignment, but for constancy.. */
    FIXUP_ALIGNMENT(DMAP_INT8);

    return *(DMAP_INT8*)buf;
}

static DMAP_UINT8 readBigEndian_UINT8(const void *buf, size_t size)
{
    FIXUP_ALIGNMENT(DMAP_UINT8);

    return *(DMAP_UINT8*)buf;
}

static DMAP_INT16 readBigEndian_INT16(const void *buf, size_t size)
{
    FIXUP_ALIGNMENT(DMAP_INT16);

    return __Swap16(*(DMAP_INT16*)buf);
}

static DMAP_UINT16 readBigEndian_UINT16(const void *buf, size_t size)
{
    FIXUP_ALIGNMENT(DMAP_UINT16);

    return __Swap16(*(DMAP_UINT16*)buf);
}

static DMAP_INT32 readBigEndian_INT32(const void *buf, size_t size)
{
    FIXUP_ALIGNMENT(DMAP_INT32);

    return __Swap32(*(DMAP_INT32*)buf);
}

static DMAP_UINT32 readBigEndian_UINT32(const void *buf, size_t size)
{
    FIXUP_ALIGNMENT(DMAP_UINT32);

    return __Swap32(*(DMAP_UINT32*)buf);
}

static DMAP_INT64 readBigEndian_INT64(const void *buf, size_t size)
{
    DMAP_INT64 val;
    FIXUP_ALIGNMENT(DMAP_INT64);

    val = *(DMAP_INT64*)buf;
    __SwapPtr64(&val);
    return val;
}

static DMAP_UINT64 readBigEndian_UINT64(const void *buf, size_t size)
{
    DMAP_INT64 val;
    FIXUP_ALIGNMENT(DMAP_UINT64);

    val = *(DMAP_INT64*)buf;
    __SwapPtr64(&val);
    return val;
}

static dmap_contentCodeFOURCC read_fourcc(const void *buf, size_t size)
{
    const char *c = (char*)buf;
    if (size != sizeof(dmap_contentCodeFOURCC))
        FIXME("funny sized\n");
    return MAKEFOURCC(c[0], c[1], c[2], c[3]);
}

static DMAP_VERSION read_version(const void *buf, size_t size)
{
    DMAP_VERSION v;
    if (size != sizeof(DMAP_VERSION))
        FIXME("funny sized\n");
    v.v1 = readBigEndian_UINT16(buf, 2);
    v.v2 = readBigEndian_UINT16((char*)buf+2, 2);

    return v;
}

static char* read_string_withalloc(const void *buf, size_t size)
{
    char *str = (char*)malloc(size + 1);
    strncpy(str, (char*)buf, size);
    str[size] = 0;
    return str;
}

#endif /* DAAP_READTYPES */

