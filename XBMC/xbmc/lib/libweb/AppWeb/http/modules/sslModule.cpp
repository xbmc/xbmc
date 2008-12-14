///
///	@file 	sslModule.cpp
/// @brief 	Secure Sockets Layer (SSL) module
///	@overview The secure sockets module supports the configuration of 
///		the SSL protocol. The actual SSL protocol is implemented via 
///		3rd party SSL protocol providers such as MatrixSSL, OpenSSL 
///		or Mocana SSL. 
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

#include	"http.h"
#include	"sslModule.h"

/////////////////////////////////// Locals /////////////////////////////////////
#if BLD_FEATURE_SSL_MODULE

static MaSslProvider *sslProvider;

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaSslModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprSslInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	new MaSslModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaSslModule::MaSslModule(void *handle) : MaModule("ssl", handle)
{
#if BLD_FEATURE_LOG
	log = new MprLogModule("sslModule");
#endif
	sslConfigTable = new MprHashTable(29);
}

////////////////////////////////////////////////////////////////////////////////

MaSslModule::~MaSslModule()
{
#if BLD_FEATURE_LOG
	delete log;
#endif
	delete sslConfigTable;
}

////////////////////////////////////////////////////////////////////////////////

int MaSslModule::start()
{
	MaSslConfig	*config;

	//
	//	Allow the providers to setup SSL for each host configuraion
	// 
	config = (MaSslConfig*) sslConfigTable->getFirst();
	while (config) {
		if (config->start() < 0) {
			//
			//	If initialization fails, we must reverse out listeners who
			//	are setup for SSL.
			//
			MaHost *host = config->host;
			host->setSecure(0);
			host->server->setSslListeners(host, config);
		}
		config = (MaSslConfig*) sslConfigTable->getNext(config);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaSslModule::stop()
{
	MaSslConfig	*config;

	config = (MaSslConfig*) sslConfigTable->getFirst();
	while (config) {
		config->stop();
		config = (MaSslConfig*) sslConfigTable->getNext(config);
	}
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE

int MaSslModule::parseConfig(char *key, char *value, MaServer *server, 
	MaHost *host, MaAuth *auth, MaDir *dir, MaLocation *location)
{
	MaSslConfig	*config;
	char		pathBuf[MPR_MAX_FNAME], prefix[MPR_MAX_FNAME];
	char		*tok, *word, *enable, *provider;
	int			protoMask, mask;

	mprStrcpy(prefix, sizeof(prefix), key);
	prefix[3] = '\0';
	if (mprStrCmpAnyCase(prefix, "SSL") != 0) {
		return 0;
	}

	if (mprStrCmpAnyCase(key, "SSLEngine") == 0) {
		enable = mprStrTok(value, " \t", &tok);
		provider = mprStrTok(0, " \t", &tok);
		if (mprStrCmpAnyCase(value, "on") == 0) {
			if (sslProvider == 0) {
				mprError(MPR_L, MPR_LOG, "Missing an SSL Provider");
				return MPR_ERR_BAD_SYNTAX;
			}
			config = (MaSslConfig*) sslConfigTable->lookup(host->getName());
			if (config == 0) {
				config = sslProvider->newConfig(host);
				mprAssert(config);
				sslConfigTable->insert(config);
			}
			host->setSecure(1);
			host->server->setSslListeners(host, config);
		}
		return 1;
	}

	config = (MaSslConfig*) sslConfigTable->lookup(host->getName());
	if (config == 0) {
		mprError(MPR_L, MPR_LOG, "Missing SSLEngine directive");
		return MPR_ERR_BAD_SYNTAX;
	}

	if (host->makePath(pathBuf, sizeof(pathBuf), mprStrTrim(value, '\"')) 
			== 0) {
		mprError(MPR_L, MPR_LOG, "SSL path is too long");
		return MPR_ERR_BAD_SYNTAX;
    }

	if (mprStrCmpAnyCase(key, "SSLCACertificatePath") == 0) {
		config->setCaPath(pathBuf);
		return 1;

	} else if (mprStrCmpAnyCase(key, "SSLCACertificateFile") == 0) {
		config->setCaFile(pathBuf);
		return 1;

	} else if (mprStrCmpAnyCase(key, "SSLCertificateFile") == 0) {
		config->setCertFile(pathBuf);
		return 1;

	} else if (mprStrCmpAnyCase(key, "SSLCertificateKeyFile") == 0) {
		config->setKeyFile(pathBuf);
		return 1;

	} else if (mprStrCmpAnyCase(key, "SSLCipherSuite") == 0) {
		config->setCipherSuite(value);
		return 1;

	} else if (mprStrCmpAnyCase(key, "SSLVerifyClient") == 0) {
		if (mprStrCmpAnyCase(value, "require") == 0) {
			config->setVerifyClient(1);
		} else if (mprStrCmpAnyCase(value, "none") == 0) {
			config->setVerifyClient(0);
		} else {
			return -1;
		}
		return 1;

	} else if (mprStrCmpAnyCase(key, "SSLProtocol") == 0) {
		protoMask = 0;
		word = mprStrTok(value, " \t", &tok);
		while (word) {
			mask = -1;
			if (*word == '-') {
				word++;
				mask = 0;
			} else if (*word == '+') {
				word++;
			}
			if (mprStrCmpAnyCase(word, "SSLv2") == 0) {
				protoMask &= ~(MPR_HTTP_PROTO_SSLV2 & ~mask);
				protoMask |= (MPR_HTTP_PROTO_SSLV2 & mask);

			} else if (mprStrCmpAnyCase(word, "SSLv3") == 0) {
				protoMask &= ~(MPR_HTTP_PROTO_SSLV3 & ~mask);
				protoMask |= (MPR_HTTP_PROTO_SSLV3 & mask);

			} else if (mprStrCmpAnyCase(word, "TLSv1") == 0) {
				protoMask &= ~(MPR_HTTP_PROTO_TLSV1 & ~mask);
				protoMask |= (MPR_HTTP_PROTO_TLSV1 & mask);

			} else if (mprStrCmpAnyCase(word, "ALL") == 0) {
				protoMask &= ~(MPR_HTTP_PROTO_ALL & ~mask);
				protoMask |= (MPR_HTTP_PROTO_ALL & mask);
			}
			word = mprStrTok(0, " \t", &tok);
		}
		config->setSslProto(protoMask);
		return 1;
	}
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaSslProvider ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaSslProvider::MaSslProvider(char *name)
{
	if (sslProvider) {
		mprError(MPR_L, MPR_LOG, 
			"Multiple SSL providers defined. Ignoring %s", name);
	} else {
		sslProvider = this;
	}
	this->name = mprStrdup(name);
}

////////////////////////////////////////////////////////////////////////////////

MaSslProvider::~MaSslProvider()
{
	sslProvider = 0;
	mprFree(name);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual function -- overridden
//

MaSslConfig *MaSslProvider::newConfig(MaHost *host)
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaSslConfig /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	This class is subclassed by SSL providers
//

MaSslConfig::MaSslConfig(MaHost *host) : MprHashEntry(host->getName())
{
	keyFile = 0;
	certFile = 0;
	caFile = 0;
	caPath = 0;
	ciphers = 0;
	protocols = MPR_HTTP_PROTO_SSLV3 | MPR_HTTP_PROTO_TLSV1;
	verifyDepth = 6;
	verifyClient = 0;

	this->host = host;
}

////////////////////////////////////////////////////////////////////////////////

MaSslConfig::~MaSslConfig()
{
	mprFree(keyFile);
	mprFree(certFile);
	mprFree(caFile);
	mprFree(caPath);
	mprFree(ciphers);
}

////////////////////////////////////////////////////////////////////////////////

int MaSslConfig::start()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::stop()
{
}

////////////////////////////////////////////////////////////////////////////////

MprSocket *MaSslConfig::newSocket()
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setKeyFile(char *path) 
{
	mprFree(keyFile);
	keyFile = mprStrdup(path);
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setCertFile(char *path) 
{
	mprFree(certFile);
	certFile = mprStrdup(path);
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setCaFile(char *path) 
{
	mprFree(caFile);
	caFile = mprStrdup(path);
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setCaPath(char *path) 
{
	mprFree(caPath);
	caPath = mprStrdup(path);
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setCipherSuite(char *ciphers) 
{
	mprFree(this->ciphers);
	this->ciphers = mprStrdup(ciphers);
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setVerifyClient(bool require) 
{
	verifyClient = require;
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setVerifyDepth(int depth) 
{
	verifyDepth = depth;
}

////////////////////////////////////////////////////////////////////////////////

void MaSslConfig::setSslProto(int mask) 
{
	protocols = mask;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaSslSocket /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	This class is subclassed by SSL providers
//
MaSslSocket::MaSslSocket(MaSslConfig *config) : MprSocket()
{
	secure = 1;
	connTraced = 0;
	initialized = 0;
	this->config = config;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual destructor
//

MaSslSocket::~MaSslSocket()
{
}

////////////////////////////////////////////////////////////////////////////////

MaSslConfig *MaSslSocket::getConfig()
{
	return config; 
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return true if end of file
//

bool MaSslSocket::getEof()
{
	return (flags & MPR_SOCKET_EOF) != 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Called in to accept new connections. Virtual. Provider must override.
//

MprSocket *MaSslSocket::newSocket()
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual. Provider must override
//

int MaSslSocket::initConnection()
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual. Provider must override
//

bool MaSslSocket::dispose()
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual. Provider must override
//

void MaSslSocket::close(int how)
{
	mprAssert(0);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual. Provider must override
//

int MaSslSocket::flush()
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual. Provider must override
//
 
int	MaSslSocket::read(char *buf, int len)
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual. Provider must override
//

int	MaSslSocket::write(char *buf, int len)
{
	mprAssert(0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaSslSocket::ioProc(int mask, int isPoolThread)
{
	if (mask & MPR_READABLE && !initialized) {
		if (initConnection() == 0) {
			initialized = 1;
		}
	}
	this->MprSocket::ioProc(mask, isPoolThread);
}

////////////////////////////////////////////////////////////////////////////////
#else
void mprSslModuleDummy() {}

#endif // BLD_FEATURE_SSL_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
