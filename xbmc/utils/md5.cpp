/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "md5.h"

static void MD5Init (MD5_CTX *mdContext);
static void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen);
static void MD5Final (unsigned char digest[16], MD5_CTX *mdContext);

XBMC::MD5::MD5(void)
{
  MD5Init(&m_ctx);
}

void XBMC::MD5::append(unsigned char *inBuf, unsigned int inLen)
{
  MD5Update(&m_ctx, inBuf, inLen);
}

void XBMC::MD5::append(const CStdString& str)
{
  append((unsigned char*) str.c_str(), (unsigned int) str.length());
}

void XBMC::MD5::getDigest(unsigned char digest[16])
{
  MD5Final(digest, &m_ctx);
}

/*
 **********************************************************************
 ** md5.c                                                            **
 ** RSA Data Security, Inc. MD5 Message Digest Algorithm             **
 ** Created: 2/17/90 RLR                                             **
 ** Revised: 1/91 SRD,AJ,BSK,JT Reference C Version                  **
 **********************************************************************
 */

/*
 **********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved. **
 **                                                                  **
 ** License to copy and use this software is granted provided that   **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message     **
 ** Digest Algorithm" in all material mentioning or referencing this **
 ** software or this function.                                       **
 **                                                                  **
 ** License is also granted to make and use derivative works         **
 ** provided that such works are identified as "derived from the RSA **
 ** Data Security, Inc. MD5 Message Digest Algorithm" in all         **
 ** material mentioning or referencing the derived work.             **
 **                                                                  **
 ** RSA Data Security, Inc. makes no representations concerning      **
 ** either the merchantability of this software or the suitability   **
 ** of this software for any particular purpose.  It is provided "as **
 ** is" without express or implied warranty of any kind.             **
 **                                                                  **
 ** These notices must be retained in any copies of any part of this **
 ** documentation and/or software.                                   **
 **********************************************************************
 */

/* forward declaration */
static void Transform (UINT4* buf, UINT4* in);

static unsigned char PADDING[64] = {
  0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* F, G and H are basic MD5 functions: selection, majority, parity */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
  {(a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) \
  {(a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) \
  {(a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) \
  {(a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
   (a) = ROTATE_LEFT ((a), (s)); \
   (a) += (b); \
  }

static void MD5Init (MD5_CTX *mdContext)
{
  mdContext->i[0] = mdContext->i[1] = (UINT4)0;

  /* Load magic initialization constants.
   */
  mdContext->buf[0] = (UINT4)0x67452301;
  mdContext->buf[1] = (UINT4)0xefcdab89;
  mdContext->buf[2] = (UINT4)0x98badcfe;
  mdContext->buf[3] = (UINT4)0x10325476;
}

static void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen)
{
  UINT4 in[16];
  int mdi;
  unsigned int i, ii;

  /* compute number of bytes mod 64 */
  mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

  /* update number of bits */
  if ((mdContext->i[0] + ((UINT4)inLen << 3)) < mdContext->i[0])
    mdContext->i[1]++;
  mdContext->i[0] += ((UINT4)inLen << 3);
  mdContext->i[1] += ((UINT4)inLen >> 29);

  while (inLen--) {
    /* add new character to buffer, increment mdi */
    mdContext->in[mdi++] = *inBuf++;

    /* transform if necessary */
    if (mdi == 0x40) {
      for (i = 0, ii = 0; i < 16; i++, ii += 4)
        in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
                (((UINT4)mdContext->in[ii+2]) << 16) |
                (((UINT4)mdContext->in[ii+1]) << 8) |
                ((UINT4)mdContext->in[ii]);
      Transform (mdContext->buf, in);
      mdi = 0;
    }
  }
}

static void MD5Final (unsigned char digest[16], MD5_CTX *mdContext)
{
  UINT4 in[16];
  int mdi;
  unsigned int i, ii;
  unsigned int padLen;

  /* save number of bits */
  in[14] = mdContext->i[0];
  in[15] = mdContext->i[1];

  /* compute number of bytes mod 64 */
  mdi = (int)((mdContext->i[0] >> 3) & 0x3F);

  /* pad out to 56 mod 64 */
  padLen = (mdi < 56) ? (56 - mdi) : (120 - mdi);
  MD5Update (mdContext, PADDING, padLen);

  /* append length in bits and transform */
  for (i = 0, ii = 0; i < 14; i++, ii += 4)
    in[i] = (((UINT4)mdContext->in[ii+3]) << 24) |
            (((UINT4)mdContext->in[ii+2]) << 16) |
            (((UINT4)mdContext->in[ii+1]) << 8) |
            ((UINT4)mdContext->in[ii]);
  Transform (mdContext->buf, in);

  /* store buffer in digest */
  for (i = 0, ii = 0; i < 4; i++, ii += 4) {
    digest[ii] = (unsigned char)(mdContext->buf[i] & 0xFF);
    digest[ii+1] =
      (unsigned char)((mdContext->buf[i] >> 8) & 0xFF);
    digest[ii+2] =
      (unsigned char)((mdContext->buf[i] >> 16) & 0xFF);
    digest[ii+3] =
      (unsigned char)((mdContext->buf[i] >> 24) & 0xFF);
  }
}

/* Basic MD5 step. Transform buf based on in.
 */
static void Transform (UINT4* buf, UINT4* in)
{
  UINT4 a = buf[0], b = buf[1], c = buf[2], d = buf[3];

  /* Round 1 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
  FF (a, b, c, d, in[ 0], S11, 0xd76aa478); /* 1 */
  FF (d, a, b, c, in[ 1], S12, 0xe8c7b756); /* 2 */
  FF (c, d, a, b, in[ 2], S13, 0x242070db); /* 3 */
  FF (b, c, d, a, in[ 3], S14, 0xc1bdceee); /* 4 */
  FF (a, b, c, d, in[ 4], S11, 0xf57c0faf); /* 5 */
  FF (d, a, b, c, in[ 5], S12, 0x4787c62a); /* 6 */
  FF (c, d, a, b, in[ 6], S13, 0xa8304613); /* 7 */
  FF (b, c, d, a, in[ 7], S14, 0xfd469501); /* 8 */
  FF (a, b, c, d, in[ 8], S11, 0x698098d8); /* 9 */
  FF (d, a, b, c, in[ 9], S12, 0x8b44f7af); /* 10 */
  FF (c, d, a, b, in[10], S13, 0xffff5bb1); /* 11 */
  FF (b, c, d, a, in[11], S14, 0x895cd7be); /* 12 */
  FF (a, b, c, d, in[12], S11, 0x6b901122); /* 13 */
  FF (d, a, b, c, in[13], S12, 0xfd987193); /* 14 */
  FF (c, d, a, b, in[14], S13, 0xa679438e); /* 15 */
  FF (b, c, d, a, in[15], S14, 0x49b40821); /* 16 */

  /* Round 2 */
#define S21 5
#define S22 9
#define S23 14
#define S24 20
  GG (a, b, c, d, in[ 1], S21, 0xf61e2562); /* 17 */
  GG (d, a, b, c, in[ 6], S22, 0xc040b340); /* 18 */
  GG (c, d, a, b, in[11], S23, 0x265e5a51); /* 19 */
  GG (b, c, d, a, in[ 0], S24, 0xe9b6c7aa); /* 20 */
  GG (a, b, c, d, in[ 5], S21, 0xd62f105d); /* 21 */
  GG (d, a, b, c, in[10], S22,  0x2441453); /* 22 */
  GG (c, d, a, b, in[15], S23, 0xd8a1e681); /* 23 */
  GG (b, c, d, a, in[ 4], S24, 0xe7d3fbc8); /* 24 */
  GG (a, b, c, d, in[ 9], S21, 0x21e1cde6); /* 25 */
  GG (d, a, b, c, in[14], S22, 0xc33707d6); /* 26 */
  GG (c, d, a, b, in[ 3], S23, 0xf4d50d87); /* 27 */
  GG (b, c, d, a, in[ 8], S24, 0x455a14ed); /* 28 */
  GG (a, b, c, d, in[13], S21, 0xa9e3e905); /* 29 */
  GG (d, a, b, c, in[ 2], S22, 0xfcefa3f8); /* 30 */
  GG (c, d, a, b, in[ 7], S23, 0x676f02d9); /* 31 */
  GG (b, c, d, a, in[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
#define S31 4
#define S32 11
#define S33 16
#define S34 23
  HH (a, b, c, d, in[ 5], S31, 0xfffa3942); /* 33 */
  HH (d, a, b, c, in[ 8], S32, 0x8771f681); /* 34 */
  HH (c, d, a, b, in[11], S33, 0x6d9d6122); /* 35 */
  HH (b, c, d, a, in[14], S34, 0xfde5380c); /* 36 */
  HH (a, b, c, d, in[ 1], S31, 0xa4beea44); /* 37 */
  HH (d, a, b, c, in[ 4], S32, 0x4bdecfa9); /* 38 */
  HH (c, d, a, b, in[ 7], S33, 0xf6bb4b60); /* 39 */
  HH (b, c, d, a, in[10], S34, 0xbebfbc70); /* 40 */
  HH (a, b, c, d, in[13], S31, 0x289b7ec6); /* 41 */
  HH (d, a, b, c, in[ 0], S32, 0xeaa127fa); /* 42 */
  HH (c, d, a, b, in[ 3], S33, 0xd4ef3085); /* 43 */
  HH (b, c, d, a, in[ 6], S34,  0x4881d05); /* 44 */
  HH (a, b, c, d, in[ 9], S31, 0xd9d4d039); /* 45 */
  HH (d, a, b, c, in[12], S32, 0xe6db99e5); /* 46 */
  HH (c, d, a, b, in[15], S33, 0x1fa27cf8); /* 47 */
  HH (b, c, d, a, in[ 2], S34, 0xc4ac5665); /* 48 */

  /* Round 4 */
#define S41 6
#define S42 10
#define S43 15
#define S44 21
  II ( a, b, c, d, in[ 0], S41, 0xf4292244); /* 49 */
  II ( d, a, b, c, in[ 7], S42, 0x432aff97); /* 50 */
  II ( c, d, a, b, in[14], S43, 0xab9423a7); /* 51 */
  II ( b, c, d, a, in[ 5], S44, 0xfc93a039); /* 52 */
  II ( a, b, c, d, in[12], S41, 0x655b59c3); /* 53 */
  II ( d, a, b, c, in[ 3], S42, 0x8f0ccc92); /* 54 */
  II ( c, d, a, b, in[10], S43, 0xffeff47d); /* 55 */
  II ( b, c, d, a, in[ 1], S44, 0x85845dd1); /* 56 */
  II ( a, b, c, d, in[ 8], S41, 0x6fa87e4f); /* 57 */
  II ( d, a, b, c, in[15], S42, 0xfe2ce6e0); /* 58 */
  II ( c, d, a, b, in[ 6], S43, 0xa3014314); /* 59 */
  II ( b, c, d, a, in[13], S44, 0x4e0811a1); /* 60 */
  II ( a, b, c, d, in[ 4], S41, 0xf7537e82); /* 61 */
  II ( d, a, b, c, in[11], S42, 0xbd3af235); /* 62 */
  II ( c, d, a, b, in[ 2], S43, 0x2ad7d2bb); /* 63 */
  II ( b, c, d, a, in[ 9], S44, 0xeb86d391); /* 64 */

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}

