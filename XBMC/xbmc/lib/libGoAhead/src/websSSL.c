/*
 * websSSL.c -- SSL envrionment creation
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 * $Id: websSSL.c,v 1.3 2003/09/29 20:30:48 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module implements a patch into SSL implementations for the webs
 *	module.
 */

#ifndef __ENABLE_MOCANA_SSL_SERVER__ 
/********************************* Includes ***********************************/

#include	"wsIntrn.h"
#include	"webs.h"
#include	"websSSL.h"

/******************************* Definitions **********************************/

#define DEFAULT_CERT_FILE	"./server.pem"
#define DEFAULT_KEY_FILE	"./certs/cakey.pem"
#define DEFAULT_CA_FILE		"./certs/cacert.pem"
#define DEFAULT_CA_PATH		"./certs/"
#define SSL_PORT			443

/*
 *	Define the components of the apps_startup() macro
 */

#ifdef SIGPIPE
#define do_pipe_sig()	signal(SIGPIPE,SIG_IGN)
#else
#define do_pipe_sig()
#endif

#ifdef OPENSSL
#define SSLC_add_all_algorithms()	SSLeay_add_all_algorithms()
#else
extern void SSLC_add_all_algorithms(void);
#endif

/*
 *	Define the apps_startup() macro
 */

#  if defined(MSDOS) || defined(WIN16) || defined(WIN32)
#    ifdef _O_BINARY
#      define apps_startup() \
		_fmode=_O_BINARY; do_pipe_sig(); CRYPTO_malloc_init(); \
		SSLC_add_all_algorithms()
#    else
#      define apps_startup() \
		_fmode=O_BINARY; do_pipe_sig(); CRYPTO_malloc_init(); \
		SSLC_add_all_algorithms()
#    endif
#  else
#    define apps_startup()	do_pipe_sig(); SSLC_add_all_algorithms();
#  endif

/*************************** Forward Declarations *****************************/

static int		websSSLSetCertStuff(SSL_CTX *ctx, 
									char *cert_file, 
									char *key_file);
static int		websSSLVerifyCallback(int ok, X509_STORE_CTX *ctx);
static RSA		*websSSLTempRSACallback(SSL *s, int is_export, int keylength);

static int		websSSLReadEvent (webs_t wp);
static int		websSSLAccept(int sid, char *ipaddr, int port, int listenSid);
static void		websSSLSocketEvent(int sid, int mask, int data);

/*********************************** Locals ***********************************/

static int		sslListenSock = -1;			/* Listen socket */
static SSL_CTX	*sslctx = NULL;

/******************************************************************************/
/*
 *	Start up the SSL Context for the application, and start a listen on the
 *	SSL port (usually 443, and defined by SSL_PORT)
 *	Return 0 on success, -1 on failure.
 */

int websSSLOpen()
{
	char		*certFile, *keyFile, *CApath, *CAfile;
	SSL_METHOD	*meth;
	
/*
 *	Install and initialize the SSL library
 */
	apps_startup();
	trace(7, T("SSL: Initializing SSL\n")); 

#ifdef SSLC
	SSL_library_init();
#endif

	SSL_load_error_strings();

#ifdef OPENSSL
	SSLeay_add_ssl_algorithms();
#endif

/*
 *	Important!  Enable both SSL versions 2 and 3
 */
	meth = SSLv23_server_method();
	sslctx = SSL_CTX_new(meth);

	a_assert(sslctx);

	if (sslctx == NULL) {
		trace(2, T("SSL: Unable to create SSL context!\n")); 
		return -1;
	}

/*
 *	Adjust some SSL Context variables
 */
	SSL_CTX_set_quiet_shutdown(sslctx, 1);
	SSL_CTX_set_options(sslctx, 0);
	SSL_CTX_sess_set_cache_size(sslctx, 128);

/*
 *	Set the certificate verification locations
 */
	CApath = DEFAULT_CA_PATH;
	CAfile = DEFAULT_CA_FILE;
	if ((!SSL_CTX_load_verify_locations(sslctx, CAfile, CApath)) ||
		(!SSL_CTX_set_default_verify_paths(sslctx))) {
		trace(2, T("SSL: Unable to set cert verification locations!\n")); 
		websSSLClose();
		return -1;
	}

/*
 *	Set the certificate and key files for the SSL context
 */
	certFile = DEFAULT_CERT_FILE;
	keyFile = NULL;
	if (websSSLSetCertStuff(sslctx, certFile, keyFile) != 0) {
		websSSLClose();
		return -1;
	}

/*
 *	Set the RSA callback for the SSL context
 */
	SSL_CTX_set_tmp_rsa_callback(sslctx, websSSLTempRSACallback);

/*
 *	Set the verification callback for the SSL context
 */
	SSL_CTX_set_verify(sslctx, SSL_VERIFY_NONE, websSSLVerifyCallback);

/*
 *	Set the certificate authority list for the client
 */
	SSL_CTX_set_client_CA_list(sslctx, SSL_load_client_CA_file(CAfile));

/*
 *	Open the socket
 */
	sslListenSock = socketOpenConnection(NULL, SSL_PORT, 
		websSSLAccept, SOCKET_BLOCK);

	if (sslListenSock < 0) {
		trace(2, T("SSL: Unable to open SSL socket on port <%d>!\n"), 
			SSL_PORT);
		return -1;
	}

	return 0;
}

/******************************************************************************/
/*
 *	Return TRUE if websSSL has been opened
 */

int websSSLIsOpen()
{
	return (sslListenSock != -1);
}

/******************************************************************************/
/*
 *	Stops the SSL
 */

void websSSLClose()
{
	trace(7, T("SSL: Closing SSL\n")); 

	if (sslctx != NULL) {
		SSL_CTX_free(sslctx);
		sslctx = NULL;
	}

	if (sslListenSock != -1) {
		socketCloseConnection(sslListenSock);
		sslListenSock = -1;
	}

#ifdef SSLC
	SSL_library_cleanup();
#endif
}

/******************************************************************************/
/*
 *	Accept a connection
 */

int websSSLAccept(int sid, char *ipaddr, int port, int listenSid)
{
	webs_t	wp;
	int		wid;

	a_assert(ipaddr && *ipaddr);
	a_assert(sid >= 0);
	a_assert(port >= 0);

/*
 *	Allocate a new handle for this accepted connection. This will allocate
 *	a webs_t structure in the webs[] list
 */
	if ((wid = websAlloc(sid)) < 0) {
		return -1;
	}
	wp = webs[wid];
	a_assert(wp);
	wp->listenSid = listenSid;

	ascToUni(wp->ipaddr, ipaddr, min(sizeof(wp->ipaddr), strlen(ipaddr)+1));

/*
 *	Check if this is a request from a browser on this system. This is useful
 *	to know for permitting administrative operations only for local access
 */
	if (gstrcmp(wp->ipaddr, T("127.0.0.1")) == 0 || 
			gstrcmp(wp->ipaddr, websIpaddr) == 0 || 
			gstrcmp(wp->ipaddr, websHost) == 0) {
		wp->flags |= WEBS_LOCAL_REQUEST;
	}
/*
 *	Since the acceptance came in on this channel, it must be secure
 */
	wp->flags |= WEBS_SECURE;

/*
 *	Arrange for websSocketEvent to be called when read data is available
 */
	socketCreateHandler(sid, SOCKET_READABLE, websSSLSocketEvent, (int) wp);

/*
 *	Arrange for a timeout to kill hung requests
 */
	wp->timeout = emfSchedCallback(WEBS_TIMEOUT, websTimeout, (void *) wp);
	trace(8, T("webs: accept request\n"));
	return 0;
}

/******************************************************************************/
/*
 *	The webs socket handler.  Called in response to I/O. We just pass control
 *	to the relevant read or write handler. A pointer to the webs structure
 *	is passed as an (int) in iwp.
 */

static void websSSLSocketEvent(int sid, int mask, int iwp)
{
	webs_t	wp;

	wp = (webs_t) iwp;
	a_assert(wp);

	if (! websValid(wp)) {
		return;
	}

	if (mask & SOCKET_READABLE) {
		websSSLReadEvent(wp);
	} 
	if (mask & SOCKET_WRITABLE) {
		if (wp->writeSocket) {
			(*wp->writeSocket)(wp);
		}
	} 
}

/******************************************************************************/
/*
 *	Handler for SSL Read Events
 */

static int websSSLReadEvent (webs_t wp)
{
	int			ret, sock;
	socket_t	*sptr;
	SSL			*ssl;
	BIO			*bio, *bioSSL, *bioSock;
#ifdef DEV
	const char	*ciphers;
#endif

	a_assert (wp);
	a_assert(websValid(wp));

	sptr = socketPtr(wp->sid);
	a_assert(sptr);

	sock = sptr->sock;

/*
 *	Create a new BIO and SSL session for this web request
 */
	bio = BIO_new(BIO_f_buffer());
	a_assert(bio);

	if (!BIO_set_write_buffer_size(bio, 128)) {
		return -1;
	}

	ssl = (SSL *) SSL_new(sslctx);
	a_assert(ssl);

	if (ssl == NULL) {
		return -1;
	}

	SSL_set_session(ssl, NULL);

	bioSSL =  BIO_new(BIO_f_ssl());
	a_assert(bioSSL);

	bioSock = BIO_new_socket(sock, BIO_NOCLOSE);
	a_assert(bioSock);

	SSL_set_bio(ssl, bioSock, bioSock);
	SSL_set_accept_state(ssl);

	ret = BIO_set_ssl(bioSSL, ssl, BIO_CLOSE);
	BIO_push(bio, bioSSL);

#ifdef DEV
	ciphers = SSL_get_cipher_list(ssl, 10);
#endif

/*
 *	Create the SSL data structure in the wp.
 */
#ifdef WEBS_SSL_SUPPORT
	wp->wsp = balloc(B_L, sizeof(websSSL_t));
	a_assert (wp->wsp);
	(wp->wsp)->bio = bio;
	(wp->wsp)->ssl = ssl;
#endif

/*
 *	Call the default Read Event
 */
	websReadEvent(wp);

	return ret;
}


/******************************************************************************/
/*
 *	SSL Verification Callback
 */

static int sslVerifyDepth = 0;
static int sslVerifyError = X509_V_OK;

int websSSLVerifyCallback(int ok, X509_STORE_CTX *ctx)
{
	char	buf[256];
	X509	*errCert;
	int		err;
	int		depth;

	errCert =	X509_STORE_CTX_get_current_cert(ctx);
	err =		X509_STORE_CTX_get_error(ctx);
	depth =		X509_STORE_CTX_get_error_depth(ctx);

	X509_NAME_oneline(X509_get_subject_name(errCert), buf, 256);

	if (!ok) {
		if (sslVerifyDepth >= depth)	{
			ok = 1;
			sslVerifyError = X509_V_OK;
		} else {
			ok=0;
			sslVerifyError = X509_V_ERR_CERT_CHAIN_TOO_LONG;
		}
	}

	switch (err)	{
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
#ifdef OPENSSL
		X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
#endif
		break;

	case X509_V_ERR_CERT_NOT_YET_VALID:
	case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	case X509_V_ERR_CERT_HAS_EXPIRED:
	case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		break;
	}

	return ok;
}

/******************************************************************************/
/*
 *	Set the SSL certificate and key for the SSL context
 */

int websSSLSetCertStuff(SSL_CTX *ctx, char *certFile, char *keyFile)
{
	a_assert (ctx);
	a_assert (certFile);

	if (certFile != NULL) {
		if (SSL_CTX_use_certificate_file(ctx, certFile, 
			SSL_FILETYPE_PEM) <= 0) {
			trace(2, T("SSL: Unable to set certificate file <%s>\n"),
				certFile); 
			return -1;
		}

		if (keyFile == NULL) {
			keyFile = certFile;
		}

		if (SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM) <= 0) {
			trace(2, T("SSL: Unable to set private key file <%s>\n"),
				keyFile); 
			return -1;
		}

/*		
 *		Now we know that a key and cert have been set against
 *		the SSL context 
 */
		if (!SSL_CTX_check_private_key(ctx)) {
			trace(2, T("SSL: Check of private key file <%s> FAILED!\n"),
				keyFile); 
			return -1;
		}
	}

	return 0;
}

/******************************************************************************/
/*
 *	Set certificate file for SSL context
 */

int websSSLSetCertFile(char_t *certFile)
{
	a_assert (sslctx);
	a_assert (certFile);

	if (sslctx == NULL) {
		return -1;
	}

	if (SSL_CTX_use_certificate_file(sslctx, certFile, 
		SSL_FILETYPE_PEM) <= 0) {
		return -1;
	}
/*		
 *	Confirm that the certificate and the private key jive.
 */
	if (!SSL_CTX_check_private_key(sslctx)) {
		return -1;
	}

	return 0;
}

/******************************************************************************/
/*
 *	Set key file for SSL context
 */

int websSSLSetKeyFile(char_t *keyFile)
{
	a_assert (sslctx);
	a_assert (keyFile);

	if (sslctx == NULL) {
		return -1;
	}

	if (SSL_CTX_use_PrivateKey_file(sslctx, keyFile, SSL_FILETYPE_PEM) <= 0) {
		return -1;
	}
/*		
 *	Confirm that the certificate and the private key jive.
 */
	if (!SSL_CTX_check_private_key(sslctx)) {
		return -1;
	}

	return 0;
}

#ifdef SSLC
extern RSA *RSA_new(void);
#endif

/******************************************************************************/
/*
 *	the Temporary RSA callback
 */

static RSA *websSSLTempRSACallback(SSL *ssl, int isExport, int keyLength)
{
	static RSA *rsaTemp = NULL;

	if (rsaTemp == NULL) {

#ifdef OPENSSL
		rsaTemp = RSA_generate_key(keyLength, RSA_F4, NULL, NULL);
#endif

#ifdef SSLC
		rsaTemp = RSA_new();
#endif

	}

	return rsaTemp;
}

/******************************************************************************/
/*
 *	Free SSL resources 
 */

int websSSLFree(websSSL_t *wsp)
{
	if (wsp == NULL) {
		return -1;
	}

/* 
 *	Make sure we re-use sessions
 */
	if (wsp->ssl != NULL) {
		SSL_set_shutdown(wsp->ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
	}

	if (wsp->bio != NULL) {
		BIO_free_all(wsp->bio);
	}

	bfree(B_L, wsp);

	return 0;
}

/******************************************************************************/
/*
 *	Return Eof for the SSL BIO
 */

int websSSLEof(websSSL_t *wsp)
{
	a_assert(wsp);

	if ((wsp == NULL) || (wsp->bio == NULL)) {
		return -1;
	} 

	return BIO_eof(wsp->bio);
}

/******************************************************************************/
/*
 *	Perform a read of the SSL BIO
 */

int websSSLRead(websSSL_t *wsp, char_t *buf, int len)
{
	a_assert(wsp);
	a_assert(buf);

	if ((wsp == NULL) || (wsp->bio == NULL)) {
		return -1;
	} 

	return BIO_read(wsp->bio, buf, len);
}

/******************************************************************************/
/*
 *	Perform a gets of the SSL BIO, returning an balloc'ed string
 */

#define BUF_BLOCK 256

int websSSLGets(websSSL_t *wsp, char_t **buf)
{
	int		rc,	len, lenBuf;
	char	c;

	a_assert(wsp);
	a_assert(buf);

	lenBuf = 0;
	len = 0;

	if ((wsp == NULL) || (wsp->bio == NULL)) {
		return -1;
	} 

	while (1) {

		if ((rc = BIO_read(wsp->bio, &c, 1)) < 0) {
			return rc;
		}
		
		if (rc == 0) {
/*
 *			If there is a partial line and we are at EOF, pretend we saw a '\n'
 */
			if (len > 0 && BIO_eof(wsp->bio)) {
				c = '\n';
			} else {
				return -1;
			}
		}
/*
 *		If a newline is seen, return the data excluding the new line to the
 *		caller. If carriage return is seen, just eat it.
 */
		if (c == '\n') {
			if ((len > 0) && (len < lenBuf)) {
				(*buf)[len] = 0;
			}
			return len;
		} else if (c == '\r') {
			continue;
		} 
/*
 *		Append character to buf
 */
		if (len >= lenBuf) {
			lenBuf += BUF_BLOCK;
			*buf = brealloc(B_L, *buf, lenBuf);
		}

		a_assert(*buf);
		(*buf)[len] = c;
		len++;
	}
}

/******************************************************************************/
/*
 *	Perform a write to the SSL BIO
 */

int websSSLWrite(websSSL_t *wsp, char_t *buf, int len)
{
	a_assert(wsp);
	a_assert(buf);

	if ((wsp == NULL) || (wsp->bio == NULL)) {
		return -1;
	} 

	return BIO_write(wsp->bio, buf, len);
}

/******************************************************************************/
/*
 *	Perform a flush of the SSL BIO
 */

int websSSLFlush(websSSL_t *wsp)
{
	a_assert(wsp);

	if ((wsp == NULL) || (wsp->bio == NULL)) {
		return -1;
	} 

	return BIO_flush(wsp->bio);
}

/******************************************************************************/

#endif
