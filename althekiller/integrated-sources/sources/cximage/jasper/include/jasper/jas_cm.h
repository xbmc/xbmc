/*
 * Copyright (c) 2002-2003 Michael David Adams.
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
 * Color Management
 *
 * $Id$
 */

#ifndef JAS_CM_H
#define JAS_CM_H

#include <jasper/jas_config.h>
#include <jasper/jas_icc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int jas_clrspc_t;

/* transform operations */
#define	JAS_CMXFORM_OP_FWD	0
#define	JAS_CMXFORM_OP_REV	1
#define	JAS_CMXFORM_OP_PROOF	2
#define	JAS_CMXFORM_OP_GAMUT	3

/* rendering intents */
#define	JAS_CMXFORM_INTENT_PER		0
#define	JAS_CMXFORM_INTENT_RELCLR	1
#define	JAS_CMXFORM_INTENT_ABSCLR	2
#define	JAS_CMXFORM_INTENT_SAT		3
#define	JAS_CMXFORM_NUMINTENTS		4

#define	JAS_CMXFORM_OPTM_SPEED	0
#define JAS_CMXFORM_OPTM_SIZE	1
#define	JAS_CMXFORM_OPTM_ACC	2


#define	jas_clrspc_create(fam, mbr)	(((fam) << 8) | (mbr))
#define	jas_clrspc_fam(clrspc)	((clrspc) >> 8)
#define	jas_clrspc_mbr(clrspc)	((clrspc) & 0xff)
#define	jas_clrspc_isgeneric(clrspc)	(!jas_clrspc_mbr(clrspc))
#define	jas_clrspc_isunknown(clrspc)	((clrspc) & JAS_CLRSPC_UNKNOWNMASK)

#define	JAS_CLRSPC_UNKNOWNMASK	0x4000

/* color space families */
#define	JAS_CLRSPC_FAM_UNKNOWN	0
#define	JAS_CLRSPC_FAM_XYZ	1
#define	JAS_CLRSPC_FAM_LAB	2
#define	JAS_CLRSPC_FAM_GRAY	3
#define	JAS_CLRSPC_FAM_RGB	4
#define	JAS_CLRSPC_FAM_YCBCR	5

/* specific color spaces */
#define	JAS_CLRSPC_UNKNOWN	JAS_CLRSPC_UNKNOWNMASK
#define	JAS_CLRSPC_CIEXYZ	jas_clrspc_create(JAS_CLRSPC_FAM_XYZ, 1)
#define	JAS_CLRSPC_CIELAB	jas_clrspc_create(JAS_CLRSPC_FAM_LAB, 1)
#define	JAS_CLRSPC_SGRAY	jas_clrspc_create(JAS_CLRSPC_FAM_GRAY, 1)
#define	JAS_CLRSPC_SRGB		jas_clrspc_create(JAS_CLRSPC_FAM_RGB, 1)
#define	JAS_CLRSPC_SYCBCR	jas_clrspc_create(JAS_CLRSPC_FAM_YCBCR, 1)

/* generic color spaces */
#define	JAS_CLRSPC_GENRGB	jas_clrspc_create(JAS_CLRSPC_FAM_RGB, 0)
#define	JAS_CLRSPC_GENGRAY	jas_clrspc_create(JAS_CLRSPC_FAM_GRAY, 0)
#define	JAS_CLRSPC_GENYCBCR	jas_clrspc_create(JAS_CLRSPC_FAM_YCBCR, 0)

#define	JAS_CLRSPC_CHANIND_YCBCR_Y	0
#define	JAS_CLRSPC_CHANIND_YCBCR_CB	1
#define	JAS_CLRSPC_CHANIND_YCBCR_CR	2

#define	JAS_CLRSPC_CHANIND_RGB_R	0
#define	JAS_CLRSPC_CHANIND_RGB_G	1
#define	JAS_CLRSPC_CHANIND_RGB_B	2

#define	JAS_CLRSPC_CHANIND_GRAY_Y	0

typedef double jas_cmreal_t;

struct jas_cmpxform_s;

typedef struct {
	long *buf;
	int prec;
	int sgnd;
	int width;
	int height;
} jas_cmcmptfmt_t;

typedef struct {
	int numcmpts;
	jas_cmcmptfmt_t *cmptfmts;
} jas_cmpixmap_t;

typedef struct {
	void (*destroy)(struct jas_cmpxform_s *pxform);
	int (*apply)(struct jas_cmpxform_s *pxform, jas_cmreal_t *in, jas_cmreal_t *out, int cnt);
	void (*dump)(struct jas_cmpxform_s *pxform);
} jas_cmpxformops_t;

typedef struct {
	jas_cmreal_t *data;
	int size;
} jas_cmshapmatlut_t;

typedef struct {
	int mono;
	int order;
	int useluts;
	int usemat;
	jas_cmshapmatlut_t luts[3];
	jas_cmreal_t mat[3][4];
} jas_cmshapmat_t;

typedef struct {
	int order;
} jas_cmshaplut_t;

typedef struct {
	int inclrspc;
	int outclrspc;
} jas_cmclrspcconv_t;

#define	jas_align_t	double

typedef struct jas_cmpxform_s {
	int refcnt;
	jas_cmpxformops_t *ops;
	int numinchans;
	int numoutchans;
	union {
		jas_align_t dummy;
		jas_cmshapmat_t shapmat;
		jas_cmshaplut_t shaplut;
		jas_cmclrspcconv_t clrspcconv;
	} data;
} jas_cmpxform_t;

typedef struct {
	int numpxforms;
	int maxpxforms;
	jas_cmpxform_t **pxforms;
} jas_cmpxformseq_t;

typedef struct {
	int numinchans;
	int numoutchans;
	jas_cmpxformseq_t *pxformseq;
} jas_cmxform_t;

#define	JAS_CMPROF_TYPE_DEV	1
#define	JAS_CMPROF_TYPE_CLRSPC	2

#define	JAS_CMPROF_NUMPXFORMSEQS	13

typedef struct {
	int clrspc;
	int numchans;
	int refclrspc;
	int numrefchans;
	jas_iccprof_t *iccprof;
	jas_cmpxformseq_t *pxformseqs[JAS_CMPROF_NUMPXFORMSEQS];
} jas_cmprof_t;

/* Create a profile. */

/* Destroy a profile. */
void jas_cmprof_destroy(jas_cmprof_t *prof);

#if 0
typedef int_fast32_t jas_cmattrname_t;
typedef int_fast32_t jas_cmattrval_t;
typedef int_fast32_t jas_cmattrtype_t;
/* Load a profile. */
int jas_cmprof_load(jas_cmprof_t *prof, jas_stream_t *in, int fmt);
/* Save a profile. */
int jas_cmprof_save(jas_cmprof_t *prof, jas_stream_t *out, int fmt);
/* Set an attribute of a profile. */
int jas_cm_prof_setattr(jas_cm_prof_t *prof, jas_cm_attrname_t name, void *val);
/* Get an attribute of a profile. */
void *jas_cm_prof_getattr(jas_cm_prof_t *prof, jas_cm_attrname_t name);
#endif

jas_cmxform_t *jas_cmxform_create(jas_cmprof_t *inprof, jas_cmprof_t *outprof,
  jas_cmprof_t *proofprof, int op, int intent, int optimize);

void jas_cmxform_destroy(jas_cmxform_t *xform);

/* Apply a transform to data. */
int jas_cmxform_apply(jas_cmxform_t *xform, jas_cmpixmap_t *in,
  jas_cmpixmap_t *out);

int jas_cxform_optimize(jas_cmxform_t *xform, int optimize);

int jas_clrspc_numchans(int clrspc);
jas_cmprof_t *jas_cmprof_createfromiccprof(jas_iccprof_t *iccprof);
jas_cmprof_t *jas_cmprof_createfromclrspc(int clrspc);
jas_iccprof_t *jas_iccprof_createfromcmprof(jas_cmprof_t *prof);

#define	jas_cmprof_clrspc(prof) ((prof)->clrspc)
jas_cmprof_t *jas_cmprof_copy(jas_cmprof_t *prof);

#ifdef __cplusplus
}
#endif

#endif
