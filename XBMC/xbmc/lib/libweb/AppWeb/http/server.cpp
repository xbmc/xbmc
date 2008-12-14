///
///	@file 	server.cpp
/// @brief 	Server Class to manage a single server
/// @overview An instance of the MaServer Class may be created for 
///		each http.conf file. Each server can manage multiple hosts 
///		(standard, virtual or SSL). This file parses the http.conf 
///		file and creates all the necessary MaHost, MaDir and MaLocation 
///		objects to manage the server's operation.
///		\n\n
///		For convenience, the MaHostAddress, MaListen and MaVhost classes are 
///		contained in this file.
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

#if BLD_FEATURE_SSL_MODULE
#include	"sslModule.h"
#endif

//////////////////////////////////// Locals ////////////////////////////////////

struct ConfigStack {
	MaLocation	*location;
	MaDir		*dir;
	MaHost		*host;
};

MaServer	*MaServer::defaultServer;

////////////////////////////// Forward Declarations ////////////////////////////

static void	acceptWrapper(void *data, MprSocket *sock, char *ipAddr, int port, 
				MprSocket *lp, int isPoolThread);
#if !BLD_FEATURE_ROMFS
static void mapPathDelim(char *s);
#endif
#if BLD_FEATURE_DLL
static int 	locateModule(MprFileSystem *fileSystem, MaHost *host, 
				char *pathBuf, int pathSize, char *moduleDirs, char *module);
#endif
#if BLD_FEATURE_LICENSE
static void expiryTimeoutCheck(void *data, MprTimer *tp);
#endif

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Server /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaServer::MaServer(MaHttp *hs, char *name, char *root, char *ipAddr, int port)
{
	char	ipAddrBuf[MPR_MAX_IP_ADDR_PORT];

	mprAssert(hs);
	mprAssert(name && *name);

	this->name = mprStrdup(name);
	http = hs;
	fileSystem = defaultFileSystem = new MprFileSystem();
	http->insertServer(this);
	hostAddresses = new MprHashTable(53);
	defaultHost = 0;
	lineNumber = 0;
#if BLD_FEATURE_LOG
	alreadyLogging = 0;
	tMod = new MprLogModule("httpServer");
#endif
#if BLD_FEATURE_LICENSE
	license = 0;						// In eval mode
	evalDays = 0;						// Days passed during evaluation
	evalExtension = 1;					// Extension factor
	expiryTimer = 0;
	expired = 0;
#endif
	if (defaultServer == 0) {
		defaultServer = this;
	}
	serverRoot = 0;
	if (root) {
		setServerRoot(root);
	}

	if (ipAddr && port > 0) {
		listens.insert(new MaListen(ipAddr, port, 0));
		mprSprintf(ipAddrBuf, sizeof(ipAddrBuf), "%s:%d", ipAddr, port);
		hostAddresses->insert(new MaHostAddress(ipAddrBuf));
	}

	appObj = mprCreateObjVar("application", MA_HTTP_HASH_SIZE);
	mprSetVarDeleteProtect(&appObj, 1);
}

////////////////////////////////////////////////////////////////////////////////

MaServer::~MaServer()
{
	MaHost		*hp, *nextHp;
	MaListen	*lp, *nextLp;

	mprDestroyVar(&appObj);

	delete hostAddresses;
	delete defaultFileSystem;

	lp = (MaListen*) listens.getFirst();
	while (lp) {
		nextLp = (MaListen*) listens.getNext(lp);
		listens.remove(lp);
		delete lp;
		lp = nextLp;
	}

	hp = (MaHost*) hosts.getFirst();
	while (hp) {
		nextHp = (MaHost*) hosts.getNext(hp);
		hosts.remove(hp);
		delete hp;
		hp = nextHp;
	}

	mprFree(name);
	mprFree(serverRoot);

#if BLD_FEATURE_LOG
	delete tMod;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaServer *MaServer::getDefaultServer()
{
	return defaultServer;
}

////////////////////////////////////////////////////////////////////////////////

void MaServer::setDefaultServer(MaServer *server)
{
	defaultServer = server;
}

////////////////////////////////////////////////////////////////////////////////

int MaServer::start()
{
	MaHost		*hp;
	MaListen	*lp;
	char		*proto, *ipaddr;
	int			count, warned;

#if BLD_FEATURE_LICENSE
	char		timeStamp[MPR_MAX_FNAME];
	int			fd, now, version, major, minor, versionWord;
	char		*tok, *cp, *versionStr;

	//
	//	Get the products current major+minor version number
	//
	tok = versionStr = mprStrdup(BLD_VERSION);
	if ((cp = strchr(tok, '.')) != 0) {
		*cp++ = '\0';
	}
	major = atoi(tok);
	minor = 0;
	if (cp) {
		tok = cp;
		if ((cp = strchr(tok, '.')) != 0) {
			*cp++ = '\0';
		}
		minor = atoi(tok);
	}
	mprFree(versionStr);
	version = major << 16 | minor << 8;

	//
	//	Create an install ID. Put the key file in a non-descript place.
	//
#if WIN
	char	drive[MAX_PATH];
	GetSystemDirectory(drive, sizeof(drive));
	mprSprintf(timeStamp, sizeof(timeStamp), "%s/amwzbb.dll", drive);
#endif
#if LINUX
	//	TODO -- MAC and FREEBSD
	mprSprintf(timeStamp, sizeof(timeStamp), "/usr/tmp/.libamwzbb.so");
#endif
	installTime = 0;
	now = 0;
	if ((fd = open(timeStamp, O_RDONLY | O_BINARY)) >= 0) {
		if (read(fd, &versionWord, sizeof(versionWord)) < 0) {
			close(fd);
			fd = -1;
		}
		if (read(fd, &installTime, sizeof(installTime)) < 0) {
			close(fd);
			fd = -1;
		}
		if (versionWord != version) {
			close(fd);
			fd = -1;
		}
	}
	if (fd < 0) {
		installTime = (uint) time(0);
		unlink(timeStamp);
		fd = open(timeStamp, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0664);
		if (fd >= 0) {
			write(fd, &version, sizeof(version));
			write(fd, &installTime, sizeof(installTime));
		}
	}
	if (fd >= 0) {
		close(fd);
	}

	mprAssert(installTime > 0);

	if (license) {
		mprLog(MPR_CONFIG, "Licensed copy %04LX-%04LX-%04LX-%04LX\n", 
			(license >> (3 * 16)) & 0xFFFF,
			(license >> (2 * 16)) & 0xFFFF,
			(license >> (1 * 16)) & 0xFFFF,
			(license >> (0 * 16)) & 0xFFFF);

	} else {
	
		calcExpiryDays();
		if (expired) {
			return MPR_ERR_TIMEOUT;
		}
		expiryTimer = new MprTimer(3600 * 1000, expiryTimeoutCheck, this);
	}
#endif	// BLD_FEATURE_LICENSE

	//
	//	Start the hosts
	//
	hp = (MaHost*) hosts.getFirst();
	while (hp) {
		mprLog(MPR_CONFIG, "Starting host named: \"%s\"\n", hp->getName());
		if (hp->start() < 0) {
			return MPR_ERR_CANT_INITIALIZE;
		}
		hp = (MaHost*) hosts.getNext(hp);
	}

	//
	//	Listen to all required ipAddr:ports
	//
	count = 0;
	warned = 0;
	lp = (MaListen*) listens.getFirst();
	while (lp) {
		if (lp->open(this) < 0) {
			mprError(MPR_L, MPR_LOG, "Can't listen for HTTP on %s:%d", 
				lp->getIpAddr(), lp->getPort());
			warned++;
			break;

		} else {
#if BLD_FEATURE_SSL_MODULE
			if (lp->isSecure()) {
				proto = "HTTPS";
			} else
#endif
				proto = "HTTP";
			ipaddr = lp->getIpAddr();
			if (ipaddr == 0 || *ipaddr == '\0') {
				ipaddr = "*";
			}
			mprLog(MPR_CONFIG, "Listening for %s on %s:%d\n", 
				proto, ipaddr, lp->getPort());
			count++;
		}
		lp = (MaListen*) listens.getNext(lp);
	}
	if (count == 0) {
		if (! warned) {
			mprError(MPR_L, MPR_USER, 
				"Server is not listening on any addresses");
		}
		return MPR_ERR_CANT_OPEN;
	}

	//
	//	Now change user and group to the desired identities (user must be last)
	//
	if (http->changeGroup() < 0 || http->changeUser() < 0) {
		return MPR_ERR_CANT_COMPLETE;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaServer::stop()
{
	MaHost		*hp;
	MaListen	*lp;

	lp = (MaListen*) listens.getFirst();
	while (lp) {
		lp->close();
		lp = (MaListen*) listens.getNext(lp);
	}

	hp = (MaHost*) hosts.getFirst();
	while (hp) {
		hp->stop();
		hp = (MaHost*) hosts.getNext(hp);
	}
#if BLD_FEATURE_LICENSE
	if (expiryTimer) {
		expiryTimer->dispose();
		expiryTimer = 0;
	}
#endif	// BLD_FEATURE_LICENSE
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_LICENSE

void MaServer::calcExpiryDays()
{
	int		lastDays;

	lastDays = evalDays;

	if (expired || license) {
		return;
	}
	evalDays = BLD_FEATURE_EVAL_PERIOD - 
		(((uint) time(0) - installTime) / 86400);
	evalDays *= evalExtension;

	if (lastDays != evalDays) {
		//
		//	In evaluation mode
		//
		if (evalDays <= 0) {
			mprError(MPR_L, MPR_LOG, "The %s evaluation period is over\n"
				"Please visit http://www.mbedthis.com/ to purchase a license.",
				BLD_NAME);
			expired = 1;

		} else if (evalDays <= 7) {
			mprError(MPR_L, MPR_LOG, 
				"The %s evaluation period has %d days remaining\n"
				"Please visit http://www.mbedthis.com/ to purchase a license.",
				BLD_NAME, evalDays);
		} else {
			mprLog(MPR_CONFIG, 
				"The evaluation period has %d days remaining.\n", evalDays);
			mprLog(MPR_CONFIG,
				"Visit http://www.mbedthis.com/ to purchase a license.\n");
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

static void expiryTimeoutCheck(void *data, MprTimer *tp)
{
	MaServer 	*sp = (MaServer*) data;

	sp->calcExpiryDays();
	tp->reschedule();
}

////////////////////////////////////////////////////////////////////////////////
#endif	// BLD_FEATURE_LICENSE
//
//	Convenience function to create a new default host
//

MaHost *MaServer::newHost(char *docRoot, char *ipSpec)
{
	MaHost			*host;
	MaListen		*lp;
	MaHostAddress	*address;
	char			addrBuf[MPR_MAX_IP_ADDR_PORT], pathBuf[MPR_MAX_FNAME];
	char			*ipBufPtr;
	int				port;

	port = 0;
	if (ipSpec == 0) {
		//
		//	If no IP:PORT specified, find the first listening endpoint
		//	We expect the caller to have setup the lisenting endponts and
		//	to have added them to the host address hash.
		//
		lp = (MaListen*) listens.getFirst();
		if (lp) {
			mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", lp->getIpAddr(), lp->getPort());
		} else {
			//	Default is an ipv4 address
			mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", "localhost", MA_SERVER_DEFAULT_PORT_NUM);
			listens.insert(new MaListen("localhost", MA_SERVER_DEFAULT_PORT_NUM));
		}
		ipSpec = addrBuf;
		host = new MaHost(this, ipSpec);

	} else {
		//
		//	Create a listening endpoint
		//

		ipBufPtr = mprGetIpAddrPort(ipSpec, &port, 0);
		
		listens.insert(new MaListen(ipBufPtr, port));

		mprFree(ipBufPtr);

		host = new MaHost(this, ipSpec);
	}

	if (host->openMimeTypes("mime.types") < 0) {
		mprError(MPR_L, MPR_CONFIG, "Missing valid mime.types");
		delete host;
		return 0;
	}

	if (host->makePath(pathBuf, sizeof(pathBuf), docRoot, 1) == 0) {
		mprError(MPR_L, MPR_LOG, "DocumentRoot is too long");
		return 0;
	}

	//
	//	Insert the host and create a directory object for the docRoot
	//
	hosts.insert(host);
	host->insertDir(new MaDir(host, pathBuf));
	host->setDocumentRoot(docRoot);

	//
	//	Ensure we are in the hash lookup of all the addresses to listen to
	//	acceptWrapper uses this hash to find the host to serve the request.
	//
	address = (MaHostAddress*) hostAddresses->lookup(ipSpec);
	if (address == 0) {
		address = new MaHostAddress(ipSpec);
		hostAddresses->insert(address);
	}
	address->insertVhost(new MaVhost(host));

	if (defaultHost == 0) {
		defaultHost = host;
	}
	return host;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Called for hosts or for NameVirtualHost directives (host == 0)
//

int MaServer::createHostAddresses(MaHost *host, char *value)
{
	MaListen		*lp, *nextLp;
	MaHostAddress	*address;
	char			*ipAddrPort, *tok, *ipAddr;
	char			addrBuf[MPR_MAX_IP_ADDR_PORT];
	int				port;

	address = 0;
	ipAddrPort = mprStrTok(value, " \t", &tok);

	while (ipAddrPort) {
		if (mprStrCmpAnyCase(ipAddrPort, "_default_") == 0) {
			ipAddrPort = "*:*";
		}

		ipAddr = mprGetIpAddrPort(ipAddrPort, &port, -1);
		if (ipAddr[0] == '*') {
			ipAddr = mprStrdup("");
		}

		lp = (MaListen*) listens.getFirst();
		while (lp) {
			nextLp = (MaListen*) listens.getNext(lp);
			if (port > 0 && port != lp->getPort()) {
				lp = nextLp;
				continue;
			}
			if (lp->getIpAddr()[0] != '\0' && ipAddr[0] != '\0' && strcmp(ipAddr, lp->getIpAddr()) != 0) {
				lp = nextLp;
				continue;
			}

			if (lp->getIpAddr()[0] != '\0') {
				mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", lp->getIpAddr(), lp->getPort());
			} else {
				mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", ipAddr, lp->getPort());
			}

			if (host) {
				host->setName(addrBuf);
			}
				
			address = (MaHostAddress*) hostAddresses->lookup(addrBuf);
			if (address == 0) {
				address = new MaHostAddress(addrBuf);
				hostAddresses->insert(address);
			}
			if (host) {
				address->insertVhost(new MaVhost(host));
			} else {
				address->setNamedVhost();
			}
			lp = nextLp;
		}
		mprFree(ipAddr);
		ipAddrPort = mprStrTok(0, " \t", &tok);
	}

	if (host) {
		if (address == 0) {
			mprError(MPR_L, MPR_USER, "No valid IP address for host %s", host->getName());
			return MPR_ERR_CANT_INITIALIZE;
		}
		if (address->isNamedVhost()) {
			host->setNamedVhost();
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// 	Load an Appweb module. Returns 0 if the modules is successfully loaded
//	either statically or dynamically.
//

int MaServer::loadModule(char *name, char *libname)
{
	MaModule	*module;

	module = http->findModule(name);
	if (module == 0) {

#if BLD_FEATURE_DLL
		MaHost	*host;
		char	withExtBuf[MPR_MAX_FNAME];
		char	entryPoint[MPR_MAX_FNAME];
		char	pathBuf[MPR_MAX_FNAME];
		int		rc;

		if (libname) {
			mprSprintf(withExtBuf, sizeof(withExtBuf), "%s%s", libname, 
				MPR_DLL_EXT);
		} else {
			mprSprintf(withExtBuf, sizeof(withExtBuf), "lib%sModule%s", name, 
				MPR_DLL_EXT);
		}
		mprSprintf(entryPoint, sizeof(entryPoint), "mpr%sInit", name);
		entryPoint[3] = toupper(entryPoint[3]);

		host = getDefaultHost();
		if (locateModule(fileSystem, host, pathBuf, sizeof(pathBuf), 
				host->getModuleDirs(), withExtBuf) < 0) {
			mprError(MPR_L, MPR_LOG, "Can't find module %s", name);
			return MPR_ERR_CANT_ACCESS;
		}

		rc = mprGetMpr()->loadDll(pathBuf, entryPoint, http, 0);

		if (rc < 0) {
			if (rc == MPR_ERR_NOT_INITIALIZED) {
				mprError(MPR_L, MPR_USER, 
					"Objects are not initialized.\nPossibly loading a "
					"DLL into a statically linked program.\n"
					"You must statically link all modules.");	
			}
			// Error message already output
			return MPR_ERR_CANT_CREATE;
		}
		mprLog(MPR_CONFIG, "Activating module (Loadable) %s\n", name);
#else
		mprError(MPR_L, MPR_LOG, "Can't find module %s", name);
		return MPR_ERR_CANT_ACCESS;
#endif
	} else {
		mprLog(MPR_CONFIG, "Activating module (Builtin) %s\n", name);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_DLL

static int locateModule(MprFileSystem *fileSystem, MaHost *host, 
	char *pathBuf, int pathSize, char *moduleDirs, char *module)
{
	MprFileInfo		info;
	char			*tok, *path;
	char			*cp, dirs[MPR_MAX_FNAME], tryBuf[MPR_MAX_FNAME];
	int				sep;

	//
	//	Try the given path first without using the LoadModulePath directive.
	//
	if (host->makePath(pathBuf, pathSize, module, 0) == 0) {
		mprError(MPR_L, MPR_LOG, "Path is too long");
		return MPR_ERR_BAD_SYNTAX;
	}
	if (fileSystem->stat(pathBuf, &info) == 0) {
		return 0;
	}

	//
	//	Try using the moduleDirs path if one was specified
	//
	if (moduleDirs != 0) {
		mprStrcpy(dirs, sizeof(dirs), moduleDirs);

		/*
		 *	TODO, this may generate blanks between quoted args
		 */
		sep = strchr(dirs, '"') ? '"' : ' ';
		for (cp = dirs; *cp; cp++) {
			if (*cp == sep) {
				*cp = '\001';
			}
		}
		path = mprStrTok(dirs, "\001\n", &tok);

		while (path) {

			mprSprintf(tryBuf, sizeof(tryBuf), "%s/%s", path, 
				mprGetBaseName(module));

			if (host->makePath(pathBuf, pathSize, tryBuf, 0) != 0) {
				if (fileSystem->stat(pathBuf, &info) == 0) {
					return 0;
				}
			}
			path = mprStrTok(0, "\001\n", &tok);
		}
	}
	return MPR_ERR_CANT_ACCESS;
}

#endif	// BLD_FEATURE_DLL
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_SSL_MODULE
//
//	Called by SslModule::start
//

int MaServer::setSslListeners(MaHost *host, MaSslConfig *config)
{
	MaListen	*lp, *nextLp;
	char		*ipAddr;
	char		*ipAddr2;
	int			port;

	ipAddr = mprStrdup(host->getIpSpec());
	if (mprStrCmpAnyCase(ipAddr, "_default_") == 0) {
		ipAddr = "*:*";
	}

	ipAddr2 = mprGetIpAddrPort(ipAddr, &port, -1);

	lp = (MaListen*) listens.getFirst();
	while (lp) {
		nextLp = (MaListen*) listens.getNext(lp);
		if (port > 0 && port != lp->getPort()) {
			lp = nextLp;
			continue;
		}
		if (ipAddr2[0] != '*' && strcmp(ipAddr2, lp->getIpAddr()) != 0) {
			lp = nextLp;
			continue;
		}
		if (host->isSecure()) {
			if (host->isNamedVhost()) {
				mprError(MPR_L, MPR_LOG, 
					"SSL does not support named virtual hosts");
				mprFree(ipAddr2);
				mprFree(ipAddr);
				return MPR_ERR_CANT_INITIALIZE;
			}
			lp->setSslConfig(config);
		}
		lp = nextLp;
	}
	mprFree(ipAddr2);
	mprFree(ipAddr);
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Set the Server Root directory. We convert path into an absolute path.
//

void MaServer::setServerRoot(char *path)
{
	char	rootPath[MPR_MAX_FNAME];

	mprAssert(path && *path);

#if BLD_FEATURE_ROMFS
	mprStrcpy(rootPath, sizeof(rootPath), ".");
#else

	//
	//	Convert the server root directory path into an absolute path.
	//
	char	dir[MPR_MAX_FNAME];
	char	*cp;

	dir[sizeof(dir) - 1] = '\0';

	if (path == 0) {
#if WIN && UNUSED
		//
		//	On windows, we define a default server root to be the location 
		//	holding the executable so that we can co-locate DLLs with the 
		//	executable. 
		//
		char	program[MPR_MAX_FNAME];
		GetModuleFileName(0, program, sizeof(program) - 1);
		mprGetDirName(dir, sizeof(dir), program);
		mapPathDelim(dir);
		path = dir;
#else
		getcwd(dir, sizeof(dir) - 1);
		path = dir;
#endif

	} else if (*path != '/' && 
#if WIN
			!(path[1] == ':' && path[2] == '/')
#elif VXWORKS
			(strchr(path, ':') == 0)
#else
			1
#endif
		) {
		/*
		 *	If path is a relative path
		 */
		getcwd(dir, sizeof(dir) - 1);
		mapPathDelim(dir);
		cp = path;
		if (*cp == '.' && cp[1] == '/') {
			cp += 2;
		}
		if (*cp == '.' && cp[1] == '\0') {
			path = dir;

		} else {
			mprStrcat(dir, sizeof(dir), "/", cp, (void*) 0);
			path = dir;
		}
	}
	mprGetFullPathName(rootPath, sizeof(rootPath), path);
#endif // !BLD_FEATURE_ROMFS

	mprFree(serverRoot);
	serverRoot = mprStrdup(rootPath);

#if WIN
	//
	//	Windows is case insensitive for file names. Always map to lower case.
	//
	mprStrLower(serverRoot);
#endif
}

////////////////////////////////////////////////////////////////////////////////

char *MaServer::getServerRoot()
{
	return serverRoot;
}

////////////////////////////////////////////////////////////////////////////////

char *MaServer::getName()
{
	return name;
}

////////////////////////////////////////////////////////////////////////////////

int MaServer::splitValue(char **s1, char **s2, char *buf, int quotes)
{
	char	*next;

	if (getValue(s1, buf, &next, quotes) < 0 || 
		getValue(s2, next, &next, quotes) < 0) {
		return MPR_ERR_BAD_SYNTAX;
	}
	if (*s1 == 0 || *s2 == 0) {
		return MPR_ERR_BAD_SYNTAX;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaServer::getValue(char **arg, char *buf, char **nextToken, int quotes)
{
	char	*endp;

	if (buf == 0) {
		return -1;
	}
	while (isspace(*buf)) {
		buf++;
	}

	if (quotes && *buf == '\"') {
		*arg = ++buf;
		if ((endp = strchr(buf, '\"')) != 0) {
			*endp++ = '\0';
		} else {
			return MPR_ERR_BAD_SYNTAX;
		}
		while (isspace(*endp)) {
			endp++;
		}
		*nextToken = endp;
	} else {
		*arg = mprStrTok(buf, " \t\n", nextToken);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void acceptWrapper(void *data, MprSocket *sock, char *ipAddr, int port, 
	MprSocket *lp, int isPoolThread)
{
	MprHashTable	*addrHash;
	MaHostAddress	*address;
	MaServer		*server;
	MaRequest		*rq;
	MaHost			*host;
	char			key[MPR_MAX_IP_ADDR_PORT];

	server = (MaServer*) data;

	mprLog(3, server->tMod, "New connection from %s for %s:%d %s\n", 
		ipAddr, lp->getIpAddr(), lp->getPort(),
		lp->isSecure() ? "(secure)" : "");

	//
	//	Map the address onto a suitable host. We take the first host in the
	//	address list. Host the request initially on the first host in the 
	//	chain. If this is a vhost, the first will be the default unless 
	//	the request contains a "Host:" header which it should. If a "Host:" 
	//	header is found, the request will be reassigned to the correct 
	//	virtual host once the "Host" header is read.
	//
	addrHash = server->getHostAddresses();

	mprSprintf(key, sizeof(key), "%s:%d", lp->getIpAddr(), lp->getPort());

	address = (MaHostAddress*) addrHash->lookup(key);
	mprLog(6, "Accept connection: lookup %s\n", key);

	if (address == 0 || ((host = address->findHost(0)) == 0)) {
		mprError(MPR_L, MPR_LOG, "No host configured for request %s:%d", ipAddr, lp->getPort());
		sock->dispose();
		return;
	}

	rq = new MaRequest(address, host);
	host->insertRequest(rq);

	rq->acceptEvent(data, sock, ipAddr, port, lp, isPoolThread);

	//
	//	FUTURE -- we could poll for any I/O events here
	//
}

////////////////////////////////////////////////////////////////////////////////

MaHost *MaServer::getDefaultHost()
{
	return defaultHost;
}

////////////////////////////////////////////////////////////////////////////////

void MaServer::setDefaultHost(MaHost *host)
{
	defaultHost = host;
}

////////////////////////////////////////////////////////////////////////////////

void MaServer::setDefaultPage(char *path, char *fileName)
{
	MaHost	*host;
	MaDir	*dir;
	char	pathBuf[MPR_MAX_FNAME];

	host = (MaHost*) hosts.getFirst();
	while (host) {
		mprAssert(host);
		mprSprintf(pathBuf, sizeof(pathBuf), "%s/%s", host->getDocumentRoot(), 
			path);
		dir = host->findBestDir(pathBuf);
		mprAssert(dir);
		dir->setIndex(fileName);
		host = (MaHost*) hosts.getNext(host);
	}
}

////////////////////////////////////////////////////////////////////////////////

MaHost *MaServer::findHost(char *name)
{
	MaHost	*hp;

	hp = (MaHost*) hosts.getFirst();
	while (hp) {
		if (strcmp(hp->getName(), name) == 0) {
			return hp;
		}
		hp = (MaHost*) hosts.getNext(hp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED

bool MaServer::isNamedHost(MaHost *host)
{
	HostAddress		*ha;
	HashEntry		*ep;

	ep = hostAddresses->getFirst(&index);
	while (ep) {
		ha = (MaHostAddress*) ep->getObjectValue();
		vp = (MaVhost*) ha->vhosts.getFirst();
		while (vp) {
			if (vp->getHost() == hp) {
				if (ha->isNamedVhost()) {
					return 1;
				}
			}
			vp = (MaVhost*) ha->vhosts.getNext(vp);
		}
		ep = hostAddresses->getNext(ep, &index);
	}
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////

MprHashTable *MaServer::getHostAddresses()
{
	return hostAddresses;
}

////////////////////////////////////////////////////////////////////////////////

void MaServer::setFileSystem(MprFileSystem *fs)
{
	fileSystem = fs;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// MaHostAddress //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaHostAddress::MaHostAddress(char *ipAddrPort) : MprHashEntry(ipAddrPort)
{
	char	addrBuf[MPR_MAX_IP_ADDR_PORT];

	mprAssert(ipAddrPort && *ipAddrPort);

	flags = 0;

	ipAddr = mprGetIpAddrPort(ipAddrPort, &port, 80);

	//
	//	Reset the key as we want the port to always be in the key and not
	//	default to port 80
	//
	mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", ipAddr, port);

	setKey(addrBuf);
}

////////////////////////////////////////////////////////////////////////////////

MaHostAddress::~MaHostAddress()
{
	MaVhost	*vp, *nextVp;

	vp = (MaVhost*) vhosts.getFirst();
	while (vp) {
		nextVp = (MaVhost*) vhosts.getNext(vp);
		vhosts.remove(vp);
		delete vp;
		vp = nextVp;
	}
	mprFree(ipAddr);
}

////////////////////////////////////////////////////////////////////////////////
//
//	No locking. Only called at config time
//

void MaHostAddress::setNamedVhost()
{
	flags |= MPR_HTTP_IPADDR_VHOST;
}

////////////////////////////////////////////////////////////////////////////////
//
//	No locking. Only called at config time
//

bool MaHostAddress::isNamedVhost()
{
	return flags & MPR_HTTP_IPADDR_VHOST;
}

////////////////////////////////////////////////////////////////////////////////

void MaHostAddress::insertVhost(MaVhost *vhost)
{
	vhosts.insert(vhost);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Look for a host with the right ServerName 
//

MaHost *MaHostAddress::findHost(char *hostStr)
{
	MaVhost		*vp;
	char		*cp;
	int			len;

	if (hostStr == 0) {
		vp = (MaVhost*) vhosts.getFirst();
		return vp->host;
	}

	if ((cp = strchr(hostStr, ':')) != 0) {
		len = cp - hostStr;
	} else {
		len = strlen(hostStr);
	}

	vp = (MaVhost*) vhosts.getFirst();
	while (vp) {
		//	FUTURE -- need to support aliases
		mprLog(6, "findHost: Comparing %s to %s\n", hostStr, vp->host->getName());
		if (strncmp(hostStr, vp->host->getName(), len) == 0) {
			return vp->host;
		}
		vp = (MaVhost*) vhosts.getNext(vp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaHostAddress::getPort()
{
	return port;
}

////////////////////////////////////////////////////////////////////////////////

char *MaHostAddress::getIpAddr()
{
	return ipAddr;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaVhost ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaVhost::MaVhost(MaHost *hp)
{
	host = hp;
}

////////////////////////////////////////////////////////////////////////////////

MaVhost::~MaVhost()
{
}

////////////////////////////////////////////////////////////////////////////////

MaHost *MaVhost::getHost()
{
	return host;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// MaListen ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*
 * 	Listen on an ipAddress:port
 * 	NOTE: ipAddress may be empty which means bind to all addresses
 */

MaListen::MaListen(char *ipName, int portNum, int flags)
{
#if OLD
#if BLD_FEATURE_IPV6
	struct addrinfo 		hints, *res;
	struct sockaddr_storage	addr;
	socklen_t 				addrlen;
	char 					hbuf[MPR_MAX_IP_NAME];
	int 					rc, ipv6 = 0;
#endif
	struct hostent			*hostent;
	char					ipBuf[MPR_MAX_IP_ADDR];

	mprAssert(ipName);
	mprAssert(portNum > 0);

	this->flags = flags;
	port = portNum;
	ipAddr = mprStrdup(ipName);

#if !BLD_FEATURE_IPV6

	if (ipName && ipName[0] == '[') {
		ipv6 = 1;
		mprStrcpy(ipBuf, sizeof(ipBuf), &ipName[1]);
		mprAssert(ipBuf[strlen(ipBuf) - 1] == ']');
		ipBuf[strlen(ipBuf) - 1] = '\0';
		ipName = ipBuf;
	}
	
	/*
	 *	Convert string to a sockaddr (performing DNS lookup)
	 *
	 *	Performance: Based on my tests, on Linux, if ipName is already an 
	 *	ip addr, getaddrinfo will not contact the DNS server so there should 
	 *	not be a performance/delay issues if the DNS server is not available.
	 */
	if (ipv6) {
    	memset((char *) &hints, '\0', sizeof(hints));
    	memset((char *) &addr, '\0', sizeof(struct sockaddr_storage));
    	hints.ai_socktype = SOCK_STREAM;
    	
    	rc = getaddrinfo(ipName, NULL, &hints, &res);
    	if (rc == 0) {
    		memcpy(&addr, res->ai_addr, res->ai_addrlen);
    		addrlen = res->ai_addrlen;
    		freeaddrinfo(res);
    
    		/* 
    		 *	Convert sockaddr back to a string so that system can
    		 *	canonicalize it - hash tables may need the ip addr to
    		 *	match exactly 
    		 */
    		//	MOB -- linux specific API
    		rc = getnameinfo((struct sockaddr*) &addr, addrlen, hbuf, sizeof(hbuf),
    			NULL, 0, NI_NUMERICHOST);
    	}
    
		ipAddr = mprStrdup((rc == 0) ? hbuf: ipName);
    		
	} else 
#endif
	{
		//	ipv4.
    	if (!isdigit(*ipName)) {
    		hostent = mprGetHostByName(ipName);
    	} else {
    		hostent = 0;
    	}
    	if (hostent) {
    		mprInetNtoa(ipBuf, sizeof(ipBuf), *((struct in_addr*) hostent->h_addr_list[0]));
    		ipAddr = mprStrdup(ipBuf);
    	} else {
    		ipAddr = mprStrdup(ipName);
    	}
	}
#else

	mprAssert(ipName);
	mprAssert(portNum > 0);

	this->flags = flags;
	port = portNum;
	ipAddr = mprStrdup(ipName);

#endif /* OLD */
   
	sock = 0;

#if BLD_FEATURE_SSL_MODULE
	secure = 0;
	sslConfig = 0;
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaListen::~MaListen()
{
	mprFree(ipAddr);
	if (sock) {
		sock->dispose();
	}
}

////////////////////////////////////////////////////////////////////////////////

int MaListen::open(MaServer *sp)
{
	MaLimits	*limits;

#if BLD_FEATURE_SSL_MODULE
	if (secure && sslConfig) {
		sock = sslConfig->newSocket();
	} else
#endif
	{
		sock = new MprSocket();
	}
	if (sock->openServer(ipAddr, port, acceptWrapper, (void*) sp, MPR_SOCKET_NODELAY) < 0) {
		mprError(MPR_L, MPR_USER, "Can't open a socket on %s, port %d", ipAddr, port);
		return MPR_ERR_CANT_OPEN;
	}
	limits = sp->http->getLimits();
	if (limits->sendBufferSize > 0) {
		sock->setBufSize(limits->sendBufferSize, -1);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaListen::close()
{
	if (sock) {
		sock->forcedClose();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_SSL_MODULE

void MaListen::setSslConfig(MaSslConfig *config)
{
	secure = 1;
	sslConfig = config;
}

#endif
////////////////////////////////////////////////////////////////////////////////
#if !BLD_FEATURE_ROMFS

static void mapPathDelim(char *s)
{
	while (*s) {
		if (*s == '\\') {
			*s = '/';
		}
		s++;
	}
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
