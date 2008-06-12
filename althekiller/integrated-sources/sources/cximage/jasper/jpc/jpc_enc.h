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
 * $Id$
 */

#ifndef JPC_ENC_H
#define JPC_ENC_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_seq.h"

#include "jpc_t2cod.h"
#include "jpc_mqenc.h"
#include "jpc_cod.h"
#include "jpc_tagtree.h"
#include "jpc_cs.h"
#include "jpc_flt.h"
#include "jpc_tsfb.h"

/******************************************************************************\
* Constants.
\******************************************************************************/

/* The number of bits used in various lookup tables. */
#define	JPC_NUMEXTRABITS	JPC_NMSEDEC_FRACBITS

/* An invalid R-D slope value. */
#define	JPC_BADRDSLOPE	(-1)

/******************************************************************************\
* Coding parameters types.
\******************************************************************************/

/* Per-component coding paramters. */

typedef struct {

	/* The horizontal sampling period. */
	uint_fast8_t sampgrdstepx;

	/* The vertical sampling period. */
	uint_fast8_t sampgrdstepy;

	/* The sample alignment horizontal offset. */
	uint_fast8_t sampgrdsubstepx;

	/* The sample alignment vertical offset. */
	uint_fast8_t sampgrdsubstepy;

	/* The precision of the samples. */
	uint_fast8_t prec;

	/* The signedness of the samples. */
	bool sgnd;

	/* The number of step sizes. */
	uint_fast16_t numstepsizes;

	/* The quantizer step sizes. */
	uint_fast16_t stepsizes[JPC_MAXBANDS];

} jpc_enc_ccp_t;

/* Per-tile coding parameters. */

typedef struct {

	/* The coding mode. */
	bool intmode;

	/* The coding style (i.e., SOP, EPH). */
	uint_fast8_t csty;

	/* The progression order. */
	uint_fast8_t prg;

	/* The multicomponent transform. */
	uint_fast8_t mctid;

	/* The number of layers. */
	uint_fast16_t numlyrs;

	/* The normalized bit rates associated with the various
	  intermediate layers. */
	jpc_fix_t *ilyrrates;

} jpc_enc_tcp_t;

/* Per tile-component coding parameters. */

typedef struct {

	/* The coding style (i.e., explicit precinct sizes). */
	uint_fast8_t csty;

	/* The maximum number of resolution levels allowed. */
	uint_fast8_t maxrlvls;

	/* The exponent for the nominal code block width. */
	uint_fast16_t cblkwidthexpn;

	/* The exponent for the nominal code block height. */
	uint_fast16_t cblkheightexpn;

	/* The code block style parameters (e.g., lazy, terminate all,
	  segmentation symbols, causal, reset probability models). */
	uint_fast8_t cblksty;

	/* The QMFB. */
	uint_fast8_t qmfbid;

	/* The precinct width values. */
	uint_fast16_t prcwidthexpns[JPC_MAXRLVLS];

	/* The precinct height values. */
	uint_fast16_t prcheightexpns[JPC_MAXRLVLS];

	/* The number of guard bits. */
	uint_fast8_t numgbits;

} jpc_enc_tccp_t;

/* Coding parameters. */

typedef struct {

	/* The debug level. */
	int debug;

	/* The horizontal offset from the origin of the reference grid to the
	  left edge of the image area. */
	uint_fast32_t imgareatlx;

	/* The vertical offset from the origin of the reference grid to the
	  top edge of the image area. */
	uint_fast32_t imgareatly;

	/* The horizontal offset from the origin of the reference grid to the
	  right edge of the image area (plus one). */
	uint_fast32_t refgrdwidth;

	/* The vertical offset from the origin of the reference grid to the
	  bottom edge of the image area (plus one). */
	uint_fast32_t refgrdheight;

	/* The horizontal offset from the origin of the tile grid to the
	  origin of the reference grid. */
	uint_fast32_t tilegrdoffx;

	/* The vertical offset from the origin of the tile grid to the
	  origin of the reference grid. */
	uint_fast32_t tilegrdoffy;

	/* The nominal tile width in units of the image reference grid. */
	uint_fast32_t tilewidth;

	/* The nominal tile height in units of the image reference grid. */
	uint_fast32_t tileheight;

	/* The number of tiles spanning the image area in the horizontal
	  direction. */
	uint_fast32_t numhtiles;

	/* The number of tiles spanning the image area in the vertical
	  direction. */
	uint_fast32_t numvtiles;

	/* The number of tiles. */
	uint_fast32_t numtiles;

	/* The number of components. */
	uint_fast16_t numcmpts;

	/* The per-component coding parameters. */
	jpc_enc_ccp_t *ccps;

	/* The per-tile coding parameters. */
	jpc_enc_tcp_t tcp;

	/* The per-tile-component coding parameters. */
	jpc_enc_tccp_t tccp;

	/* The target code stream length in bytes. */
	uint_fast32_t totalsize;

	/* The raw (i.e., uncompressed) size of the image in bytes. */
	uint_fast32_t rawsize;

} jpc_enc_cp_t;

/******************************************************************************\
* Encoder class.
\******************************************************************************/

/* Encoder per-coding-pass state information. */

typedef struct {

	/* The starting offset for this pass. */
	int start;

	/* The ending offset for this pass. */
	int end;

	/* The type of data in this pass (i.e., MQ or raw). */
	int type;

	/* Flag indicating that this pass is terminated. */
	int term;

	/* The entropy coder state after coding this pass. */
	jpc_mqencstate_t mqencstate;

	/* The layer to which this pass has been assigned. */
	int lyrno;

	/* The R-D slope for this pass. */
	jpc_flt_t rdslope;

	/* The weighted MSE reduction associated with this pass. */
	jpc_flt_t wmsedec;

	/* The cumulative weighted MSE reduction. */
	jpc_flt_t cumwmsedec;

	/* The normalized MSE reduction. */
	long nmsedec;

} jpc_enc_pass_t;

/* Encoder per-code-block state information. */

typedef struct {

	/* The number of passes. */
	int numpasses;

	/* The per-pass information. */
	jpc_enc_pass_t *passes;

	/* The number of passes encoded so far. */
	int numencpasses;

	/* The number of insignificant MSBs. */
	int numimsbs;

	/* The number of bits used to encode pass data lengths. */
	int numlenbits;

	/* The byte stream for this code block. */
	jas_stream_t *stream;

	/* The entropy encoder. */
	jpc_mqenc_t *mqenc;

	/* The data for this code block. */
	jas_matrix_t *data;

	/* The state for this code block. */
	jas_matrix_t *flags;

	/* The number of bit planes required for this code block. */
	int numbps;

	/* The next pass to be encoded. */
	jpc_enc_pass_t *curpass;

	/* The per-code-block-group state information. */
	struct jpc_enc_prc_s *prc;

	/* The saved current pass. */
	/* This is used by the rate control code. */
	jpc_enc_pass_t *savedcurpass;

	/* The saved length indicator size. */
	/* This is used by the rate control code. */
	int savednumlenbits;

	/* The saved number of encoded passes. */
	/* This is used by the rate control code. */
	int savednumencpasses;

} jpc_enc_cblk_t;

/* Encoder per-code-block-group state information. */

typedef struct jpc_enc_prc_s {

	/* The x-coordinate of the top-left corner of the precinct. */
	uint_fast32_t tlx;

	/* The y-coordinate of the top-left corner of the precinct. */
	uint_fast32_t tly;

	/* The x-coordinate of the bottom-right corner of the precinct
	  (plus one). */
	uint_fast32_t brx;

	/* The y-coordinate of the bottom-right corner of the precinct
	  (plus one). */
	uint_fast32_t bry;

	/* The number of code blocks spanning the precinct in the horizontal
	direction. */
	int numhcblks;

	/* The number of code blocks spanning the precinct in the vertical
	direction. */
	int numvcblks;

	/* The total number of code blocks. */
	int numcblks;

	/* The per-code-block information. */
	jpc_enc_cblk_t *cblks;

	/* The inclusion tag tree. */
	jpc_tagtree_t *incltree;

	/* The insignifcant MSBs tag tree. */
	jpc_tagtree_t *nlibtree;

	/* The per-band information. */
	struct jpc_enc_band_s *band;

	/* The saved inclusion tag tree. */
	/* This is used by rate control. */
	jpc_tagtree_t *savincltree;

	/* The saved leading-insignificant-bit-planes tag tree. */
	/* This is used by rate control. */
	jpc_tagtree_t *savnlibtree;

} jpc_enc_prc_t;

/* Encoder per-band state information. */

typedef struct jpc_enc_band_s {

	/* The per precinct information. */
	jpc_enc_prc_t *prcs;

	/* The coefficient data for this band. */
	jas_matrix_t *data;

	/* The orientation of this band (i.e., LL, LH, HL, or HH). */
	int orient;

	/* The number of bit planes associated with this band. */
	int numbps;

	/* The quantizer step size. */
	jpc_fix_t absstepsize;

	/* The encoded quantizer step size. */
	int stepsize;

	/* The L2 norm of the synthesis basis functions associated with
	  this band.  (The MCT is not considered in this value.) */
	jpc_fix_t synweight;

	/* The analysis gain for this band. */
	int analgain;

	/* The per-resolution-level information. */
	struct jpc_enc_rlvl_s *rlvl;

} jpc_enc_band_t;

/* Encoder per-resolution-level state information. */

typedef struct jpc_enc_rlvl_s {

	/* The x-coordinate of the top-left corner of the tile-component
	  at this resolution. */
	uint_fast32_t tlx;

	/* The y-coordinate of the top-left corner of the tile-component
	  at this resolution. */
	uint_fast32_t tly;

	/* The x-coordinate of the bottom-right corner of the tile-component
	  at this resolution (plus one). */
	uint_fast32_t brx;

	/* The y-coordinate of the bottom-right corner of the tile-component
	  at this resolution (plus one). */
	uint_fast32_t bry;

	/* The exponent value for the nominal precinct width measured
	  relative to the associated LL band. */
	int prcwidthexpn;

	/* The exponent value for the nominal precinct height measured
	  relative to the associated LL band. */
	int prcheightexpn;

	/* The number of precincts spanning the resolution level in the
	  horizontal direction. */
	int numhprcs;

	/* The number of precincts spanning the resolution level in the
	  vertical direction. */
	int numvprcs;

	/* The total number of precincts. */
	int numprcs;

	/* The exponent value for the nominal code block group width.
	  This quantity is associated with the next lower resolution level
	  (assuming that there is one). */
	int cbgwidthexpn;

	/* The exponent value for the nominal code block group height.
	  This quantity is associated with the next lower resolution level
	  (assuming that there is one). */
	int cbgheightexpn;

	/* The exponent value for the code block width. */
	uint_fast16_t cblkwidthexpn;

	/* The exponent value for the code block height. */
	uint_fast16_t cblkheightexpn;

	/* The number of bands associated with this resolution level. */
	int numbands;

	/* The per-band information. */
	jpc_enc_band_t *bands;

	/* The parent tile-component. */
	struct jpc_enc_tcmpt_s *tcmpt;

} jpc_enc_rlvl_t;

/* Encoder per-tile-component state information. */

typedef struct jpc_enc_tcmpt_s {

	/* The number of resolution levels. */
	int numrlvls;

	/* The per-resolution-level information. */
	jpc_enc_rlvl_t *rlvls;

	/* The tile-component data. */
	jas_matrix_t *data;

	/* The QMFB. */
	int qmfbid;

	/* The number of bands. */
	int numbands;

	/* The TSFB. */
	jpc_tsfb_t *tsfb;

	/* The synthesis energy weight (for the MCT). */
	jpc_fix_t synweight;

	/* The precinct width exponents. */
	int prcwidthexpns[JPC_MAXRLVLS];

	/* The precinct height exponents. */
	int prcheightexpns[JPC_MAXRLVLS];

	/* The code block width exponent. */
	int cblkwidthexpn;

	/* The code block height exponent. */
	int cblkheightexpn;

	/* Coding style (i.e., explicit precinct sizes). */
	int csty;

	/* Code block style. */
	int cblksty;

	/* The number of quantizer step sizes. */
	int numstepsizes;

	/* The encoded quantizer step sizes. */
	uint_fast16_t stepsizes[JPC_MAXBANDS];

	/* The parent tile. */
	struct jpc_enc_tile_s *tile;

} jpc_enc_tcmpt_t;

/* Encoder per-tile state information. */

typedef struct jpc_enc_tile_s {

	/* The tile number. */
	uint_fast32_t tileno;

	/* The x-coordinate of the top-left corner of the tile measured with
	  respect to the reference grid. */
	uint_fast32_t tlx;

	/* The y-coordinate of the top-left corner of the tile measured with
	  respect to the reference grid. */
	uint_fast32_t tly;

	/* The x-coordinate of the bottom-right corner of the tile measured
	  with respect to the reference grid (plus one). */
	uint_fast32_t brx;

	/* The y-coordinate of the bottom-right corner of the tile measured
	  with respect to the reference grid (plus one). */
	uint_fast32_t bry;

	/* The coding style. */
	uint_fast8_t csty;

	/* The progression order. */
	uint_fast8_t prg;

	/* The number of layers. */
	int numlyrs;

	/* The MCT to employ (if any). */
	uint_fast8_t mctid;

	/* The packet iterator (used to determine the order of packet
	  generation). */
	jpc_pi_t *pi;

	/* The coding mode (i.e., integer or real). */
	bool intmode;

	/* The number of bytes to allocate to the various layers. */
	uint_fast32_t *lyrsizes;

	/* The number of tile-components. */
	int numtcmpts;

	/* The per tile-component information. */
	jpc_enc_tcmpt_t *tcmpts;

	/* The raw (i.e., uncompressed) size of this tile. */
	uint_fast32_t rawsize;

} jpc_enc_tile_t;

/* Encoder class. */

typedef struct jpc_enc_s {

	/* The image being encoded. */
	jas_image_t *image;

	/* The output stream. */
	jas_stream_t *out;

	/* The coding parameters. */
	jpc_enc_cp_t *cp;

	/* The tile currently being processed. */
	jpc_enc_tile_t *curtile;

	/* The code stream state. */
	jpc_cstate_t *cstate;

	/* The number of bytes output so far. */
	uint_fast32_t len;

	/* The number of bytes available for the main body of the code stream. */
	/* This is used for rate allocation purposes. */
	uint_fast32_t mainbodysize;

	/* The marker segment currently being processed. */
	/* This member is a convenience for making cleanup easier. */
	jpc_ms_t *mrk;

	/* The stream used to temporarily hold tile-part data. */
	jas_stream_t *tmpstream;

} jpc_enc_t;

#endif
