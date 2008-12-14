///
///	@file 	openSslModule.cpp
/// @brief 	Support for secure sockets via OpenSSL
///	@overview This module integrates support for OpenSSL into Appweb 
///		via a dynamically loadable module.
//
/////////////////////////////////// Copyright //////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"openSslModule.h"

//////////////////////////////////// Locals ////////////////////////////////////
#if BLD_FEATURE_OPENSSL_MODULE

#include	"openSslDh.h"

#if BLD_FEATURE_MULTITHREAD
static MprMutex	**locks;
static int		numLocks;
#endif

////////////////////////////// Forward Declarations ////////////////////////////

#if BLD_FEATURE_MULTITHREAD
static DynLock	*sslCreateDynLock(const char *file, int line);
static void 	sslDynLock(int mode, DynLock *dl, const char *file, int line);
static void 	sslDestroyDynLock(DynLock *dl, const char *file, int line);
static void		sslStaticLock(int mode, int n, const char *file, int line);
static ulong 	sslThreadId();
#endif

static int		configureCertificates(SSL_CTX *ctx, char *key, char *cert);
static RSA		*rsaCallback(SSL *ssl, int isExport, int keyLength);
static DH		*dhCallback(SSL *ssl, int isExport, int keyLength);
static int 		verifyX509Certificate(int ok, X509_STORE_CTX *ctx);

//////////////////////////////////// Locals ////////////////////////////////////

static MaOpenSslModule *openSslModule;
static MaOpenSslProvider *openSslProvider;

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaOpenSslModule ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprOpenSslInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaOpenSslModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaOpenSslModule::MaOpenSslModule(void *handle) : MaModule("openSsl", handle)
{
	openSslModule = this;
	openSslProvider = new MaOpenSslProvider("openSsl");
}

////////////////////////////////////////////////////////////////////////////////

MaOpenSslModule::~MaOpenSslModule()
{
	delete openSslProvider;
}

////////////////////////////////////////////////////////////////////////////////
//
//	This is called from Http::start after all the modules are loaded.
//	The code here is global for this handler and is not required per handler
//	instance.
//
int MaOpenSslModule::start()
{
	RandBuf		randBuf;
	static int	once = 0;

	//
	//	Depending on the order in the configuration file, we will get called 
	//	by the module mechanism and by OpenSslConfig::start(). But we only 
	//	want to run once.
	//
	if (once++ > 0) {
		return 0;
	}
	randBuf.pid = getpid();
	randBuf.time = (int) time(0);
	randBuf.msec = (int) mprGetTime(0);
	RAND_seed((void*) &randBuf, sizeof(randBuf));

#if SOLARIS || LINUX || MACOSX || FREEBSD
	mprLog(6, "OpenSsl: Before calling RAND_load_file\n");
	RAND_load_file("/dev/urandom", 256);
	mprLog(6, "OpenSsl: After calling RAND_load_file\n");
#endif

#if BLD_FEATURE_MULTITHREAD
	numLocks = CRYPTO_num_locks();
	locks = (MprMutex**) mprMalloc(numLocks * sizeof(MprMutex*));
	for (int i = 0; i < numLocks; i++) {
		locks[i] = new MprMutex();
	}
	CRYPTO_set_id_callback(sslThreadId);
	CRYPTO_set_locking_callback(sslStaticLock);

	CRYPTO_set_dynlock_create_callback(sslCreateDynLock);
	CRYPTO_set_dynlock_destroy_callback(sslDestroyDynLock);
	CRYPTO_set_dynlock_lock_callback(sslDynLock);
#endif

#if WIN
	// _fmode=_O_BINARY;
	//	CRYPTO_malloc_init();
	//	SSLC_add_all_algorithms();
#else
	OpenSSL_add_all_algorithms();
#endif

	SSL_library_init();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaOpenSslModule::stop()
{
#if BLD_FEATURE_MULTITHREAD
	int		i;

	if (locks) {
		for (i = 0; i < numLocks; i++) {
			delete locks[i];
		}
		mprFree(locks);
		locks = 0;
		numLocks = 0;

		CRYPTO_set_id_callback(0);
		CRYPTO_set_locking_callback(0);

		CRYPTO_set_dynlock_create_callback(0);
		CRYPTO_set_dynlock_destroy_callback(0);
		CRYPTO_set_dynlock_lock_callback(0);
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaOpenSslProvider //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	The SSL provider class implements and decouples the interface between 
//	Appweb and any SSL stack.
//

MaOpenSslProvider::MaOpenSslProvider(char *name) : MaSslProvider(name)
{
}

////////////////////////////////////////////////////////////////////////////////

MaOpenSslProvider::~MaOpenSslProvider()
{
}

////////////////////////////////////////////////////////////////////////////////

MaSslConfig *MaOpenSslProvider::newConfig(MaHost *host)
{
	return new MaOpenSslConfig(host);
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaOpenSslConfig ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// An instance is created for each SSL server
//

MaOpenSslConfig::MaOpenSslConfig(MaHost *host) : MaSslConfig(host)
{
	context = 0;
	dhKey512 = 0;
	dhKey1024 = 0;
	rsaKey512 = 0;
	rsaKey1024 = 0;
}

////////////////////////////////////////////////////////////////////////////////

MaOpenSslConfig::~MaOpenSslConfig()
{
	if (context != 0) {
		SSL_CTX_free(context);
	}
	if (rsaKey512) {
		RSA_free(rsaKey512);
	}
	if (rsaKey1024) {
		RSA_free(rsaKey1024);
	}
	if (dhKey512) {
		DH_free(dhKey512);
	}
	if (dhKey1024) {
		DH_free(dhKey1024);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	This method is called to open listening sockets using this SslConfig setup. 
//

MprSocket *MaOpenSslConfig::newSocket()
{
	MaOpenSslSocket	*socket;

	socket = new MaOpenSslSocket(this);

	return (MprSocket*) socket;
}

////////////////////////////////////////////////////////////////////////////////
//
//	This is started for each SSL connection. With Keep-alive this may service
//	many requests.
//

int MaOpenSslConfig::start()
{
	SSL_METHOD	*meth;
	char		*hostName;

	if (keyFile == 0) {
		mprError(MPR_L, MPR_LOG, "OpenSSL: Cant start SSL: missing key file");
		return MPR_ERR_CANT_INITIALIZE;
	}
	if (certFile == 0) {
		mprError(MPR_L, MPR_LOG, "OpenSSL: Cant start SSL: missing certificate file");
		return MPR_ERR_CANT_INITIALIZE;
	}

	//
	//	Depending on the order in the configuration file, we may get called 
	//	by sslModule::start() before OpenSslModule::start has run. So we 
	//	must initialize here.
	//
	openSslModule->start();
	
	hostName = host->getName();
	
	if (protocols == MPR_HTTP_PROTO_SSLV2) {
		meth = SSLv2_server_method();
	} else {
		meth = SSLv23_server_method();
	}
	context = SSL_CTX_new(meth);
	mprAssert(context);
	if (context == 0) {
		mprError(MPR_L, MPR_LOG, "OpenSSL: Unable to create SSL context"); 
		return MPR_ERR_CANT_CREATE;
	}

    SSL_CTX_set_app_data(context, (void*) this);
	SSL_CTX_set_quiet_shutdown(context, 1);
	SSL_CTX_sess_set_cache_size(context, 512);

	//
	//	Configure the certificate for this host
	//
	if (configureCertificates(context, keyFile, certFile) != 0) {
		SSL_CTX_free(context);
		context = 0;
		return MPR_ERR_CANT_INITIALIZE;
	}

	mprLog(4, "SSL: %s: Using ciphers %s\n", hostName, ciphers);
	SSL_CTX_set_cipher_list(context, ciphers);

	//
	//	Configure the client verification certificate locations
	//
	if (verifyClient) {
		if (caFile == 0 && caPath == 0) {
			mprError(MPR_L, MPR_LOG, 
				"OpenSSL: Must define CA certificates if using client verification");
			SSL_CTX_free(context);
			context = 0;
			return MPR_ERR_BAD_STATE;
		}
		if (caFile || caPath) {
			if ((!SSL_CTX_load_verify_locations(context, caFile, caPath)) ||
					(!SSL_CTX_set_default_verify_paths(context))) {
				mprError(MPR_L, MPR_LOG, 
					"OpenSSL: Unable to set certificate locations"); 
				SSL_CTX_free(context);
				context = 0;
				return MPR_ERR_CANT_ACCESS;
			}
			if (caFile) {
				STACK_OF(X509_NAME) *certNames;
				certNames = SSL_load_client_CA_file(caFile);
				if (certNames == 0) {
				} else {
					//
					//	Define the list of CA certificates to send to the client
					//	before they send their client certificate for validation
					//
					SSL_CTX_set_client_CA_list(context, certNames);
				}
			}
		}
		mprLog(4, "SSL: %s: is verifying client connections\n", hostName);
		if (caFile) {
			mprLog(4, "SSL: %s: Using certificates from %s\n", hostName, 
				caFile);
		} else if (caPath) {
			mprLog(4, "SSL: %s: Using certificates from directory %s\n", 
				hostName, caPath);
		}
		SSL_CTX_set_verify(context, 
			SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 
			verifyX509Certificate);
		SSL_CTX_set_verify_depth(context, verifyDepth);

	} else {
		SSL_CTX_set_verify(context, SSL_VERIFY_NONE, verifyX509Certificate);
	}

	//
	//	Define callbacks
	//
	SSL_CTX_set_tmp_rsa_callback(context, rsaCallback);
	SSL_CTX_set_tmp_dh_callback(context, dhCallback);

	//
	//	Enable all buggy client work-arounds 
	//
	SSL_CTX_set_options(context, SSL_OP_ALL);

	//
	//	Select the required protocols
	//
	if (!(protocols & MPR_HTTP_PROTO_SSLV2)) {
		SSL_CTX_set_options(context, SSL_OP_NO_SSLv2);
		mprLog(4, "SSL: %s: Disabling SSLv2\n", hostName);
	}
	if (!(protocols & MPR_HTTP_PROTO_SSLV3)) {
		SSL_CTX_set_options(context, SSL_OP_NO_SSLv3);
		mprLog(4, "SSL: %s: Disabling SSLv3\n", hostName);
	}
	if (!(protocols & MPR_HTTP_PROTO_TLSV1)) {
		SSL_CTX_set_options(context, SSL_OP_NO_TLSv1);
		mprLog(4, "SSL: %s: Disabling TLSv1\n", hostName);
	}

	//
	//	Ensure we generate a new private key for each connection
	//
    SSL_CTX_set_options(context, SSL_OP_SINGLE_DH_USE);

	//
	//	Pre-generate some keys that are slow to compute
	//
	rsaKey512 = RSA_generate_key(512, RSA_F4, 0, 0);
	rsaKey1024 = RSA_generate_key(1024, RSA_F4, 0, 0);

	dhKey512 = get_dh512();
	dhKey1024 = get_dh1024();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaOpenSslConfig::stop()
{
}

////////////////////////////////////////////////////////////////////////////////

SSL_CTX *MaOpenSslConfig::getContext()
{
	return context;
}

////////////////////////////////////////////////////////////////////////////////

///
///	Called to verify X509 client certificates
///
static int verifyX509Certificate(int ok, X509_STORE_CTX *xContext)
{
	X509			*cert;
	SSL				*ssl;
	MaOpenSslSocket	*sslSocket;
	MaOpenSslConfig	*config;
	char			subject[260], issuer[260], peer[260];
	int				error, depth;

	subject[0] = issuer[0] = '\0';

	ssl = (SSL*) X509_STORE_CTX_get_app_data(xContext);
	sslSocket = (MaOpenSslSocket*) SSL_get_app_data(ssl);
	config = (MaOpenSslConfig*) sslSocket->getConfig();

	cert = X509_STORE_CTX_get_current_cert(xContext);
	depth =	X509_STORE_CTX_get_error_depth(xContext);
	error = X509_STORE_CTX_get_error(xContext);

	if (X509_NAME_oneline(X509_get_subject_name(cert), subject, 
			sizeof(subject) - 1) < 0) {
		ok = 0;
	}
	//
	//	FUTURE -- should compare subject name and host name. Need smart compare
	//
	if (X509_NAME_oneline(X509_get_issuer_name(xContext->current_cert), issuer, 
			sizeof(issuer) - 1) < 0) {
		ok = 0;
	}
	if (X509_NAME_get_text_by_NID(X509_get_subject_name(xContext->current_cert),
		 	NID_commonName, peer, sizeof(peer) - 1) < 0) {
		ok = 0;
	}

	//
	//	Customizers: add your own code here to validate client certificates
	//
	if (ok && config->verifyDepth < depth) {
		if (error == 0) {
			error = X509_V_ERR_CERT_CHAIN_TOO_LONG;
		}
		ok = 0;
	}

	if (error != 0) {
		mprAssert(!ok);
	}

#if UNUSED
	switch (error) {
	case X509_V_ERR_CERT_HAS_EXPIRED:
	case X509_V_ERR_CERT_NOT_YET_VALID:
	case X509_V_ERR_CERT_REJECTED:
	case X509_V_ERR_CERT_SIGNATURE_FAILURE:
	case X509_V_ERR_CERT_UNTRUSTED:
	case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
	case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	case X509_V_ERR_INVALID_CA:
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
	default:
		ok = 0;
		break;
	}
#endif

	if (!ok) {
		mprLog(0, "SSL: Certification failed: subject %s\n", subject);
		mprLog(4, "SSL: Issuer: %s\n", issuer);
		mprLog(4, "SSL: Peer: %s\n", peer);
		mprLog(4, "SSL: Error: %d: %s\n", error, 
			X509_verify_cert_error_string(error));

	} else {
		mprLog(0, "SSL: Certificate verified: subject %s\n", subject);
		mprLog(4, "SSL: Issuer: %s\n", issuer);
		mprLog(4, "SSL: Peer: %s\n", peer);
	}
	return ok;
}

////////////////////////////////////////////////////////////////////////////////

///
///	Configure the SSL certificate information
///

static int configureCertificates(SSL_CTX *ctx, char *key, char *cert)
{
	mprAssert(ctx);
	mprAssert(cert);

	if (cert == 0) {
		return 0;
	}

	if (SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0){
		mprError(MPR_L, MPR_LOG, "OpenSSL: Can't define certificate file: %s", cert); 
		return -1;
	}

	key = (key == 0) ? cert : key;
	if (SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
		mprError(MPR_L, MPR_LOG, "OpenSSL: Can't define private key file: %s", key); 
		return -1;
	}

	if (!SSL_CTX_check_private_key(ctx)) {
		mprError(MPR_L, MPR_LOG, "OpenSSL: Check of private key file failed: %s", key);
		return -1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaOpenSslSocket ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaOpenSslSocket::MaOpenSslSocket(MaOpenSslConfig *config) : MaSslSocket(config)
{
	ssl = 0;
	bio = 0;
	context = config->getContext();
}

////////////////////////////////////////////////////////////////////////////////

MaOpenSslSocket::~MaOpenSslSocket()
{
	mprAssert(ssl == 0);
	mprAssert(bio == 0);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Called to accept new connections. When a new connection arrives, this
//	method is called to create a new socket object using the same SSL "config"
//	as the listening socket.
//

MprSocket *MaOpenSslSocket::newSocket()
{
	MaOpenSslSocket	*socket;

	socket = new MaOpenSslSocket((MaOpenSslConfig*) config);

	return (MprSocket*) socket;
}

////////////////////////////////////////////////////////////////////////////////
//
//	initConnection is called on the first I/O on a new connection. It must
//	setup the SSL structures ready to exchange data.
//
//	Init connection must return -1 on errors. It must also be able to reject
//	subsequent calls to read()/write(). On errors we leave bio == 0. This 
//	will cause any calls to read() to error.
//

int MaOpenSslSocket::initConnection()
{
	BIO		*bioSSL, *bioSock;

	if (bio) {
		return 0;
	}
	bio = BIO_new(BIO_f_buffer());
	if (bio == 0) {
		return MPR_ERR_CANT_INITIALIZE;
	}

	BIO_set_write_buffer_size(bio, 128);
	ssl = (SSL*) SSL_new(context);
	mprAssert(ssl);
	if (ssl == 0) {
		return MPR_ERR_CANT_INITIALIZE;
	}
    SSL_set_app_data(ssl, (void*) this);

	SSL_set_session(ssl, 0);
	bioSSL = BIO_new(BIO_f_ssl());
	mprAssert(bioSSL);

	bioSock = BIO_new_socket(sock, BIO_NOCLOSE);
	mprAssert(bioSock);

	SSL_set_bio(ssl, bioSock, bioSock);
	SSL_set_accept_state(ssl);

	BIO_set_ssl(bioSSL, ssl, BIO_CLOSE);
	BIO_push(bio, bioSSL);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MaOpenSslSocket::dispose()
{
	if (ssl) {
		SSL_set_shutdown(ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
		ssl = 0;
	}
	if (bio) {
		BIO_free_all(bio);
		bio = 0;
	}

	//
	//	Disposing a socket is a thread-safe way of calling a destructor. It
	//	uses reference counting and will be deleted when the last reference
	//	is released.
	//
	return this->MprSocket::dispose();
}

////////////////////////////////////////////////////////////////////////////////

void MaOpenSslSocket::close(int how)
{
	(void) BIO_flush(bio);
	this->MprSocket::close(how);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Write data. Return the number of bytes written or -1 on errors.
//

int	MaOpenSslSocket::write(char *buf, int len)
{
	int		rc, written, totalWritten;

	if (bio == 0 || ssl == 0 || len <= 0) {
		return -1;
	}

	BIO_clear_retry_flags(bio);

	totalWritten = 0;

	do {
		written = BIO_write(bio, buf, len);

		mprLog(7, "written %d, len %d\n", written, len);

		if (written >= 0) {
			do {
				rc = BIO_flush(bio);
				mprLog(7, "BIO_flush rc %d\n", rc);
				if (rc > 0) {
					//	Success
					break;
				}
				//
				//	Nap to prevent busy waiting.
				//
				mprSleep(10);
			} while (rc <= 0 && BIO_should_retry(bio));

			totalWritten += written;
			buf += written;
			len -= written;
		}

		mprLog(7, "write: len %d, written %d, total %d, should_retry %d\n",
			len, written, totalWritten, BIO_should_retry(bio));

		/*
 		 *	If written is < 0 and an error occurred, then BIO_should_retry
 		 *	will return false.
		 */
	} while (len > 0 && (written > 0 || BIO_should_retry(bio)));

	if (totalWritten <= 0 && BIO_should_retry(bio) == 0) {
		mprLog(7, "connection closed\n");
		/* 
		 *	If totalWritten is not Zero, then we don't return an Error.
		 * 	Next time this function is called, it will return an Error.
		 */
		return -1; // return Error ( connection closed )
	}
	return totalWritten;
}

////////////////////////////////////////////////////////////////////////////////

int MaOpenSslSocket::flush()
{
	return BIO_flush(bio);
}

////////////////////////////////////////////////////////////////////////////////
 
int	MaOpenSslSocket::read(char *buf, int len)
{
	int		rc;

	if (bio == 0 || ssl == 0) {
		return -1;
	}

	rc = BIO_read(bio, buf, len);

#if DEBUG
	if (rc > 0 && !connTraced) {
		X509_NAME	*xSubject;
		X509		*cert;
		char		subject[260], issuer[260], peer[260];

		mprLog(4, "%d: SSL Connected using: \"%s\"\n", 
			sock, SSL_get_cipher(ssl));

		cert = SSL_get_peer_certificate(ssl);
		if (cert == 0) {
			mprLog(4, "%d: SSL Details: client supplied no certificate\n", 
				sock);
		} else {
			xSubject = X509_get_subject_name(cert);
			X509_NAME_oneline(xSubject, subject, sizeof(subject) -1);
			X509_NAME_oneline(X509_get_issuer_name(cert), issuer, 
				sizeof(issuer) -1);
			X509_NAME_get_text_by_NID(xSubject, NID_commonName, peer, 
				sizeof(peer) - 1);
			mprLog(4, "%d: SSL Subject %s\n", sock, subject);
			mprLog(4, "%d: SSL Issuer: %s\n", sock, issuer);
			mprLog(4, "%d: SSL Peer: %s\n", sock, peer);
			X509_free(cert);
		}
		connTraced = 1;
	}
#endif

	if (rc > 0) {
		return rc;

	} else if (rc == 0) {
		if (BIO_should_retry(bio)) {
			return 0;
		}
		flags |= MPR_SOCKET_EOF;
		return 0;
	}
	if (BIO_should_retry(bio)) {
		return 0;
	}
	return rc;
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED
//
//	Return true if end of file
//

bool MaOpenSslSocket::getEof()
{
	bool	rc;

	rc = (BIO_eof(bio) != 0);
	return rc;
}

#endif

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_MULTITHREAD

static ulong sslThreadId()
{
	return (long) mprGetCurrentThread();
}

////////////////////////////////////////////////////////////////////////////////

static void sslStaticLock(int mode, int n, const char *file, int line)
{
	mprAssert(0 <= n && n < numLocks);
	if (mode & CRYPTO_LOCK) {
		locks[n]->lock();
	} else {
		locks[n]->unlock();
	}
}

////////////////////////////////////////////////////////////////////////////////

static DynLock *sslCreateDynLock(const char *file, int line)
{
	DynLock		*dl;

	dl = (DynLock*) mprMalloc(sizeof(DynLock));
	dl->mutex = new MprMutex();
	return dl;
}

////////////////////////////////////////////////////////////////////////////////

static void sslDestroyDynLock(DynLock *dl, const char *file, int line)
{
	delete dl->mutex;
	mprFree(dl);
}

////////////////////////////////////////////////////////////////////////////////

static void sslDynLock(int mode, DynLock *dl, const char *file, int line)
{
	if (mode & CRYPTO_LOCK) {
		dl->mutex->lock();
	} else {
		dl->mutex->unlock();
	}
}

#endif // BLD_FEATURE_MULTITHREAD
////////////////////////////////////////////////////////////////////////////////
//
//	Used for ephemeral RSA keys
//

static RSA *rsaCallback(SSL *ssl, int isExport, int keyLength)
{
	MaOpenSslSocket	*sslSocket;
	MaOpenSslConfig	*config;
	RSA 			*key;

	sslSocket = (MaOpenSslSocket*) SSL_get_app_data(ssl);
	config = (MaOpenSslConfig*) sslSocket->getConfig();

	key = 0;
	switch (keyLength) {
	case 512:
		key = config->rsaKey512;
		break;

	case 1024:
	default:
		key = config->rsaKey1024;
	}
	return key;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Used for ephemeral DH keys
//

static DH *dhCallback(SSL *ssl, int isExport, int keyLength)
{
	MaOpenSslSocket	*sslSocket;
	MaOpenSslConfig	*config;
	DH	 			*key;

	sslSocket = (MaOpenSslSocket*) SSL_get_app_data(ssl);
	config = (MaOpenSslConfig*) sslSocket->getConfig();

	key = 0;
	switch (keyLength) {
	case 512:
		key = config->dhKey512;
		break;

	case 1024:
	default:
		key = config->dhKey1024;
	}
	return key;
}

////////////////////////////////////////////////////////////////////////////////
#else
void mprOpenSslModuleDummy() {}

#endif // BLD_FEATURE_OPENSSL_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
