/* gchecksum.h - data hashing functions
 *
 * Copyright (C) 2007  Emmanuele Bassi  <ebassi@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include "glibconfig.h"
#include "gchecksum.h"
#include "glib.h"
#include "glibintl.h"

#include "galias.h"

#define IS_VALID_TYPE(type)     ((type) >= G_CHECKSUM_MD5 && (type) <= G_CHECKSUM_SHA256)

/* The fact that these are lower case characters is part of the ABI */
static const gchar hex_digits[] = "0123456789abcdef";

#define MD5_DATASIZE    64
#define MD5_DIGEST_LEN  16

typedef struct
{
  guint32 buf[4];
  guint32 bits[2];
  
  guchar data[MD5_DATASIZE];

  guchar digest[MD5_DIGEST_LEN];
} Md5sum;

#define SHA1_DATASIZE   64
#define SHA1_DIGEST_LEN 20

typedef struct
{
  guint32 buf[5];
  guint32 bits[2];

  /* we pack 64 unsigned chars into 16 32-bit unsigned integers */
  guint32 data[16];

  guchar digest[SHA1_DIGEST_LEN];
} Sha1sum;

#define SHA256_DATASIZE         64
#define SHA256_DIGEST_LEN       32

typedef struct
{
  guint32 buf[8];
  guint32 bits[2];

  guint8 data[SHA256_DATASIZE];

  guchar digest[SHA256_DIGEST_LEN];
} Sha256sum;

struct _GChecksum
{
  GChecksumType type;

  gchar *digest_str;

  union {
    Md5sum md5;
    Sha1sum sha1;
    Sha256sum sha256;
  } sum;
};

/* we need different byte swapping functions because MD5 expects buffers
 * to be little-endian, while SHA1 and SHA256 expect them in big-endian
 * form.
 */

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define md5_byte_reverse(buffer,length)
#else
/* assume that the passed buffer is integer aligned */
static inline void
md5_byte_reverse (guchar *buffer,
                  gulong  length)
{
  guint32 bit;

  do
    {
      bit = (guint32) ((unsigned) buffer[3] << 8 | buffer[2]) << 16 |
                      ((unsigned) buffer[1] << 8 | buffer[0]);
      * (guint32 *) buffer = bit;
      buffer += 4;
    }
  while (--length);
}
#endif /* G_BYTE_ORDER == G_LITTLE_ENDIAN */

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define sha_byte_reverse(buffer,length)
#else
static inline void
sha_byte_reverse (guint32 *buffer,
                  gint     length)
{
  length /= sizeof (guint32);
  while (length--)
    {
      *buffer = GUINT32_SWAP_LE_BE (*buffer);
      ++buffer;
    }
}
#endif /* G_BYTE_ORDER == G_BIG_ENDIAN */

static gchar *
digest_to_string (guint8 *digest,
                  gsize   digest_len)
{
  gint len = digest_len * 2;
  gint i;
  gchar *retval;

  retval = g_new (gchar, len + 1);

  for (i = 0; i < digest_len; i++)
    {
      guint8 byte = digest[i];

      retval[2 * i] = hex_digits[byte >> 4];
      retval[2 * i + 1] = hex_digits[byte & 0xf];
    }

  retval[len] = 0;

  return retval;
}

/*
 * MD5 Checksum
 */

/* This MD5 digest computation is based on the equivalent code
 * written by Colin Plumb. It came with this notice:
 *
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 */

static void
md5_sum_init (Md5sum *md5)
{
  /* arbitrary constants */
  md5->buf[0] = 0x67452301;
  md5->buf[1] = 0xefcdab89;
  md5->buf[2] = 0x98badcfe;
  md5->buf[3] = 0x10325476;

  md5->bits[0] = md5->bits[1] = 0;
}

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  md5_sum_update()
 * blocks the data and converts bytes into longwords for this routine.
 */
static void 
md5_transform (guint32	     buf[4],
	       guint32 const in[16])
{
  register guint32 a, b, c, d;

/* The four core functions - F1 is optimized somewhat */
#define F1(x, y, z)     (z ^ (x & (y ^ z)))
#define F2(x, y, z)     F1 (z, x, y)
#define F3(x, y, z)     (x ^ y ^ z)
#define F4(x, y, z)     (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define md5_step(f, w, x, y, z, data, s) \
	( w += f (x, y, z) + data,  w = w << s | w >> (32 - s),  w += x )

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  md5_step (F1, a, b, c, d, in[0]  + 0xd76aa478,  7);
  md5_step (F1, d, a, b, c, in[1]  + 0xe8c7b756, 12);
  md5_step (F1, c, d, a, b, in[2]  + 0x242070db, 17);
  md5_step (F1, b, c, d, a, in[3]  + 0xc1bdceee, 22);
  md5_step (F1, a, b, c, d, in[4]  + 0xf57c0faf,  7);
  md5_step (F1, d, a, b, c, in[5]  + 0x4787c62a, 12);
  md5_step (F1, c, d, a, b, in[6]  + 0xa8304613, 17);
  md5_step (F1, b, c, d, a, in[7]  + 0xfd469501, 22);
  md5_step (F1, a, b, c, d, in[8]  + 0x698098d8,  7);
  md5_step (F1, d, a, b, c, in[9]  + 0x8b44f7af, 12);
  md5_step (F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
  md5_step (F1, b, c, d, a, in[11] + 0x895cd7be, 22);
  md5_step (F1, a, b, c, d, in[12] + 0x6b901122,  7);
  md5_step (F1, d, a, b, c, in[13] + 0xfd987193, 12);
  md5_step (F1, c, d, a, b, in[14] + 0xa679438e, 17);
  md5_step (F1, b, c, d, a, in[15] + 0x49b40821, 22);
	
  md5_step (F2, a, b, c, d, in[1]  + 0xf61e2562,  5);
  md5_step (F2, d, a, b, c, in[6]  + 0xc040b340,  9);
  md5_step (F2, c, d, a, b, in[11] + 0x265e5a51, 14);
  md5_step (F2, b, c, d, a, in[0]  + 0xe9b6c7aa, 20);
  md5_step (F2, a, b, c, d, in[5]  + 0xd62f105d,  5);
  md5_step (F2, d, a, b, c, in[10] + 0x02441453,  9);
  md5_step (F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
  md5_step (F2, b, c, d, a, in[4]  + 0xe7d3fbc8, 20);
  md5_step (F2, a, b, c, d, in[9]  + 0x21e1cde6,  5);
  md5_step (F2, d, a, b, c, in[14] + 0xc33707d6,  9);
  md5_step (F2, c, d, a, b, in[3]  + 0xf4d50d87, 14);
  md5_step (F2, b, c, d, a, in[8]  + 0x455a14ed, 20);
  md5_step (F2, a, b, c, d, in[13] + 0xa9e3e905,  5);
  md5_step (F2, d, a, b, c, in[2]  + 0xfcefa3f8,  9);
  md5_step (F2, c, d, a, b, in[7]  + 0x676f02d9, 14);
  md5_step (F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

  md5_step (F3, a, b, c, d, in[5]  + 0xfffa3942,  4);
  md5_step (F3, d, a, b, c, in[8]  + 0x8771f681, 11);
  md5_step (F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
  md5_step (F3, b, c, d, a, in[14] + 0xfde5380c, 23);
  md5_step (F3, a, b, c, d, in[1]  + 0xa4beea44,  4);
  md5_step (F3, d, a, b, c, in[4]  + 0x4bdecfa9, 11);
  md5_step (F3, c, d, a, b, in[7]  + 0xf6bb4b60, 16);
  md5_step (F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
  md5_step (F3, a, b, c, d, in[13] + 0x289b7ec6,  4);
  md5_step (F3, d, a, b, c, in[0]  + 0xeaa127fa, 11);
  md5_step (F3, c, d, a, b, in[3]  + 0xd4ef3085, 16);
  md5_step (F3, b, c, d, a, in[6]  + 0x04881d05, 23);
  md5_step (F3, a, b, c, d, in[9]  + 0xd9d4d039,  4);
  md5_step (F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
  md5_step (F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
  md5_step (F3, b, c, d, a, in[2]  + 0xc4ac5665, 23);

  md5_step (F4, a, b, c, d, in[0]  + 0xf4292244,  6);
  md5_step (F4, d, a, b, c, in[7]  + 0x432aff97, 10);
  md5_step (F4, c, d, a, b, in[14] + 0xab9423a7, 15);
  md5_step (F4, b, c, d, a, in[5]  + 0xfc93a039, 21);
  md5_step (F4, a, b, c, d, in[12] + 0x655b59c3,  6);
  md5_step (F4, d, a, b, c, in[3]  + 0x8f0ccc92, 10);
  md5_step (F4, c, d, a, b, in[10] + 0xffeff47d, 15);
  md5_step (F4, b, c, d, a, in[1]  + 0x85845dd1, 21);
  md5_step (F4, a, b, c, d, in[8]  + 0x6fa87e4f,  6);
  md5_step (F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
  md5_step (F4, c, d, a, b, in[6]  + 0xa3014314, 15);
  md5_step (F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
  md5_step (F4, a, b, c, d, in[4]  + 0xf7537e82,  6);
  md5_step (F4, d, a, b, c, in[11] + 0xbd3af235, 10);
  md5_step (F4, c, d, a, b, in[2]  + 0x2ad7d2bb, 15);
  md5_step (F4, b, c, d, a, in[9]  + 0xeb86d391, 21);

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;

#undef F1
#undef F2
#undef F3
#undef F4
#undef md5_step
}

static void
md5_sum_update (Md5sum       *md5,
                const guchar *data,
                gsize         length)
{
  guint32 bit;

  bit = md5->bits[0];
  md5->bits[0] = bit + ((guint32) length << 3);

  /* carry from low to high */
  if (md5->bits[0] < bit)
    md5->bits[1] += 1;

  md5->bits[1] += length >> 29;

  /* bytes already in Md5sum->data */
  bit = (bit >> 3) & 0x3f;

  /* handle any leading odd-sized chunks */
  if (bit)
    {
      guchar *p = (guchar *) md5->data + bit;

      bit = MD5_DATASIZE - bit;
      if (length < bit)
        {
          memcpy (p, data, length);
          return;
        }

      memcpy (p, data, bit);
      
      md5_byte_reverse (md5->data, 16);
      md5_transform (md5->buf, (guint32 *) md5->data);

      data += bit;
      length -= bit;
    }

  /* process data in 64-byte chunks */
  while (length >= MD5_DATASIZE)
    {
      memcpy (md5->data, data, MD5_DATASIZE);
      
      md5_byte_reverse (md5->data, 16);
      md5_transform (md5->buf, (guint32 *) md5->data);

      data += MD5_DATASIZE;
      length -= MD5_DATASIZE;
    }

  /* handle any remaining bytes of data */
  memcpy (md5->data, data, length);
}

/* closes a checksum */
static void
md5_sum_close (Md5sum *md5)
{
  guint count;
  guchar *p;

  /* Compute number of bytes mod 64 */
  count = (md5->bits[0] >> 3) & 0x3F;

  /* Set the first char of padding to 0x80.
   * This is safe since there is always at least one byte free
   */
  p = md5->data + count;
  *p++ = 0x80;

  /* Bytes of padding needed to make 64 bytes */
  count = MD5_DATASIZE - 1 - count;

  /* Pad out to 56 mod 64 */
  if (count < 8)
    {
      /* Two lots of padding:  Pad the first block to 64 bytes */
      memset (p, 0, count);
      
      md5_byte_reverse (md5->data, 16);
      md5_transform (md5->buf, (guint32 *) md5->data);

      /* Now fill the next block with 56 bytes */
      memset (md5->data, 0, MD5_DATASIZE - 8);
    }
  else
    {
      /* Pad block to 56 bytes */
      memset (p, 0, count - 8);
    }

  md5_byte_reverse (md5->data, 14);

  /* Append length in bits and transform */
  ((guint32 *) md5->data)[14] = md5->bits[0];
  ((guint32 *) md5->data)[15] = md5->bits[1];

  md5_transform (md5->buf, (guint32 *) md5->data);
  md5_byte_reverse ((guchar *) md5->buf, 4);
  
  memcpy (md5->digest, md5->buf, 16);

  /* Reset buffers in case they contain sensitive data */
  memset (md5->buf, 0, sizeof (md5->buf));
  memset (md5->data, 0, sizeof (md5->data));
}

static gchar *
md5_sum_to_string (Md5sum *md5)
{
  return digest_to_string (md5->digest, MD5_DIGEST_LEN);
}

static void
md5_sum_digest (Md5sum *md5,
                guint8 *digest)
{
  gint i;

  for (i = 0; i < MD5_DIGEST_LEN; i++)
    digest[i] = md5->digest[i];
}

/*
 * SHA-1 Checksum
 */

/* The following implementation comes from D-Bus dbus-sha.c. I've changed
 * it to use GLib types and to work more like the MD5 implementation above.
 * I left the comments to have an history of this code.
 *      -- Emmanuele Bassi, ebassi@gnome.org
 */

/* The following comments have the history of where this code
 * comes from. I actually copied it from GNet in GNOME CVS.
 * - hp@redhat.com
 */

/*
 *  sha.h : Implementation of the Secure Hash Algorithm
 *
 * Part of the Python Cryptography Toolkit, version 1.0.0
 *
 * Copyright (C) 1995, A.M. Kuchling
 *
 * Distribute and use freely; there are no restrictions on further
 * dissemination and usage except those imposed by the laws of your
 * country of residence.
 *
 */

/* SHA: NIST's Secure Hash Algorithm */

/* Based on SHA code originally posted to sci.crypt by Peter Gutmann
   in message <30ajo5$oe8@ccu2.auckland.ac.nz>.
   Modified to test for endianness on creation of SHA objects by AMK.
   Also, the original specification of SHA was found to have a weakness
   by NSA/NIST.  This code implements the fixed version of SHA.
*/

/* Here's the first paragraph of Peter Gutmann's posting:

The following is my SHA (FIPS 180) code updated to allow use of the "fixed"
SHA, thanks to Jim Gillogly and an anonymous contributor for the information on
what's changed in the new version.  The fix is a simple change which involves
adding a single rotate in the initial expansion function.  It is unknown
whether this is an optimal solution to the problem which was discovered in the
SHA or whether it's simply a bandaid which fixes the problem with a minimum of
effort (for example the reengineering of a great many Capstone chips).
*/

static void
sha1_sum_init (Sha1sum *sha1)
{
  /* initialize constants */
  sha1->buf[0] = 0x67452301L;
  sha1->buf[1] = 0xEFCDAB89L;
  sha1->buf[2] = 0x98BADCFEL;
  sha1->buf[3] = 0x10325476L;
  sha1->buf[4] = 0xC3D2E1F0L;

  /* initialize bits */
  sha1->bits[0] = sha1->bits[1] = 0;
}

/* The SHA f()-functions. */

#define f1(x,y,z)       (z ^ (x & (y ^ z)))             /* Rounds  0-19 */
#define f2(x,y,z)       (x ^ y ^ z)                     /* Rounds 20-39 */
#define f3(x,y,z)       (( x & y) | (z & (x | y)))      /* Rounds 40-59 */
#define f4(x,y,z)       (x ^ y ^ z)                     /* Rounds 60-79 */

/* The SHA Mysterious Constants */
#define K1  0x5A827999L                                 /* Rounds  0-19 */
#define K2  0x6ED9EBA1L                                 /* Rounds 20-39 */
#define K3  0x8F1BBCDCL                                 /* Rounds 40-59 */
#define K4  0xCA62C1D6L                                 /* Rounds 60-79 */

/* 32-bit rotate left - kludged with shifts */
#define ROTL(n,X) (((X) << n ) | ((X) >> (32 - n)))

/* The initial expanding function.  The hash function is defined over an
   80-word expanded input array W, where the first 16 are copies of the input
   data, and the remaining 64 are defined by

        W[ i ] = W[ i - 16 ] ^ W[ i - 14 ] ^ W[ i - 8 ] ^ W[ i - 3 ]

   This implementation generates these values on the fly in a circular
   buffer - thanks to Colin Plumb, colin@nyx10.cs.du.edu for this
   optimization.

   The updated SHA changes the expanding function by adding a rotate of 1
   bit.  Thanks to Jim Gillogly, jim@rand.org, and an anonymous contributor
   for this information */

#define expand(W,i) (W[ i & 15 ] = ROTL (1, (W[ i       & 15] ^ \
                                             W[(i - 14) & 15] ^ \
                                             W[(i -  8) & 15] ^ \
                                             W[(i -  3) & 15])))


/* The prototype SHA sub-round.  The fundamental sub-round is:

        a' = e + ROTL( 5, a ) + f( b, c, d ) + k + data;
        b' = a;
        c' = ROTL( 30, b );
        d' = c;
        e' = d;

   but this is implemented by unrolling the loop 5 times and renaming the
   variables ( e, a, b, c, d ) = ( a', b', c', d', e' ) each iteration.
   This code is then replicated 20 times for each of the 4 functions, using
   the next 20 values from the W[] array each time */

#define subRound(a, b, c, d, e, f, k, data) \
   (e += ROTL (5, a) + f(b, c, d) + k + data, b = ROTL (30, b))

static void
sha1_transform (guint32  buf[5],
                guint32  in[16])
{
  guint32 A, B, C, D, E;

  A = buf[0];
  B = buf[1];
  C = buf[2];
  D = buf[3];
  E = buf[4];

  /* Heavy mangling, in 4 sub-rounds of 20 interations each. */
  subRound (A, B, C, D, E, f1, K1, in[0]);
  subRound (E, A, B, C, D, f1, K1, in[1]);
  subRound (D, E, A, B, C, f1, K1, in[2]);
  subRound (C, D, E, A, B, f1, K1, in[3]);
  subRound (B, C, D, E, A, f1, K1, in[4]);
  subRound (A, B, C, D, E, f1, K1, in[5]);
  subRound (E, A, B, C, D, f1, K1, in[6]);
  subRound (D, E, A, B, C, f1, K1, in[7]);
  subRound (C, D, E, A, B, f1, K1, in[8]);
  subRound (B, C, D, E, A, f1, K1, in[9]);
  subRound (A, B, C, D, E, f1, K1, in[10]);
  subRound (E, A, B, C, D, f1, K1, in[11]);
  subRound (D, E, A, B, C, f1, K1, in[12]);
  subRound (C, D, E, A, B, f1, K1, in[13]);
  subRound (B, C, D, E, A, f1, K1, in[14]);
  subRound (A, B, C, D, E, f1, K1, in[15]);
  subRound (E, A, B, C, D, f1, K1, expand (in, 16));
  subRound (D, E, A, B, C, f1, K1, expand (in, 17));
  subRound (C, D, E, A, B, f1, K1, expand (in, 18));
  subRound (B, C, D, E, A, f1, K1, expand (in, 19));

  subRound (A, B, C, D, E, f2, K2, expand (in, 20));
  subRound (E, A, B, C, D, f2, K2, expand (in, 21));
  subRound (D, E, A, B, C, f2, K2, expand (in, 22));
  subRound (C, D, E, A, B, f2, K2, expand (in, 23));
  subRound (B, C, D, E, A, f2, K2, expand (in, 24));
  subRound (A, B, C, D, E, f2, K2, expand (in, 25));
  subRound (E, A, B, C, D, f2, K2, expand (in, 26));
  subRound (D, E, A, B, C, f2, K2, expand (in, 27));
  subRound (C, D, E, A, B, f2, K2, expand (in, 28));
  subRound (B, C, D, E, A, f2, K2, expand (in, 29));
  subRound (A, B, C, D, E, f2, K2, expand (in, 30));
  subRound (E, A, B, C, D, f2, K2, expand (in, 31));
  subRound (D, E, A, B, C, f2, K2, expand (in, 32));
  subRound (C, D, E, A, B, f2, K2, expand (in, 33));
  subRound (B, C, D, E, A, f2, K2, expand (in, 34));
  subRound (A, B, C, D, E, f2, K2, expand (in, 35));
  subRound (E, A, B, C, D, f2, K2, expand (in, 36));
  subRound (D, E, A, B, C, f2, K2, expand (in, 37));
  subRound (C, D, E, A, B, f2, K2, expand (in, 38));
  subRound (B, C, D, E, A, f2, K2, expand (in, 39));
  
  subRound (A, B, C, D, E, f3, K3, expand (in, 40));
  subRound (E, A, B, C, D, f3, K3, expand (in, 41));
  subRound (D, E, A, B, C, f3, K3, expand (in, 42));
  subRound (C, D, E, A, B, f3, K3, expand (in, 43));
  subRound (B, C, D, E, A, f3, K3, expand (in, 44));
  subRound (A, B, C, D, E, f3, K3, expand (in, 45));
  subRound (E, A, B, C, D, f3, K3, expand (in, 46));
  subRound (D, E, A, B, C, f3, K3, expand (in, 47));
  subRound (C, D, E, A, B, f3, K3, expand (in, 48));
  subRound (B, C, D, E, A, f3, K3, expand (in, 49));
  subRound (A, B, C, D, E, f3, K3, expand (in, 50));
  subRound (E, A, B, C, D, f3, K3, expand (in, 51));
  subRound (D, E, A, B, C, f3, K3, expand (in, 52));
  subRound (C, D, E, A, B, f3, K3, expand (in, 53));
  subRound (B, C, D, E, A, f3, K3, expand (in, 54));
  subRound (A, B, C, D, E, f3, K3, expand (in, 55));
  subRound (E, A, B, C, D, f3, K3, expand (in, 56));
  subRound (D, E, A, B, C, f3, K3, expand (in, 57));
  subRound (C, D, E, A, B, f3, K3, expand (in, 58));
  subRound (B, C, D, E, A, f3, K3, expand (in, 59));

  subRound (A, B, C, D, E, f4, K4, expand (in, 60));
  subRound (E, A, B, C, D, f4, K4, expand (in, 61));
  subRound (D, E, A, B, C, f4, K4, expand (in, 62));
  subRound (C, D, E, A, B, f4, K4, expand (in, 63));
  subRound (B, C, D, E, A, f4, K4, expand (in, 64));
  subRound (A, B, C, D, E, f4, K4, expand (in, 65));
  subRound (E, A, B, C, D, f4, K4, expand (in, 66));
  subRound (D, E, A, B, C, f4, K4, expand (in, 67));
  subRound (C, D, E, A, B, f4, K4, expand (in, 68));
  subRound (B, C, D, E, A, f4, K4, expand (in, 69));
  subRound (A, B, C, D, E, f4, K4, expand (in, 70));
  subRound (E, A, B, C, D, f4, K4, expand (in, 71));
  subRound (D, E, A, B, C, f4, K4, expand (in, 72));
  subRound (C, D, E, A, B, f4, K4, expand (in, 73));
  subRound (B, C, D, E, A, f4, K4, expand (in, 74));
  subRound (A, B, C, D, E, f4, K4, expand (in, 75));
  subRound (E, A, B, C, D, f4, K4, expand (in, 76));
  subRound (D, E, A, B, C, f4, K4, expand (in, 77));
  subRound (C, D, E, A, B, f4, K4, expand (in, 78));
  subRound (B, C, D, E, A, f4, K4, expand (in, 79));

  /* Build message digest */
  buf[0] += A;
  buf[1] += B;
  buf[2] += C;
  buf[3] += D;
  buf[4] += E;
}

#undef K1
#undef K2
#undef K3
#undef K4
#undef f1
#undef f2
#undef f3
#undef f4
#undef ROTL
#undef expand
#undef subRound

static void
sha1_sum_update (Sha1sum      *sha1,
                 const guchar *buffer,
                 gsize         count)
{
  guint32 tmp;
  guint dataCount;

  /* Update bitcount */
  tmp = sha1->bits[0];
  if ((sha1->bits[0] = tmp + ((guint32) count << 3) ) < tmp)
    sha1->bits[1] += 1;             /* Carry from low to high */
  sha1->bits[1] += count >> 29;

  /* Get count of bytes already in data */
  dataCount = (guint) (tmp >> 3) & 0x3F;

  /* Handle any leading odd-sized chunks */
  if (dataCount)
    {
      guchar *p = (guchar *) sha1->data + dataCount;

      dataCount = SHA1_DATASIZE - dataCount;
      if (count < dataCount)
        {
          memcpy (p, buffer, count);
          return;
        }
      
      memcpy (p, buffer, dataCount);

      sha_byte_reverse (sha1->data, SHA1_DATASIZE);
      sha1_transform (sha1->buf, sha1->data);

      buffer += dataCount;
      count -= dataCount;
    }

  /* Process data in SHA1_DATASIZE chunks */
  while (count >= SHA1_DATASIZE)
    {
      memcpy (sha1->data, buffer, SHA1_DATASIZE);
      
      sha_byte_reverse (sha1->data, SHA1_DATASIZE);
      sha1_transform (sha1->buf, sha1->data);

      buffer += SHA1_DATASIZE;
      count -= SHA1_DATASIZE;
    }

  /* Handle any remaining bytes of data. */
  memcpy (sha1->data, buffer, count);
}

/* Final wrapup - pad to SHA_DATASIZE-byte boundary with the bit pattern
   1 0* (64-bit count of bits processed, MSB-first) */
static void
sha1_sum_close (Sha1sum *sha1)
{
  gint count;
  guchar *data_p;

  /* Compute number of bytes mod 64 */
  count = (gint) ((sha1->bits[0] >> 3) & 0x3f);

  /* Set the first char of padding to 0x80.  This is safe since there is
     always at least one byte free */
  data_p = (guchar *) sha1->data + count;
  *data_p++ = 0x80;

  /* Bytes of padding needed to make 64 bytes */
  count = SHA1_DATASIZE - 1 - count;

  /* Pad out to 56 mod 64 */
  if (count < 8)
    {
      /* Two lots of padding:  Pad the first block to 64 bytes */
      memset (data_p, 0, count);

      sha_byte_reverse (sha1->data, SHA1_DATASIZE);
      sha1_transform (sha1->buf, sha1->data);

      /* Now fill the next block with 56 bytes */
      memset (sha1->data, 0, SHA1_DATASIZE - 8);
    }
  else
    {
      /* Pad block to 56 bytes */
      memset (data_p, 0, count - 8);
    }

  /* Append length in bits and transform */
  sha1->data[14] = sha1->bits[1];
  sha1->data[15] = sha1->bits[0];

  sha_byte_reverse (sha1->data, SHA1_DATASIZE - 8);
  sha1_transform (sha1->buf, sha1->data);
  sha_byte_reverse (sha1->buf, SHA1_DIGEST_LEN);

  memcpy (sha1->digest, sha1->buf, SHA1_DIGEST_LEN);

  /* Reset buffers in case they contain sensitive data */
  memset (sha1->buf, 0, sizeof (sha1->buf));
  memset (sha1->data, 0, sizeof (sha1->data));
}

static gchar *
sha1_sum_to_string (Sha1sum *sha1)
{
  return digest_to_string (sha1->digest, SHA1_DIGEST_LEN);
}

static void
sha1_sum_digest (Sha1sum *sha1,
                 guint8  *digest)
{
  gint i;

  for (i = 0; i < SHA1_DIGEST_LEN; i++)
    digest[i] = sha1->digest[i];
}

/*
 * SHA-256 Checksum
 */

/* adapted from the SHA256 implementation in gsk/src/hash/gskhash.c.
 *
 * Copyright (C) 2006 Dave Benson
 * Released under the terms of the GNU Lesser General Public License
 */

static void
sha256_sum_init (Sha256sum *sha256)
{
  sha256->buf[0] = 0x6a09e667;
  sha256->buf[1] = 0xbb67ae85;
  sha256->buf[2] = 0x3c6ef372;
  sha256->buf[3] = 0xa54ff53a;
  sha256->buf[4] = 0x510e527f;
  sha256->buf[5] = 0x9b05688c;
  sha256->buf[6] = 0x1f83d9ab;
  sha256->buf[7] = 0x5be0cd19;

  sha256->bits[0] = sha256->bits[1] = 0;
}

#define GET_UINT32(n,b,i)               G_STMT_START{   \
    (n) = ((guint32) (b)[(i)    ] << 24)                \
        | ((guint32) (b)[(i) + 1] << 16)                \
        | ((guint32) (b)[(i) + 2] <<  8)                \
        | ((guint32) (b)[(i) + 3]      ); } G_STMT_END

#define PUT_UINT32(n,b,i)               G_STMT_START{   \
    (b)[(i)    ] = (guint8) ((n) >> 24);                \
    (b)[(i) + 1] = (guint8) ((n) >> 16);                \
    (b)[(i) + 2] = (guint8) ((n) >>  8);                \
    (b)[(i) + 3] = (guint8) ((n)      ); } G_STMT_END

static void
sha256_transform (guint32      buf[8],
                  guint8 const data[64])
{
  guint32 temp1, temp2, W[64];
  guint32 A, B, C, D, E, F, G, H;

  GET_UINT32 (W[0],  data,  0);
  GET_UINT32 (W[1],  data,  4);
  GET_UINT32 (W[2],  data,  8);
  GET_UINT32 (W[3],  data, 12);
  GET_UINT32 (W[4],  data, 16);
  GET_UINT32 (W[5],  data, 20);
  GET_UINT32 (W[6],  data, 24);
  GET_UINT32 (W[7],  data, 28);
  GET_UINT32 (W[8],  data, 32);
  GET_UINT32 (W[9],  data, 36);
  GET_UINT32 (W[10], data, 40);
  GET_UINT32 (W[11], data, 44);
  GET_UINT32 (W[12], data, 48);
  GET_UINT32 (W[13], data, 52);
  GET_UINT32 (W[14], data, 56);
  GET_UINT32 (W[15], data, 60);

#define SHR(x,n)        ((x & 0xFFFFFFFF) >> n)
#define ROTR(x,n)       (SHR (x,n) | (x << (32 - n)))

#define S0(x) (ROTR (x, 7) ^ ROTR (x,18) ^  SHR (x, 3))
#define S1(x) (ROTR (x,17) ^ ROTR (x,19) ^  SHR (x,10))
#define S2(x) (ROTR (x, 2) ^ ROTR (x,13) ^ ROTR (x,22))
#define S3(x) (ROTR (x, 6) ^ ROTR (x,11) ^ ROTR (x,25))

#define F0(x,y,z) ((x & y) | (z & (x | y)))
#define F1(x,y,z) (z ^ (x & (y ^ z)))

#define R(t)    (W[t] = S1(W[t -  2]) + W[t -  7] + \
                        S0(W[t - 15]) + W[t - 16])

#define P(a,b,c,d,e,f,g,h,x,K)          G_STMT_START {  \
        temp1 = h + S3(e) + F1(e,f,g) + K + x;          \
        temp2 = S2(a) + F0(a,b,c);                      \
        d += temp1; h = temp1 + temp2; } G_STMT_END

  A = buf[0];
  B = buf[1];
  C = buf[2];
  D = buf[3];
  E = buf[4];
  F = buf[5];
  G = buf[6];
  H = buf[7];

  P (A, B, C, D, E, F, G, H, W[ 0], 0x428A2F98);
  P (H, A, B, C, D, E, F, G, W[ 1], 0x71374491);
  P (G, H, A, B, C, D, E, F, W[ 2], 0xB5C0FBCF);
  P (F, G, H, A, B, C, D, E, W[ 3], 0xE9B5DBA5);
  P (E, F, G, H, A, B, C, D, W[ 4], 0x3956C25B);
  P (D, E, F, G, H, A, B, C, W[ 5], 0x59F111F1);
  P (C, D, E, F, G, H, A, B, W[ 6], 0x923F82A4);
  P (B, C, D, E, F, G, H, A, W[ 7], 0xAB1C5ED5);
  P (A, B, C, D, E, F, G, H, W[ 8], 0xD807AA98);
  P (H, A, B, C, D, E, F, G, W[ 9], 0x12835B01);
  P (G, H, A, B, C, D, E, F, W[10], 0x243185BE);
  P (F, G, H, A, B, C, D, E, W[11], 0x550C7DC3);
  P (E, F, G, H, A, B, C, D, W[12], 0x72BE5D74);
  P (D, E, F, G, H, A, B, C, W[13], 0x80DEB1FE);
  P (C, D, E, F, G, H, A, B, W[14], 0x9BDC06A7);
  P (B, C, D, E, F, G, H, A, W[15], 0xC19BF174);
  P (A, B, C, D, E, F, G, H, R(16), 0xE49B69C1);
  P (H, A, B, C, D, E, F, G, R(17), 0xEFBE4786);
  P (G, H, A, B, C, D, E, F, R(18), 0x0FC19DC6);
  P (F, G, H, A, B, C, D, E, R(19), 0x240CA1CC);
  P (E, F, G, H, A, B, C, D, R(20), 0x2DE92C6F);
  P (D, E, F, G, H, A, B, C, R(21), 0x4A7484AA);
  P (C, D, E, F, G, H, A, B, R(22), 0x5CB0A9DC);
  P (B, C, D, E, F, G, H, A, R(23), 0x76F988DA);
  P (A, B, C, D, E, F, G, H, R(24), 0x983E5152);
  P (H, A, B, C, D, E, F, G, R(25), 0xA831C66D);
  P (G, H, A, B, C, D, E, F, R(26), 0xB00327C8);
  P (F, G, H, A, B, C, D, E, R(27), 0xBF597FC7);
  P (E, F, G, H, A, B, C, D, R(28), 0xC6E00BF3);
  P (D, E, F, G, H, A, B, C, R(29), 0xD5A79147);
  P (C, D, E, F, G, H, A, B, R(30), 0x06CA6351);
  P (B, C, D, E, F, G, H, A, R(31), 0x14292967);
  P (A, B, C, D, E, F, G, H, R(32), 0x27B70A85);
  P (H, A, B, C, D, E, F, G, R(33), 0x2E1B2138);
  P (G, H, A, B, C, D, E, F, R(34), 0x4D2C6DFC);
  P (F, G, H, A, B, C, D, E, R(35), 0x53380D13);
  P (E, F, G, H, A, B, C, D, R(36), 0x650A7354);
  P (D, E, F, G, H, A, B, C, R(37), 0x766A0ABB);
  P (C, D, E, F, G, H, A, B, R(38), 0x81C2C92E);
  P (B, C, D, E, F, G, H, A, R(39), 0x92722C85);
  P (A, B, C, D, E, F, G, H, R(40), 0xA2BFE8A1);
  P (H, A, B, C, D, E, F, G, R(41), 0xA81A664B);
  P (G, H, A, B, C, D, E, F, R(42), 0xC24B8B70);
  P (F, G, H, A, B, C, D, E, R(43), 0xC76C51A3);
  P (E, F, G, H, A, B, C, D, R(44), 0xD192E819);
  P (D, E, F, G, H, A, B, C, R(45), 0xD6990624);
  P (C, D, E, F, G, H, A, B, R(46), 0xF40E3585);
  P (B, C, D, E, F, G, H, A, R(47), 0x106AA070);
  P (A, B, C, D, E, F, G, H, R(48), 0x19A4C116);
  P (H, A, B, C, D, E, F, G, R(49), 0x1E376C08);
  P (G, H, A, B, C, D, E, F, R(50), 0x2748774C);
  P (F, G, H, A, B, C, D, E, R(51), 0x34B0BCB5);
  P (E, F, G, H, A, B, C, D, R(52), 0x391C0CB3);
  P (D, E, F, G, H, A, B, C, R(53), 0x4ED8AA4A);
  P (C, D, E, F, G, H, A, B, R(54), 0x5B9CCA4F);
  P (B, C, D, E, F, G, H, A, R(55), 0x682E6FF3);
  P (A, B, C, D, E, F, G, H, R(56), 0x748F82EE);
  P (H, A, B, C, D, E, F, G, R(57), 0x78A5636F);
  P (G, H, A, B, C, D, E, F, R(58), 0x84C87814);
  P (F, G, H, A, B, C, D, E, R(59), 0x8CC70208);
  P (E, F, G, H, A, B, C, D, R(60), 0x90BEFFFA);
  P (D, E, F, G, H, A, B, C, R(61), 0xA4506CEB);
  P (C, D, E, F, G, H, A, B, R(62), 0xBEF9A3F7);
  P (B, C, D, E, F, G, H, A, R(63), 0xC67178F2);

#undef SHR
#undef ROTR
#undef S0
#undef S1
#undef S2
#undef S3
#undef F0
#undef F1
#undef R
#undef P

  buf[0] += A;
  buf[1] += B;
  buf[2] += C;
  buf[3] += D;
  buf[4] += E;
  buf[5] += F;
  buf[6] += G;
  buf[7] += H;
}

static void
sha256_sum_update (Sha256sum    *sha256,
                   const guchar *buffer,
                   gsize         length)
{
  guint32 left, fill;
  const guint8 *input = buffer;

  if (length == 0)
    return;

  left = sha256->bits[0] & 0x3F;
  fill = 64 - left;

  sha256->bits[0] += length;
  sha256->bits[0] &= 0xFFFFFFFF;

  if (sha256->bits[0] < length)
      sha256->bits[1]++;

  if (left > 0 && length >= fill)
    {
      memcpy ((sha256->data + left), input, fill);

      sha256_transform (sha256->buf, sha256->data);
      length -= fill;
      input += fill;

      left = 0;
    }

  while (length >= SHA256_DATASIZE)
    {
      sha256_transform (sha256->buf, input);

      length -= 64;
      input += 64;
    }

  if (length)
    memcpy (sha256->data + left, input, length);
}

static guint8 sha256_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void
sha256_sum_close (Sha256sum *sha256)
{
  guint32 last, padn;
  guint32 high, low;
  guint8 msglen[8];

  high = (sha256->bits[0] >> 29)
       | (sha256->bits[1] <<  3);
  low  = (sha256->bits[0] <<  3);

  PUT_UINT32 (high, msglen, 0);
  PUT_UINT32 (low, msglen, 4);

  last = sha256->bits[0] & 0x3F;
  padn = (last < 56) ? (56 - last) : (120 - last);

  sha256_sum_update (sha256, sha256_padding, padn);
  sha256_sum_update (sha256, msglen, 8);

  PUT_UINT32 (sha256->buf[0], sha256->digest,  0);
  PUT_UINT32 (sha256->buf[1], sha256->digest,  4);
  PUT_UINT32 (sha256->buf[2], sha256->digest,  8);
  PUT_UINT32 (sha256->buf[3], sha256->digest, 12);
  PUT_UINT32 (sha256->buf[4], sha256->digest, 16);
  PUT_UINT32 (sha256->buf[5], sha256->digest, 20);
  PUT_UINT32 (sha256->buf[6], sha256->digest, 24);
  PUT_UINT32 (sha256->buf[7], sha256->digest, 28);
}

#undef PUT_UINT32
#undef GET_UINT32

static gchar *
sha256_sum_to_string (Sha256sum *sha256)
{
  return digest_to_string (sha256->digest, SHA256_DIGEST_LEN);
}

static void
sha256_sum_digest (Sha256sum *sha256,
                   guint8    *digest)
{
  gint i;

  for (i = 0; i < SHA256_DIGEST_LEN; i++)
    digest[i] = sha256->digest[i];
}


/*
 * Public API
 */

/**
 * g_checksum_type_get_length:
 * @checksum_type: a #GChecksumType
 *
 * Gets the length in bytes of digests of type @checksum_type
 *
 * Return value: the checksum length, or -1 if @checksum_type is
 * not supported.
 * 
 * Since: 2.16
 */
gssize
g_checksum_type_get_length (GChecksumType checksum_type)
{
  gssize len = -1;

  switch (checksum_type)
    {
    case G_CHECKSUM_MD5:
      len = MD5_DIGEST_LEN;
      break;
    case G_CHECKSUM_SHA1:
      len = SHA1_DIGEST_LEN;
      break;
    case G_CHECKSUM_SHA256:
      len = SHA256_DIGEST_LEN;
      break;
    default:
      len = -1;
      break;
    }

  return len;
}

/**
 * g_checksum_new:
 * @checksum_type: the desired type of checksum
 *
 * Creates a new #GChecksum, using the checksum algorithm @checksum_type. 
 * If the @checksum_type is not known, %NULL is returned.
 * A #GChecksum can be used to compute the checksum, or digest, of an
 * arbitrary binary blob, using different hashing algorithms.
 *
 * A #GChecksum works by feeding a binary blob through g_checksum_update()
 * until there is data to be checked; the digest can then be extracted
 * using g_checksum_get_string(), which will return the checksum as a
 * hexadecimal string; or g_checksum_get_digest(), which will return a
 * vector of raw bytes. Once either g_checksum_get_string() or
 * g_checksum_get_digest() have been called on a #GChecksum, the checksum
 * will be closed and it won't be possible to call g_checksum_update()
 * on it anymore.
 *
 * Return value: the newly created #GChecksum, or %NULL. 
 *   Use g_checksum_free() to free the memory allocated by it.
 *
 * Since: 2.16
 */
GChecksum *
g_checksum_new (GChecksumType checksum_type)
{
  GChecksum *checksum;

  if (! IS_VALID_TYPE (checksum_type))
    return NULL;

  checksum = g_slice_new0 (GChecksum);
  checksum->type = checksum_type;

  g_checksum_reset (checksum);

  return checksum;
}

/**
 * g_checksum_reset:
 * @checksum: the #GChecksum to reset
 *
 * Resets the state of the @checksum back to its initial state.
 *
 * Since: 2.18
 **/
void
g_checksum_reset (GChecksum *checksum)
{
  g_return_if_fail (checksum != NULL);

  g_free (checksum->digest_str);
  checksum->digest_str = NULL;

  switch (checksum->type)
    {
    case G_CHECKSUM_MD5:
      md5_sum_init (&(checksum->sum.md5));
      break;
    case G_CHECKSUM_SHA1:
      sha1_sum_init (&(checksum->sum.sha1));
      break;
    case G_CHECKSUM_SHA256:
      sha256_sum_init (&(checksum->sum.sha256));
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

/**
 * g_checksum_copy:
 * @checksum: the #GChecksum to copy
 *
 * Copies a #GChecksum. If @checksum has been closed, by calling
 * g_checksum_get_string() or g_checksum_get_digest(), the copied
 * checksum will be closed as well.
 *
 * Return value: the copy of the passed #GChecksum. Use g_checksum_free()
 *   when finished using it.
 *
 * Since: 2.16
 */
GChecksum *
g_checksum_copy (const GChecksum *checksum)
{
  GChecksum *copy;

  g_return_val_if_fail (checksum != NULL, NULL);

  copy = g_slice_new (GChecksum);
  *copy = *checksum;

  copy->digest_str = g_strdup (checksum->digest_str);

  return copy;
}

/**
 * g_checksum_free:
 * @checksum: a #GChecksum
 *
 * Frees the memory allocated for @checksum.
 *
 * Since: 2.16
 */
void
g_checksum_free (GChecksum *checksum)
{
  if (G_LIKELY (checksum))
    {
      g_free (checksum->digest_str);

      g_slice_free (GChecksum, checksum);
    }
}

/**
 * g_checksum_update:
 * @checksum: a #GChecksum
 * @data: buffer used to compute the checksum
 * @length: size of the buffer, or -1 if it is a null-terminated string.
 *
 * Feeds @data into an existing #GChecksum. The checksum must still be
 * open, that is g_checksum_get_string() or g_checksum_get_digest() must
 * not have been called on @checksum.
 *
 * Since: 2.16
 */
void
g_checksum_update (GChecksum    *checksum,
                   const guchar *data,
                   gssize        length)
{
  g_return_if_fail (checksum != NULL);
  g_return_if_fail (data != NULL);

  if (length < 0)
    length = strlen ((const gchar *) data);

  if (checksum->digest_str)
    {
      g_warning ("The checksum `%s' has been closed and cannot be updated "
                 "anymore.",
                 checksum->digest_str);
      return;
    }

  switch (checksum->type)
    {
    case G_CHECKSUM_MD5:
      md5_sum_update (&(checksum->sum.md5), data, length);
      break;
    case G_CHECKSUM_SHA1:
      sha1_sum_update (&(checksum->sum.sha1), data, length);
      break;
    case G_CHECKSUM_SHA256:
      sha256_sum_update (&(checksum->sum.sha256), data, length);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

/**
 * g_checksum_get_string:
 * @checksum: a #GChecksum
 *
 * Gets the digest as an hexadecimal string.
 *
 * Once this function has been called the #GChecksum can no longer be
 * updated with g_checksum_update().
 * 
 * The hexadecimal characters will be lower case. 
 *
 * Return value: the hexadecimal representation of the checksum. The
 *   returned string is owned by the checksum and should not be modified
 *   or freed.
 * 
 * Since: 2.16
 */
G_CONST_RETURN gchar *
g_checksum_get_string (GChecksum *checksum)
{
  gchar *str = NULL;

  g_return_val_if_fail (checksum != NULL, NULL);
  
  if (checksum->digest_str)
    return checksum->digest_str;

  switch (checksum->type)
    {
    case G_CHECKSUM_MD5:
      md5_sum_close (&(checksum->sum.md5));
      str = md5_sum_to_string (&(checksum->sum.md5));
      break;
    case G_CHECKSUM_SHA1:
      sha1_sum_close (&(checksum->sum.sha1));
      str = sha1_sum_to_string (&(checksum->sum.sha1));
      break;
    case G_CHECKSUM_SHA256:
      sha256_sum_close (&(checksum->sum.sha256));
      str = sha256_sum_to_string (&(checksum->sum.sha256));
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  checksum->digest_str = str;

  return checksum->digest_str;
}

/**
 * g_checksum_get_digest:
 * @checksum: a #GChecksum
 * @buffer: output buffer
 * @digest_len: an inout parameter. The caller initializes it to the size of @buffer.
 *   After the call it contains the length of the digest.
 *
 * Gets the digest from @checksum as a raw binary vector and places it
 * into @buffer. The size of the digest depends on the type of checksum.
 *
 * Once this function has been called, the #GChecksum is closed and can
 * no longer be updated with g_checksum_update().
 *
 * Since: 2.16
 */
void
g_checksum_get_digest (GChecksum  *checksum,
                       guint8     *buffer,
                       gsize      *digest_len)
{
  gboolean checksum_open = FALSE;
  gchar *str = NULL;
  gsize len;

  g_return_if_fail (checksum != NULL);

  len = g_checksum_type_get_length (checksum->type);
  g_return_if_fail (*digest_len >= len);

  checksum_open = !!(checksum->digest_str == NULL);

  switch (checksum->type)
    {
    case G_CHECKSUM_MD5:
      if (checksum_open)
        {
          md5_sum_close (&(checksum->sum.md5));
          str = md5_sum_to_string (&(checksum->sum.md5));
        }
      md5_sum_digest (&(checksum->sum.md5), buffer);
      break;
    case G_CHECKSUM_SHA1:
      if (checksum_open)
        {
          sha1_sum_close (&(checksum->sum.sha1));
          str = sha1_sum_to_string (&(checksum->sum.sha1));
        }
      sha1_sum_digest (&(checksum->sum.sha1), buffer);
      break;
    case G_CHECKSUM_SHA256:
      if (checksum_open)
        {
          sha256_sum_close (&(checksum->sum.sha256));
          str = sha256_sum_to_string (&(checksum->sum.sha256));
        }
      sha256_sum_digest (&(checksum->sum.sha256), buffer);
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  if (str)
    checksum->digest_str = str;

  *digest_len = len;
}

/**
 * g_compute_checksum_for_data:
 * @checksum_type: a #GChecksumType
 * @data: binary blob to compute the digest of
 * @length: length of @data
 *
 * Computes the checksum for a binary @data of @length. This is a
 * convenience wrapper for g_checksum_new(), g_checksum_get_string()
 * and g_checksum_free().
 * 
 * The hexadecimal string returned will be in lower case.
 *
 * Return value: the digest of the binary data as a string in hexadecimal.
 *   The returned string should be freed with g_free() when done using it.
 *
 * Since: 2.16
 */
gchar *
g_compute_checksum_for_data (GChecksumType  checksum_type,
                             const guchar  *data,
                             gsize          length)
{
  GChecksum *checksum;
  gchar *retval;

  g_return_val_if_fail (IS_VALID_TYPE (checksum_type), NULL);
  g_return_val_if_fail (data != NULL, NULL);

  checksum = g_checksum_new (checksum_type);
  if (!checksum)
    return NULL;

  g_checksum_update (checksum, data, length);
  retval = g_strdup (g_checksum_get_string (checksum));
  g_checksum_free (checksum);

  return retval;
}

/**
 * g_compute_checksum_for_string:
 * @checksum_type: a #GChecksumType
 * @str: the string to compute the checksum of
 * @length: the length of the string, or -1 if the string is null-terminated.
 *
 * Computes the checksum of a string.
 * 
 * The hexadecimal string returned will be in lower case.
 *
 * Return value: the checksum as a hexadecimal string. The returned string
 *   should be freed with g_free() when done using it.
 *
 * Since: 2.16
 */
gchar *
g_compute_checksum_for_string (GChecksumType  checksum_type,
                               const gchar   *str,
                               gssize         length)
{
  g_return_val_if_fail (IS_VALID_TYPE (checksum_type), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  if (length < 0)
    length = strlen (str);

  return g_compute_checksum_for_data (checksum_type, (const guchar *) str, length);
}

#define __G_CHECKSUM_C__
#include "galiasdef.c"
