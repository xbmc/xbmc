/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
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

#include "private/memory.h"
#include "FLAC/assert.h"
#include "share/alloc.h"

void *FLAC__memory_alloc_aligned(size_t bytes, void **aligned_address)
{
	void *x;

	FLAC__ASSERT(0 != aligned_address);

#ifdef FLAC__ALIGN_MALLOC_DATA
	/* align on 32-byte (256-bit) boundary */
	x = safe_malloc_add_2op_(bytes, /*+*/31);
#ifdef SIZEOF_VOIDP
#if SIZEOF_VOIDP == 4
		/* could do  *aligned_address = x + ((unsigned) (32 - (((unsigned)x) & 31))) & 31; */
		*aligned_address = (void*)(((unsigned)x + 31) & -32);
#elif SIZEOF_VOIDP == 8
		*aligned_address = (void*)(((FLAC__uint64)x + 31) & (FLAC__uint64)(-((FLAC__int64)32)));
#else
# error  Unsupported sizeof(void*)
#endif
#else
	/* there's got to be a better way to do this right for all archs */
	if(sizeof(void*) == sizeof(unsigned))
		*aligned_address = (void*)(((unsigned)x + 31) & -32);
	else if(sizeof(void*) == sizeof(FLAC__uint64))
		*aligned_address = (void*)(((FLAC__uint64)x + 31) & (FLAC__uint64)(-((FLAC__int64)32)));
	else
		return 0;
#endif
#else
	x = safe_malloc_(bytes);
	*aligned_address = x;
#endif
	return x;
}

FLAC__bool FLAC__memory_alloc_aligned_int32_array(unsigned elements, FLAC__int32 **unaligned_pointer, FLAC__int32 **aligned_pointer)
{
	FLAC__int32 *pu; /* unaligned pointer */
	union { /* union needed to comply with C99 pointer aliasing rules */
		FLAC__int32 *pa; /* aligned pointer */
		void        *pv; /* aligned pointer alias */
	} u;

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	if((size_t)elements > SIZE_MAX / sizeof(*pu)) /* overflow check */
		return false;

	pu = (FLAC__int32*)FLAC__memory_alloc_aligned(sizeof(*pu) * (size_t)elements, &u.pv);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = u.pa;
		return true;
	}
}

FLAC__bool FLAC__memory_alloc_aligned_uint32_array(unsigned elements, FLAC__uint32 **unaligned_pointer, FLAC__uint32 **aligned_pointer)
{
	FLAC__uint32 *pu; /* unaligned pointer */
	union { /* union needed to comply with C99 pointer aliasing rules */
		FLAC__uint32 *pa; /* aligned pointer */
		void         *pv; /* aligned pointer alias */
	} u;

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	if((size_t)elements > SIZE_MAX / sizeof(*pu)) /* overflow check */
		return false;

	pu = (FLAC__uint32*)FLAC__memory_alloc_aligned(sizeof(*pu) * elements, &u.pv);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = u.pa;
		return true;
	}
}

FLAC__bool FLAC__memory_alloc_aligned_uint64_array(unsigned elements, FLAC__uint64 **unaligned_pointer, FLAC__uint64 **aligned_pointer)
{
	FLAC__uint64 *pu; /* unaligned pointer */
	union { /* union needed to comply with C99 pointer aliasing rules */
		FLAC__uint64 *pa; /* aligned pointer */
		void         *pv; /* aligned pointer alias */
	} u;

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	if((size_t)elements > SIZE_MAX / sizeof(*pu)) /* overflow check */
		return false;

	pu = (FLAC__uint64*)FLAC__memory_alloc_aligned(sizeof(*pu) * elements, &u.pv);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = u.pa;
		return true;
	}
}

FLAC__bool FLAC__memory_alloc_aligned_unsigned_array(unsigned elements, unsigned **unaligned_pointer, unsigned **aligned_pointer)
{
	unsigned *pu; /* unaligned pointer */
	union { /* union needed to comply with C99 pointer aliasing rules */
		unsigned *pa; /* aligned pointer */
		void     *pv; /* aligned pointer alias */
	} u;

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	if((size_t)elements > SIZE_MAX / sizeof(*pu)) /* overflow check */
		return false;

	pu = (unsigned*)FLAC__memory_alloc_aligned(sizeof(*pu) * elements, &u.pv);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = u.pa;
		return true;
	}
}

#ifndef FLAC__INTEGER_ONLY_LIBRARY

FLAC__bool FLAC__memory_alloc_aligned_real_array(unsigned elements, FLAC__real **unaligned_pointer, FLAC__real **aligned_pointer)
{
	FLAC__real *pu; /* unaligned pointer */
	union { /* union needed to comply with C99 pointer aliasing rules */
		FLAC__real *pa; /* aligned pointer */
		void       *pv; /* aligned pointer alias */
	} u;

	FLAC__ASSERT(elements > 0);
	FLAC__ASSERT(0 != unaligned_pointer);
	FLAC__ASSERT(0 != aligned_pointer);
	FLAC__ASSERT(unaligned_pointer != aligned_pointer);

	if((size_t)elements > SIZE_MAX / sizeof(*pu)) /* overflow check */
		return false;

	pu = (FLAC__real*)FLAC__memory_alloc_aligned(sizeof(*pu) * elements, &u.pv);
	if(0 == pu) {
		return false;
	}
	else {
		if(*unaligned_pointer != 0)
			free(*unaligned_pointer);
		*unaligned_pointer = pu;
		*aligned_pointer = u.pa;
		return true;
	}
}

#endif
