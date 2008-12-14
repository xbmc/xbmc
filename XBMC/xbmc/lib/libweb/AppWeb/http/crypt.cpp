///
///	@file 	crypt.cpp
/// @brief 	One line description
///	@overview This file provides facilities for basic encoding and 
///		decoding using the base64 encoding scheme and the MD5 
///		Message-Digest algorithms developed by RSA. This module is 
///		used by both basic and digest authentication services. The 
///		base64 encode/decode algorithms originally from GoAhead.
///
/////////////////////////////////// Copyright //////////////////////////////////
//
//	@copy	default.g.r
//	
//	MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	Portions Copyright (C) 1991-2, RSA Data Security, Inc. All rights reserved. 
//	Portions Copyright (C) 1995-2000, GoAhead Software. All rights reserved. 
//	
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions
//	are met:
//	
//	1. Redistributions of source code must retain the above copyright
//	   notice, this list of conditions and the following disclaimer.
//	
//	2. Redistributions in binary form must reproduce the above copyright
//	   notice, this list of conditions and the following disclaimer in the
//	   documentation and/or other materials provided with the distribution.
//	
//	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
//	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
//	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
//	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
//	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
//	SUCH DAMAGE.
//	
//	RSA License Details
//	-------------------
//	
//	License to copy and use this software is granted provided that it is 
//	identified as the "RSA Data Security, Inc. MD5 Message-Digest Algorithm" 
//	in all material mentioning or referencing this software or this function.
//	
//	License is also granted to make and use derivative works provided that such
//	works are identified as "derived from the RSA Data Security, Inc. MD5 
//	Message-Digest Algorithm" in all material mentioning or referencing the 
//	derived work.
//	
//	RSA Data Security, Inc. makes no representations concerning either the 
//	merchantability of this software or the suitability of this software for 
//	any particular purpose. It is provided "as is" without express or implied 
//	warranty of any kind.
//	
//	These notices must be retained in any copies of any part of this
//	documentation and/or software.
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"shared.h"

//////////////////////////////////// Locals ////////////////////////////////////
//
//	Constants for MD5Transform routine.
//
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static uchar PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// F, G, H and I are basic MD5 functions.
 
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

// ROTATE_LEFT rotates x left n bits.
 
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
 
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

//////////////////////////////// Base 64 Data //////////////////////////////////

#define CRYPT_HASH_SIZE   16

//
//	Encoding map lookup
//
static char	encodeMap[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
};

//
//	Decode map
//
static signed char decodeMap[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

//////////////////////////// Forward Declarations //////////////////////////////

static void MD5Transform(uint [4], uchar [64]);
static void Encode(uchar *, uint *, uint);
static void Decode(uint *, uchar *, uint);
static void MD5_memcpy(uchar *, uchar *, uint);
static void MD5_memset(uchar *, int, uint);

//////////////////////////////////// Code //////////////////////////////////////

int maDecode64(char *buffer, int bufsize, char *s)
{
	uint		bitBuf;
	char		*bp;
	int			c, i, j, shift;

	bp = buffer;
	*bp = '\0';
	while (*s && *s != '=') {
		bitBuf = 0;
		shift = 18;
		for (i = 0; i < 4 && *s && *s != '='; i++, s++) {
			c = decodeMap[*s & 0xff];
			if (c == -1) {
				return -1;
			} 
			bitBuf = bitBuf | (c << shift);
			shift -= 6;
		}
		--i;
		if ((bp + i) >= &buffer[bufsize]) {
			*buffer = '\0';
			return MPR_ERR_WONT_FIT;
		}
		for (j = 0; j < i; j++) {
			*bp++ = (char) ((bitBuf >> (8 * (2 - j))) & 0xff);
		}
		*bp = '\0';
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void maEncode64(char *buffer, int bufsize, char *s)
{
	uint	shiftbuf;
	char	*bp;
	int		x, i, j, shift;

	bp = buffer;
	*bp = '\0';
	while (*s) {
		shiftbuf = 0;
		for (j = 2; j >= 0 && *s; j--, s++) {
			shiftbuf |= ((*s & 0xff) << (j * 8));
		}
		shift = 18;
		for (i = ++j; i < 4 && bp < &buffer[bufsize] ; i++) {
			x = (shiftbuf >> shift) & 0x3f;
			*bp++ = encodeMap[(shiftbuf >> shift) & 0x3f];
			shift -= 6;
		}
		while (j-- > 0) {
			*bp++ = '=';
		}
		*bp = '\0';
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	maMD5binary returns the MD5 hash. FUTURE -- better name
//

char *maMD5binary(uchar *buf, int length)
{
    const char		*hex = "0123456789abcdef";
    MD5_CONTEXT		md5ctx;
    uchar			hash[CRYPT_HASH_SIZE];
    char			*r, *str;
	char			result[(CRYPT_HASH_SIZE * 2) + 1];
    int				i;

	//
	//	Take the MD5 hash of the string argument.
	//
    maMD5Init(&md5ctx);
    maMD5Update(&md5ctx, buf, (uint) length);
    maMD5Final(hash, &md5ctx);

    for (i = 0, r = result; i < 16; i++) {
		*r++ = hex[hash[i] >> 4];
		*r++ = hex[hash[i] & 0xF];
    }
    *r = '\0';

	str = (char*) mprMalloc(sizeof(result));
	strcpy(str, result);

    return str;
}

///////////////////////////////////////////////////////////////////////////////
//
//	Convenience call to webMD5binary 
// 

char *maMD5(char *string)
{
	return maMD5binary((uchar*)string, strlen(string));
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get a Nonce value for passing along to the client.  This function composes 
//	the string "secret:eTag:time:realm" and calculates the MD5 digest.
// 

int maCalcNonce(char **nonce, char *secret, char *etag, char *realm)
{
	time_t		now;
	char		nonceBuf[256];

	mprAssert(realm && *realm);

	time(&now);

	mprSprintf(nonceBuf, sizeof(nonceBuf), "%s:%s:%x:%s", secret, etag, now, 
		realm); 

    *nonce = maMD5(nonceBuf);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get a Digest value using the MD5 algorithm -- See RFC 2617 to understand 
//	this code.
// 

int maCalcDigest(char *userName, char *password, char *realm, char *uri, 
		char *nonce, char *qop, char *nc, char *cnonce, char *method, 
		char **digest)
{
	char	a1Buf[256], a2Buf[256], digestBuf[256];
	char	*ha1, *ha2;

	mprAssert(qop);

	//
	//	Compute HA1. If userName == 0, then the password is already expected 
	//	to be in the HA1 format (MD5(userName:realm:password). This is the 
	//	format that httpPassword stores all passwords, so we are "sweet".
	//
	if (userName == 0) {
		ha1 = mprStrdup(password);
	} else {
		mprSprintf(a1Buf, sizeof(a1Buf), "%s:%s:%s", userName, realm, password);
		ha1 = maMD5(a1Buf);
	}

	//
	//	HA2
	// 
	mprSprintf(a2Buf, sizeof(a2Buf), "%s:%s", method, uri);
	ha2 = maMD5(a2Buf);

	//
	//	H(HA1:nonce:HA2)
	//
	if (strcmp(qop, "auth") == 0) {
		mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s:%s:%s:%s", ha1, 
			nonce, nc, cnonce, qop, ha2);
	} else if (strcmp(qop, "auth-int") == 0) {
		mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s:%s:%s:%s", ha1, 
			nonce, nc, cnonce, qop, ha2);
	} else {
		mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s", ha1, nonce, ha2);
	}
	*digest = maMD5(digestBuf);
	mprFree(ha1);
	mprFree(ha2);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 initialization. Begins an MD5 operation, writing a new context.
// 

void maMD5Init(MD5_CONTEXT *context)
{
	context->count[0] = context->count[1] = 0;

	//
	// Load constants
	//
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 block update operation. Continues an MD5 message-digest operation, 
//	processing another message block, and updating the context.
//

void maMD5Update(MD5_CONTEXT *context, uchar *input, uint inputLen)
{
	uint 	i, index, partLen;

	index = (uint) ((context->count[0] >> 3) & 0x3F);

	if ((context->count[0] += ((uint)inputLen << 3)) < ((uint)inputLen << 3)){
		context->count[1]++;
	}
	context->count[1] += ((uint)inputLen >> 29);
	partLen = 64 - index;

	if (inputLen >= partLen) {
		MD5_memcpy((uchar*) &context->buffer[index], (uchar*) input, partLen);
		MD5Transform(context->state, context->buffer);

		for (i = partLen; i + 63 < inputLen; i += 64) {
			MD5Transform (context->state, &input[i]);
		}
		index = 0;
	} else {
		i = 0;
	}

	MD5_memcpy((uchar*) &context->buffer[index], (uchar*) &input[i], 
		inputLen-i);
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 finalization. Ends an MD5 message-digest operation, writing the message
//	digest and zeroizing the context.
// 

void maMD5Final(uchar digest[16], MD5_CONTEXT *context)
{
	uchar 	bits[8];
	uint	index, padLen;

	// Save number of bits 
	Encode(bits, context->count, 8);

	// Pad out to 56 mod 64.
	index = (uint)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	maMD5Update(context, PADDING, padLen);

	// Append length (before padding) 
	maMD5Update(context, bits, 8);
	// Store state in digest 
	Encode(digest, context->state, 16);

	// Zeroize sensitive information.
	MD5_memset((uchar*)context, 0, sizeof (*context));
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 basic transformation. Transforms state based on block.
//
 
static void MD5Transform(uint state[4], uchar block[64])
{
	uint a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	Decode (x, block, 64);

	// Round 1 
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); // 1 
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); // 2 
	FF (c, d, a, b, x[ 2], S13, 0x242070db); // 3 
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); // 4 
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); // 5 
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); // 6 
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); // 7 
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); // 8 
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); // 9 
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); // 10 
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); // 11 
	FF (b, c, d, a, x[11], S14, 0x895cd7be); // 12 
	FF (a, b, c, d, x[12], S11, 0x6b901122); // 13 
	FF (d, a, b, c, x[13], S12, 0xfd987193); // 14 
	FF (c, d, a, b, x[14], S13, 0xa679438e); // 15 
	FF (b, c, d, a, x[15], S14, 0x49b40821); // 16 

	// Round 2 
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); // 17 
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); // 18 
	GG (c, d, a, b, x[11], S23, 0x265e5a51); // 19 
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); // 20 
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); // 21 
	GG (d, a, b, c, x[10], S22,  0x2441453); // 22 
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); // 23 
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); // 24 
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); // 25 
	GG (d, a, b, c, x[14], S22, 0xc33707d6); // 26 
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); // 27 
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); // 28 
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); // 29 
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); // 30 
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); // 31 
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); // 32 

	// Round 3 
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); // 33 
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); // 34 
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); // 35 
	HH (b, c, d, a, x[14], S34, 0xfde5380c); // 36 
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); // 37 
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); // 38 
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); // 39 
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); // 40 
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); // 41 
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); // 42 
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); // 43 
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); // 44 
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); // 45 
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); // 46 
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); // 47 
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); // 48 

	// Round 4 
	II (a, b, c, d, x[ 0], S41, 0xf4292244); // 49 
	II (d, a, b, c, x[ 7], S42, 0x432aff97); // 50 
	II (c, d, a, b, x[14], S43, 0xab9423a7); // 51 
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); // 52 
	II (a, b, c, d, x[12], S41, 0x655b59c3); // 53 
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); // 54 
	II (c, d, a, b, x[10], S43, 0xffeff47d); // 55 
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); // 56 
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); // 57 
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); // 58 
	II (c, d, a, b, x[ 6], S43, 0xa3014314); // 59 
	II (b, c, d, a, x[13], S44, 0x4e0811a1); // 60 
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); // 61 
	II (d, a, b, c, x[11], S42, 0xbd3af235); // 62 
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); // 63 
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); // 64 

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	MD5_memset ((uchar*) x, 0, sizeof (x));
}

////////////////////////////////////////////////////////////////////////////////
//
//	Encodes input (uint) into output (uchar). Assumes len is a multiple of 4.
//
 
static void Encode(uchar *output, uint *input, uint len)
{
	uint i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = (uchar) (input[i] & 0xff);
		output[j+1] = (uchar) ((input[i] >> 8) & 0xff);
		output[j+2] = (uchar) ((input[i] >> 16) & 0xff);
		output[j+3] = (uchar) ((input[i] >> 24) & 0xff);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Decodes input (uchar) into output (uint). Assumes len is a multiple of 4.
//
 
static void Decode(uint *output, uchar *input, uint len)
{
	uint 	i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((uint) input[j]) | (((uint) input[j+1]) << 8) |
			(((uint) input[j+2]) << 16) | (((uint) input[j+3]) << 24);
}

////////////////////////////////////////////////////////////////////////////////
//
//	FUTURE: Replace "for loop" with standard memcpy if possible.
//

static void MD5_memcpy(uchar *output, uchar *input, uint len)
{
	uint 	i;

	for (i = 0; i < len; i++)
		output[i] = input[i];
}

////////////////////////////////////////////////////////////////////////////////
//
// FUTURE: Replace "for loop" with standard memset if possible.
//
 
static void MD5_memset(uchar *output, int value, uint len)
{
	uint i;

	for (i = 0; i < len; i++)
		((char*) output)[i] = (char) value;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
