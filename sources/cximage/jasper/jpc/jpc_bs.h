/*
 * Copyright (c) 1999-2000 Image Power, Inc. and the University of
 *   British Columbia.
 * Copyright (c) 2001-2002 Michael David Adams.
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

#ifndef JPC_BS_H
#define JPC_BS_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include <stdio.h>

#include "jasper/jas_types.h"
#include "jasper/jas_stream.h"

/******************************************************************************\
* Constants.
\******************************************************************************/

/*
 * Bit stream open mode flags.
 */

/* Bit stream open for reading. */
#define	JPC_BITSTREAM_READ	0x01
/* Bit stream open for writing. */
#define	JPC_BITSTREAM_WRITE	0x02

/*
 * Bit stream flags.
 */

/* Do not close underlying character stream. */
#define	JPC_BITSTREAM_NOCLOSE	0x01
/* End of file has been reached while reading. */
#define	JPC_BITSTREAM_EOF	0x02
/* An I/O error has occured. */
#define	JPC_BITSTREAM_ERR	0x04

/******************************************************************************\
* Types.
\******************************************************************************/

/* Bit stream class. */

typedef struct {

	/* Some miscellaneous flags. */
	int flags_;

	/* The input/output buffer. */
	uint_fast16_t buf_;

	/* The number of bits remaining in the byte being read/written. */
	int cnt_;

	/* The underlying stream associated with this bit stream. */
	jas_stream_t *stream_;

	/* The mode in which this bit stream was opened. */
	int openmode_;

} jpc_bitstream_t;

/******************************************************************************\
* Functions/macros for opening and closing bit streams..
\******************************************************************************/

/* Open a stream as a bit stream. */
jpc_bitstream_t *jpc_bitstream_sopen(jas_stream_t *stream, char *mode);

/* Close a bit stream. */
int jpc_bitstream_close(jpc_bitstream_t *bitstream);

/******************************************************************************\
* Functions/macros for reading from and writing to bit streams..
\******************************************************************************/

/* Read a bit from a bit stream. */
#if defined(DEBUG)
#define	jpc_bitstream_getbit(bitstream) \
	jpc_bitstream_getbit_func(bitstream)
#else
#define jpc_bitstream_getbit(bitstream) \
	jpc_bitstream_getbit_macro(bitstream)
#endif

/* Write a bit to a bit stream. */
#if defined(DEBUG)
#define	jpc_bitstream_putbit(bitstream, v) \
	jpc_bitstream_putbit_func(bitstream, v)
#else
#define	jpc_bitstream_putbit(bitstream, v) \
	jpc_bitstream_putbit_macro(bitstream, v)
#endif

/* Read one or more bits from a bit stream. */
long jpc_bitstream_getbits(jpc_bitstream_t *bitstream, int n);

/* Write one or more bits to a bit stream. */
int jpc_bitstream_putbits(jpc_bitstream_t *bitstream, int n, long v);

/******************************************************************************\
* Functions/macros for flushing and aligning bit streams.
\******************************************************************************/

/* Align the current position within the bit stream to the next byte
  boundary. */
int jpc_bitstream_align(jpc_bitstream_t *bitstream);

/* Align the current position in the bit stream with the next byte boundary,
  ensuring that certain bits consumed in the process match a particular
  pattern. */
int jpc_bitstream_inalign(jpc_bitstream_t *bitstream, int fillmask,
  int filldata);

/* Align the current position in the bit stream with the next byte boundary,
  writing bits from the specified pattern (if necessary) in the process. */
int jpc_bitstream_outalign(jpc_bitstream_t *bitstream, int filldata);

/* Check if a bit stream needs alignment. */
int jpc_bitstream_needalign(jpc_bitstream_t *bitstream);

/* How many additional bytes would be output if the bit stream was aligned? */
int jpc_bitstream_pending(jpc_bitstream_t *bitstream);

/******************************************************************************\
* Functions/macros for querying state information for bit streams.
\******************************************************************************/

/* Has EOF been encountered on a bit stream? */
#define jpc_bitstream_eof(bitstream) \
	((bitstream)->flags_ & JPC_BITSTREAM_EOF)

/******************************************************************************\
* Internals.
\******************************************************************************/

/* DO NOT DIRECTLY INVOKE ANY OF THE MACROS OR FUNCTIONS BELOW.  THEY ARE
  FOR INTERNAL USE ONLY. */

int jpc_bitstream_getbit_func(jpc_bitstream_t *bitstream);

int jpc_bitstream_putbit_func(jpc_bitstream_t *bitstream, int v);

int jpc_bitstream_fillbuf(jpc_bitstream_t *bitstream);

#define	jpc_bitstream_getbit_macro(bitstream) \
	(assert((bitstream)->openmode_ & JPC_BITSTREAM_READ), \
	  (--(bitstream)->cnt_ >= 0) ? \
	  ((int)(((bitstream)->buf_ >> (bitstream)->cnt_) & 1)) : \
	  jpc_bitstream_fillbuf(bitstream))

#define jpc_bitstream_putbit_macro(bitstream, bit) \
	(assert((bitstream)->openmode_ & JPC_BITSTREAM_WRITE), \
	  (--(bitstream)->cnt_ < 0) ? \
	  ((bitstream)->buf_ = ((bitstream)->buf_ << 8) & 0xffff, \
	  (bitstream)->cnt_ = ((bitstream)->buf_ == 0xff00) ? 6 : 7, \
	  (bitstream)->buf_ |= ((bit) & 1) << (bitstream)->cnt_, \
	  (jas_stream_putc((bitstream)->stream_, (bitstream)->buf_ >> 8) == EOF) \
	  ? (EOF) : ((bit) & 1)) : \
	  ((bitstream)->buf_ |= ((bit) & 1) << (bitstream)->cnt_, \
	  (bit) & 1))

#endif
