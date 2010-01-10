/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: version_lnx.h
 *
 *  Description: Version numbering for the driver use.
 *
 *  AU
 *
 *  HISTORY:
 *
 ********************************************************************
 * This header is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License.
 *
 * This header is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************/

#ifndef _BC_DTS_VERSION_LNX_
#define _BC_DTS_VERSION_LNX_
//
// The version format that we are adopting is
// MajorVersion.MinorVersion.Revision
// This will be the same for all the components.
//
//
#define STRINGIFY_VERSION(MAJ,MIN,REV) STRINGIFIED_VERSION(MAJ,MIN,REV)
#define STRINGIFIED_VERSION(MAJ,MIN,REV) #MAJ "." #MIN "." #REV

#define STRINGIFY_VERSION_W(MAJ,MIN,REV) STRINGIFIED_VERSION_W(MAJ,MIN,REV)
#define STRINGIFIED_VERSION_W(MAJ,MIN,REV) #MAJ "." #MIN "." #REV

//
//  Product Version number is:
//  x.y.z.a
//
//  x = Major release.      1 = Dozer, 2 = Dozer + Link
//  y = Minor release.      Should increase +1 per "real" release.
//  z = Branch release.     0 for main branch.  This is +1 per branch release.
//  a = Build number	+1 per candidate release.  Reset to 0 every "real" release.
//
//
// Enabling Check-In rules enforcement 08092007
//
#define INVALID_VERSION		0xFFFF

/*========================== Common For All Components =================================*/
#define RC_COMPANY_NAME		"Broadcom Corporation\0"
#define RC_PRODUCT_VERSION	"2.7.0.23"
#define RC_W_PRODUCT_VERSION	L"2.7.0.23"
#define RC_PRODUCT_NAME		"Broadcom Crystal HD Video Decoder\0"
#define RC_COMMENTS		"Broadcom BCM70012 Controller\0"
#define RC_COPYRIGHT		"Copyright(c) 2007-2009 Broadcom Corporation"
#define RC_PRIVATE_BUILD	"Broadcom Corp. Private\0"
#define RC_LEGAL_TRADEMARKS	" \0"
#define BRCM_MAJOR_VERSION	0


/*======================= Device Interface Library ========================*/
#define DIL_MAJOR_VERSION	BRCM_MAJOR_VERSION
#define DIL_MINOR_VERSION	9
#define DIL_REVISION		25

#define DIL_RC_FILE_VERSION	STRINGIFY_VERSION(DIL_MAJOR_VERSION,DIL_MINOR_VERSION,DIL_REVISION)

/*========================== deconf utility ==============================*/
#define DECONF_MAJOR_VERSION	BRCM_MAJOR_VERSION
#define DECONF_MINOR_VERSION	9
#define DECONF_REVISION		17
#define DECONF_RC_FILE_VERSION  STRINGIFY_VERSION(DIL_MAJOR_VERSION,DIL_MINOR_VERSION,DIL_REVISION)


#endif
