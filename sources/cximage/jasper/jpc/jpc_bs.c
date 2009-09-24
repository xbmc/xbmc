/*
 * Copyright (c) 1999-2000, Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2003 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer Software License
 * 
 * IMAGE POWER JPEG-2000 PUBLIC LICENSE
 * ************************************
 * 
 * GRANT:
 * 
 * Permission is hereby granted, free of charge, to any person (the "User")
 * obtaining a copy of this software and associated documentation, to deal
 * in the JasPer Software without restriction, including without limitation
 * the right to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the JasPer Software (in source and binary forms),
 * and to permit persons to whom the JasPer Software is furnished to do so,
 * provided further that the License Conditions below are met.
 * 
 * License Conditions
 * ******************
 * 
 * A.  Redistributions of source code must retain the above copyright notice,
 * and this list of conditions, and the following disclaimer.
 * 
 * B.  Redistributions in binary form must reproduce the above copyright
 * notice, and this list of conditions, and the following disclaimer in
 * the documentation and/or other materials provided with the distribution.
 * 
 * C.  Neither the name of Image Power, Inc. nor any other contributor
 * (including, but not limited to, the University of British Columbia and
 * Michael David Adams) may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * D.  User agrees that it shall not commence any action against Image Power,
 * Inc., the University of British Columbia, Michael David Adams, or any
 * other contributors (collectively "Licensors") for infringement of any
 * intellectual property rights ("IPR") held by the User in respect of any
 * technology that User owns or has a right to license or sublicense and
 * which is an element required in order to claim compliance with ISO/IEC
 * 15444-1 (i.e., JPEG-2000 Part 1).  "IPR" means all intellectual property
 * rights worldwide arising under statutory or common law, and whether
 * or not perfected, including, without limitation, all (i) patents and
 * patent applications owned or licensable by User; (ii) rights associated
 * with works of authorship including copyrights, copyright applications,
 * copyright registrations, mask work rights, mask work applications,
 * mask work registrations; (iii) rights relating to the protection of
 * trade secrets and confidential information; (iv) any right analogous
 * to those set forth in subsections (i), (ii), or (iii) and any other
 * proprietary rights relating to intangible property (other than trademark,
 * trade dress, or service mark rights); and (v) divisions, continuations,
 * renewals, reissues and extensions of the foregoing (as and to the extent
 * applicable) now existing, hereafter filed, issued or acquired.
 * 
 * E.  If User commences an infringement action against any Licensor(s) then
 * such Licensor(s) shall have the right to terminate User's license and
 * all sublicenses that have been granted hereunder by User to other parties.
 * 
 * F.  This software is for use only in hardware or software products that
 * are compliant with ISO/IEC 15444-1 (i.e., JPEG-2000 Part 1).  No license
 * or right to this Software is granted for products that do not comply
 * with ISO/IEC 15444-1.  The JPEG-2000 Part 1 standard can be purchased
 * from the ISO.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE.
 * NO USE OF THE JASPER SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE JASPER SOFTWARE IS PROVIDED BY THE LICENSORS AND
 * CONTRIBUTORS UNDER THIS LICENSE ON AN ``AS-IS'' BASIS, WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT LIMITATION,
 * WARRANTIES THAT THE JASPER SOFTWARE IS FREE OF DEFECTS, IS MERCHANTABLE,
 * IS FIT FOR A PARTICULAR PURPOSE OR IS NON-INFRINGING.  THOSE INTENDING
 * TO USE THE JASPER SOFTWARE OR MODIFICATIONS THEREOF FOR USE IN HARDWARE
 * OR SOFTWARE PRODUCTS ARE ADVISED THAT THEIR USE MAY INFRINGE EXISTING
 * PATENTS, COPYRIGHTS, TRADEMARKS, OR OTHER INTELLECTUAL PROPERTY RIGHTS.
 * THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE JASPER SOFTWARE
 * IS WITH THE USER.  SHOULD ANY PART OF THE JASPER SOFTWARE PROVE DEFECTIVE
 * IN ANY RESPECT, THE USER (AND NOT THE INITIAL DEVELOPERS, THE UNIVERSITY
 * OF BRITISH COLUMBIA, IMAGE POWER, INC., MICHAEL DAVID ADAMS, OR ANY
 * OTHER CONTRIBUTOR) SHALL ASSUME THE COST OF ANY NECESSARY SERVICING,
 * REPAIR OR CORRECTION.  UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL THEORY,
 * WHETHER TORT (INCLUDING NEGLIGENCE), CONTRACT, OR OTHERWISE, SHALL THE
 * INITIAL DEVELOPER, THE UNIVERSITY OF BRITISH COLUMBIA, IMAGE POWER, INC.,
 * MICHAEL DAVID ADAMS, ANY OTHER CONTRIBUTOR, OR ANY DISTRIBUTOR OF THE
 * JASPER SOFTWARE, OR ANY SUPPLIER OF ANY OF SUCH PARTIES, BE LIABLE TO
 * THE USER OR ANY OTHER PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES OF ANY CHARACTER INCLUDING, WITHOUT LIMITATION,
 * DAMAGES FOR LOSS OF GOODWILL, WORK STOPPAGE, COMPUTER FAILURE OR
 * MALFUNCTION, OR ANY AND ALL OTHER COMMERCIAL DAMAGES OR LOSSES, EVEN IF
 * SUCH PARTY HAD BEEN INFORMED, OR OUGHT TO HAVE KNOWN, OF THE POSSIBILITY
 * OF SUCH DAMAGES.  THE JASPER SOFTWARE AND UNDERLYING TECHNOLOGY ARE NOT
 * FAULT-TOLERANT AND ARE NOT DESIGNED, MANUFACTURED OR INTENDED FOR USE OR
 * RESALE AS ON-LINE CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING
 * FAIL-SAFE PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT
 * LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 * JASPER SOFTWARE OR UNDERLYING TECHNOLOGY OR PRODUCT COULD LEAD DIRECTLY
 * TO DEATH, PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE
 * ("HIGH RISK ACTIVITIES").  LICENSOR SPECIFICALLY DISCLAIMS ANY EXPRESS
 * OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.  USER WILL NOT
 * KNOWINGLY USE, DISTRIBUTE OR RESELL THE JASPER SOFTWARE OR UNDERLYING
 * TECHNOLOGY OR PRODUCTS FOR HIGH RISK ACTIVITIES AND WILL ENSURE THAT ITS
 * CUSTOMERS AND END-USERS OF ITS PRODUCTS ARE PROVIDED WITH A COPY OF THE
 * NOTICE SPECIFIED IN THIS SECTION.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/*
 * Bit Stream Class
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#include "jasper/jas_malloc.h"
#include "jasper/jas_math.h"
#include "jasper/jas_debug.h"

#include "jpc_bs.h"

/******************************************************************************\
* Local function prototypes.
\******************************************************************************/

static jpc_bitstream_t *jpc_bitstream_alloc(void);

/******************************************************************************\
* Code for opening and closing bit streams.
\******************************************************************************/

/* Open a bit stream from a stream. */
jpc_bitstream_t *jpc_bitstream_sopen(jas_stream_t *stream, char *mode)
{
	jpc_bitstream_t *bitstream;

	/* Ensure that the open mode is valid. */
	assert(!strcmp(mode, "r") || !strcmp(mode, "w") || !strcmp(mode, "r+")
	  || !strcmp(mode, "w+"));

	if (!(bitstream = jpc_bitstream_alloc())) {
		return 0;
	}

	/* By default, do not close the underlying (character) stream, upon
	  the close of the bit stream. */
	bitstream->flags_ = JPC_BITSTREAM_NOCLOSE;

	bitstream->stream_ = stream;
	bitstream->openmode_ = (mode[0] == 'w') ? JPC_BITSTREAM_WRITE :
	  JPC_BITSTREAM_READ;

	/* Mark the data buffer as empty. */
	bitstream->cnt_ = (bitstream->openmode_ == JPC_BITSTREAM_READ) ? 0 : 8;
	bitstream->buf_ = 0;

	return bitstream;
}

/* Close a bit stream. */
int jpc_bitstream_close(jpc_bitstream_t *bitstream)
{
	int ret = 0;

	/* Align to the next byte boundary while considering the effects of
	  bit stuffing. */
	if (jpc_bitstream_align(bitstream)) {
		ret = -1;
	}

	/* If necessary, close the underlying (character) stream. */
	if (!(bitstream->flags_ & JPC_BITSTREAM_NOCLOSE) && bitstream->stream_) {
		if (jas_stream_close(bitstream->stream_)) {
			ret = -1;
		}
		bitstream->stream_ = 0;
	}

	jas_free(bitstream);
	return ret;
}

/* Allocate a new bit stream. */
static jpc_bitstream_t *jpc_bitstream_alloc()
{
	jpc_bitstream_t *bitstream;

	/* Allocate memory for the new bit stream object. */
	if (!(bitstream = jas_malloc(sizeof(jpc_bitstream_t)))) {
		return 0;
	}
	/* Initialize all of the data members. */
	bitstream->stream_ = 0;
	bitstream->cnt_ = 0;
	bitstream->flags_ = 0;
	bitstream->openmode_ = 0;

	return bitstream;
}

/******************************************************************************\
* Code for reading/writing from/to bit streams.
\******************************************************************************/

/* Get a bit from a bit stream. */
int jpc_bitstream_getbit_func(jpc_bitstream_t *bitstream)
{
	int ret;
	JAS_DBGLOG(1000, ("jpc_bitstream_getbit_func(%p)\n", bitstream));
	ret = jpc_bitstream_getbit_macro(bitstream);
	JAS_DBGLOG(1000, ("jpc_bitstream_getbit_func -> %d\n", ret));
	return ret;
}

/* Put a bit to a bit stream. */
int jpc_bitstream_putbit_func(jpc_bitstream_t *bitstream, int b)
{
	int ret;
	JAS_DBGLOG(1000, ("jpc_bitstream_putbit_func(%p, %d)\n", bitstream, b));
	ret = jpc_bitstream_putbit_macro(bitstream, b);
	JAS_DBGLOG(1000, ("jpc_bitstream_putbit_func() -> %d\n", ret));
	return ret;
}

/* Get one or more bits from a bit stream. */
long jpc_bitstream_getbits(jpc_bitstream_t *bitstream, int n)
{
	long v;
	int u;

	/* We can reliably get at most 31 bits since ISO/IEC 9899 only
	  guarantees that a long can represent values up to 2^31-1. */
	assert(n >= 0 && n < 32);

	/* Get the number of bits requested from the specified bit stream. */
	v = 0;
	while (--n >= 0) {
		if ((u = jpc_bitstream_getbit(bitstream)) < 0) {
			return -1;
		}
		v = (v << 1) | u;
	}
	return v;
}

/* Put one or more bits to a bit stream. */
int jpc_bitstream_putbits(jpc_bitstream_t *bitstream, int n, long v)
{
	int m;

	/* We can reliably put at most 31 bits since ISO/IEC 9899 only
	  guarantees that a long can represent values up to 2^31-1. */
	assert(n >= 0 && n < 32);
	/* Ensure that only the bits to be output are nonzero. */
	assert(!(v & (~JAS_ONES(n))));

	/* Put the desired number of bits to the specified bit stream. */
	m = n - 1;
	while (--n >= 0) {
		if (jpc_bitstream_putbit(bitstream, (v >> m) & 1) == EOF) {
			return EOF;
		}
		v <<= 1;
	}
	return 0;
}

/******************************************************************************\
* Code for buffer filling and flushing.
\******************************************************************************/

/* Fill the buffer for a bit stream. */
int jpc_bitstream_fillbuf(jpc_bitstream_t *bitstream)
{
	int c;
	/* Note: The count has already been decremented by the caller. */
	assert(bitstream->openmode_ & JPC_BITSTREAM_READ);
	assert(bitstream->cnt_ <= 0);

	if (bitstream->flags_ & JPC_BITSTREAM_ERR) {
		bitstream->cnt_ = 0;
		return -1;
	}

	if (bitstream->flags_ & JPC_BITSTREAM_EOF) {
		bitstream->buf_ = 0x7f;
		bitstream->cnt_ = 7;
		return 1;
	}

	bitstream->buf_ = (bitstream->buf_ << 8) & 0xffff;
	if ((c = jas_stream_getc((bitstream)->stream_)) == EOF) {
		bitstream->flags_ |= JPC_BITSTREAM_EOF;
		return 1;
	}
	bitstream->cnt_ = (bitstream->buf_ == 0xff00) ? 6 : 7;
	bitstream->buf_ |= c & ((1 << (bitstream->cnt_ + 1)) - 1);
	return (bitstream->buf_ >> bitstream->cnt_) & 1;
}


/******************************************************************************\
* Code related to flushing.
\******************************************************************************/

/* Does the bit stream need to be aligned to a byte boundary (considering
  the effects of bit stuffing)? */
int jpc_bitstream_needalign(jpc_bitstream_t *bitstream)
{
	if (bitstream->openmode_ & JPC_BITSTREAM_READ) {
		/* The bit stream is open for reading. */
		/* If there are any bits buffered for reading, or the
		  previous byte forced a stuffed bit, alignment is
		  required. */
		if ((bitstream->cnt_ < 8 && bitstream->cnt_ > 0) ||
		  ((bitstream->buf_ >> 8) & 0xff) == 0xff) {
			return 1;
		}
	} else if (bitstream->openmode_ & JPC_BITSTREAM_WRITE) {
		/* The bit stream is open for writing. */
		/* If there are any bits buffered for writing, or the
		  previous byte forced a stuffed bit, alignment is
		  required. */
		if ((bitstream->cnt_ < 8 && bitstream->cnt_ >= 0) ||
		  ((bitstream->buf_ >> 8) & 0xff) == 0xff) {
			return 1;
		}
	} else {
		/* This should not happen.  Famous last words, eh? :-) */
		assert(0);
		return -1;
	}
	return 0;
}

/* How many additional bytes would be output if we align the bit stream? */
int jpc_bitstream_pending(jpc_bitstream_t *bitstream)
{
	if (bitstream->openmode_ & JPC_BITSTREAM_WRITE) {
		/* The bit stream is being used for writing. */
#if 1
		/* XXX - Is this really correct?  Check someday... */
		if (bitstream->cnt_ < 8) {
			return 1;
		}
#else
		if (bitstream->cnt_ < 8) {
			if (((bitstream->buf_ >> 8) & 0xff) == 0xff) {
				return 2;
			}
			return 1;
		}
#endif
		return 0;
	} else {
		/* This operation should not be invoked on a bit stream that
		  is being used for reading. */
		return -1;
	}
}

/* Align the bit stream to a byte boundary. */
int jpc_bitstream_align(jpc_bitstream_t *bitstream)
{
	int ret;
	if (bitstream->openmode_ & JPC_BITSTREAM_READ) {
		ret = jpc_bitstream_inalign(bitstream, 0, 0);
	} else if (bitstream->openmode_ & JPC_BITSTREAM_WRITE) {
		ret = jpc_bitstream_outalign(bitstream, 0);
	} else {
		abort();
	}
	return ret;
}

/* Align a bit stream in the input case. */
int jpc_bitstream_inalign(jpc_bitstream_t *bitstream, int fillmask,
  int filldata)
{
	int n;
	int v;
	int u;
	int numfill;
	int m;

	numfill = 7;
	m = 0;
	v = 0;
	if (bitstream->cnt_ > 0) {
		n = bitstream->cnt_;
	} else if (!bitstream->cnt_) {
		n = ((bitstream->buf_ & 0xff) == 0xff) ? 7 : 0;
	} else {
		n = 0;
	}
	if (n > 0) {
		if ((u = jpc_bitstream_getbits(bitstream, n)) < 0) {
			return -1;
		}
		m += n;
		v = (v << n) | u;
	}
	if ((bitstream->buf_ & 0xff) == 0xff) {
		if ((u = jpc_bitstream_getbits(bitstream, 7)) < 0) {
			return -1;
		}
		v = (v << 7) | u;
		m += 7;
	}
	if (m > numfill) {
		v >>= m - numfill;
	} else {
		filldata >>= numfill - m;
		fillmask >>= numfill - m;
	}
	if (((~(v ^ filldata)) & fillmask) != fillmask) {
		/* The actual fill pattern does not match the expected one. */
		return 1;
	}

	return 0;
}

/* Align a bit stream in the output case. */
int jpc_bitstream_outalign(jpc_bitstream_t *bitstream, int filldata)
{
	int n;
	int v;

	/* Ensure that this bit stream is open for writing. */
	assert(bitstream->openmode_ & JPC_BITSTREAM_WRITE);

	/* Ensure that the first bit of fill data is zero. */
	/* Note: The first bit of fill data must be zero.  If this were not
	  the case, the fill data itself could cause further bit stuffing to
	  be required (which would cause numerous complications). */
	assert(!(filldata & (~0x3f)));

	if (!bitstream->cnt_) {
		if ((bitstream->buf_ & 0xff) == 0xff) {
			n = 7;
			v = filldata;
		} else {
			n = 0;
			v = 0;
		}
	} else if (bitstream->cnt_ > 0 && bitstream->cnt_ < 8) {
		n = bitstream->cnt_;
		v = filldata >> (7 - n);
	} else {
		n = 0;
		v = 0;
		return 0;
	}

	/* Write the appropriate fill data to the bit stream. */
	if (n > 0) {
		if (jpc_bitstream_putbits(bitstream, n, v)) {
			return -1;
		}
	}
	if (bitstream->cnt_ < 8) {
		assert(bitstream->cnt_ >= 0 && bitstream->cnt_ < 8);
		assert((bitstream->buf_ & 0xff) != 0xff);
		/* Force the pending byte of output to be written to the
		  underlying (character) stream. */
		if (jas_stream_putc(bitstream->stream_, bitstream->buf_ & 0xff) == EOF) {
			return -1;
		}
		bitstream->cnt_ = 8;
		bitstream->buf_ = (bitstream->buf_ << 8) & 0xffff;
	}

	return 0;
}
