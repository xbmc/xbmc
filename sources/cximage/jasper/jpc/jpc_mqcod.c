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
 * MQ Arithmetic Coder
 *
 * $Id$
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_malloc.h"

#include "jpc_mqcod.h"

/******************************************************************************\
* Data.
\******************************************************************************/

/* MQ coder per-state information. */

jpc_mqstate_t jpc_mqstates[47 * 2] = {
	{0x5601, 0, &jpc_mqstates[ 2], &jpc_mqstates[ 3]},
	{0x5601, 1, &jpc_mqstates[ 3], &jpc_mqstates[ 2]},
	{0x3401, 0, &jpc_mqstates[ 4], &jpc_mqstates[12]},
	{0x3401, 1, &jpc_mqstates[ 5], &jpc_mqstates[13]},
	{0x1801, 0, &jpc_mqstates[ 6], &jpc_mqstates[18]},
	{0x1801, 1, &jpc_mqstates[ 7], &jpc_mqstates[19]},
	{0x0ac1, 0, &jpc_mqstates[ 8], &jpc_mqstates[24]},
	{0x0ac1, 1, &jpc_mqstates[ 9], &jpc_mqstates[25]},
	{0x0521, 0, &jpc_mqstates[10], &jpc_mqstates[58]},
	{0x0521, 1, &jpc_mqstates[11], &jpc_mqstates[59]},
	{0x0221, 0, &jpc_mqstates[76], &jpc_mqstates[66]},
	{0x0221, 1, &jpc_mqstates[77], &jpc_mqstates[67]},
	{0x5601, 0, &jpc_mqstates[14], &jpc_mqstates[13]},
	{0x5601, 1, &jpc_mqstates[15], &jpc_mqstates[12]},
	{0x5401, 0, &jpc_mqstates[16], &jpc_mqstates[28]},
	{0x5401, 1, &jpc_mqstates[17], &jpc_mqstates[29]},
	{0x4801, 0, &jpc_mqstates[18], &jpc_mqstates[28]},
	{0x4801, 1, &jpc_mqstates[19], &jpc_mqstates[29]},
	{0x3801, 0, &jpc_mqstates[20], &jpc_mqstates[28]},
	{0x3801, 1, &jpc_mqstates[21], &jpc_mqstates[29]},
	{0x3001, 0, &jpc_mqstates[22], &jpc_mqstates[34]},
	{0x3001, 1, &jpc_mqstates[23], &jpc_mqstates[35]},
	{0x2401, 0, &jpc_mqstates[24], &jpc_mqstates[36]},
	{0x2401, 1, &jpc_mqstates[25], &jpc_mqstates[37]},
	{0x1c01, 0, &jpc_mqstates[26], &jpc_mqstates[40]},
	{0x1c01, 1, &jpc_mqstates[27], &jpc_mqstates[41]},
	{0x1601, 0, &jpc_mqstates[58], &jpc_mqstates[42]},
	{0x1601, 1, &jpc_mqstates[59], &jpc_mqstates[43]},
	{0x5601, 0, &jpc_mqstates[30], &jpc_mqstates[29]},
	{0x5601, 1, &jpc_mqstates[31], &jpc_mqstates[28]},
	{0x5401, 0, &jpc_mqstates[32], &jpc_mqstates[28]},
	{0x5401, 1, &jpc_mqstates[33], &jpc_mqstates[29]},
	{0x5101, 0, &jpc_mqstates[34], &jpc_mqstates[30]},
	{0x5101, 1, &jpc_mqstates[35], &jpc_mqstates[31]},
	{0x4801, 0, &jpc_mqstates[36], &jpc_mqstates[32]},
	{0x4801, 1, &jpc_mqstates[37], &jpc_mqstates[33]},
	{0x3801, 0, &jpc_mqstates[38], &jpc_mqstates[34]},
	{0x3801, 1, &jpc_mqstates[39], &jpc_mqstates[35]},
	{0x3401, 0, &jpc_mqstates[40], &jpc_mqstates[36]},
	{0x3401, 1, &jpc_mqstates[41], &jpc_mqstates[37]},
	{0x3001, 0, &jpc_mqstates[42], &jpc_mqstates[38]},
	{0x3001, 1, &jpc_mqstates[43], &jpc_mqstates[39]},
	{0x2801, 0, &jpc_mqstates[44], &jpc_mqstates[38]},
	{0x2801, 1, &jpc_mqstates[45], &jpc_mqstates[39]},
	{0x2401, 0, &jpc_mqstates[46], &jpc_mqstates[40]},
	{0x2401, 1, &jpc_mqstates[47], &jpc_mqstates[41]},
	{0x2201, 0, &jpc_mqstates[48], &jpc_mqstates[42]},
	{0x2201, 1, &jpc_mqstates[49], &jpc_mqstates[43]},
	{0x1c01, 0, &jpc_mqstates[50], &jpc_mqstates[44]},
	{0x1c01, 1, &jpc_mqstates[51], &jpc_mqstates[45]},
	{0x1801, 0, &jpc_mqstates[52], &jpc_mqstates[46]},
	{0x1801, 1, &jpc_mqstates[53], &jpc_mqstates[47]},
	{0x1601, 0, &jpc_mqstates[54], &jpc_mqstates[48]},
	{0x1601, 1, &jpc_mqstates[55], &jpc_mqstates[49]},
	{0x1401, 0, &jpc_mqstates[56], &jpc_mqstates[50]},
	{0x1401, 1, &jpc_mqstates[57], &jpc_mqstates[51]},
	{0x1201, 0, &jpc_mqstates[58], &jpc_mqstates[52]},
	{0x1201, 1, &jpc_mqstates[59], &jpc_mqstates[53]},
	{0x1101, 0, &jpc_mqstates[60], &jpc_mqstates[54]},
	{0x1101, 1, &jpc_mqstates[61], &jpc_mqstates[55]},
	{0x0ac1, 0, &jpc_mqstates[62], &jpc_mqstates[56]},
	{0x0ac1, 1, &jpc_mqstates[63], &jpc_mqstates[57]},
	{0x09c1, 0, &jpc_mqstates[64], &jpc_mqstates[58]},
	{0x09c1, 1, &jpc_mqstates[65], &jpc_mqstates[59]},
	{0x08a1, 0, &jpc_mqstates[66], &jpc_mqstates[60]},
	{0x08a1, 1, &jpc_mqstates[67], &jpc_mqstates[61]},
	{0x0521, 0, &jpc_mqstates[68], &jpc_mqstates[62]},
	{0x0521, 1, &jpc_mqstates[69], &jpc_mqstates[63]},
	{0x0441, 0, &jpc_mqstates[70], &jpc_mqstates[64]},
	{0x0441, 1, &jpc_mqstates[71], &jpc_mqstates[65]},
	{0x02a1, 0, &jpc_mqstates[72], &jpc_mqstates[66]},
	{0x02a1, 1, &jpc_mqstates[73], &jpc_mqstates[67]},
	{0x0221, 0, &jpc_mqstates[74], &jpc_mqstates[68]},
	{0x0221, 1, &jpc_mqstates[75], &jpc_mqstates[69]},
	{0x0141, 0, &jpc_mqstates[76], &jpc_mqstates[70]},
	{0x0141, 1, &jpc_mqstates[77], &jpc_mqstates[71]},
	{0x0111, 0, &jpc_mqstates[78], &jpc_mqstates[72]},
	{0x0111, 1, &jpc_mqstates[79], &jpc_mqstates[73]},
	{0x0085, 0, &jpc_mqstates[80], &jpc_mqstates[74]},
	{0x0085, 1, &jpc_mqstates[81], &jpc_mqstates[75]},
	{0x0049, 0, &jpc_mqstates[82], &jpc_mqstates[76]},
	{0x0049, 1, &jpc_mqstates[83], &jpc_mqstates[77]},
	{0x0025, 0, &jpc_mqstates[84], &jpc_mqstates[78]},
	{0x0025, 1, &jpc_mqstates[85], &jpc_mqstates[79]},
	{0x0015, 0, &jpc_mqstates[86], &jpc_mqstates[80]},
	{0x0015, 1, &jpc_mqstates[87], &jpc_mqstates[81]},
	{0x0009, 0, &jpc_mqstates[88], &jpc_mqstates[82]},
	{0x0009, 1, &jpc_mqstates[89], &jpc_mqstates[83]},
	{0x0005, 0, &jpc_mqstates[90], &jpc_mqstates[84]},
	{0x0005, 1, &jpc_mqstates[91], &jpc_mqstates[85]},
	{0x0001, 0, &jpc_mqstates[90], &jpc_mqstates[86]},
	{0x0001, 1, &jpc_mqstates[91], &jpc_mqstates[87]},
	{0x5601, 0, &jpc_mqstates[92], &jpc_mqstates[92]},
	{0x5601, 1, &jpc_mqstates[93], &jpc_mqstates[93]},
};
