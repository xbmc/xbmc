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
 * JP2 Library
 *
 * $Id$
 */

#ifndef JP2_COD_H
#define JP2_COD_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_types.h"

/******************************************************************************\
* Macros.
\******************************************************************************/

#define	JP2_SPTOBPC(s, p) \
	((((p) - 1) & 0x7f) | (((s) & 1) << 7))

/******************************************************************************\
* Box class.
\******************************************************************************/

#define	JP2_BOX_HDRLEN	8

/* Box types. */
#define	JP2_BOX_JP		0x6a502020	/* Signature */
#define JP2_BOX_FTYP	0x66747970	/* File Type */
#define	JP2_BOX_JP2H	0x6a703268	/* JP2 Header */
#define	JP2_BOX_IHDR	0x69686472	/* Image Header */
#define	JP2_BOX_BPCC	0x62706363	/* Bits Per Component */
#define	JP2_BOX_COLR	0x636f6c72	/* Color Specification */
#define	JP2_BOX_PCLR	0x70636c72	/* Palette */
#define	JP2_BOX_CMAP	0x636d6170	/* Component Mapping */
#define	JP2_BOX_CDEF	0x63646566	/* Channel Definition */
#define	JP2_BOX_RES		0x72657320	/* Resolution */
#define	JP2_BOX_RESC	0x72657363	/* Capture Resolution */
#define	JP2_BOX_RESD	0x72657364	/* Default Display Resolution */
#define	JP2_BOX_JP2C	0x6a703263	/* Contiguous Code Stream */
#define	JP2_BOX_JP2I	0x6a703269	/* Intellectual Property */
#define	JP2_BOX_XML		0x786d6c20	/* XML */
#define	JP2_BOX_UUID	0x75756964	/* UUID */
#define	JP2_BOX_UINF	0x75696e66	/* UUID Info */
#define	JP2_BOX_ULST	0x75637374	/* UUID List */
#define	JP2_BOX_URL		0x75726c20	/* URL */

#define	JP2_BOX_SUPER	0x01
#define	JP2_BOX_NODATA	0x02

/* JP box data. */

#define	JP2_JP_MAGIC	0x0d0a870a
#define	JP2_JP_LEN		12

typedef struct {
	uint_fast32_t magic;
} jp2_jp_t;

/* FTYP box data. */

#define	JP2_FTYP_MAXCOMPATCODES	32
#define	JP2_FTYP_MAJVER		0x6a703220
#define	JP2_FTYP_MINVER		0
#define	JP2_FTYP_COMPATCODE		JP2_FTYP_MAJVER

typedef struct {
	uint_fast32_t majver;
	uint_fast32_t minver;
	uint_fast32_t numcompatcodes;
	uint_fast32_t compatcodes[JP2_FTYP_MAXCOMPATCODES];
} jp2_ftyp_t;

/* IHDR box data. */

#define	JP2_IHDR_COMPTYPE	7
#define	JP2_IHDR_BPCNULL	255

typedef struct {
	uint_fast32_t width;
	uint_fast32_t height;
	uint_fast16_t numcmpts;
	uint_fast8_t bpc;
	uint_fast8_t comptype;
	uint_fast8_t csunk;
	uint_fast8_t ipr;
} jp2_ihdr_t;

/* BPCC box data. */

typedef struct {
	uint_fast16_t numcmpts;
	uint_fast8_t *bpcs;
} jp2_bpcc_t;

/* COLR box data. */

#define	JP2_COLR_ENUM	1
#define	JP2_COLR_ICC	2
#define	JP2_COLR_PRI	0

#define	JP2_COLR_SRGB	16
#define	JP2_COLR_SGRAY	17
#define	JP2_COLR_SYCC	18

typedef struct {
	uint_fast8_t method;
	uint_fast8_t pri;
	uint_fast8_t approx;
	uint_fast32_t csid;
	uint_fast8_t *iccp;
	int iccplen;
	/* XXX - Someday we ought to add ICC profile data here. */
} jp2_colr_t;

/* PCLR box data. */

typedef struct {
	uint_fast16_t numlutents;
	uint_fast8_t numchans;
	int_fast32_t *lutdata;
	uint_fast8_t *bpc;
} jp2_pclr_t;

/* CDEF box per-channel data. */

#define JP2_CDEF_RGB_R	1
#define JP2_CDEF_RGB_G	2
#define JP2_CDEF_RGB_B	3

#define JP2_CDEF_YCBCR_Y	1
#define JP2_CDEF_YCBCR_CB	2
#define JP2_CDEF_YCBCR_CR	3

#define	JP2_CDEF_GRAY_Y	1

#define	JP2_CDEF_TYPE_COLOR	0
#define	JP2_CDEF_TYPE_OPACITY	1
#define	JP2_CDEF_TYPE_UNSPEC	65535
#define	JP2_CDEF_ASOC_ALL	0
#define	JP2_CDEF_ASOC_NONE	65535

typedef struct {
	uint_fast16_t channo;
	uint_fast16_t type;
	uint_fast16_t assoc;
} jp2_cdefchan_t;

/* CDEF box data. */

typedef struct {
	uint_fast16_t numchans;
	jp2_cdefchan_t *ents;
} jp2_cdef_t;

typedef struct {
	uint_fast16_t cmptno;
	uint_fast8_t map;
	uint_fast8_t pcol;
} jp2_cmapent_t;

typedef struct {
	uint_fast16_t numchans;
	jp2_cmapent_t *ents;
} jp2_cmap_t;

#define	JP2_CMAP_DIRECT		0
#define	JP2_CMAP_PALETTE	1

/* Generic box. */

struct jp2_boxops_s;
typedef struct {

	struct jp2_boxops_s *ops;
	struct jp2_boxinfo_s *info;

	uint_fast32_t type;
	uint_fast32_t len;

	union {
		jp2_jp_t jp;
		jp2_ftyp_t ftyp;
		jp2_ihdr_t ihdr;
		jp2_bpcc_t bpcc;
		jp2_colr_t colr;
		jp2_pclr_t pclr;
		jp2_cdef_t cdef;
		jp2_cmap_t cmap;
	} data;

} jp2_box_t;

typedef struct jp2_boxops_s {
	void (*init)(jp2_box_t *box);
	void (*destroy)(jp2_box_t *box);
	int (*getdata)(jp2_box_t *box, jas_stream_t *in);
	int (*putdata)(jp2_box_t *box, jas_stream_t *out);
	void (*dumpdata)(jp2_box_t *box, FILE *out);
} jp2_boxops_t;

/******************************************************************************\
*
\******************************************************************************/

typedef struct jp2_boxinfo_s {
	int type;
	char *name;
	int flags;
	jp2_boxops_t ops;
} jp2_boxinfo_t;

/******************************************************************************\
* Box class.
\******************************************************************************/

jp2_box_t *jp2_box_create(int type);
void jp2_box_destroy(jp2_box_t *box);
jp2_box_t *jp2_box_get(jas_stream_t *in);
int jp2_box_put(jp2_box_t *box, jas_stream_t *out);

#define JP2_DTYPETOBPC(dtype) \
  ((JAS_IMAGE_CDT_GETSGND(dtype) << 7) | (JAS_IMAGE_CDT_GETPREC(dtype) - 1))
#define	JP2_BPCTODTYPE(bpc) \
  (JAS_IMAGE_CDT_SETSGND(bpc >> 7) | JAS_IMAGE_CDT_SETPREC((bpc & 0x7f) + 1))

#define ICC_CS_RGB	0x52474220
#define ICC_CS_YCBCR	0x59436272
#define ICC_CS_GRAY	0x47524159

jp2_cdefchan_t *jp2_cdef_lookup(jp2_cdef_t *cdef, int channo);


#endif
