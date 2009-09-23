/* 
 *	websda.h -- GoAhead Digest Access Authentication public header
 *
 * Copyright (c) GoAhead Software Inc., 1992-2000. All Rights Reserved.
 *
 *	See the file "license.txt" for information on usage and redistribution
 *
 * $Id: websda.h,v 1.3 2002/10/24 14:44:50 bporter Exp $
 */

#ifndef _h_WEBSDA
#define _h_WEBSDA 1

/******************************** Description *********************************/

/* 
 *	GoAhead Digest Access Authentication header. This defines the Digest 
 *	access authentication public APIs.  Include this header for files that 
 *	use DAA functions
 */

/********************************* Includes ***********************************/

#ifndef UEMF
	#include	"basic/basic.h"
	#include	"emf/emf.h"
#else
	#include	"uemf.h"
#endif

#include		"webs.h"

/****************************** Definitions ***********************************/

extern char_t 	*websCalcNonce(webs_t wp);
extern char_t 	*websCalcOpaque(webs_t wp);
extern char_t 	*websCalcDigest(webs_t wp);

#endif /* _h_WEBSDA */

/******************************************************************************/

