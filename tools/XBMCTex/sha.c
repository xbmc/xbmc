/*
 *      Copyright (C) 2004-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string.h>
#ifdef _LINUX
#include <stdint.h>
#else
#include "stdint_win.h"
#endif

typedef unsigned long u32;
typedef unsigned char u8;

#ifdef _LINUX
#include <string.h>
#define __forceinline inline
#define __int64 int64_t
#endif

__forceinline static u32 rol(u32 x, u8 n)
{
	return (x << n) | (x >> (32-n));
}

static void bswapcpy(void* dst, const void* src, u32 n)
{
  uint32_t d, b0, b1, b2, b3;
  uint32_t *nDst = (uint32_t *)dst;
  uint32_t *nSrc = (uint32_t *)src;
  n >>= 2;
  while (n != 0)
  {
    d = *nSrc;
    b0 = d >> 24;
    b1 = (d >> 8) & 0x0000ff00;
    b2 = (d << 8) & 0x00ff0000;
    b3 = (d << 24);
    *nDst = b3 | b2 | b1 | b0;
    --n;
    ++nSrc;
    ++nDst;
  }
}

void SHA1(const u8* buf, u32 len, u8 hash[20])
{
	u32 a, b, c, d, e;
	u32 h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
	u32 w[80];
	u32 n = len;
	int t;
	int done = 0;

	while (done != 2)
	{
		if (n >= 64)
		{
			bswapcpy(w, buf, 64);
			buf += 64;
			n -= 64;
		}
		else
		{
			u8 tmpbuf[64];
			memcpy(tmpbuf, buf, n);
			memset(tmpbuf+n, 0, 64-n);
			if (!done)
				tmpbuf[n] = 0x80;
			bswapcpy(w, tmpbuf, 64);
			if (n <= 55)
			{
				u32 bitlen = len * 8;
				w[14] = *(((u32*)&bitlen)+1);
				w[15] = *((u32*)&bitlen);
				done = 2;
			}
			else
				done = 1;
			n = 0;
		}

		for (t = 16; t < 80; ++t)
		{
			w[t] = rol(w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16], 1);
		}

		a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];

		for (t = 0; t < 20; ++t)
		{
			u32 temp = rol(a, 5) + ((b & c) | ((~b) & d)) + e + w[t] + 0x5A827999;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}
		for ( ; t < 40; ++t)
		{
			u32 temp = rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0x6ED9EBA1;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}
		for ( ; t < 60; ++t)
		{
			u32 temp = rol(a, 5) + ((b & c) | (b & d) | (c & d)) + e + w[t] + 0x8F1BBCDC;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}
		for ( ; t < 80; ++t)
		{
			u32 temp = rol(a, 5) + (b ^ c ^ d) + e + w[t] + 0xCA62C1D6;
			e = d; d = c; c = rol(b, 30); b = a; a = temp;
		}

		h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
	}

	bswapcpy(hash, h, 20);
}
