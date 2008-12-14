///
///	@file 	authHandler.cpp
/// @brief 	Authorization handler for basic and digest authentication.
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
////////////////////////////////// Includes ////////////////////////////////////

#include	"authHandler.h"
#include	"http.h"

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_AUTH_MODULE

static char* trimWhiteSpace(char *str);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// AuthModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

int mprAuthInit(void *handle)
{
	if (maGetHttp() == 0) {
		return MPR_ERR_NOT_INITIALIZED;
	}
	(void) new MaAuthModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaAuthModule::MaAuthModule(void *handle) : MaModule("auth", handle)
{
	authHandlerService = new MaAuthHandlerService();
}

////////////////////////////////////////////////////////////////////////////////

MaAuthModule::~MaAuthModule()
{
	delete authHandlerService;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MaAuthHandlerService ////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaAuthHandlerService::MaAuthHandlerService() : MaHandlerService("authHandler")
{
#if BLD_FEATURE_LOG
	log = new MprLogModule("auth");
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaAuthHandlerService::~MaAuthHandlerService()
{
#if BLD_FEATURE_LOG
	delete log;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaAuthHandlerService::newHandler(MaServer *server, MaHost *host, 
	char *ext)
{
	return new MaAuthHandler(log);
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaAuthHandler ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaAuthHandler::MaAuthHandler(MprLogModule *serviceLog) : 
	MaHandler("authHandler", 0, MPR_HANDLER_ALWAYS)
{
	log = serviceLog;
	flags = 0;
	basicPassword = 0;
	groupFile = 0;
	userFile = 0;
	userName = 0;

#if BLD_FEATURE_DIGEST
	//
	//	Names chosen to best match the spec (RFC 2616)
	//
	cnonce = 0;
	nc = 0;
	nonce = 0;
	opaque = 0;
	qop = 0;
	realm = 0;
	responseDigest = 0;
	uri = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaAuthHandler::~MaAuthHandler()
{
	mprFree(basicPassword);
	mprFree(groupFile);
	mprFree(userFile);
	mprFree(userName);
#if BLD_FEATURE_DIGEST
	mprFree(cnonce);
	mprFree(nc);
	mprFree(nonce);
	mprFree(opaque);
	mprFree(qop);
	mprFree(realm);
	mprFree(responseDigest);
	mprFree(uri);
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaAuthHandler::cloneHandler()
{
	return new MaAuthHandler(log);
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE

int MaAuthHandler::parseConfig(char *key, char *value, MaServer *server, 
	MaHost *host, MaAuth *auth, MaDir *dir, MaLocation *location)
{
	MaAcl	acl;
	char	pathBuf[MPR_MAX_FNAME];
	char	*path, *names, *tok, *type, *aclSpec;

	if (mprStrCmpAnyCase(key, "AuthGroupFile") == 0) {
		path = mprStrTrim(value, '\"');
		if (host->makePath(pathBuf, sizeof(pathBuf), path) == 0) {
			mprError(MPR_L, MPR_LOG, "AuthGroupFile is too long");
			return MPR_ERR_BAD_SYNTAX;
		}
		if (readGroupFile(server, auth, pathBuf) < 0) {
			mprError(MPR_L, MPR_LOG, "Can't open AuthGroupFile %s", pathBuf);
			return MPR_ERR_BAD_SYNTAX;
		}
		return 1;

	} else if (mprStrCmpAnyCase(key, "AuthName") == 0) {
		auth->setRealm(mprStrTrim(value, '\"'));
		return 1;
		
	} else if (mprStrCmpAnyCase(key, "AuthType") == 0) {
		value = mprStrTrim(value, '\"');
		if (mprStrCmpAnyCase(value, "Basic") == 0) {
			auth->setType(MPR_HTTP_AUTH_BASIC);
#if BLD_FEATURE_DIGEST
		} else if (mprStrCmpAnyCase(value, "Digest") == 0) {
			auth->setType(MPR_HTTP_AUTH_DIGEST);
#endif
		} else {
			mprError(MPR_L, MPR_LOG, "Unsupported authorization protocol");
			return MPR_ERR_BAD_SYNTAX;
		}
		return 1;
		
	} else if (mprStrCmpAnyCase(key, "AuthUserFile") == 0) {
		path = mprStrTrim(value, '\"');
		if (host->makePath(pathBuf, sizeof(pathBuf), path) == 0) {
			mprError(MPR_L, MPR_LOG, "AuthUserFile is too long");
			return MPR_ERR_BAD_SYNTAX;
		}
		if (readUserFile(server, auth, pathBuf) < 0) {
			mprError(MPR_L, MPR_LOG, "Can't open AuthUserFile %s", pathBuf);
			return MPR_ERR_BAD_SYNTAX;
		}
		return 1;

#if BLD_FEATURE_DIGEST
	} else if (mprStrCmpAnyCase(key, "AuthDigestQop") == 0) {
		value = mprStrTrim(value, '\"');
		mprStrLower(value);
		if (strcmp(value, "none") != 0 && strcmp(value, "auth") != 0 && 
				strcmp(value, "auth-int") != 0) {
			return MPR_ERR_BAD_SYNTAX;
		}
		auth->setQop(value);
		return 1;

	} else if (mprStrCmpAnyCase(key, "AuthDigestAlgorithm") == 0) {
		return 1;

	} else if (mprStrCmpAnyCase(key, "AuthDigestDomain") == 0) {
		return 1;

	} else if (mprStrCmpAnyCase(key, "AuthDigestNonceLifetime") == 0) {
		return 1;

#endif
	} else if (mprStrCmpAnyCase(key, "Require") == 0) {
		if (server->getValue(&type, value, &tok, 1) < 0) {
			return MPR_ERR_BAD_SYNTAX;
		}
		if (mprStrCmpAnyCase(type, "acl") == 0) {
			aclSpec = mprStrTrim(tok, '\"');
			acl = auth->parseAcl(aclSpec);
			auth->setRequiredAcl(acl);
		} else if (mprStrCmpAnyCase(type, "valid-user") == 0) {
			auth->setAnyValidUser();
		} else {
			names = mprStrTrim(tok, '\"');
			if (mprStrCmpAnyCase(type, "user") == 0) {
				auth->setRequiredUsers(names);
			} else if (mprStrCmpAnyCase(type, "group") == 0) {
				auth->setRequiredGroups(names);
			} else {
				mprError(MPR_L, MPR_LOG, "Bad Require syntax: %s", type);
				return MPR_ERR_BAD_SYNTAX;
			}
		}
		return 1;
	}

	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////

int MaAuthHandler::readGroupFile(MaServer *server, MaAuth *auth, char *path)
{
	MprFile		*file;
	MaAcl		acl;
	char		buf[MPR_MAX_STRING];
	char		*users, *groupName, *enabled, *aclSpec, *tok, *cp;

	mprFree(groupFile);
	groupFile = mprStrdup(path);

	file = server->fileSystem->newFile();
	if (file->open(path, O_RDONLY | O_TEXT, 0444) < 0) {
		delete file;
		return MPR_ERR_CANT_OPEN;
	}

	while (file->gets(buf, sizeof(buf))) {
		enabled = mprStrTok(buf, " :\t", &tok);

		for (cp = enabled; isspace(*cp); cp++) {
			;
		}
		if (*cp == '\0' || *cp == '#') {
			continue;
		}

		aclSpec = mprStrTok(0, " :\t", &tok);
		groupName = mprStrTok(0, " :\t", &tok);
		users = mprStrTok(0, "\r\n", &tok);

		acl = auth->parseAcl(aclSpec);
		auth->addGroup(groupName, acl, (*enabled == '0') ? 0 : 1);
		auth->addUsersToGroup(groupName, users);
	}
	file->close();
	delete file;

	auth->updateUserAcls();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaAuthHandler::readUserFile(MaServer *server, MaAuth *auth, char *path)
{
	MprFile		*file;
	char		buf[MPR_MAX_STRING];
	char		*enabled, *userName, *password, *realm, *tok, *cp;

	mprFree(userFile);
	userFile = mprStrdup(path);

	file = server->fileSystem->newFile();
	if (file->open(path, O_RDONLY | O_TEXT, 0444) < 0) {
		delete file;
		return MPR_ERR_CANT_OPEN;
	}

	while (file->gets(buf, sizeof(buf))) {
		enabled = mprStrTok(buf, " :\t", &tok);

		for (cp = enabled; isspace(*cp); cp++) {
			;
		}
		if (*cp == '\0' || *cp == '#') {
			continue;
		}

		userName = mprStrTok(0, " :\t", &tok);
		realm = mprStrTok(0, ":", &tok);
		password = mprStrTok(0, " \t\r\n", &tok);

		userName = trimWhiteSpace(userName);
		realm = trimWhiteSpace(realm);
		password = trimWhiteSpace(password);

		auth->addUser(userName, realm, password, (*enabled == '0' ? 0 : 1));
	}
	file->close();
	delete file;

	auth->updateUserAcls();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaAuthHandler::writeUserFile(MaServer *server, MaAuth *auth, char *path)
{
	MprHashTable	*userHash;
	MprFile			*file;
	MaUser			*up;
	char			buf[MPR_HTTP_MAX_PASS * 2];
	char			tempFile[MPR_MAX_FNAME];

	file = server->fileSystem->newFile();
	mprMakeTempFileName(tempFile, sizeof(tempFile), 0);

	if (file->open(tempFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0444) < 0) {
		mprError(MPR_L, MPR_USER, "Can't open %s", tempFile);
		delete file;
		return MPR_ERR_CANT_OPEN;
	}

	userHash = auth->getUserHash();
	up = (MaUser*) userHash->getFirst();
	while (up) {
		mprSprintf(buf, sizeof(buf), "%d: %s: %s: %s\n", up->getEnabled(), 
			up->getName(), up->getRealm(), up->getPassword());
		file->write(buf, strlen(buf));
		up = (MaUser*) userHash->getNext(up);
	}

	file->close();
	delete file;

	unlink(path);
	if (rename(tempFile, path) < 0) {
		mprError(MPR_L, MPR_USER, "Can't create new %s", path);
		return MPR_ERR_CANT_WRITE;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaAuthHandler::writeGroupFile(MaServer *server, MaAuth *auth, char *path)
{
	MprHashTable	*groupHash;
	MprFile			*file;
	MaGroup			*gp;
	char			buf[MPR_MAX_STRING];
	char			tempFile[MPR_MAX_FNAME];

	file = server->fileSystem->newFile();
	mprMakeTempFileName(tempFile, sizeof(tempFile), 0);
	if (file->open(tempFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0444) < 0) {
		mprError(MPR_L, MPR_USER, "Can't open %s", tempFile);
		delete file;
		return MPR_ERR_CANT_OPEN;
	}

	groupHash = auth->getGroupHash();
	gp = (MaGroup*) groupHash->getFirst();
	while (gp) {
		mprSprintf(buf, sizeof(buf), "%d: %x: %s: %s\n", gp->getEnabled(), 
			gp->getAcl(), gp->getName(), gp->getUsers());
		file->write(buf, strlen(buf));
		gp = (MaGroup*) groupHash->getNext(gp);
	}

	file->close();
	delete file;

	unlink(path);
	if (rename(tempFile, path) < 0) {
		mprError(MPR_L, MPR_USER, "Can't create new %s", path);
		return MPR_ERR_CANT_WRITE;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Override default matchRequest and handle all requests
//

int MaAuthHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

int MaAuthHandler::run(MaRequest *rq)
{
	MaAuth		*auth;
	MaAuthType	authType;
	char		*requestAuthType, *uri, *cp, *authDetails;

	auth = rq->getAuth();
	if (!rq->host->isAuthEnabled() || !auth->isAuthRequired()) {
		return 0;
	}

	uri = rq->getUri();
	requestAuthType = rq->getAuthType();
	authDetails = rq->getAuthDetails();

	if (requestAuthType == 0) {
		formatAuthResponse(rq, auth, 401, 
			"Access Denied, Authorization required.", 0);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}
	if (authDetails == 0) {
		formatAuthResponse(rq, auth, 401, 
			"Access Denied, Missing authorization details.", 0);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	if (mprStrCmpAnyCase(requestAuthType, "basic") == 0) {
		char	decodedDetails[64];

		maDecode64(decodedDetails, sizeof(decodedDetails), authDetails);
		if ((cp = strchr(decodedDetails, ':')) != 0) {
			*cp++ = '\0';
		}
		if (cp) {
			userName = mprStrdup(decodedDetails);
			basicPassword = mprStrdup(cp);
		} else {
			userName = mprStrdup("");
			basicPassword = mprStrdup("");
		}
		authType = MPR_HTTP_AUTH_BASIC;
		rq->setUser(userName);
		rq->setPassword(basicPassword);

#if BLD_FEATURE_DIGEST
	} else if (mprStrCmpAnyCase(requestAuthType, "digest") == 0) {
		if (decodeDigestDetails(rq, authDetails) < 0) {
			rq->requestError(400, "Bad authorization header");
			rq->finishRequest();
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}
		authType = MPR_HTTP_AUTH_DIGEST;
		rq->setUser(userName);
		rq->setPassword(basicPassword);
#endif
	} else {
		authType = MPR_HTTP_AUTH_UNKNOWN;
	}
	mprLog(4, log, "run: type %d, uri %s\nDetails %s\n", authType, uri, 
		authDetails);

	if (userName == 0) {
		formatAuthResponse(rq, auth, 401, 
			"Access Denied, Missing user name.", 0);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	if (auth->getType() != authType) {
		formatAuthResponse(rq, auth, 401, 
			"Access Denied, Wrong authentication protocol.", 0);
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

#if BLD_FEATURE_DIGEST
	if (authType == MPR_HTTP_AUTH_DIGEST) {
		if (strcmp(qop, auth->getQop()) != 0) {
			formatAuthResponse(rq, auth, 401, 
				"Access Denied, Quality of protection does not match.", 0);
			return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
		}
	}
#endif

	if (! auth->isUserValid(userName, auth->getRealm())) {
		formatAuthResponse(rq, auth, 401, 
			"Access denied, authentication error", 
			"Access Denied, Unknown User.");
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}

	if (!isPasswordValid(rq)) {
		formatAuthResponse(rq, auth, 401, 
			"Access Denied, authentication error",
			"Access Denied, Wrong Password.");
		return MPR_HTTP_HANDLER_FINISHED_PROCESSING;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaAuthHandler::writeHeaders(MaRequest *rq)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_DIGEST

//	FUTURE -- comment here about what the input format is

int MaAuthHandler::decodeDigestDetails(MaRequest *rq, char *authDetails)
{
	char	*value, *tok, *key, *dp, *sp;
	int		seenComma;

	authDetails = mprStrdup(authDetails);
	key = authDetails;

	while (*key) {
		while (*key && isspace(*key)) {
			key++;
		}
		tok = key;
		while (*tok && !isspace(*tok) && *tok != ',' && *tok != '=') {
			tok++;
		}
		*tok++ = '\0';

		while (isspace(*tok)) {
			tok++;
		}
		seenComma = 0;
		if (*tok == '\"') {
			value = ++tok;
			while (*tok != '\"' && *tok != '\0') {
				tok++;
			}
		} else {
			value = tok;
			while (*tok != ',' && *tok != '\0') {
				tok++;
			}
			seenComma++;
		}
		*tok++ = '\0';

		//
		//	Handle back-quoting
		//
		if (strchr(value, '\\')) {
			for (dp = sp = value; *sp; sp++) {
				if (*sp == '\\') {
					sp++;
				}
				*dp++ = *sp++;
			}
			*dp = '\0';
		}

		//
		//	username, response, oqaque, uri, realm, nonce, nc, cnonce, qop
		//
		switch (tolower(*key)) {
		case 'a':
			if (mprStrCmpAnyCase(key, "algorithm") == 0) {
				break;
			} else if (mprStrCmpAnyCase(key, "auth-param") == 0) {
				break;
			}
			break;

		case 'c':
			if (mprStrCmpAnyCase(key, "cnonce") == 0) {
				cnonce = mprStrdup(value);
			}
			break;

		case 'd':
			if (mprStrCmpAnyCase(key, "domain") == 0) {
				break;
			}
			break;

		case 'n':
			if (mprStrCmpAnyCase(key, "nc") == 0) {
				nc = mprStrdup(value);
			} else if (mprStrCmpAnyCase(key, "nonce") == 0) {
				nonce = mprStrdup(value);
			}
			break;

		case 'o':
			if (mprStrCmpAnyCase(key, "opaque") == 0) {
				opaque = mprStrdup(value);
			}
			break;

		case 'q':
			if (mprStrCmpAnyCase(key, "qop") == 0) {
				qop = mprStrdup(value);
			}
			break;

		case 'r':
			if (mprStrCmpAnyCase(key, "realm") == 0) {
				realm = mprStrdup(value);
			} else if (mprStrCmpAnyCase(key, "response") == 0) {
				responseDigest = mprStrdup(value);
			}
			break;

		case 's':
			if (mprStrCmpAnyCase(key, "stale") == 0) {
				break;
			}
		
		case 'u':
			if (mprStrCmpAnyCase(key, "uri") == 0) {
				uri = mprStrdup(value);
			} else if (mprStrCmpAnyCase(key, "username") == 0) {
				userName = mprStrdup(value);
			}
			break;

		default:
			//	Just ignore keywords we don't understand
			;
		}
		key = tok;
		if (!seenComma) {
			while (*key && *key != ',') {
				key++;
			}
			if (*key) {
				key++;
			}
		}
	}
	mprFree(authDetails);
	if (userName == 0 || realm == 0 || nonce == 0 || uri == 0 ||
			responseDigest == 0) {
		return MPR_ERR_BAD_ARGS;
	}
	if (qop && (cnonce == 0 || nc == 0)) {
		return MPR_ERR_BAD_ARGS;
	}
	if (qop == 0) {
		qop = mprStrdup("");
	}
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////

bool MaAuthHandler::isPasswordValid(MaRequest *rq)
{
	MaAuth	*auth;
	char	requiredPass[MPR_HTTP_MAX_PASS], buf[MPR_HTTP_MAX_PASS * 2];
	char	*encoded;
	int		rc;

	auth = rq->getAuth();
	if (auth->getPassword(requiredPass, sizeof(requiredPass), userName, 
			auth->getRealm()) < 0) {
		return 0;
	}

	if (auth->getType() == MPR_HTTP_AUTH_BASIC) {
		mprSprintf(buf, sizeof(buf), "%s:%s:%s", userName, auth->getRealm(), 
			basicPassword);
		encoded = maMD5(buf);
		rc = strcmp(encoded, requiredPass);
		mprFree(encoded);
		return rc == 0;

#if BLD_FEATURE_DIGEST
	} else if (auth->getType() == MPR_HTTP_AUTH_DIGEST) {
		char *requiredDigest;
		maCalcDigest(0, requiredPass, 0, uri, nonce, qop, nc, 
			cnonce, rq->getMethod(), &requiredDigest);
		rc = strcmp(responseDigest, requiredDigest);
		mprFree(requiredDigest);
		return rc == 0;
#endif
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaAuthHandler::formatAuthResponse(MaRequest *rq, MaAuth *auth, int code, 
	char *userMsg, char *logMsg)
{
#if BLD_FEATURE_DIGEST
	char	*qopClass, *nonceStr, *etag;
#endif
	char	*buf, *headers;

	rq->incAccessError();

	if (logMsg == 0) {
		logMsg = userMsg;
	}
	mprLog(3, log, "formatAuthResponse: code %d, %s\n", code, logMsg);
	mprAllocSprintf(&buf, MPR_HTTP_BUFSIZE, 
		"<HTML><HEAD>\n<TITLE>Authentication Error: %s</TITLE>\n</HEAD>\r\n"
		"<BODY><H2>Authentication Error: %s</H2></BODY>\n</HTML>\r\n",
		userMsg, userMsg);

	headers = 0;
	if (auth->getType() == MPR_HTTP_AUTH_BASIC) {
		mprAllocSprintf(&headers, MPR_MAX_STRING,
			"WWW-Authenticate: Basic realm=\"%s\"", auth->getRealm());

#if BLD_FEATURE_DIGEST
	} else if (auth->getType() == MPR_HTTP_AUTH_DIGEST) {

		qopClass = auth->getQop();

		//
		//	Use the etag as our opaque string
		//
		etag = rq->getEtag();
		if (etag == 0) {
			etag = "";
		}
		maCalcNonce(&nonceStr, rq->host->getSecret(), etag, auth->getRealm());

		if (strcmp(qopClass, "auth") == 0) {
			mprAllocSprintf(&headers, MPR_MAX_STRING,
				"WWW-Authenticate: Digest realm=\"%s\", domain=\"%s\", "
				"qop=\"auth\", nonce=\"%s\", opaque=\"%s\", algorithm=\"MD5\", "
				"stale=\"FALSE\"", 
				auth->getRealm(), rq->host->getName(), nonceStr, etag);

		} else if (strcmp(qopClass, "auth-int") == 0) {
			mprAllocSprintf(&headers, MPR_MAX_STRING,
				"WWW-Authenticate: Digest realm=\"%s\", domain=\"%s\", "
				"qop=\"auth\", nonce=\"%s\", opaque=\"%s\", algorithm=\"MD5\", "
				"stale=\"FALSE\"", 
				auth->getRealm(), rq->host->getName(), nonceStr, etag);

		} else {
			mprAllocSprintf(&headers, MPR_MAX_STRING,
				"WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"",
				auth->getRealm(), nonceStr);
		}
		mprFree(nonceStr);
#endif
	}

	if (headers) {
		rq->setHeader(headers);
	}

	rq->formatAltResponse(code, buf, MPR_HTTP_DONT_ESCAPE);
	rq->flushOutput(MPR_HTTP_FOREGROUND_FLUSH, MPR_HTTP_FINISH_REQUEST);
	mprFree(headers);
	mprFree(buf);
}

////////////////////////////////////////////////////////////////////////////////

static char* trimWhiteSpace(char *str)
{
	int		len;

	if (str == 0) {
		return str;
	}
	while (isspace(*str)) {
		str++;
	}
	len = strlen(str) - 1;
	while (isspace(str[len])) {
		str[len--] = '\0';
	}
	return str;
}

////////////////////////////////////////////////////////////////////////////////

#else
void mprAuthHandlerDummy() {}

#endif // BLD_FEATURE_AUTH

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
