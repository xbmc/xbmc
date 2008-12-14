///
///	@file 	matrixSslModule.h
/// @brief 	Header for the Matrix SSL module
//	@copy	default.p
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	Copyright (c) PeerSec Networks LLC, 2002-2005. All Rights Reserved.
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
//////////////////////////////// Documentation /////////////////////////////////
///
///	This module integrates support for MatrixSSL into Appweb via a dynamically
///	loaded module.  The MatrixSSL library, including the latest version of
///	this file is available as source code at http://www.matrixssl.org.
///
////////////////////////////////// Includes ////////////////////////////////////

#ifndef _h_MATRIXSSL_MODULE
#define _h_MATRIXSSL_MODULE 1

#include	"http.h"

#if BLD_FEATURE_MATRIXSSL_MODULE
#include	"sslModule.h"

	/*
	 *	Protect from genDepend incase matrixSsl is not installed
	 */
	#include	"matrixSsl.h"

////////////////////////////// Forward Definitions /////////////////////////////

class MatrixSslModule;

extern "C" {
	extern int mprMatrixSslInit(void *handle);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MaMatrixSslModule ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaMatrixSslModule : public MaModule {
  private:
  public:
					MaMatrixSslModule(void *handle);
					~MaMatrixSslModule();
	int				start();
	void			stop();
};

////////////////////////////////////////////////////////////////////////////////

class MaMatrixSslProvider : public MaSslProvider {
  private:
	MprStr			name;
  public:
					MaMatrixSslProvider(char *name);
					~MaMatrixSslProvider();
	MaSslConfig		*newConfig(MaHost *host);
};

////////////////////////////////////////////////////////////////////////////////
//
//	Per host SSL configuration information
//

class MaMatrixSslConfig : public MaSslConfig {
  public:
	sslKeys_t		*keys;

  public:
					MaMatrixSslConfig(MaHost *host);
					~MaMatrixSslConfig();

	MprSocket 		*newSocket();
	int				start();
	void			stop();
};

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaMatrixSslSocket //////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaMatrixSslSocket : public MaSslSocket {
  private:
	ssl_t			*ssl;
	sslBuf_t		insock;				// Cached ciphertext from socket
	sslBuf_t		inbuf;				// Cached (decoded) plaintext
	sslBuf_t		outsock;			// Cached ciphertext to socket
	int				outBufferCount;		// Count of outgoing data we've buffered

  public:
					MaMatrixSslSocket(MaMatrixSslConfig *config);
					~MaMatrixSslSocket();
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

void mprMatrixSslDummy();

#endif // BLD_FEATURE_MATRIXSSL_MODULE
#endif // _h_MATRIXSSL_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
