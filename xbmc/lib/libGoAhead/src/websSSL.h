/* 
 *	websSSL.h -- SSL Patch header
 *
 * Copyright (c) GoAhead Software Inc., 1992-2000. All Rights Reserved.
 *
 *	See the file "license.txt" for information on usage and redistribution
 *
 *	$Id: websSSL.h,v 1.3 2003/09/29 19:50:50 bporter Exp $
 */

#ifndef _h_websSSL
#define _h_websSSL 1

/******************************** Description *********************************/

/* 
 *	Header file for the GoAhead Patch for SSL. This defines the interface to 
 *	integrate SSL into the GoAhead Webserver.
 */

/********************************* Includes ***********************************/

#define OPENSSL
#ifdef OPENSSL
#define SSLEAY		/* turn off a few special case MONOLITH macros */
#define USE_SOCKETS /* needed for the _O_BINARY defs in the MS world */
#include <openssl/ssl.h>
#else
#include <sslc.h>
#endif

#ifndef UEMF
	#include	"basic/basic.h"
	#include	"emf/emf.h"
#else
	#include	"uemf.h"
#endif

/********************************** Defines ***********************************/
typedef struct {
	SSL	*ssl;
	BIO	*bio;
} websSSL_t;

/******************************** Prototypes **********************************/

extern int	websSSLOpen();
extern int	websSSLIsOpen();
extern void websSSLClose();

extern int	websSSLWrite(websSSL_t *wsp, char_t *buf, int nChars);
extern int	websSSLGets(websSSL_t *wsp, char_t **buf);
extern int	websSSLRead(websSSL_t *wsp, char_t *buf, int nChars);
extern int	websSSLEof(websSSL_t *wsp);

extern int	websSSLFree(websSSL_t *wsp);
extern int	websSSLFlush(websSSL_t *wsp);

extern int	websSSLSetKeyFile(char_t *keyFile);
extern int	websSSLSetCertFile(char_t *certFile);


#endif /* _h_websSSL */

/*****************************************************************************/

