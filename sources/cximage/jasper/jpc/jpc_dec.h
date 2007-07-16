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
 * JPEG-2000 Decoder
 *
 * $Id$
 */

#ifndef JPC_DEC_H
#define JPC_DEC_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_stream.h"

#include "jpc_tsfb.h"
#include "jpc_bs.h"
#include "jpc_tagtree.h"
#include "jpc_cs.h"
#include "jpc_cod.h"
#include "jpc_mqdec.h"
#include "jpc_t2cod.h"

/******************************************************************************\
* Below are some ugly warts necessary to support packed packet headers.
\******************************************************************************/

/* PPM/PPT marker segment table entry. */

typedef struct {

	/* The index for this entry. */
	uint_fast16_t ind;

	/* The data length. */
	uint_fast32_t len;

	/* The data. */
	uchar *data;

} jpc_ppxstabent_t;

/* PPM/PPT marker segment table. */

typedef struct {

	/* The number of entries. */
	int numents;

	/* The maximum number of entries (i.e., the allocated size of the array
	  below). */
	int maxents;

	/* The table entries. */
	jpc_ppxstabent_t **ents;

} jpc_ppxstab_t;

/* Stream list class. */

typedef struct {

	/* The number of streams in this list. */
	int numstreams;

	/* The maximum number of streams that can be accomodated without
	  growing the streams array. */
	int maxstreams;

	/* The streams. */
	jas_stream_t **streams;

} jpc_streamlist_t;

/******************************************************************************\
* Coding parameters class.
\******************************************************************************/

/* Per-component coding parameters. */

typedef struct {

	/* How were various coding parameters set? */
	int flags;

	/* Per-component coding style parameters (e.g., explicit precinct sizes) */
	uint_fast8_t csty;

	/* The number of resolution levels. */
	uint_fast8_t numrlvls;

	/* The code block width exponent. */
	uint_fast8_t cblkwidthexpn;

	/* The code block height exponent. */
	uint_fast8_t cblkheightexpn;

	/* The QMFB ID. */
	uint_fast8_t qmfbid;

	/* The quantization style. */
	uint_fast8_t qsty;

	/* The number of quantizer step sizes. */
	uint_fast16_t numstepsizes;

	/* The step sizes. */
	uint_fast16_t stepsizes[3 * JPC_MAXRLVLS + 1];

	/* The number of guard bits. */
	uint_fast8_t numguardbits;

	/* The ROI shift value. */
	uint_fast8_t roishift;

	/* The code block parameters. */
	uint_fast8_t cblkctx;

	/* The precinct width exponents. */
	uint_fast8_t prcwidthexpns[JPC_MAXRLVLS];

	/* The precinct height exponents. */
	uint_fast8_t prcheightexpns[JPC_MAXRLVLS];

} jpc_dec_ccp_t;

/* Coding paramters. */

typedef struct {

	/* How were these coding parameters set? */
	int flags;

	/* Progression change list. */
	jpc_pchglist_t *pchglist;

	/* Progression order. */
	uint_fast8_t prgord;

	/* The number of layers. */
	uint_fast16_t numlyrs;

	/* The MCT ID. */
	uint_fast8_t mctid;

	/* The coding style parameters (e.g., SOP, EPH). */
	uint_fast8_t csty;

	/* The number of components. */
	int numcomps;

	/* The per-component coding parameters. */
	jpc_dec_ccp_t *ccps;

} jpc_dec_cp_t;

/******************************************************************************\
* Decoder class.
\******************************************************************************/

/* Decoder per-segment state information. */

typedef struct jpc_dec_seg_s {

	/* The next segment in the list. */
	struct jpc_dec_seg_s *next;

	/* The previous segment in the list. */
	struct jpc_dec_seg_s *prev;

	/* The starting pass number for this segment. */
	int passno;

	/* The number of passes in this segment. */
	int numpasses;

	/* The maximum number of passes in this segment. */
	int maxpasses;

	/* The type of data in this segment (i.e., MQ or raw). */
	int type;

	/* A stream containing the data for this segment. */
	jas_stream_t *stream;

	/* The number of bytes destined for this segment from the packet
	  currently being decoded. */
	int cnt;

	/* A flag indicating if this segment has been terminated. */
	int complete;

	/* The layer number to which this segment belongs. */
	/* If the segment spans multiple layers, then the largest layer number
	  spanned by the segment is used. */
	int lyrno;

} jpc_dec_seg_t;

/* Decoder segment list. */

typedef struct {

	/* The first entry in the list. */
	jpc_dec_seg_t *head;

	/* The last entry in the list. */
	jpc_dec_seg_t *tail;

} jpc_dec_seglist_t;

/* Decoder per-code-block state information. */

typedef struct {

	/* The number of passes. */
	int numpasses;

	/* A list of segments that still need to be decoded. */
	jpc_dec_seglist_t segs;

	/* The first incomplete/partial segment. */
	jpc_dec_seg_t *curseg;

	/* The number of leading insignificant bit planes for this code block. */
	int numimsbs;

	/* The number of bits used to encode pass data lengths. */
	int numlenbits;

	/* The first pass number containing data for this code block. */
	int firstpassno;

	/* The MQ decoder. */
	jpc_mqdec_t *mqdec;

	/* The raw bit stream decoder. */
	jpc_bitstream_t *nulldec;

	/* The per-sample state information for this code block. */
	jas_matrix_t *flags;

	/* The sample data associated with this code block. */
	jas_matrix_t *data;

} jpc_dec_cblk_t;

/* Decoder per-code-block-group state information. */

typedef struct {

	/* The x-coordinate of the top-left corner of the precinct. */
	uint_fast32_t xstart;

	/* The y-coordinate of the top-left corner of the precinct. */
	uint_fast32_t ystart;

	/* The x-coordinate of the bottom-right corner of the precinct
	  (plus one). */
	uint_fast32_t xend;

	/* The y-coordinate of the bottom-right corner of the precinct
	  (plus one). */
	uint_fast32_t yend;

	/* The number of code blocks spanning this precinct in the horizontal
	  direction. */
	int numhcblks;

	/* The number of code blocks spanning this precinct in the vertical
	  direction. */
	int numvcblks;

	/* The total number of code blocks in this precinct. */
	int numcblks;

	/* The per code block information. */
	jpc_dec_cblk_t *cblks;

	/* The inclusion tag tree. */
	jpc_tagtree_t *incltagtree;

	/* The insignificant MSBs tag tree. */
	jpc_tagtree_t *numimsbstagtree;

} jpc_dec_prc_t;

/* Decoder per-band state information. */

typedef struct {

	/* The per-code-block-group state information. */
	jpc_dec_prc_t *prcs;

	/* The sample data associated with this band. */
	jas_matrix_t *data;

	/* The orientation of this band (i.e., LL, LH, HL, or HH). */
	int orient;

	/* The encoded quantizer step size. */
	int stepsize;

	/* The absolute quantizer step size. */
	jpc_fix_t absstepsize;

	/* The number of bit planes for this band. */
	int numbps;

	/* The analysis gain associated with this band. */
	int analgain;

	/* The ROI shift value for this band. */
	int roishift;

} jpc_dec_band_t;

/* Decoder per-resolution-level state information. */

typedef struct {

	/* The number of bands associated with this resolution level. */
	int numbands;

	/* The per-band information. */
	jpc_dec_band_t *bands;

	/* The x-coordinate of the top-left corner of the tile-component
	  at this resolution. */
	uint_fast32_t xstart;

	/* The y-coordinate of the top-left corner of the tile-component
	  at this resolution. */
	uint_fast32_t ystart;

	/* The x-coordinate of the bottom-right corner of the tile-component
	  at this resolution (plus one). */
	uint_fast32_t xend;

	/* The y-coordinate of the bottom-right corner of the tile-component
	  at this resolution (plus one). */
	uint_fast32_t yend;

	/* The exponent value for the nominal precinct width measured
	  relative to the associated LL band. */
	int prcwidthexpn;

	/* The exponent value for the nominal precinct height measured
	  relative to the associated LL band. */
	int prcheightexpn;

	/* The number of precincts in the horizontal direction. */
	int numhprcs;

	/* The number of precincts in the vertical direction. */
	int numvprcs;

	/* The total number of precincts. */
	int numprcs;

	/* The exponent value for the nominal code block group width.
	  This quantity is associated with the next lower resolution level
	  (assuming that there is one). */
	int cbgwidthexpn;

	/* The exponent value for the nominal code block group height
	  This quantity is associated with the next lower resolution level
	  (assuming that there is one). */
	int cbgheightexpn;

	/* The exponent value for the code block width. */
	uint_fast16_t cblkwidthexpn;

	/* The exponent value for the code block height. */
	uint_fast16_t cblkheightexpn;

} jpc_dec_rlvl_t;

/* Decoder per-tile-component state information. */

typedef struct {

	/* The x-coordinate of the top-left corner of the tile-component
	  in the coordinate system of the tile-component. */
	uint_fast32_t xstart;

	/* The y-coordinate of the top-left corner of the tile-component
	  in the coordinate system of the tile-component. */
	uint_fast32_t ystart;

	/* The x-coordinate of the bottom-right corner of the tile-component
	  in the coordinate system of the tile-component (plus one). */
	uint_fast32_t xend;

	/* The y-coordinate of the bottom-right corner of the tile-component
	  in the coordinate system of the tile-component (plus one). */
	uint_fast32_t yend;

	/* The component data for the current tile. */
	jas_matrix_t *data;

	/* The number of resolution levels. */
	int numrlvls;

	/* The per resolution level information. */
	jpc_dec_rlvl_t *rlvls;

	/* The TSFB. */
	jpc_tsfb_t *tsfb;

} jpc_dec_tcomp_t;

/*
 * Tile states.
 */

#define	JPC_TILE_INIT	0
#define	JPC_TILE_ACTIVE	1
#define	JPC_TILE_ACTIVELAST	2
#define	JPC_TILE_DONE	3

/* Decoder per-tile state information. */

typedef struct {

	/* The processing state for this tile. */
	int state;

	/* The x-coordinate of the top-left corner of the tile on the reference
	  grid. */
	uint_fast32_t xstart;

	/* The y-coordinate of the top-left corner of the tile on the reference
	  grid. */
	uint_fast32_t ystart;

	/* The x-coordinate of the bottom-right corner of the tile on the
	  reference grid (plus one). */
	uint_fast32_t xend;

	/* The y-coordinate of the bottom-right corner of the tile on the
	  reference grid (plus one). */
	uint_fast32_t yend;

	/* The packed packet header data for this tile. */
	jpc_ppxstab_t *pptstab;

	/* A stream containing the packed packet header data for this tile. */
	jas_stream_t *pkthdrstream;

	/* The current position within the packed packet header stream. */
	long pkthdrstreampos;

	/* The coding parameters for this tile. */
	jpc_dec_cp_t *cp;

	/* The per tile-component information. */
	jpc_dec_tcomp_t *tcomps;

	/* The next expected tile-part number. */
	int partno;

	/* The number of tile-parts. */
	int numparts;

	/* The coding mode. */
	int realmode;

	/* The packet iterator for this tile. */
	jpc_pi_t *pi;

} jpc_dec_tile_t;

/* Decoder per-component state information. */

typedef struct {

	/* The horizontal sampling period. */
	uint_fast32_t hstep;

	/* The vertical sampling period. */
	uint_fast32_t vstep;

	/* The number of samples in the horizontal direction. */
	uint_fast32_t width;

	/* The number of samples in the vertical direction. */
	uint_fast32_t height;

	/* The precision of the sample data. */
	uint_fast16_t prec;

	/* The signedness of the sample data. */
	bool sgnd;

	/* The sample alignment horizontal offset. */
	uint_fast32_t hsubstep;
	
	/* The sample alignment vertical offset. */
	uint_fast32_t vsubstep;

} jpc_dec_cmpt_t;

/* Decoder state information. */

typedef struct {

	/* The decoded image. */
	jas_image_t *image;

	/* The x-coordinate of the top-left corner of the image area on
	  the reference grid. */
	uint_fast32_t xstart;

	/* The y-coordinate of the top-left corner of the image area on
	  the reference grid. */
	uint_fast32_t ystart;

	/* The x-coordinate of the bottom-right corner of the image area on
	  the reference grid (plus one). */
	uint_fast32_t xend;

	/* The y-coordinate of the bottom-right corner of the image area on
	  the reference grid (plus one). */
	uint_fast32_t yend;

	/* The nominal tile width in units of the image reference grid. */
	uint_fast32_t tilewidth;

	/* The nominal tile height in units of the image reference grid. */
	uint_fast32_t tileheight;

	/* The horizontal offset from the origin of the reference grid to the
	  left side of the first tile. */
	uint_fast32_t tilexoff;

	/* The vertical offset from the origin of the reference grid to the
	  top side of the first tile. */
	uint_fast32_t tileyoff;

	/* The number of tiles spanning the image area in the vertical
	  direction. */
	int numhtiles;

	/* The number of tiles spanning the image area in the horizontal
	  direction. */
	int numvtiles;

	/* The total number of tiles. */
	int numtiles;

	/* The per-tile information. */
	jpc_dec_tile_t *tiles;

	/* The tile currently being processed. */
	jpc_dec_tile_t *curtile;

	/* The number of components. */
	int numcomps;

	/* The stream containing the input JPEG-2000 code stream data. */
	jas_stream_t *in;

	/* The default coding parameters for all tiles. */
	jpc_dec_cp_t *cp;

	/* The maximum number of layers that may be decoded. */
	int maxlyrs;

	/* The maximum number of packets that may be decoded. */
	int maxpkts;

	/* The number of packets decoded so far in the processing of the entire
	  code stream. */
	int numpkts;

	/* The next expected PPM marker segment sequence number. */
	int ppmseqno;

	/* The current state for code stream processing. */
	int state;

	/* The per-component information. */
	jpc_dec_cmpt_t *cmpts;

	/* The information from PPM marker segments. */
	jpc_ppxstab_t *ppmstab;

	/* A list of streams containing packet header data from PPM marker
	  segments. */
	jpc_streamlist_t *pkthdrstreams;

	/* The expected ending offset for a tile-part. */
	long curtileendoff;

	/* This is required by the tier-2 decoder. */
	jpc_cstate_t *cstate;

} jpc_dec_t;

/* Decoder options. */

typedef struct {

	/* The debug level for the decoder. */
	int debug;

	/* The maximum number of layers to decode. */
	int maxlyrs;

	/* The maximum number of packets to decode. */
	int maxpkts;

} jpc_dec_importopts_t;

/******************************************************************************\
* Functions.
\******************************************************************************/

/* Create a decoder segment object. */
jpc_dec_seg_t *jpc_seg_alloc(void);

/* Destroy a decoder segment object. */
void jpc_seg_destroy(jpc_dec_seg_t *seg);

/* Remove a segment from a segment list. */
void jpc_seglist_remove(jpc_dec_seglist_t *list, jpc_dec_seg_t *node);

/* Insert a segment into a segment list. */
void jpc_seglist_insert(jpc_dec_seglist_t *list, jpc_dec_seg_t *ins,
  jpc_dec_seg_t *node);

#endif
