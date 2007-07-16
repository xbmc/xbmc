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
 * Tier-2 Coding Library
 *
 * $Id$
 */

#ifndef JPC_T2COD_H
#define	JPC_T2COD_H

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jpc_cs.h"

/******************************************************************************\
* Types.
\******************************************************************************/

/* Progression change list. */

typedef struct {

	/* The number of progression changes. */
	int numpchgs;

	/* The maximum number of progression changes that can be accomodated
	  without growing the progression change array. */
	int maxpchgs;

	/* The progression changes. */
	jpc_pchg_t **pchgs;

} jpc_pchglist_t;

/* Packet iterator per-resolution-level information. */

typedef struct {

	/* The number of precincts. */
	int numprcs;

	/* The last layer processed for each precinct. */
	int *prclyrnos;

	/* The precinct width exponent. */
	int prcwidthexpn;

	/* The precinct height exponent. */
	int prcheightexpn;

	/* The number of precincts spanning the resolution level in the horizontal
	  direction. */
	int numhprcs;

} jpc_pirlvl_t;

/* Packet iterator per-component information. */

typedef struct {

	/* The number of resolution levels. */
	int numrlvls;

	/* The per-resolution-level information. */
	jpc_pirlvl_t *pirlvls;

	/* The horizontal sampling period. */
	int hsamp;

	/* The vertical sampling period. */
	int vsamp;

} jpc_picomp_t;

/* Packet iterator class. */

typedef struct {

	/* The number of layers. */
	int numlyrs;

	/* The number of resolution levels. */
	int maxrlvls;

	/* The number of components. */
	int numcomps;

	/* The per-component information. */
	jpc_picomp_t *picomps;

	/* The current component. */
	jpc_picomp_t *picomp;

	/* The current resolution level. */
	jpc_pirlvl_t *pirlvl;

	/* The number of the current component. */
	int compno;

	/* The number of the current resolution level. */
	int rlvlno;

	/* The number of the current precinct. */
	int prcno;

	/* The number of the current layer. */
	int lyrno;

	/* The x-coordinate of the current position. */
	int x;

	/* The y-coordinate of the current position. */
	int y;

	/* The horizontal step size. */
	int xstep;

	/* The vertical step size. */
	int ystep;

	/* The x-coordinate of the top-left corner of the tile on the reference
	  grid. */
	int xstart;

	/* The y-coordinate of the top-left corner of the tile on the reference
	  grid. */
	int ystart;

	/* The x-coordinate of the bottom-right corner of the tile on the
	  reference grid (plus one). */
	int xend;

	/* The y-coordinate of the bottom-right corner of the tile on the
	  reference grid (plus one). */
	int yend;

	/* The current progression change. */
	jpc_pchg_t *pchg;

	/* The progression change list. */
	jpc_pchglist_t *pchglist;

	/* The progression to use in the absense of explicit specification. */
	jpc_pchg_t defaultpchg;

	/* The current progression change number. */
	int pchgno;

	/* Is this the first time in the current progression volume? */
	bool prgvolfirst;

	/* Is the current iterator value valid? */
	bool valid;

	/* The current packet number. */
	int pktno;

} jpc_pi_t;

/******************************************************************************\
* Functions/macros for packet iterators.
\******************************************************************************/

/* Create a packet iterator. */
jpc_pi_t *jpc_pi_create0(void);

/* Destroy a packet iterator. */
void jpc_pi_destroy(jpc_pi_t *pi);

/* Add a progression change to a packet iterator. */
int jpc_pi_addpchg(jpc_pi_t *pi, jpc_pocpchg_t *pchg);

/* Prepare a packet iterator for iteration. */
int jpc_pi_init(jpc_pi_t *pi);

/* Set the iterator to the first packet. */
int jpc_pi_begin(jpc_pi_t *pi);

/* Proceed to the next packet in sequence. */
int jpc_pi_next(jpc_pi_t *pi);

/* Get the index of the current packet. */
#define	jpc_pi_getind(pi)	((pi)->pktno)

/* Get the component number of the current packet. */
#define jpc_pi_cmptno(pi)	(assert(pi->valid), (pi)->compno)

/* Get the resolution level of the current packet. */
#define jpc_pi_rlvlno(pi)	(assert(pi->valid), (pi)->rlvlno)

/* Get the layer number of the current packet. */
#define jpc_pi_lyrno(pi)	(assert(pi->valid), (pi)->lyrno)

/* Get the precinct number of the current packet. */
#define jpc_pi_prcno(pi)	(assert(pi->valid), (pi)->prcno)

/* Get the progression order for the current packet. */
#define jpc_pi_prg(pi)	(assert(pi->valid), (pi)->pchg->prgord)

/******************************************************************************\
* Functions/macros for progression change lists.
\******************************************************************************/

/* Create a progression change list. */
jpc_pchglist_t *jpc_pchglist_create(void);

/* Destroy a progression change list. */
void jpc_pchglist_destroy(jpc_pchglist_t *pchglist);

/* Insert a new element into a progression change list. */
int jpc_pchglist_insert(jpc_pchglist_t *pchglist, int pchgno, jpc_pchg_t *pchg);

/* Remove an element from a progression change list. */
jpc_pchg_t *jpc_pchglist_remove(jpc_pchglist_t *pchglist, int pchgno);

/* Get an element from a progression change list. */
jpc_pchg_t *jpc_pchglist_get(jpc_pchglist_t *pchglist, int pchgno);

/* Copy a progression change list. */
jpc_pchglist_t *jpc_pchglist_copy(jpc_pchglist_t *pchglist);

/* Get the number of elements in a progression change list. */
int jpc_pchglist_numpchgs(jpc_pchglist_t *pchglist);

/******************************************************************************\
* Functions/macros for progression changes.
\******************************************************************************/

/* Destroy a progression change. */
void jpc_pchg_destroy(jpc_pchg_t *pchg);

/* Copy a progression change. */
jpc_pchg_t *jpc_pchg_copy(jpc_pchg_t *pchg);

#endif
