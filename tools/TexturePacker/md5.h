/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 *
 * Changed so as no longer to depend on Colin Plumb's `usual.h' header
 * definitions; now uses stuff from dpkg's config.h.
 *  - Ian Jackson <ian@chiark.greenend.org.uk>.
 * Still in the public domain.
 */

#ifndef MD5_H
#define MD5_H

#include <string.h>		/* for memcpy() */
#include <stdint.h>

struct MD5Context 
{
  uint32_t buf[4];
  uint32_t bytes[2];
  uint32_t in[16];
};

void MD5Init(struct MD5Context *ctx);
void MD5Update(struct MD5Context *ctx, const uint8_t *buf, unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *ctx);

#endif
