/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcpy(), memset() */
#ifdef _MSC_VER
#include <winsock.h> /* for ntohl() */
#elif defined FLAC__SYS_DARWIN
#include <machine/endian.h> /* for ntohl() */
#elif defined __MINGW32__
#include <winsock.h> /* for ntohl() */
#else
#include <netinet/in.h> /* for ntohl() */
#endif
#if 0 /* UNUSED */
#include "private/bitmath.h"
#endif
#include "private/bitwriter.h"
#include "private/crc.h"
#include "FLAC/assert.h"
#include "share/alloc.h"

/* Things should be fastest when this matches the machine word size */
/* WATCHOUT: if you change this you must also change the following #defines down to SWAP_BE_WORD_TO_HOST below to match */
/* WATCHOUT: there are a few places where the code will not work unless bwword is >= 32 bits wide */
typedef FLAC__uint32 bwword;
#define FLAC__BYTES_PER_WORD 4
#define FLAC__BITS_PER_WORD 32
#define FLAC__WORD_ALL_ONES ((FLAC__uint32)0xffffffff)
/* SWAP_BE_WORD_TO_HOST swaps bytes in a bwword (which is always big-endian) if necessary to match host byte order */
#if WORDS_BIGENDIAN
#define SWAP_BE_WORD_TO_HOST(x) (x)
#else
#ifdef _MSC_VER
#define SWAP_BE_WORD_TO_HOST(x) local_swap32_(x)
#else
#define SWAP_BE_WORD_TO_HOST(x) ntohl(x)
#endif
#endif

/*
 * The default capacity here doesn't matter too much.  The buffer always grows
 * to hold whatever is written to it.  Usually the encoder will stop adding at
 * a frame or metadata block, then write that out and clear the buffer for the
 * next one.
 */
static const unsigned FLAC__BITWRITER_DEFAULT_CAPACITY = 32768u / sizeof(bwword); /* size in words */
/* When growing, increment 4K at a time */
static const unsigned FLAC__BITWRITER_DEFAULT_INCREMENT = 4096u / sizeof(bwword); /* size in words */

#define FLAC__WORDS_TO_BITS(words) ((words) * FLAC__BITS_PER_WORD)
#define FLAC__TOTAL_BITS(bw) (FLAC__WORDS_TO_BITS((bw)->words) + (bw)->bits)

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

/* adjust for compilers that can't understand using LLU suffix for uint64_t literals */
#ifdef _MSC_VER
#define FLAC__U64L(x) x
#else
#define FLAC__U64L(x) x##LLU
#endif

#ifndef FLaC__INLINE
#define FLaC__INLINE
#endif

struct FLAC__BitWriter {
	bwword *buffer;
	bwword accum; /* accumulator; bits are right-justified; when full, accum is appended to buffer */
	unsigned capacity; /* capacity of buffer in words */
	unsigned words; /* # of complete words in buffer */
	unsigned bits; /* # of used bits in accum */
};

#ifdef _MSC_VER
/* OPT: an MSVC built-in would be better */
static _inline FLAC__uint32 local_swap32_(FLAC__uint32 x)
{
	x = ((x<<8)&0xFF00FF00) | ((x>>8)&0x00FF00FF);
	return (x>>16) | (x<<16);
}
#endif

/* * WATCHOUT: The current implementation only grows the buffer. */
static FLAC__bool bitwriter_grow_(FLAC__BitWriter *bw, unsigned bits_to_add)
{
	unsigned new_capacity;
	bwword *new_buffer;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);

	/* calculate total words needed to store 'bits_to_add' additional bits */
	new_capacity = bw->words + ((bw->bits + bits_to_add + FLAC__BITS_PER_WORD - 1) / FLAC__BITS_PER_WORD);

	/* it's possible (due to pessimism in the growth estimation that
	 * leads to this call) that we don't actually need to grow
	 */
	if(bw->capacity >= new_capacity)
		return true;

	/* round up capacity increase to the nearest FLAC__BITWRITER_DEFAULT_INCREMENT */
	if((new_capacity - bw->capacity) % FLAC__BITWRITER_DEFAULT_INCREMENT)
		new_capacity += FLAC__BITWRITER_DEFAULT_INCREMENT - ((new_capacity - bw->capacity) % FLAC__BITWRITER_DEFAULT_INCREMENT);
	/* make sure we got everything right */
	FLAC__ASSERT(0 == (new_capacity - bw->capacity) % FLAC__BITWRITER_DEFAULT_INCREMENT);
	FLAC__ASSERT(new_capacity > bw->capacity);
	FLAC__ASSERT(new_capacity >= bw->words + ((bw->bits + bits_to_add + FLAC__BITS_PER_WORD - 1) / FLAC__BITS_PER_WORD));

	new_buffer = (bwword*)safe_realloc_mul_2op_(bw->buffer, sizeof(bwword), /*times*/new_capacity);
	if(new_buffer == 0)
		return false;
	bw->buffer = new_buffer;
	bw->capacity = new_capacity;
	return true;
}


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

FLAC__BitWriter *FLAC__bitwriter_new(void)
{
	FLAC__BitWriter *bw = (FLAC__BitWriter*)calloc(1, sizeof(FLAC__BitWriter));
	/* note that calloc() sets all members to 0 for us */
	return bw;
}

void FLAC__bitwriter_delete(FLAC__BitWriter *bw)
{
	FLAC__ASSERT(0 != bw);

	FLAC__bitwriter_free(bw);
	free(bw);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__bool FLAC__bitwriter_init(FLAC__BitWriter *bw)
{
	FLAC__ASSERT(0 != bw);

	bw->words = bw->bits = 0;
	bw->capacity = FLAC__BITWRITER_DEFAULT_CAPACITY;
	bw->buffer = (bwword*)malloc(sizeof(bwword) * bw->capacity);
	if(bw->buffer == 0)
		return false;

	return true;
}

void FLAC__bitwriter_free(FLAC__BitWriter *bw)
{
	FLAC__ASSERT(0 != bw);

	if(0 != bw->buffer)
		free(bw->buffer);
	bw->buffer = 0;
	bw->capacity = 0;
	bw->words = bw->bits = 0;
}

void FLAC__bitwriter_clear(FLAC__BitWriter *bw)
{
	bw->words = bw->bits = 0;
}

void FLAC__bitwriter_dump(const FLAC__BitWriter *bw, FILE *out)
{
	unsigned i, j;
	if(bw == 0) {
		fprintf(out, "bitwriter is NULL\n");
	}
	else {
		fprintf(out, "bitwriter: capacity=%u words=%u bits=%u total_bits=%u\n", bw->capacity, bw->words, bw->bits, FLAC__TOTAL_BITS(bw));

		for(i = 0; i < bw->words; i++) {
			fprintf(out, "%08X: ", i);
			for(j = 0; j < FLAC__BITS_PER_WORD; j++)
				fprintf(out, "%01u", bw->buffer[i] & (1 << (FLAC__BITS_PER_WORD-j-1)) ? 1:0);
			fprintf(out, "\n");
		}
		if(bw->bits > 0) {
			fprintf(out, "%08X: ", i);
			for(j = 0; j < bw->bits; j++)
				fprintf(out, "%01u", bw->accum & (1 << (bw->bits-j-1)) ? 1:0);
			fprintf(out, "\n");
		}
	}
}

FLAC__bool FLAC__bitwriter_get_write_crc16(FLAC__BitWriter *bw, FLAC__uint16 *crc)
{
	const FLAC__byte *buffer;
	size_t bytes;

	FLAC__ASSERT((bw->bits & 7) == 0); /* assert that we're byte-aligned */

	if(!FLAC__bitwriter_get_buffer(bw, &buffer, &bytes))
		return false;

	*crc = (FLAC__uint16)FLAC__crc16(buffer, bytes);
	FLAC__bitwriter_release_buffer(bw);
	return true;
}

FLAC__bool FLAC__bitwriter_get_write_crc8(FLAC__BitWriter *bw, FLAC__byte *crc)
{
	const FLAC__byte *buffer;
	size_t bytes;

	FLAC__ASSERT((bw->bits & 7) == 0); /* assert that we're byte-aligned */

	if(!FLAC__bitwriter_get_buffer(bw, &buffer, &bytes))
		return false;

	*crc = FLAC__crc8(buffer, bytes);
	FLAC__bitwriter_release_buffer(bw);
	return true;
}

FLAC__bool FLAC__bitwriter_is_byte_aligned(const FLAC__BitWriter *bw)
{
	return ((bw->bits & 7) == 0);
}

unsigned FLAC__bitwriter_get_input_bits_unconsumed(const FLAC__BitWriter *bw)
{
	return FLAC__TOTAL_BITS(bw);
}

FLAC__bool FLAC__bitwriter_get_buffer(FLAC__BitWriter *bw, const FLAC__byte **buffer, size_t *bytes)
{
	FLAC__ASSERT((bw->bits & 7) == 0);
	/* double protection */
	if(bw->bits & 7)
		return false;
	/* if we have bits in the accumulator we have to flush those to the buffer first */
	if(bw->bits) {
		FLAC__ASSERT(bw->words <= bw->capacity);
		if(bw->words == bw->capacity && !bitwriter_grow_(bw, FLAC__BITS_PER_WORD))
			return false;
		/* append bits as complete word to buffer, but don't change bw->accum or bw->bits */
		bw->buffer[bw->words] = SWAP_BE_WORD_TO_HOST(bw->accum << (FLAC__BITS_PER_WORD-bw->bits));
	}
	/* now we can just return what we have */
	*buffer = (FLAC__byte*)bw->buffer;
	*bytes = (FLAC__BYTES_PER_WORD * bw->words) + (bw->bits >> 3);
	return true;
}

void FLAC__bitwriter_release_buffer(FLAC__BitWriter *bw)
{
	/* nothing to do.  in the future, strict checking of a 'writer-is-in-
	 * get-mode' flag could be added everywhere and then cleared here
	 */
	(void)bw;
}

FLaC__INLINE FLAC__bool FLAC__bitwriter_write_zeroes(FLAC__BitWriter *bw, unsigned bits)
{
	unsigned n;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);

	if(bits == 0)
		return true;
	/* slightly pessimistic size check but faster than "<= bw->words + (bw->bits+bits+FLAC__BITS_PER_WORD-1)/FLAC__BITS_PER_WORD" */
	if(bw->capacity <= bw->words + bits && !bitwriter_grow_(bw, bits))
		return false;
	/* first part gets to word alignment */
	if(bw->bits) {
		n = min(FLAC__BITS_PER_WORD - bw->bits, bits);
		bw->accum <<= n;
		bits -= n;
		bw->bits += n;
		if(bw->bits == FLAC__BITS_PER_WORD) {
			bw->buffer[bw->words++] = SWAP_BE_WORD_TO_HOST(bw->accum);
			bw->bits = 0;
		}
		else
			return true;
	}
	/* do whole words */
	while(bits >= FLAC__BITS_PER_WORD) {
		bw->buffer[bw->words++] = 0;
		bits -= FLAC__BITS_PER_WORD;
	}
	/* do any leftovers */
	if(bits > 0) {
		bw->accum = 0;
		bw->bits = bits;
	}
	return true;
}

FLaC__INLINE FLAC__bool FLAC__bitwriter_write_raw_uint32(FLAC__BitWriter *bw, FLAC__uint32 val, unsigned bits)
{
	register unsigned left;

	/* WATCHOUT: code does not work with <32bit words; we can make things much faster with this assertion */
	FLAC__ASSERT(FLAC__BITS_PER_WORD >= 32);

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);

	FLAC__ASSERT(bits <= 32);
	if(bits == 0)
		return true;

	/* slightly pessimistic size check but faster than "<= bw->words + (bw->bits+bits+FLAC__BITS_PER_WORD-1)/FLAC__BITS_PER_WORD" */
	if(bw->capacity <= bw->words + bits && !bitwriter_grow_(bw, bits))
		return false;

	left = FLAC__BITS_PER_WORD - bw->bits;
	if(bits < left) {
		bw->accum <<= bits;
		bw->accum |= val;
		bw->bits += bits;
	}
	else if(bw->bits) { /* WATCHOUT: if bw->bits == 0, left==FLAC__BITS_PER_WORD and bw->accum<<=left is a NOP instead of setting to 0 */
		bw->accum <<= left;
		bw->accum |= val >> (bw->bits = bits - left);
		bw->buffer[bw->words++] = SWAP_BE_WORD_TO_HOST(bw->accum);
		bw->accum = val;
	}
	else {
		bw->accum = val;
		bw->bits = 0;
		bw->buffer[bw->words++] = SWAP_BE_WORD_TO_HOST(val);
	}

	return true;
}

FLaC__INLINE FLAC__bool FLAC__bitwriter_write_raw_int32(FLAC__BitWriter *bw, FLAC__int32 val, unsigned bits)
{
	/* zero-out unused bits */
	if(bits < 32)
		val &= (~(0xffffffff << bits));

	return FLAC__bitwriter_write_raw_uint32(bw, (FLAC__uint32)val, bits);
}

FLaC__INLINE FLAC__bool FLAC__bitwriter_write_raw_uint64(FLAC__BitWriter *bw, FLAC__uint64 val, unsigned bits)
{
	/* this could be a little faster but it's not used for much */
	if(bits > 32) {
		return
			FLAC__bitwriter_write_raw_uint32(bw, (FLAC__uint32)(val>>32), bits-32) &&
			FLAC__bitwriter_write_raw_uint32(bw, (FLAC__uint32)val, 32);
	}
	else
		return FLAC__bitwriter_write_raw_uint32(bw, (FLAC__uint32)val, bits);
}

FLaC__INLINE FLAC__bool FLAC__bitwriter_write_raw_uint32_little_endian(FLAC__BitWriter *bw, FLAC__uint32 val)
{
	/* this doesn't need to be that fast as currently it is only used for vorbis comments */

	if(!FLAC__bitwriter_write_raw_uint32(bw, val & 0xff, 8))
		return false;
	if(!FLAC__bitwriter_write_raw_uint32(bw, (val>>8) & 0xff, 8))
		return false;
	if(!FLAC__bitwriter_write_raw_uint32(bw, (val>>16) & 0xff, 8))
		return false;
	if(!FLAC__bitwriter_write_raw_uint32(bw, val>>24, 8))
		return false;

	return true;
}

FLaC__INLINE FLAC__bool FLAC__bitwriter_write_byte_block(FLAC__BitWriter *bw, const FLAC__byte vals[], unsigned nvals)
{
	unsigned i;

	/* this could be faster but currently we don't need it to be since it's only used for writing metadata */
	for(i = 0; i < nvals; i++) {
		if(!FLAC__bitwriter_write_raw_uint32(bw, (FLAC__uint32)(vals[i]), 8))
			return false;
	}

	return true;
}

FLAC__bool FLAC__bitwriter_write_unary_unsigned(FLAC__BitWriter *bw, unsigned val)
{
	if(val < 32)
		return FLAC__bitwriter_write_raw_uint32(bw, 1, ++val);
	else
		return
			FLAC__bitwriter_write_zeroes(bw, val) &&
			FLAC__bitwriter_write_raw_uint32(bw, 1, 1);
}

unsigned FLAC__bitwriter_rice_bits(FLAC__int32 val, unsigned parameter)
{
	FLAC__uint32 uval;

	FLAC__ASSERT(parameter < sizeof(unsigned)*8);

	/* fold signed to unsigned; actual formula is: negative(v)? -2v-1 : 2v */
	uval = (val<<1) ^ (val>>31);

	return 1 + parameter + (uval >> parameter);
}

#if 0 /* UNUSED */
unsigned FLAC__bitwriter_golomb_bits_signed(int val, unsigned parameter)
{
	unsigned bits, msbs, uval;
	unsigned k;

	FLAC__ASSERT(parameter > 0);

	/* fold signed to unsigned */
	if(val < 0)
		uval = (unsigned)(((-(++val)) << 1) + 1);
	else
		uval = (unsigned)(val << 1);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		FLAC__ASSERT(k <= 30);

		msbs = uval >> k;
		bits = 1 + k + msbs;
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);

		bits = 1 + q + k;
		if(r >= d)
			bits++;
	}
	return bits;
}

unsigned FLAC__bitwriter_golomb_bits_unsigned(unsigned uval, unsigned parameter)
{
	unsigned bits, msbs;
	unsigned k;

	FLAC__ASSERT(parameter > 0);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		FLAC__ASSERT(k <= 30);

		msbs = uval >> k;
		bits = 1 + k + msbs;
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);

		bits = 1 + q + k;
		if(r >= d)
			bits++;
	}
	return bits;
}
#endif /* UNUSED */

FLAC__bool FLAC__bitwriter_write_rice_signed(FLAC__BitWriter *bw, FLAC__int32 val, unsigned parameter)
{
	unsigned total_bits, interesting_bits, msbs;
	FLAC__uint32 uval, pattern;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);
	FLAC__ASSERT(parameter < 8*sizeof(uval));

	/* fold signed to unsigned; actual formula is: negative(v)? -2v-1 : 2v */
	uval = (val<<1) ^ (val>>31);

	msbs = uval >> parameter;
	interesting_bits = 1 + parameter;
	total_bits = interesting_bits + msbs;
	pattern = 1 << parameter; /* the unary end bit */
	pattern |= (uval & ((1<<parameter)-1)); /* the binary LSBs */

	if(total_bits <= 32)
		return FLAC__bitwriter_write_raw_uint32(bw, pattern, total_bits);
	else
		return
			FLAC__bitwriter_write_zeroes(bw, msbs) && /* write the unary MSBs */
			FLAC__bitwriter_write_raw_uint32(bw, pattern, interesting_bits); /* write the unary end bit and binary LSBs */
}

FLAC__bool FLAC__bitwriter_write_rice_signed_block(FLAC__BitWriter *bw, const FLAC__int32 *vals, unsigned nvals, unsigned parameter)
{
	const FLAC__uint32 mask1 = FLAC__WORD_ALL_ONES << parameter; /* we val|=mask1 to set the stop bit above it... */
	const FLAC__uint32 mask2 = FLAC__WORD_ALL_ONES >> (31-parameter); /* ...then mask off the bits above the stop bit with val&=mask2*/
	FLAC__uint32 uval;
	unsigned left;
	const unsigned lsbits = 1 + parameter;
	unsigned msbits;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);
	FLAC__ASSERT(parameter < 8*sizeof(bwword)-1);
	/* WATCHOUT: code does not work with <32bit words; we can make things much faster with this assertion */
	FLAC__ASSERT(FLAC__BITS_PER_WORD >= 32);

	while(nvals) {
		/* fold signed to unsigned; actual formula is: negative(v)? -2v-1 : 2v */
		uval = (*vals<<1) ^ (*vals>>31);

		msbits = uval >> parameter;

#if 0 /* OPT: can remove this special case if it doesn't make up for the extra compare (doesn't make a statistically significant difference with msvc or gcc/x86) */
		if(bw->bits && bw->bits + msbits + lsbits <= FLAC__BITS_PER_WORD) { /* i.e. if the whole thing fits in the current bwword */
			/* ^^^ if bw->bits is 0 then we may have filled the buffer and have no free bwword to work in */
			bw->bits = bw->bits + msbits + lsbits;
			uval |= mask1; /* set stop bit */
			uval &= mask2; /* mask off unused top bits */
			/* NOT: bw->accum <<= msbits + lsbits because msbits+lsbits could be 32, then the shift would be a NOP */
			bw->accum <<= msbits;
			bw->accum <<= lsbits;
			bw->accum |= uval;
			if(bw->bits == FLAC__BITS_PER_WORD) {
				bw->buffer[bw->words++] = SWAP_BE_WORD_TO_HOST(bw->accum);
				bw->bits = 0;
				/* burying the capacity check down here means we have to grow the buffer a little if there are more vals to do */
				if(bw->capacity <= bw->words && nvals > 1 && !bitwriter_grow_(bw, 1)) {
					FLAC__ASSERT(bw->capacity == bw->words);
					return false;
				}
			}
		}
		else {
#elif 1 /*@@@@@@ OPT: try this version with MSVC6 to see if better, not much difference for gcc-4 */
		if(bw->bits && bw->bits + msbits + lsbits < FLAC__BITS_PER_WORD) { /* i.e. if the whole thing fits in the current bwword */
			/* ^^^ if bw->bits is 0 then we may have filled the buffer and have no free bwword to work in */
			bw->bits = bw->bits + msbits + lsbits;
			uval |= mask1; /* set stop bit */
			uval &= mask2; /* mask off unused top bits */
			bw->accum <<= msbits + lsbits;
			bw->accum |= uval;
		}
		else {
#endif
			/* slightly pessimistic size check but faster than "<= bw->words + (bw->bits+msbits+lsbits+FLAC__BITS_PER_WORD-1)/FLAC__BITS_PER_WORD" */
			/* OPT: pessimism may cause flurry of false calls to grow_ which eat up all savings before it */
			if(bw->capacity <= bw->words + bw->bits + msbits + 1/*lsbits always fit in 1 bwword*/ && !bitwriter_grow_(bw, msbits+lsbits))
				return false;

			if(msbits) {
				/* first part gets to word alignment */
				if(bw->bits) {
					left = FLAC__BITS_PER_WORD - bw->bits;
					if(msbits < left) {
						bw->accum <<= msbits;
						bw->bits += msbits;
						goto break1;
					}
					else {
						bw->accum <<= left;
						msbits -= left;
						bw->buffer[bw->words++] = SWAP_BE_WORD_TO_HOST(bw->accum);
						bw->bits = 0;
					}
				}
				/* do whole words */
				while(msbits >= FLAC__BITS_PER_WORD) {
					bw->buffer[bw->words++] = 0;
					msbits -= FLAC__BITS_PER_WORD;
				}
				/* do any leftovers */
				if(msbits > 0) {
					bw->accum = 0;
					bw->bits = msbits;
				}
			}
break1:
			uval |= mask1; /* set stop bit */
			uval &= mask2; /* mask off unused top bits */

			left = FLAC__BITS_PER_WORD - bw->bits;
			if(lsbits < left) {
				bw->accum <<= lsbits;
				bw->accum |= uval;
				bw->bits += lsbits;
			}
			else {
				/* if bw->bits == 0, left==FLAC__BITS_PER_WORD which will always
				 * be > lsbits (because of previous assertions) so it would have
				 * triggered the (lsbits<left) case above.
				 */
				FLAC__ASSERT(bw->bits);
				FLAC__ASSERT(left < FLAC__BITS_PER_WORD);
				bw->accum <<= left;
				bw->accum |= uval >> (bw->bits = lsbits - left);
				bw->buffer[bw->words++] = SWAP_BE_WORD_TO_HOST(bw->accum);
				bw->accum = uval;
			}
#if 1
		}
#endif
		vals++;
		nvals--;
	}
	return true;
}

#if 0 /* UNUSED */
FLAC__bool FLAC__bitwriter_write_golomb_signed(FLAC__BitWriter *bw, int val, unsigned parameter)
{
	unsigned total_bits, msbs, uval;
	unsigned k;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);
	FLAC__ASSERT(parameter > 0);

	/* fold signed to unsigned */
	if(val < 0)
		uval = (unsigned)(((-(++val)) << 1) + 1);
	else
		uval = (unsigned)(val << 1);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		unsigned pattern;

		FLAC__ASSERT(k <= 30);

		msbs = uval >> k;
		total_bits = 1 + k + msbs;
		pattern = 1 << k; /* the unary end bit */
		pattern |= (uval & ((1u<<k)-1)); /* the binary LSBs */

		if(total_bits <= 32) {
			if(!FLAC__bitwriter_write_raw_uint32(bw, pattern, total_bits))
				return false;
		}
		else {
			/* write the unary MSBs */
			if(!FLAC__bitwriter_write_zeroes(bw, msbs))
				return false;
			/* write the unary end bit and binary LSBs */
			if(!FLAC__bitwriter_write_raw_uint32(bw, pattern, k+1))
				return false;
		}
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);
		/* write the unary MSBs */
		if(!FLAC__bitwriter_write_zeroes(bw, q))
			return false;
		/* write the unary end bit */
		if(!FLAC__bitwriter_write_raw_uint32(bw, 1, 1))
			return false;
		/* write the binary LSBs */
		if(r >= d) {
			if(!FLAC__bitwriter_write_raw_uint32(bw, r+d, k+1))
				return false;
		}
		else {
			if(!FLAC__bitwriter_write_raw_uint32(bw, r, k))
				return false;
		}
	}
	return true;
}

FLAC__bool FLAC__bitwriter_write_golomb_unsigned(FLAC__BitWriter *bw, unsigned uval, unsigned parameter)
{
	unsigned total_bits, msbs;
	unsigned k;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);
	FLAC__ASSERT(parameter > 0);

	k = FLAC__bitmath_ilog2(parameter);
	if(parameter == 1u<<k) {
		unsigned pattern;

		FLAC__ASSERT(k <= 30);

		msbs = uval >> k;
		total_bits = 1 + k + msbs;
		pattern = 1 << k; /* the unary end bit */
		pattern |= (uval & ((1u<<k)-1)); /* the binary LSBs */

		if(total_bits <= 32) {
			if(!FLAC__bitwriter_write_raw_uint32(bw, pattern, total_bits))
				return false;
		}
		else {
			/* write the unary MSBs */
			if(!FLAC__bitwriter_write_zeroes(bw, msbs))
				return false;
			/* write the unary end bit and binary LSBs */
			if(!FLAC__bitwriter_write_raw_uint32(bw, pattern, k+1))
				return false;
		}
	}
	else {
		unsigned q, r, d;

		d = (1 << (k+1)) - parameter;
		q = uval / parameter;
		r = uval - (q * parameter);
		/* write the unary MSBs */
		if(!FLAC__bitwriter_write_zeroes(bw, q))
			return false;
		/* write the unary end bit */
		if(!FLAC__bitwriter_write_raw_uint32(bw, 1, 1))
			return false;
		/* write the binary LSBs */
		if(r >= d) {
			if(!FLAC__bitwriter_write_raw_uint32(bw, r+d, k+1))
				return false;
		}
		else {
			if(!FLAC__bitwriter_write_raw_uint32(bw, r, k))
				return false;
		}
	}
	return true;
}
#endif /* UNUSED */

FLAC__bool FLAC__bitwriter_write_utf8_uint32(FLAC__BitWriter *bw, FLAC__uint32 val)
{
	FLAC__bool ok = 1;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);

	FLAC__ASSERT(!(val & 0x80000000)); /* this version only handles 31 bits */

	if(val < 0x80) {
		return FLAC__bitwriter_write_raw_uint32(bw, val, 8);
	}
	else if(val < 0x800) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xC0 | (val>>6), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (val&0x3F), 8);
	}
	else if(val < 0x10000) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xE0 | (val>>12), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (val&0x3F), 8);
	}
	else if(val < 0x200000) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xF0 | (val>>18), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>12)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (val&0x3F), 8);
	}
	else if(val < 0x4000000) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xF8 | (val>>24), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>18)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>12)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (val&0x3F), 8);
	}
	else {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xFC | (val>>30), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>24)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>18)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>12)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | ((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (val&0x3F), 8);
	}

	return ok;
}

FLAC__bool FLAC__bitwriter_write_utf8_uint64(FLAC__BitWriter *bw, FLAC__uint64 val)
{
	FLAC__bool ok = 1;

	FLAC__ASSERT(0 != bw);
	FLAC__ASSERT(0 != bw->buffer);

	FLAC__ASSERT(!(val & FLAC__U64L(0xFFFFFFF000000000))); /* this version only handles 36 bits */

	if(val < 0x80) {
		return FLAC__bitwriter_write_raw_uint32(bw, (FLAC__uint32)val, 8);
	}
	else if(val < 0x800) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xC0 | (FLAC__uint32)(val>>6), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)(val&0x3F), 8);
	}
	else if(val < 0x10000) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xE0 | (FLAC__uint32)(val>>12), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)(val&0x3F), 8);
	}
	else if(val < 0x200000) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xF0 | (FLAC__uint32)(val>>18), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)(val&0x3F), 8);
	}
	else if(val < 0x4000000) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xF8 | (FLAC__uint32)(val>>24), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>18)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)(val&0x3F), 8);
	}
	else if(val < 0x80000000) {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xFC | (FLAC__uint32)(val>>30), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>24)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>18)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)(val&0x3F), 8);
	}
	else {
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0xFE, 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>30)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>24)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>18)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>12)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)((val>>6)&0x3F), 8);
		ok &= FLAC__bitwriter_write_raw_uint32(bw, 0x80 | (FLAC__uint32)(val&0x3F), 8);
	}

	return ok;
}

FLAC__bool FLAC__bitwriter_zero_pad_to_byte_boundary(FLAC__BitWriter *bw)
{
	/* 0-pad to byte boundary */
	if(bw->bits & 7u)
		return FLAC__bitwriter_write_zeroes(bw, 8 - (bw->bits & 7u));
	else
		return true;
}
