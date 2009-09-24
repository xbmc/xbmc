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
 * JPEG-2000 Code Stream Library
 *
 * $Id$
 */

#ifndef JPC_CS_H
#define JPC_CS_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_image.h"
#include "jasper/jas_stream.h"

#include "jpc_cod.h"

/******************************************************************************\
* Constants and Types.
\******************************************************************************/

/* The maximum number of resolution levels. */
#define	JPC_MAXRLVLS	33

/* The maximum number of bands. */
#define	JPC_MAXBANDS	(3 * JPC_MAXRLVLS + 1)

/* The maximum number of layers. */
#define JPC_MAXLYRS	16384

/**************************************\
* Code stream.
\**************************************/

/*
 * Code stream states.
 */

/* Initial. */
#define	JPC_CS_INIT	0
/* Main header. */
#define	JPC_CS_MHDR	1
/* Tile-part header. */
#define	JPC_CS_THDR	2
/* Main trailer. */
#define	JPC_CS_MTLR	3
/* Tile-part data. */
#define	JPC_CS_TDATA	4

/*
 * Unfortunately, the code stream syntax was not designed in such a way that
 * any given marker segment can be correctly decoded without additional state
 * derived from previously decoded marker segments.
 * For example, a RGN/COC/QCC marker segment cannot be decoded unless the
 * number of components is known.
 */

/*
 * Code stream state information.
 */

typedef struct {

	/* The number of components. */
	uint_fast16_t numcomps;

} jpc_cstate_t;

/**************************************\
* SOT marker segment parameters.
\**************************************/

typedef struct {

	/* The tile number. */
	uint_fast16_t tileno;

	/* The combined length of the marker segment and its auxilary data
	  (i.e., packet data). */
	uint_fast32_t len;

	/* The tile-part instance. */
	uint_fast8_t partno;

	/* The number of tile-parts. */
	uint_fast8_t numparts;

} jpc_sot_t;

/**************************************\
* SIZ marker segment parameters.
\**************************************/

/* Per component information. */

typedef struct {

	/* The precision of the samples. */
	uint_fast8_t prec;

	/* The signedness of the samples. */
	uint_fast8_t sgnd;

	/* The horizontal separation of samples with respect to the reference
	  grid. */
	uint_fast8_t hsamp;

	/* The vertical separation of samples with respect to the reference
	  grid. */
	uint_fast8_t vsamp;

} jpc_sizcomp_t;

/* SIZ marker segment parameters. */

typedef struct {

	/* The code stream capabilities. */
	uint_fast16_t caps;

	/* The width of the image in units of the reference grid. */
	uint_fast32_t width;

	/* The height of the image in units of the reference grid. */
	uint_fast32_t height;

	/* The horizontal offset from the origin of the reference grid to the
	  left side of the image area. */
	uint_fast32_t xoff;

	/* The vertical offset from the origin of the reference grid to the
	  top side of the image area. */
	uint_fast32_t yoff;

	/* The nominal width of a tile in units of the reference grid. */
	uint_fast32_t tilewidth;

	/* The nominal height of a tile in units of the reference grid. */
	uint_fast32_t tileheight;

	/* The horizontal offset from the origin of the reference grid to the
	  left side of the first tile. */
	uint_fast32_t tilexoff;

	/* The vertical offset from the origin of the reference grid to the
	  top side of the first tile. */
	uint_fast32_t tileyoff;

	/* The number of components. */
	uint_fast16_t numcomps;

	/* The per-component information. */
	jpc_sizcomp_t *comps;

} jpc_siz_t;

/**************************************\
* COD marker segment parameters.
\**************************************/

/*
 * Coding style constants.
 */

/* Precincts may be used. */
#define	JPC_COX_PRT	0x01
/* SOP marker segments may be used. */
#define	JPC_COD_SOP	0x02
/* EPH marker segments may be used. */
#define	JPC_COD_EPH	0x04

/*
 * Progression order constants.
 */

/* Layer-resolution-component-precinct progressive
  (i.e., progressive by fidelity). */
#define	JPC_COD_LRCPPRG	0
/* Resolution-layer-component-precinct progressive
  (i.e., progressive by resolution). */
#define	JPC_COD_RLCPPRG	1
/* Resolution-precinct-component-layer progressive. */
#define	JPC_COD_RPCLPRG	2
/* Precinct-component-resolution-layer progressive. */
#define	JPC_COD_PCRLPRG	3
/* Component-position-resolution-layer progressive. */
#define	JPC_COD_CPRLPRG	4

/*
 * Code block style constants.
 */

#define	JPC_COX_LAZY	0x01 /* Selective arithmetic coding bypass. */
#define	JPC_COX_RESET	0x02 /* Reset context probabilities. */
#define	JPC_COX_TERMALL	0x04 /* Terminate all coding passes. */
#define	JPC_COX_VSC		0x08 /* Vertical stripe causal context formation. */
#define	JPC_COX_PTERM	0x10 /* Predictable termination. */
#define	JPC_COX_SEGSYM	0x20 /* Use segmentation symbols. */

/* Transform constants. */
#define	JPC_COX_INS	0x00 /* Irreversible 9/7. */
#define	JPC_COX_RFT	0x01 /* Reversible 5/3. */

/* Multicomponent transform constants. */
#define	JPC_COD_NOMCT	0x00 /* No multicomponent transform. */
#define	JPC_COD_MCT		0x01 /* Multicomponent transform. */

/* Get the code block size value from the code block size exponent. */
#define	JPC_COX_CBLKSIZEEXPN(x)		((x) - 2)
/* Get the code block size exponent from the code block size value. */
#define	JPC_COX_GETCBLKSIZEEXPN(x)	((x) + 2)

/* Per resolution-level information. */

typedef struct {

	/* The packet partition width. */
	uint_fast8_t parwidthval;

	/* The packet partition height. */
	uint_fast8_t parheightval;

} jpc_coxrlvl_t;

/* Per component information. */

typedef struct {

	/* The coding style. */
	uint_fast8_t csty;

	/* The number of decomposition levels. */
	uint_fast8_t numdlvls;

	/* The nominal code block width specifier. */
	uint_fast8_t cblkwidthval;

	/* The nominal code block height specifier. */
	uint_fast8_t cblkheightval;

	/* The style of coding passes. */
	uint_fast8_t cblksty;

	/* The QMFB employed. */
	uint_fast8_t qmfbid;

	/* The number of resolution levels. */
	int numrlvls;

	/* The per-resolution-level information. */
	jpc_coxrlvl_t rlvls[JPC_MAXRLVLS];

} jpc_coxcp_t;

/* COD marker segment parameters. */

typedef struct {

	/* The general coding style. */
	uint_fast8_t csty;

	/* The progression order. */
	uint_fast8_t prg;

	/* The number of layers. */
	uint_fast16_t numlyrs;

	/* The multicomponent transform. */
	uint_fast8_t mctrans;

	/* Component-related parameters. */
	jpc_coxcp_t compparms;

} jpc_cod_t;

/* COC marker segment parameters. */

typedef struct {

	/* The component number. */
	uint_fast16_t compno;

	/* Component-related parameters. */
	jpc_coxcp_t compparms;

} jpc_coc_t;

/**************************************\
* RGN marker segment parameters.
\**************************************/

/* The maxshift ROI style. */
#define	JPC_RGN_MAXSHIFT	0x00

typedef struct {

	/* The component to which the marker applies. */
	uint_fast16_t compno;

	/* The ROI style. */
	uint_fast8_t roisty;

	/* The ROI shift value. */
	uint_fast8_t roishift;

} jpc_rgn_t;

/**************************************\
* QCD/QCC marker segment parameters.
\**************************************/

/*
 * Quantization style constants.
 */

#define	JPC_QCX_NOQNT	0 /* No quantization. */
#define	JPC_QCX_SIQNT	1 /* Scalar quantization, implicit. */
#define	JPC_QCX_SEQNT	2 /* Scalar quantization, explicit. */

/*
 * Stepsize manipulation macros.
 */

#define	JPC_QCX_GETEXPN(x)	((x) >> 11)
#define	JPC_QCX_GETMANT(x)	((x) & 0x07ff)
#define	JPC_QCX_EXPN(x)		(assert(!((x) & (~0x1f))), (((x) & 0x1f) << 11))
#define	JPC_QCX_MANT(x)		(assert(!((x) & (~0x7ff))), ((x) & 0x7ff))

/* Per component information. */

typedef struct {

	/* The quantization style. */
	uint_fast8_t qntsty;

	/* The number of step sizes. */
	int numstepsizes;

	/* The step sizes. */
	uint_fast16_t *stepsizes;

	/* The number of guard bits. */
	uint_fast8_t numguard;

} jpc_qcxcp_t;

/* QCC marker segment parameters. */

typedef struct {

	/* The component associated with this marker segment. */
	uint_fast16_t compno;

	/* The parameters. */
	jpc_qcxcp_t compparms;

} jpc_qcc_t;

/* QCD marker segment parameters. */

typedef struct {

	/* The parameters. */
	jpc_qcxcp_t compparms;

} jpc_qcd_t;

/**************************************\
* POD marker segment parameters.
\**************************************/

typedef struct {

	/* The progression order. */
	uint_fast8_t prgord;

	/* The lower bound (inclusive) on the resolution level for the
	  progression order volume. */
	uint_fast8_t rlvlnostart;

	/* The upper bound (exclusive) on the resolution level for the
	  progression order volume. */
	uint_fast8_t rlvlnoend;

	/* The lower bound (inclusive) on the component for the progression
	  order volume. */
	uint_fast16_t compnostart;

	/* The upper bound (exclusive) on the component for the progression
	  order volume. */
	uint_fast16_t compnoend;

	/* The upper bound (exclusive) on the layer for the progression
	  order volume. */
	uint_fast16_t lyrnoend;

} jpc_pocpchg_t;

/* An alias for the above type. */
typedef jpc_pocpchg_t jpc_pchg_t;

/* POC marker segment parameters. */

typedef struct {

	/* The number of progression order changes. */
	int numpchgs;

	/* The per-progression-order-change information. */
	jpc_pocpchg_t *pchgs;

} jpc_poc_t;

/**************************************\
* PPM/PPT marker segment parameters.
\**************************************/

/* PPM marker segment parameters. */

typedef struct {

	/* The index. */
	uint_fast8_t ind;

	/* The length. */
	uint_fast16_t len;

	/* The data. */
	uchar *data;

} jpc_ppm_t;

/* PPT marker segment parameters. */

typedef struct {

	/* The index. */
	uint_fast8_t ind;

	/* The length. */
	uint_fast32_t len;

	/* The data. */
	unsigned char *data;

} jpc_ppt_t;

/**************************************\
* COM marker segment parameters.
\**************************************/

/*
 * Registration IDs.
 */

#define	JPC_COM_BIN		0x00
#define	JPC_COM_LATIN	0x01

typedef struct {

	/* The registration ID. */
	uint_fast16_t regid;

	/* The length of the data in bytes. */
	uint_fast16_t len;

	/* The data. */
	uchar *data;

} jpc_com_t;

/**************************************\
* SOP marker segment parameters.
\**************************************/

typedef struct {

	/* The sequence number. */
	uint_fast16_t seqno;

} jpc_sop_t;

/**************************************\
* CRG marker segment parameters.
\**************************************/

/* Per component information. */

typedef struct {

	/* The horizontal offset. */
	uint_fast16_t hoff;

	/* The vertical offset. */
	uint_fast16_t voff;

} jpc_crgcomp_t;

typedef struct {

	/* The number of components. */
	int numcomps;

	/* Per component information. */
	jpc_crgcomp_t *comps;

} jpc_crg_t;

/**************************************\
* Marker segment parameters for unknown marker type.
\**************************************/

typedef struct {

	/* The data. */
	uchar *data;

	/* The length. */
	uint_fast16_t len;

} jpc_unk_t;

/**************************************\
* Generic marker segment parameters.
\**************************************/

typedef union {
	int soc;	/* unused */
	jpc_sot_t sot;
	int sod;	/* unused */
	int eoc;	/* unused */
	jpc_siz_t siz;
	jpc_cod_t cod;
	jpc_coc_t coc;
	jpc_rgn_t rgn;
	jpc_qcd_t qcd;
	jpc_qcc_t qcc;
	jpc_poc_t poc;
	/* jpc_plm_t plm; */
	/* jpc_plt_t plt; */
	jpc_ppm_t ppm;
	jpc_ppt_t ppt;
	jpc_sop_t sop;
	int eph;	/* unused */
	jpc_com_t com;
	jpc_crg_t crg;
	jpc_unk_t unk;
} jpc_msparms_t;

/**************************************\
* Marker segment.
\**************************************/

/* Marker segment IDs. */

/* The smallest valid marker value. */
#define	JPC_MS_MIN	0xff00

/* The largest valid marker value. */
#define	JPC_MS_MAX	0xffff

/* The minimum marker value that cannot occur within packet data. */
#define	JPC_MS_INMIN	0xff80
/* The maximum marker value that cannot occur within packet data. */
#define	JPC_MS_INMAX	0xffff

/* Delimiting marker segments. */
#define	JPC_MS_SOC	0xff4f /* Start of code stream (SOC). */
#define	JPC_MS_SOT	0xff90 /* Start of tile-part (SOT). */
#define	JPC_MS_SOD	0xff93 /* Start of data (SOD). */
#define	JPC_MS_EOC	0xffd9 /* End of code stream (EOC). */

/* Fixed information marker segments. */
#define	JPC_MS_SIZ	0xff51 /* Image and tile size (SIZ). */

/* Functional marker segments. */
#define	JPC_MS_COD	0xff52 /* Coding style default (COD). */
#define JPC_MS_COC	0xff53 /* Coding style component (COC). */
#define	JPC_MS_RGN	0xff5e /* Region of interest (RGN). */
#define JPC_MS_QCD	0xff5c /* Quantization default (QCD). */
#define JPC_MS_QCC	0xff5d /* Quantization component (QCC). */
#define JPC_MS_POC	0xff5f /* Progression order default (POC). */

/* Pointer marker segments. */
#define	JPC_MS_TLM	0xff55 /* Tile-part lengths, main header (TLM). */
#define	JPC_MS_PLM	0xff57 /* Packet length, main header (PLM). */
#define	JPC_MS_PLT	0xff58 /* Packet length, tile-part header (PLT). */
#define	JPC_MS_PPM	0xff60 /* Packed packet headers, main header (PPM). */
#define	JPC_MS_PPT	0xff61 /* Packet packet headers, tile-part header (PPT). */

/* In bit stream marker segments. */
#define	JPC_MS_SOP	0xff91	/* Start of packet (SOP). */
#define	JPC_MS_EPH	0xff92	/* End of packet header (EPH). */

/* Informational marker segments. */
#define	JPC_MS_CRG	0xff63 /* Component registration (CRG). */
#define JPC_MS_COM	0xff64 /* Comment (COM). */

/* Forward declaration. */
struct jpc_msops_s;

/* Generic marker segment class. */

typedef struct {

	/* The type of marker segment. */
	uint_fast16_t id;

	/* The length of the marker segment. */
	uint_fast16_t len;

	/* The starting offset within the stream. */
	uint_fast32_t off;

	/* The parameters of the marker segment. */
	jpc_msparms_t parms;

	/* The marker segment operations. */
	struct jpc_msops_s *ops;

} jpc_ms_t;

/* Marker segment operations (which depend on the marker segment type). */

typedef struct jpc_msops_s {

	/* Destroy the marker segment parameters. */
	void (*destroyparms)(jpc_ms_t *ms);

	/* Get the marker segment parameters from a stream. */
	int (*getparms)(jpc_ms_t *ms, jpc_cstate_t *cstate, jas_stream_t *in);

	/* Put the marker segment parameters to a stream. */
	int (*putparms)(jpc_ms_t *ms, jpc_cstate_t *cstate, jas_stream_t *out);

	/* Dump the marker segment parameters (for debugging). */
	int (*dumpparms)(jpc_ms_t *ms, FILE *out);

} jpc_msops_t;

/******************************************************************************\
* Macros/Functions.
\******************************************************************************/

/* Create a code-stream state object. */
jpc_cstate_t *jpc_cstate_create(void);

/* Destroy a code-stream state object. */
void jpc_cstate_destroy(jpc_cstate_t *cstate);

/* Create a marker segment. */
jpc_ms_t *jpc_ms_create(int type);

/* Destroy a marker segment. */
void jpc_ms_destroy(jpc_ms_t *ms);

/* Does a marker segment have parameters? */
#define	JPC_MS_HASPARMS(x) \
	(!((x) == JPC_MS_SOC || (x) == JPC_MS_SOD || (x) == JPC_MS_EOC || \
	  (x) == JPC_MS_EPH || ((x) >= 0xff30 && (x) <= 0xff3f)))

/* Get the marker segment type. */
#define	jpc_ms_gettype(ms) \
	((ms)->id)

/* Read a marker segment from a stream. */
jpc_ms_t *jpc_getms(jas_stream_t *in, jpc_cstate_t *cstate);

/* Write a marker segment to a stream. */
int jpc_putms(jas_stream_t *out, jpc_cstate_t *cstate, jpc_ms_t *ms);

/* Copy code stream data from one stream to another. */
int jpc_getdata(jas_stream_t *in, jas_stream_t *out, long n);

/* Copy code stream data from one stream to another. */
int jpc_putdata(jas_stream_t *out, jas_stream_t *in, long n);

/* Dump a marker segment (for debugging). */
void jpc_ms_dump(jpc_ms_t *ms, FILE *out);

/* Read a 8-bit unsigned integer from a stream. */
int jpc_getuint8(jas_stream_t *in, uint_fast8_t *val);

/* Read a 16-bit unsigned integer from a stream. */
int jpc_getuint16(jas_stream_t *in, uint_fast16_t *val);

/* Read a 32-bit unsigned integer from a stream. */
int jpc_getuint32(jas_stream_t *in, uint_fast32_t *val);

/* Write a 8-bit unsigned integer to a stream. */
int jpc_putuint8(jas_stream_t *out, uint_fast8_t val);

/* Write a 16-bit unsigned integer to a stream. */
int jpc_putuint16(jas_stream_t *out, uint_fast16_t val);

/* Write a 32-bit unsigned integer to a stream. */
int jpc_putuint32(jas_stream_t *out, uint_fast32_t val);

#endif
