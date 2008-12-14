///
///	@file 	openSslModule.h
/// @brief 	OpenSSL header for the openSslModule
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
////////////////////////////////// Includes ////////////////////////////////////

#ifndef _h_OPENSSL_MODULE
#define _h_OPENSSL_MODULE 1

#include	"http.h"

#if BLD_FEATURE_OPENSSL_MODULE
#define 	OPENSSL_NO_KRB5 1

#include	"sslModule.h"
#include	<openssl/ssl.h>
#include	<openssl/evp.h>
#include	<openssl/rand.h>

#ifndef HEADER_DH_H
#include 	<openssl/dh.h>
#endif

////////////////////////////////// Definitions /////////////////////////////////

#if BLD_FEATURE_MULTITHREAD
struct CRYPTO_dynlock_value {
	MprMutex	*mutex;
};
typedef struct CRYPTO_dynlock_value DynLock;
#endif

/////////////////////////////// Extern Definitions /////////////////////////////

class MaOpenSslModule;

extern "C" {
	extern int mprOpenSslInit(void *handle);
}

//
//	Entropy source to see the random number generator. We assume that OpenSSL
//	will augment this with /dev/random or O/S equivalent on systems where it 
//	is available.
//
struct RandBuf {
	int	pid;
	int time;
	int msec;
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// OpenSslModule /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaOpenSslModule : public MaModule {
  private:
  public:
					MaOpenSslModule(void *handle);
					~MaOpenSslModule();
	int				start();
	void			stop();
};

////////////////////////////////////////////////////////////////////////////////

class MaOpenSslProvider : public MaSslProvider {
  private:
	MprStr			name;
  public:
					MaOpenSslProvider(char *name);
					~MaOpenSslProvider();
	MaSslConfig		*newConfig(MaHost *host);
};

////////////////////////////////////////////////////////////////////////////////
//
//	Per host SSL configuration information
//

class MaOpenSslConfig : public MaSslConfig {
  public:
	SSL_CTX			*context;
	RSA				*rsaKey512;
	RSA				*rsaKey1024;
	DH				*dhKey512;
	DH				*dhKey1024;

  public:
					MaOpenSslConfig(MaHost *host);
					~MaOpenSslConfig();

	MprSocket 		*newSocket();
	SSL_CTX			*getContext();
	int				start();
	void			stop();
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaOpenSslSocket ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaOpenSslSocket : public MaSslSocket {
  private:
	SSL				*ssl;
	BIO				*bio;
	SSL_CTX			*context;				// Copy from secureContext

  public:
					MaOpenSslSocket(MaOpenSslConfig *config);
					~MaOpenSslSocket();
	void			close(int how);
	bool			dispose();
	int				flush();
	int				initConnection();
	MprSocket		*newSocket();
	int				read(char *buf, int bufsize);
	int				write(char *buf, int bufsize);
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#else

void mprOpenSslDummy();
#endif // BLD_FEATURE_OPENSSL_MODULE
#endif // _h_OPENSSL_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
