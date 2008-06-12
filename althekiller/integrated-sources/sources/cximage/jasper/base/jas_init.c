/*
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

/******************************************************************************\
* Includes.
\******************************************************************************/

#include "jasper/jas_types.h"
#include "jasper/jas_image.h"
#include "jasper/jas_init.h"

/******************************************************************************\
* Code.
\******************************************************************************/

/* Initialize the image format table. */
int jas_init()
{
	jas_image_fmtops_t fmtops;
	int fmtid;

	fmtid = 0;

#if !defined(EXCLUDE_MIF_SUPPORT)
	fmtops.decode = mif_decode;
	fmtops.encode = mif_encode;
	fmtops.validate = mif_validate;
	jas_image_addfmt(fmtid, "mif", "mif", "My Image Format (MIF)", &fmtops);
	++fmtid;
#endif

#if !defined(EXCLUDE_PNM_SUPPORT)
	fmtops.decode = pnm_decode;
	fmtops.encode = pnm_encode;
	fmtops.validate = pnm_validate;
	jas_image_addfmt(fmtid, "pnm", "pnm", "Portable Graymap/Pixmap (PNM)",
	  &fmtops);
	jas_image_addfmt(fmtid, "pnm", "pgm", "Portable Graymap/Pixmap (PNM)",
	  &fmtops);
	jas_image_addfmt(fmtid, "pnm", "ppm", "Portable Graymap/Pixmap (PNM)",
	  &fmtops);
	++fmtid;
#endif

#if !defined(EXCLUDE_BMP_SUPPORT)
	fmtops.decode = bmp_decode;
	fmtops.encode = bmp_encode;
	fmtops.validate = bmp_validate;
	jas_image_addfmt(fmtid, "bmp", "bmp", "Microsoft Bitmap (BMP)", &fmtops);
	++fmtid;
#endif

#if !defined(EXCLUDE_RAS_SUPPORT)
	fmtops.decode = ras_decode;
	fmtops.encode = ras_encode;
	fmtops.validate = ras_validate;
	jas_image_addfmt(fmtid, "ras", "ras", "Sun Rasterfile (RAS)", &fmtops);
	++fmtid;
#endif

#if !defined(EXCLUDE_JP2_SUPPORT)
	fmtops.decode = jp2_decode;
	fmtops.encode = jp2_encode;
	fmtops.validate = jp2_validate;
	jas_image_addfmt(fmtid, "jp2", "jp2",
	  "JPEG-2000 JP2 File Format Syntax (ISO/IEC 15444-1)", &fmtops);
	++fmtid;
	fmtops.decode = jpc_decode;
	fmtops.encode = jpc_encode;
	fmtops.validate = jpc_validate;
	jas_image_addfmt(fmtid, "jpc", "jpc",
	  "JPEG-2000 Code Stream Syntax (ISO/IEC 15444-1)", &fmtops);
	++fmtid;
#endif

#if !defined(EXCLUDE_JPG_SUPPORT)
	fmtops.decode = jpg_decode;
	fmtops.encode = jpg_encode;
	fmtops.validate = jpg_validate;
	jas_image_addfmt(fmtid, "jpg", "jpg", "JPEG (ISO/IEC 10918-1)", &fmtops);
	++fmtid;
#endif

#if !defined(EXCLUDE_PGX_SUPPORT)
	fmtops.decode = pgx_decode;
	fmtops.encode = pgx_encode;
	fmtops.validate = pgx_validate;
	jas_image_addfmt(fmtid, "pgx", "pgx", "JPEG-2000 VM Format (PGX)", &fmtops);
	++fmtid;
#endif

	/* We must not register the JasPer library exit handler until after
	at least one memory allocation is performed.  This is desirable
	as it ensures that the JasPer exit handler is called before the
	debug memory allocator exit handler. */
	atexit(jas_cleanup);

	return 0;
}

void jas_cleanup()
{
	jas_image_clearfmts();
}
