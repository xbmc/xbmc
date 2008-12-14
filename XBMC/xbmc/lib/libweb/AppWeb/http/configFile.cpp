///
///	@file 	configFile.cpp
/// @brief 	Server Class to manage a single server
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

#if BLD_FEATURE_AUTH_MODULE
#include	"authHandler.h"
#endif
#if BLD_FEATURE_SSL_MODULE
#include	"sslModule.h"
#include	"sslModule.h"
#endif

//////////////////////////////////// Locals ////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE

struct ConfigStack {
	MaLocation	*location;
	MaDir		*dir;
	MaHost		*host;
	int			enabled;
};

////////////////////////////// Forward Declarations ////////////////////////////

#if	BLD_FEATURE_CONFIG_SAVE
static void tabs(int fd, int indent);
static void	printAuth(int fd, MaHost *host, MaAuth *auth, int indent);
#endif

////////////////////////////////////////////////////////////////////////////////
//
//	Max stack depth is:
//		Default Server			Level 1
//			<VirtualHost>		Level 2
//				<Directory>		Level 3
//					<Location>	Level 4
//

int MaServer::configure(char *configFile)
{
	ConfigStack		stack[4];
	MprFile			*file;
	MaHostAddress	*address;
	MaListen		*lp;
	MaDir			*dir, *bestDir;
	MaHost			*host, *hp;
	MprList			*aliases;
	MaAlias			*ap;
	bool			needServerName, natServerName;
	char			buf[MPR_MAX_STRING], pathBuf[MPR_MAX_FNAME];
	char			pathBuf2[MPR_MAX_FNAME], ipAddrPort[MPR_MAX_IP_ADDR_PORT];
	char			cwd[MPR_MAX_FNAME];
	char			*cp, *tok, *key, *value, *ipAddr;
	int				i, rc, top;

	top = 0;
	memset(stack, 0, sizeof(stack));

#if BLD_FEATURE_LOG
	alreadyLogging = mprGetMpr()->logService->isLogging();
#endif

	file = fileSystem->newFile();
	if (file->open(configFile, O_RDONLY | O_TEXT, 0444) < 0) {
		mprError(MPR_L, MPR_USER, "Can't open %s for config directives", configFile);
		delete file;
		return MPR_ERR_CANT_OPEN;
	}

	//
	//	Create the default host and directory
	//
	defaultHost = host = stack[top].host = new MaHost(this, 0);
	hosts.insert(host);
	stack[top].dir = new MaDir(host);
	stack[top].enabled = 1;
	host->insertDir(stack[top].dir);
	host->setName("Main Server");

	getcwd(cwd, sizeof(cwd) - 1);
	mprGetFullPathName(pathBuf, sizeof(pathBuf), cwd);
	mprLog(MPR_CONFIG, "Current Directory: \n    \"%s\"\n", pathBuf);

	//
	//	Parse each line in http.conf
	//
	for (lineNumber = 1; file->gets(buf, sizeof(buf) - 1); lineNumber++) {
		buf[sizeof(buf) - 1] = '\0';
		cp = buf;
		while (isspace(*cp)) {
			cp++;
		}
		if (*cp == '\0' || *cp == '#') {
			continue;
		}

		cp = mprStrTrim(cp, '\r');
		cp = mprStrTrim(cp, '\n');
		cp = mprStrTrim(cp, '\t');
		cp = mprStrTrim(cp, ' ');

		key = mprStrTok(cp, " \t\n", &tok);
		value = mprStrTok(0, "\n", &tok);
		if (key == 0 || *key == '\0') {
			goto syntaxErr;
		}
		if (value) {
			while (isspace(*value)) {
				value++;
			}
			if (*value) {
				cp = &value[strlen(value) - 1];
				while (cp > value && isspace(*cp)) {
					cp--;
				}
				*++cp = '\0';
			}

		} else {
			value = "";
		}


		if (*key != '<') {

			if (!stack[top].enabled) {
				mprLog(3, "Skipping key %s at line number %d\n", key, lineNumber);
				continue;
			}

			//
			//	Keywords outside of a virtual host or directory section
			//
			rc = processSetting(key, value, stack[top].host, stack[top].dir, stack[top].location);
			if (rc == 0) {
				char	*extraMsg;
				if (strcmp(key, "SSLEngine") == 0) {
					extraMsg = "\n\nFor SSL, you must have one SSL provider loaded. Make sure\nthat either OpenSSL or MatrixSSL is loaded.";
				} else {
					extraMsg = "";
				}
				mprError(MPR_L, MPR_LOG, 
					"Ignoring unknown directive \"%s\"\nAt line %d in %s\n\n"
		"Make sure the required module is loaded and the relevant handlers\n" 
		"have been added. Ensure the directive is after all LoadModule and\n"
		"AddHandler directives. %s",
					key, lineNumber, configFile, extraMsg);
				continue;
			} else if (rc < 0) {
				mprError(MPR_L, MPR_LOG, 
					"Ignoring bad directive \"%s\" at line %d in %s", 
					key, lineNumber, configFile);
			}
			continue;
		}
		
		//
		//	Directory, Location and virtual host sections
		//
		key++;
		i = strlen(key) - 1;
		if (key[i] == '>') {
			key[i] = '\0';
		}
		if (*key != '/') {
			if (!stack[top].enabled) {
				top++;
				stack[top].enabled = 0;
				stack[top].host = stack[top - 1].host;
				stack[top].location = stack[top - 1].location;
				stack[top].dir = stack[top - 1].dir;
				mprLog(3, "Skipping key %s at line number %d\n", key, lineNumber);
				continue;
			}

			i = strlen(value) - 1;
			if (value[i] == '>') {
				value[i] = '\0';
			}

			//
			//	Opening tags
			//
			if (mprStrCmpAnyCase(key, "If") == 0) {
				value = mprStrTrim(value, '\"');

				//
				//	Want to be able to nest <if> directives.
				//
				top++;
				stack[top].enabled = featureSupported(value);
				stack[top].host = stack[top - 1].host;
				stack[top].location = stack[top - 1].location;
				stack[top].dir = stack[top - 1].dir;

				if (!stack[top].enabled) {
					mprLog(3, "If \"%s\" conditional is false at line number %d\n", value, lineNumber);
				}

			} else if (mprStrCmpAnyCase(key, "VirtualHost") == 0) {

				value = mprStrTrim(value, '\"');
				host = new MaHost(this, value);
				hosts.insert(host);
				host->setVhost();

				//
				//	We currently inherit all handlers at the start
				//	We inherit directories, locations and aliases from the 
				//	defaultHost.
				//
				host->inheritHost(defaultHost);

				//
				//	All hosts have a default directory. The VirtualHost 
				//	clones its default directory from the default servers
				//	default directory. 
				//
				top++;
				stack[top].enabled = stack[top - 1].enabled;
				stack[top].host = host;
				stack[top].location = 0;
				stack[top].dir = new MaDir(host);
				host->insertDir(stack[top].dir);
				stack[top].dir->inheritUserGroup(stack[top - 1].dir->getAuth());

				if (createHostAddresses(host, value) < 0) {
					hosts.remove(host);
					delete host;
					goto err;
				}

			} else if (mprStrCmpAnyCase(key, "Directory") == 0) {
				if (stack[top].location != 0) {
					mprError(MPR_L, MPR_USER, 
						"Can't nest Directory directive inside a Location "
						"directive");
					goto err;
				}
				value = mprStrTrim(value, '\"');
				host->replaceReferences(pathBuf2, sizeof(pathBuf2), value);
				if (host->makePath(pathBuf, sizeof(pathBuf), pathBuf2) == 0) {
					mprError(MPR_L, MPR_LOG, "Directory path is too long");
					goto err;
				}

				//
				//	Create a new directory inherit parent directory settings
				//	This means inherit authorization from the enclosing host
				//
				top++;
				stack[top].enabled = stack[top - 1].enabled;
				stack[top].host = stack[top - 1].host;
				stack[top].location = 0;

				if ((dir = host->findDir(pathBuf)) != 0) {
					stack[top].dir = dir;
				} else {
					stack[top].dir = new MaDir(host, stack[top - 1].dir);
					stack[top].dir->setPath(pathBuf);
					host->insertDir(stack[top].dir);
				}

			} else if (mprStrCmpAnyCase(key, "Location") == 0) {
				//
				//	Inherit authorization from the enclosing directory
				//
				top++;
				stack[top].enabled = stack[top - 1].enabled;
				stack[top].host = stack[top - 1].host;
				stack[top].dir = stack[top - 1].dir;
				stack[top].location = new MaLocation(stack[top].dir->getAuth());

				value = mprStrTrim(value, '\"');
				stack[top].location->setPrefix(value);
				if (host->insertLocation(stack[top].location) < 0) {
					mprError(MPR_L, MPR_USER, "Can't add location %s", value);
					goto err;
				}
			}

		} else {

			key++;

			//
			//	Closing tag
			//
			if (mprStrCmpAnyCase(key, "If") == 0) {
				top--;
				host = stack[top].host;

			} else if (mprStrCmpAnyCase(key, "VirtualHost") == 0) {
				top--;
				host = stack[top].host;

			} else if (mprStrCmpAnyCase(key, "Directory") == 0) {
				top--;

			} else if (mprStrCmpAnyCase(key, "Location") == 0) {
				top--;
			}
			if (top < 0) {
				goto syntaxErr;
			}
		}
	}

	//
	//	Validate configuration -- FUTURE split this function
	//
	if (listens.getFirst() == 0) {
		mprError(MPR_L, MPR_USER, "Must have a Listen directive");
		goto err;
	}
	if (http->getHandlerServicesCount() == 0) {
		mprError(MPR_L, MPR_USER, "Must have at least one handler defined");
		goto err;
	}

	//
	//	Ensure all hosts have mime types and all Aliases in all hosts have 
	//	directory blocks. We must be careful to inherit authorization from 
	//	the best matching directory.
	//
	hp = (MaHost*) hosts.getFirst();
	while (hp) {
		if (hp->getMimeTypes() == 0) {
			if (hp->openMimeTypes("mime.types") < 0) {
				mprError(MPR_L, MPR_USER, "Missing valid mime.types");
				goto err;
			}
		}
		aliases = hp->getAliases();
		ap = (MaAlias*) aliases->getFirst();
		while (ap) {
			host->makePath(pathBuf, sizeof(pathBuf), ap->getName());
			bestDir = hp->findBestDir(pathBuf);
			if (bestDir == 0) {
				bestDir = new MaDir(hp, stack[top].dir);
				bestDir->setPath(ap->getName());
				hp->insertDir(bestDir);
			}
			ap = (MaAlias*) aliases->getNext(ap);
		}
		hp = (MaHost*) hosts.getNext(hp);
	}

	//
	//	FUTURE -- should test here that all location handlers are defined
	//
#if BLD_FEATURE_MULTITHREAD
{
	MaLimits *limits = http->getLimits();
	if (limits->maxThreads > 0) {
		mprGetMpr()->setMaxPoolThreads(limits->maxThreads);
		mprGetMpr()->setMinPoolThreads(limits->minThreads);
	}
}
#endif

	//
	//	Add default server listening addresses to the HostAddress hash.
	//	We pretend it is a vhost. Insert at the end of the vhost list so we
	//	become the default if no other vhost matches. Ie. vhosts take precedence
	//	At the same time, define a ServerName if one has not been defined. We
	//	take the first non loopback listening address.
	//
	lp = (MaListen*) listens.getFirst();
	while (lp) {
		ipAddr = lp->getIpAddr();
		mprSprintf(ipAddrPort, sizeof(ipAddrPort), "%s:%d", ipAddr, lp->getPort());
		address = (MaHostAddress*) hostAddresses->lookup(ipAddrPort);
		if (address == 0) {
			address = new MaHostAddress(ipAddrPort);
			hostAddresses->insert(address);
		}
		address->insertVhost(new MaVhost(defaultHost));
		lp = (MaListen*) listens.getNext(lp);
	}

	//
	//	Define a ServerName if one has not been defined. We take the first 
	//	non loopback listening address.
	//
	natServerName = 0;
	needServerName = strcmp(defaultHost->getName(), "Main Server") == 0;
	lp = (MaListen*) listens.getFirst();
	while (lp) {
		ipAddr = lp->getIpAddr();
		if (needServerName && *ipAddr != '\0') {
			//
			// 	Try to get the most accessible server name possible.
			//
			if (strncmp(ipAddr, "127.", 4) == 0 ||
					strncmp(ipAddr, "localhost:", 10) == 0) {
				if (! natServerName) {
					defaultHost->setName(ipAddr);
					needServerName = 0;
				}
			} else {
				if (strncmp(ipAddr, "10.", 3) == 0 ||
						strncmp(ipAddr, "192.168.", 8) == 0 ||
						strncmp(ipAddr, "172.16.", 7) == 0) {
					natServerName = 1;
				} else {
					defaultHost->setName(ipAddr);
					needServerName = 0;
				}
			}
		}
		lp = (MaListen*) listens.getNext(lp);
	}

	//
	//	Last try to setup the server name if we don't have a non-local name.
	//
	if (needServerName && !natServerName) {
		//
		//	This code is undesirable as it makes us dependent on DNS -- bad
		//
		if (natServerName) {
			mprLog(0, 
				"WARNING: Missing ServerName directive, doing DNS lookup.\n");
			char *hostName = mprGetMpr()->getServerName();
			mprSprintf(ipAddrPort, sizeof(ipAddrPort), "%s:%d", 
				hostName, ((MaListen*) listens.getFirst())->getPort());
			defaultHost->setName(hostName);
		} else {
			defaultHost->setName(defaultHost->getIpSpec());
		}
		mprLog(2, tMod, 
			"Missing ServerName directive, ServerName set to: \"%s\"\n",
			defaultHost->getName());
	}

	if (defaultHost->getDocumentRoot() == 0) {
		mprError(MPR_L, MPR_LOG, "Host %s is missing a DocumentRoot directive", 
			defaultHost->getName());
		goto err;
	}

	file->close();
	delete file;
	return 0;

syntaxErr:
	mprError(MPR_L, MPR_LOG, "Syntax error in %s at line %d", configFile, lineNumber);
	file->close();
	delete file;
	return MPR_ERR_BAD_SYNTAX;

err:
	mprError(MPR_L, MPR_LOG, "Error in %s at line %d", configFile, lineNumber);
	file->close();
	delete file;
	return MPR_ERR_BAD_SYNTAX;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Process the configuration settings. Permissible to modify key and value.
//	Return < 0 for errors, zero if directive not found, otherwise 1 is success.
//
//	FUTURE -- this function is quite big. Could be subject to a FEATURE.
//

int MaServer::processSetting(char *key, char *value, MaHost *host, MaDir *dir, MaLocation *location)
{
	MaAuth			*auth;
	MaAlias			*ap;
	MaHandler		*handler;
	MaModule		*module;
	MaLocation		*lp;
	MprList			*handlerList;
	MprList			*moduleList;
	MaLimits		*limits;
	char			addrBuf[MPR_MAX_IP_ADDR_PORT];
	char			pathBuf[MPR_MAX_FNAME], pathBuf2[MPR_MAX_FNAME];
	char			prefixBuf[MPR_MAX_FNAME];
	char			*name, *path, *prefix, *cp, *tok, *ext, *mimeType;
	char			*url, *newUrl, *from, *spec, *extensions, *codeStr;
	char			*hostName;
	int				port, rc, code, processed, num, flags;

	mprAssert(key);
	mprAssert(host);
	mprAssert(dir);

	auth = (location ? location->getAuth() : dir->getAuth());
	processed = 0;
	limits = host->getLimits();
	flags = 0;

	switch (toupper(key[0])) {
	case 'A':
		//
		//	All these string compares are not quick, but this is only done once
		//	at startup time.
		//
		if (mprStrCmpAnyCase(key, "Alias") == 0) {
			// Scope: server, host
			if (splitValue(&prefix, &path, value, 1) < 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			if (strchr(prefix, '$') != 0) {
				//
				//	Allow $ in the prefix side also
				//
				host->replaceReferences(prefixBuf, sizeof(prefixBuf), prefix);
				prefix = prefixBuf;
			}
			host->replaceReferences(pathBuf, sizeof(pathBuf), path);
			if (host->makePath(pathBuf2, sizeof(pathBuf2), pathBuf) == 0) {
				mprError(MPR_L, MPR_LOG, "Alias path is too long");
				return MPR_ERR_BAD_SYNTAX;
			}
			if (prefix[strlen(prefix) - 1] == '/' && 
				pathBuf2[strlen(pathBuf2) - 1] != '/') {
				mprError(MPR_L, MPR_LOG, 
					"May be missing a trailing '/' on the Alias path %s."
					"In configuration file at line %d", path, lineNumber);
			}
			ap = new MaAlias(prefix, pathBuf2);
			mprLog(4, tMod, "Alias: \"%s\":\n\t\t\t\"%s\"\n", 
				prefix, pathBuf2);
			if (host->insertAlias(ap) < 0) {
				mprError(MPR_L, MPR_LOG, "Can't insert alias: %s", prefix); 
				return MPR_ERR_BAD_SYNTAX;
			}
			return 1;

		} else if (mprStrCmpAnyCase(key, "AddHandler") == 0) {
			// Scope: server, host, directory
			name = mprStrTok(value, " \t", &extensions);
			if (host->addHandler(name, extensions) < 0) {
				mprError(MPR_L, MPR_LOG, "Can't add handler %s", name); 
				return MPR_ERR_CANT_CREATE;
			}
			return 1;

		} else if (mprStrCmpAnyCase(key, "AddType") == 0) {
			if (splitValue(&mimeType, &ext, value, 1) < 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			host->addMimeType(ext, mimeType);
			return 1;

		} else if (mprStrCmpAnyCase(key, "Allow") == 0) {
			if (splitValue(&from, &spec, value, 1) < 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			// spec can be: all, host, ipAddr
			auth->setAllowSpec(spec);
			return 1;
		}
		break;

	case 'B':
		if (mprStrCmpAnyCase(key, "BrowserMatch") == 0) {
			return 1;
		}
		break;

	case 'C':
		if (mprStrCmpAnyCase(key, "CustomLog") == 0) {
#if BLD_FEATURE_ACCESS_LOG && !BLD_FEATURE_ROMFS
			char *format, *end;
			if (*value == '\"') {
				end = strchr(++value, '\"');
				if (end == 0) {
					mprError(MPR_L, MPR_LOG, "Missing closing quote");
					return MPR_ERR_BAD_SYNTAX;
				}
				*end++ = '\0';
				path = value;
				format = end;

			} else {
				path = mprStrTok(value, " \t", &format);
			}
			if (path == 0 || format == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			if (host->makePath(pathBuf, sizeof(pathBuf), path) == 0) {
				mprError(MPR_L, MPR_LOG, "CustomLog path is too long");
				return MPR_ERR_BAD_SYNTAX;
			}
			host->setLog(pathBuf, mprStrTrim(format, '\"'));
			host->setLogHost(host);
#endif
			return 1;
		}
		break;

	case 'D':
		if (mprStrCmpAnyCase(key, "Deny") == 0) {
			if (splitValue(&from, &spec, value, 1) < 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			auth->setDenySpec(spec);
			return 1;

		} else if (mprStrCmpAnyCase(key, "DirectoryIndex") == 0) {
			value = mprStrTrim(value, '\"');
			if (dir == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			dir->setIndex(value);
			return 1;

		} else if (mprStrCmpAnyCase(key, "DocumentRoot") == 0) {
			value = mprStrTrim(value, '\"');
			if (dir->getPath() != 0) {
				mprError(MPR_L, MPR_LOG, 
					"DocumentRoot is already defined to be %s", pathBuf);
				return MPR_ERR_BAD_SYNTAX;
			}
			if (host->makePath(pathBuf, sizeof(pathBuf), value, 1) == 0) {
				mprError(MPR_L, MPR_LOG, "DocumentRoot is too long");
				return MPR_ERR_BAD_SYNTAX;
			}
#if !VXWORKS
			//
			//	FUTURE - VxWorks stat() is broken if using a network FTP
			//	server.
			//
			if (! fileSystem->isDir(pathBuf)) {
				mprError(MPR_L, MPR_LOG, 
					"Can't access DocumentRoot directory");
				return MPR_ERR_BAD_SYNTAX;
			}
#endif
			host->setDocumentRoot(pathBuf);
			dir->setPath(pathBuf);
			mprLog(MPR_CONFIG, "Document Root for %s:\n"
				"        \"%s\"\n", host->getName(), pathBuf);
			return 1;
		}
		break;

	case 'E':
		if (mprStrCmpAnyCase(key, "ErrorDocument") == 0) {
			codeStr = mprStrTok(value, " \t", &url);
			if (codeStr == 0 || url == 0) {
				mprError(MPR_L, MPR_LOG, "Bad ErrorDocument directive");
				return MPR_ERR_BAD_SYNTAX;
			}
			host->addErrorDocument(codeStr, url);
			return 1;

		} else if (mprStrCmpAnyCase(key, "ErrorLog") == 0) {
			path = mprStrTrim(value, '\"');
			if (path && *path) {
#if BLD_FEATURE_LOG
				if (alreadyLogging) {
					mprLog(2, tMod,
						"Already logging. Ignoring ErrorLog directive\n");
				} else {
					mprGetMpr()->logService->stop();
					if (strncmp(path, "stdout", 6) == 0) {
						mprStrcpy(pathBuf, sizeof(pathBuf), path);
					} else if (host->makePath(pathBuf, sizeof(pathBuf), 
							path) == 0) {
						mprError(MPR_L, MPR_LOG, "ErrorLog path is too long");
						return MPR_ERR_BAD_SYNTAX;
					}
					if (strlen(pathBuf) < sizeof(pathBuf) - 3) {
						strcat(pathBuf, ":2");
					}
					if (mprGetMpr()->logService->setLogSpec(pathBuf) < 0) {
						mprError(MPR_L, MPR_USER, "Can't log errors to %s\n", 
							pathBuf);
						return MPR_ERR_BAD_SYNTAX;
					}
					mprGetMpr()->logService->start();
				}
#endif
			}
			return 1;
		}
		if (mprStrCmpAnyCase(key, "ExtraPath") == 0) {
			if (mprStrCmpAnyCase(value, "on") == 0) {
				if (location == 0) {
					mprError(MPR_L, MPR_LOG, 
						"EspExtraPath only valid in Location blocks");
					return -1;
				}
				location->setFlags(MPR_HTTP_LOC_EXTRA_PATH);
			}
			return 1;
		}
		break;

	case 'G':
		if (mprStrCmpAnyCase(key, "Group") == 0) {
			value = mprStrTrim(value, '\"');
			http->setGroup(value);
			return 1;
		}
		break;

	case 'H':
		 if (mprStrCmpAnyCase(key, "HttpChunking") == 0) {
			if (mprStrCmpAnyCase(value, "on") == 0) {
				host->setChunking(1);
			} else if (mprStrCmpAnyCase(value, "off") == 0) {
				host->setChunking(0);
			}
			return 1;
		}
		break;

	case 'K':
#if BLD_FEATURE_KEEP_ALIVE
		if (mprStrCmpAnyCase(key, "KeepAlive") == 0) {
			if (mprStrCmpAnyCase(value, "on") == 0) {
				host->setKeepAlive(1);
			} else {
				host->setKeepAlive(0);
			}
			mprLog(3, tMod, "Host %s, %s: %s\n", host->getName(), key, value);
			return 1;

		} else if (mprStrCmpAnyCase(key, "KeepAliveTimeout") == 0) {
			if (! mprGetDebugMode()) {
				host->setKeepAliveTimeout(atoi(value) * 1000);
			}
			return 1;
		}
#endif
		break;
	
	case 'L':
		if (mprStrCmpAnyCase(key, "LimitClients") == 0) {
			mprGetMpr()->socketService->setMaxClients(atoi(value));
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitRequestBody") == 0) {
			num = atoi(value);
			if (num < MPR_HTTP_BOT_BODY || num > MPR_HTTP_TOP_BODY) {
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxBody = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitRequestFields") == 0) {
			num = atoi(value);
			if (num < MPR_HTTP_BOT_NUM_HEADER || num > MPR_HTTP_TOP_NUM_HEADER){
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxNumHeader = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitRequestFieldSize") == 0) {
			num = atoi(value);
			if (num < MPR_HTTP_BOT_HEADER || num > MPR_HTTP_TOP_HEADER){
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxHeader = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitRequestLine") == 0) {
			num = atoi(value);
			if (num < MPR_HTTP_BOT_FIRST_LINE || num > MPR_HTTP_TOP_FIRST_LINE){
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxFirstLine = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitResponseBody") == 0) {
			num = atoi(value);
			if (num < MPR_HTTP_BOT_RESPONSE_BODY || 
					num > MPR_HTTP_TOP_RESPONSE_BODY) {
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxResponseBody = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitScriptSize") == 0) {
			num = atoi(value);
			if (num < MPR_HTTP_BOT_SCRIPT_SIZE || 
					num > MPR_HTTP_TOP_SCRIPT_SIZE) {
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxScriptSize = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitUrl") == 0) {
			num = atoi(value);
			if (num < MPR_HTTP_BOT_URL || num > MPR_HTTP_TOP_URL){
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxUrl = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "LimitUploadSize") == 0) {
			num = atoi(value);
			if (num != -1 && 
				(num < MPR_HTTP_BOT_UPLOAD_SIZE || num > MPR_HTTP_TOP_URL)){
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxUploadSize = num;
			return 1;

#if DEPRECATED
		} else if (mprStrCmpAnyCase(key, "ListenIF") == 0) {
			MprList			*ipList;
			MprInterface	*ip;

			//
			//	Options:
			//		interface:port
			//		interface	(default port MA_SERVER_DEFAULT_PORT_NUM)
			//
			if ((cp = strchr(value, ':')) != 0) { 			// interface:port
				do { 										// find last colon
					tok = cp;
					cp = strchr(cp + 1, ':');
				} while (cp != 0);
				cp = tok;
				*cp++ ='\0';

				port = atoi(cp);
				if (port <= 0 || port > 65535) {
					mprError(MPR_L, MPR_LOG, "Bad listen port number %d", port);
					return MPR_ERR_BAD_SYNTAX;
				}

			} else {			// interface
				port = MA_SERVER_DEFAULT_PORT_NUM;
			}

			ipList = mprGetMpr()->socketService->getInterfaceList();
			ip = (MprInterface*) ipList->getFirst();
			if (ip == 0) {
				mprError(MPR_L, MPR_LOG, 
				"Can't find interfaces, use Listen-directive with IP address.");
				return MPR_ERR_BAD_SYNTAX;
			}

			while (ip) {
				if (mprStrCmpAnyCase(ip->name, value) != 0) {
					ip = (MprInterface*) ipList->getNext(ip);
					continue;
				}

				listens.insert(new MaListen(ip->ipAddr, port));
				if (host->getIpSpec() == 0) {
					mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", 
						ip->ipAddr, port);
					host->setIpSpec(addrBuf);
				}
				break;
			}
			return 1;
#endif

		} else if (mprStrCmpAnyCase(key, "Listen") == 0) {

			//
			//	Options:
			//		ipAddr:port
			//		ipAddr			default port MA_SERVER_DEFAULT_PORT_NUM
			//		port			All ip interfaces on this port
			//
			//	Where ipAddr may be [::::::] for ipv6 addresses
			//

			value = mprStrTrim(value, '\"');

			if (isdigit(*value) && strchr(value, '.') == 0 && strchr(value, ':') == 0) {
				/*
				 *	Port only, listen on all interfaces (ipv4 + ipv6)
				 */
				port = atoi(value);
				if (port <= 0 || port > 65535) {
					mprError(MPR_L, MPR_LOG, "Bad listen port number %d", port);
					return MPR_ERR_BAD_SYNTAX;
				}
				hostName = "";
				flags = MA_LISTEN_WILD_IP;

			} else {
				/*
				 * 	Hostname with possible port number
				 */
				if (*value == '[') {
					/* ipv6 */
    				hostName = value;
					cp = strchr(value, ']');
					if (cp == 0) {
    					mprError(MPR_L, MPR_LOG, "Bad IP address %s", value);
    					return MPR_ERR_BAD_SYNTAX;
					}
					if ((cp = strrchr(cp, ':')) != 0) {
						*cp++ = '\0';
						port = atoi(cp);
					} else {
						port = MA_SERVER_DEFAULT_PORT_NUM;
        				flags = MA_LISTEN_DEFAULT_PORT;
					}
					
				} else {
					/* ipv4 */
    				hostName = value;
					if ((cp = strrchr(value, ':')) != 0) {
						*cp++ = '\0';
						port = atoi(cp);
						
					} else {
						port = MA_SERVER_DEFAULT_PORT_NUM;
        				flags = MA_LISTEN_DEFAULT_PORT;
					}
				}
			}

			listens.insert(new MaListen(hostName, port, flags));

			/*
			 *	Set the host ip spec if not already set
			 */
			if (host->getIpSpec() == 0) {
				mprSprintf(addrBuf, sizeof(addrBuf), "%s:%d", hostName, port);
				host->setIpSpec(addrBuf);
			}
			return 1;

		} else if (mprStrCmpAnyCase(key, "LogFormat") == 0) {
			return 1;

		} else if (mprStrCmpAnyCase(key, "LogLevel") == 0) {
#if BLD_FEATURE_LOG
			if (alreadyLogging) {
				mprLog(2, tMod,
					"Already logging. Ignoring LogLevel directive\n");
			} else {
				value = mprStrTrim(value, '\"');
				int level = atoi(value);
				mprGetMpr()->logService->setDefaultLevel(level);
			}
#endif
			return 1;

		} else if (mprStrCmpAnyCase(key, "LoadModulePath") == 0) {
#if BLD_FEATURE_DLL
			host->setModuleDirs(value);
#endif
			return 1;

		} else if (mprStrCmpAnyCase(key, "LoadModule") == 0) {
			name = mprStrTok(value, " \t", &tok);
			if (name == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			path = mprStrTok(0, "\n", &tok);
			if (path == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			if (loadModule(name, path) < 0) {
				//	Error messages already done
				return MPR_ERR_CANT_CREATE;
			}
			return 1;
		} else if (mprStrCmpAnyCase(key, "License") == 0) {

#if BLD_FEATURE_LICENSE
			if (license != 0) {
				mprError(MPR_L, MPR_LOG, "Duplicate license key directive\n");
				return 1;
			}

			uint64	key;
			char	*product, *tok;
			int		version, serial, productBits;

			mprStrTrim(value, '\"');
			key = 0;
			tok = value;

			//
			//	Allow 2 forms:
			//		XXXX-XXXX-XXXX-XXXX (16 hex digits)
			//	or
			//		XXXXXXXXXXXXXXXX (16 hex digits)
			//
			if (strchr(tok, '-')) {
				for (int i = 0; tok && *tok && i < 4; i++) {
					key |= mprAtoi(tok, 16);
					tok = strchr(tok, '-');
					if (tok) {
						tok++;
					}
					if (i < 3) {
						key <<= 16;
					}
				}
			} else if (strlen(value) != 16) {
				key = 0;

			} else {
				uint64	high;

				int c = tok[8];
				tok[8] = '\0';
				high = mprAtoi(tok, 16);
				tok[8] = c;
				key = mprAtoi(&tok[8], 16) | high << 32;
			}

			license = 0;
			if (mprValidateLicense(key, &product, &version, &serial, 
					&productBits) < 0) {
				mprError(MPR_L, MPR_LOG, "Bad key in License directive: %Lx\n"
					"		Continuing in evaluation mode.", key);
			} else {
				//
				//	The "eval" product enables an extension to the evaluation
				//	period. evalExtension is a multiplier for the standard
				//	evaluation period.
				//
				if (strcmp(product, BLD_PRODUCT) == 0) {
					license = key;
				} else if (strcmp(product, "eval") == 0) {
					evalExtension = productBits & 0x3;
					if (evalExtension < 0 || evalExtension >= 3) {
						evalExtension = 1;
					}
				} else {
					mprError(MPR_L, MPR_LOG, "Product does not match in key\n"
						"		Continuing in evaluation mode.");
				}
				//	FUTURE: Could validate version, serial and product bits
			}
#endif
			return 1;
		}
		break;

	case 'M':
#if BLD_FEATURE_KEEP_ALIVE
		if (mprStrCmpAnyCase(key, "MaxKeepAliveRequests") == 0) {
			host->setMaxKeepAlive(atoi(value));
			return 1;
		}
#endif
		break;

	case 'N':
		if (mprStrCmpAnyCase(key, "NameVirtualHost") == 0) {
			if (createHostAddresses(0, value) < 0) {
				return -1;
			}
			return 1;
		}
		break;

	case 'O':
		if (mprStrCmpAnyCase(key, "Order") == 0) {
			if (mprStrCmpAnyCase(mprStrTrim(value, '\"'), "Allow,Deny") == 0) {
				auth->setOrder(MPR_HTTP_ALLOW_DENY);
			} else {
				auth->setOrder(MPR_HTTP_DENY_ALLOW);
			}
			return 1;
		}
		break;

	case 'P':
		if (mprStrCmpAnyCase(key, "Protocol") == 0) {
			if (strcmp(value, "HTTP/1.0") == 0) {
				host->setHttpVersion(MPR_HTTP_1_0);
			} else if (strcmp(value, "HTTP/1.1") == 0) {
				host->setHttpVersion(MPR_HTTP_1_1);
			}
			return 1;
		}
		break;

	case 'R':
		if (mprStrCmpAnyCase(key, "Redirect") == 0) {
			if (value[0] == '/' || value[0] == 'h') {
				code = 302;
				url = mprStrTok(value, " \t", &tok);

			} else if (isdigit(value[0])) {
				cp = mprStrTok(value, " \t", &tok);
				code = atoi(cp);
				url = mprStrTok(0, " \t\n", &tok);

			} else {
				cp = mprStrTok(value, " \t", &tok);
				if (strcmp(value, "permanent") == 0) {
					code = 301;
				} else if (strcmp(value, "temp") == 0) {
					code = 302;
				} else if (strcmp(value, "seeother") == 0) {
					code = 303;
				} else if (strcmp(value, "gone") == 0) {
					code = 410;
				} else {
					return MPR_ERR_BAD_SYNTAX;
				}
				url = mprStrTok(0, " \t\n", &tok);
			}
			if (code >= 300 && code <= 399) {
				newUrl = mprStrTok(0, "\n", &tok);
			} else {
				newUrl = "";
			}
			if (code <= 0 || url == 0 || newUrl == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			url = mprStrTrim(url, '\"');
			newUrl = mprStrTrim(newUrl, '\"');
			mprLog(4, tMod, 
				"insertAlias: Redirect %d from \"%s\" to \"%s\"\n", 
				code, url, newUrl);
			ap = new MaAlias(url, newUrl, code);
			host->insertAlias(ap);
			return 1;

		} else if (mprStrCmpAnyCase(key, "ResetHandlers") == 0) {
			host->deleteHandlers();
			return 1;
		}
		break;

	case 'S':
		if (mprStrCmpAnyCase(key, "ScriptAlias") == 0) {
			if (splitValue(&prefix, &path, value, 1) < 0 || path == 0 || 
					prefix == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			//
			//	Create an alias, dir and location with a cgiHandler
			//
			host->replaceReferences(pathBuf, sizeof(pathBuf), path);
			ap = new MaAlias(prefix, pathBuf);
			mprLog(4, tMod, "ScriptAlias \"%s\":\n\t\t\t\"%s\"\n", 
				prefix, pathBuf);
			host->insertAlias(ap);
			if ((lp = host->findLocation(prefix)) == 0) {
				lp = new MaLocation(dir->getAuth());
				lp->setPrefix(prefix);
				host->insertLocation(lp);
			} else {
				lp->setPrefix(prefix);
			}
			lp->setFlags(MPR_HTTP_LOC_EXTRA_PATH);
			lp->setHandler("cgiHandler");
#if MOVED
			if (host->findDir(pathBuf) == 0) {
				dp = new MaDir(host, dir);
				dp->setPath(pathBuf);
				host->insertDir(dp);
			}
#endif
			return 1;

		} else if (mprStrCmpAnyCase(key, "SendBufferSize") == 0) {
			if (value == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			num = atoi(value);
			if (num < MPR_HTTP_BOT_SEND_BUFFER || 
					num > MPR_HTTP_TOP_SEND_BUFFER) {
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->sendBufferSize = num;
			return 1;

		} else if (mprStrCmpAnyCase(key, "ServerName") == 0) {
			value = mprStrTrim(value, '\"');
			if (strncmp(value, "http://", 7) == 0) {
				host->setName(&value[7]);
			} else {
				host->setName(value);
			}
			return 1;

		} else if (mprStrCmpAnyCase(key, "ServerRoot") == 0) {
			value = mprStrTrim(value, '\"');
			if (host->makePath(pathBuf, sizeof(pathBuf), value, 1) == 0) {
				mprError(MPR_L, MPR_LOG, "ServerRoot is too long");
				return MPR_ERR_BAD_SYNTAX;
			}
#if !VXWORKS
			//
			//	FUTURE - VxWorks stat() is broken if using a network FTP
			//	server.
			//
			if (! fileSystem->isDir(pathBuf)) {
				mprError(MPR_L, MPR_LOG, "Can't access ServerRoot directory");
				return MPR_ERR_BAD_SYNTAX;
			}
#endif
			setServerRoot(pathBuf);
#if BLD_FEATURE_ROMFS
			mprLog(MPR_CONFIG, "Server Root \"%s\" in ROM\n", pathBuf);
#else
			mprLog(MPR_CONFIG, "Server Root \"%s\"\n", pathBuf);
#endif
			return 1;

#if BLD_FEATURE_SESSION
		} else if (mprStrCmpAnyCase(key, "SessionTimeout") == 0) {
			if (value == 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			if (! mprGetDebugMode()) {
				host->setSessionTimeout(atoi(mprStrTrim(value, '\"')));
			}
			return 1;

		} else if (mprStrCmpAnyCase(key, "SessionAutoCreate") == 0) {
			if (mprStrCmpAnyCase(value, "on") == 0) {
				host->setSessionAutoCreate(1);
			}
			return 1;
		
#endif
			
		} else if (mprStrCmpAnyCase(key, "SetHandler") == 0) {
			if (location == 0) {
				mprError(MPR_L, MPR_LOG, 
					"SetHandler only valid in Location blocks");
				return MPR_ERR_BAD_SYNTAX;
			}
			value = mprStrTrim(value, '\"');
			if (http->lookupHandlerService(value) == 0) {
				mprError(MPR_L, MPR_LOG, "Can't find handler service %s", 
					value);
				return MPR_ERR_BAD_SYNTAX;
			}
			location->setHandler(mprStrTrim(value, '\"'));
			return 1;

		} else if (mprStrCmpAnyCase(key, "StartThreads") == 0) {
#if BLD_FEATURE_MULTITHREAD
			num = atoi(value);
			if (num < 0 || num > MPR_HTTP_TOP_THREADS) {
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->minThreads = num;
#endif
			return 1;
		}
		break;

	case 'T':
		if (mprStrCmpAnyCase(key, "ThreadLimit") == 0) {
#if BLD_FEATURE_MULTITHREAD
			num = atoi(value);
			if (num < 0 || num > MPR_HTTP_TOP_THREADS) {
				return MPR_ERR_BAD_SYNTAX;
			}
			limits->maxThreads = num;
#endif
			return 1;

		} else if (mprStrCmpAnyCase(key, "ThreadStackSize") == 0) {
#if BLD_FEATURE_MULTITHREAD
			num = atoi(value);
			if (num < MPR_HTTP_BOT_STACK || num > MPR_HTTP_TOP_STACK) {
				return MPR_ERR_BAD_SYNTAX;
			}
			mprGetMpr()->poolService->setStackSize(num);
			return 1;
#endif

		} else if (mprStrCmpAnyCase(key, "TimeOut") == 0) {
			if (! mprGetDebugMode()) {
				host->setTimeout(atoi(value) * 1000);
			}
			return 1;

		} else if (mprStrCmpAnyCase(key, "TraceMethod") == 0) {
			if (mprStrCmpAnyCase(value, "on") == 0) {
				host->setTraceMethod(1);
			} else {
				host->setTraceMethod(0);
			}
			return 1;

		} else if (mprStrCmpAnyCase(key, "TypesConfig") == 0) {
			path = mprStrTrim(value, '\"');
			if (host->makePath(pathBuf, sizeof(pathBuf), path, 1) == 0) {
				mprError(MPR_L, MPR_LOG, "TypesConfig path is too long");
				return MPR_ERR_BAD_SYNTAX;
			}
			if (host->openMimeTypes(pathBuf) < 0) {
				return MPR_ERR_BAD_SYNTAX;
			}
			return 1;
		}
		break;
	
	case 'U':
		if (mprStrCmpAnyCase(key, "User") == 0) {
			http->setUser(mprStrTrim(value, '\"'));
			return 1;
		}
		break;
	}

	rc = 0;
	handlerList = host->getHandlers();
	handler = (MaHandler*) handlerList->getFirst();
	while (handler) {
		rc = handler->parseConfig(key, value, this, host, auth, dir, location);
		if (rc < 0) {
			return rc;
		} else if (rc > 0) {
			break;
		}
		handler = (MaHandler*) handlerList->getNext(handler);
	}

	if (rc == 0) {
		moduleList = http->getModules();
		module = (MaModule*) moduleList->getFirst();
		while (module) {
			rc = module->parseConfig(key, value, this, host, auth, dir, 
				location);
			if (rc < 0) {
				return rc;
			} else if (rc > 0) {
				break;
			}
			module = (MaModule*) moduleList->getNext(module);
		}
	}
	return rc;
}

////////////////////////////////////////////////////////////////////////////////

int MaServer::featureSupported(char *key)
{
	if (mprStrCmpAnyCase(key, "BLD_COMMERCIAL") == 0) {
		return strcmp(BLD_COMMERCIAL, "0") == 0;

	} else if (mprStrCmpAnyCase(key, "BLD_DEBUG") == 0) {
		return BLD_DEBUG;

	} else if (mprStrCmpAnyCase(key, "ACCESS_LOG") == 0) {
		return BLD_FEATURE_ACCESS_LOG;

	} else if (mprStrCmpAnyCase(key, "ADMIN_MODULE") == 0) {
		return BLD_FEATURE_ADMIN_MODULE;

	} else if (mprStrCmpAnyCase(key, "AUTH_MODULE") == 0) {
		return BLD_FEATURE_AUTH_MODULE;

	} else if (mprStrCmpAnyCase(key, "CGI_MODULE") == 0) {
		return BLD_FEATURE_CGI_MODULE;

	} else if (mprStrCmpAnyCase(key, "COPY_MODULE") == 0) {
		return BLD_FEATURE_COPY_MODULE;

	} else if (mprStrCmpAnyCase(key, "C_API_MODULE") == 0) {
		return BLD_FEATURE_C_API_MODULE;

	} else if (mprStrCmpAnyCase(key, "DIGEST") == 0) {
		return BLD_FEATURE_DIGEST;

	} else if (mprStrCmpAnyCase(key, "DIR_MODULE") == 0) {
		return BLD_FEATURE_DIR_MODULE;

	} else if (mprStrCmpAnyCase(key, "DOC") == 0) {
		return BLD_FEATURE_DOC;

	} else if (mprStrCmpAnyCase(key, "EGI_MODULE") == 0) {
		return BLD_FEATURE_EGI_MODULE;

	} else if (mprStrCmpAnyCase(key, "ESP_MODULE") == 0) {
		return BLD_FEATURE_ESP_MODULE;

	} else if (mprStrCmpAnyCase(key, "GACOMPAT_MODULE") == 0) {
		return BLD_FEATURE_GACOMPAT_MODULE;

	} else if (mprStrCmpAnyCase(key, "IF_MODIFIED") == 0) {
		return BLD_FEATURE_IF_MODIFIED;

	} else if (mprStrCmpAnyCase(key, "IPV6") == 0) {
		return BLD_FEATURE_IPV6;

	} else if (mprStrCmpAnyCase(key, "KEEP_ALIVE") == 0) {
		return BLD_FEATURE_KEEP_ALIVE;

	} else if (mprStrCmpAnyCase(key, "LOG") == 0) {
		return BLD_FEATURE_LOG;

#if BLD_FEATURE_MATRIXSSL_MODULE
	} else if (mprStrCmpAnyCase(key, "MATRIXSSL_MODULE") == 0) {
		return BLD_FEATURE_MATRIXSSL_MODULE;
#endif

	} else if (mprStrCmpAnyCase(key, "MULTITHREAD") == 0) {
		return BLD_FEATURE_MULTITHREAD;

#if BLD_FEATURE_OPENSSL_MODULE
	} else if (mprStrCmpAnyCase(key, "OPENSSL_MODULE") == 0) {
		return BLD_FEATURE_OPENSSL_MODULE;
#endif

#if BLD_FEATURE_PHP5_MODULE
	} else if (mprStrCmpAnyCase(key, "PHP5_MODULE") == 0) {
		return BLD_FEATURE_PHP5_MODULE;
#endif

	} else if (mprStrCmpAnyCase(key, "PUT_MODULE") == 0) {
		return BLD_FEATURE_PUT_MODULE;

	} else if (mprStrCmpAnyCase(key, "RANGES") == 0) {
		return BLD_FEATURE_RANGES;

	} else if (mprStrCmpAnyCase(key, "SAMPLES") == 0) {
		return BLD_FEATURE_SAMPLES;

	} else if (mprStrCmpAnyCase(key, "SESSION") == 0) {
		return BLD_FEATURE_SESSION;

	} else if (mprStrCmpAnyCase(key, "SSL_MODULE") == 0) {
		return BLD_FEATURE_SSL_MODULE;

	} else if (mprStrCmpAnyCase(key, "SHARED") == 0) {
		return BLD_FEATURE_SHARED;

	} else if (mprStrCmpAnyCase(key, "STATIC") == 0) {
		return BLD_FEATURE_STATIC;

	} else if (mprStrCmpAnyCase(key, "TEST") == 0) {
		return BLD_FEATURE_TEST;

	} else if (mprStrCmpAnyCase(key, "UPLOAD_MODULE") == 0) {
		return BLD_FEATURE_UPLOAD_MODULE;
	}

//	mprError(MPR_L, MPR_LOG, "Unknown conditional %s", key); 
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_SAVE
//
//	Save the configuration to the named config file
//

int MaServer::saveConfig(char *configFile)
{
	MaAlias			*ap;
	MaDir			*dp, *defaultDp;
	MprFile			out;
	MaHandler		*hanp;
	MaHost			*host, *defaultHost;
	MaLimits		*limits;
	MaListen		*lp;
	MaLocation		*loc;
	MaMimeHashEntry	*mt;
	MprHashTable	*mimeTypes;
	MprList			*aliases;
	char			*ext, *path, *cp, *mimeFile;
	char			*hostName, *actionProgram;
	int				fd, indent, flags, first, code;
#if BLD_FEATURE_LOG
	MprLogService	*logService;
	char			*logSpec;
#endif

	indent = 0;
	host = 0;
	defaultHost = (MaHost*) hosts.getFirst();

	fd = open(configFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0666);
	if (fd < 0) {
		mprLog(0, "saveConfig: Can't open %s\n", configFile);
		return MPR_ERR_CANT_OPEN;
	}

	mprFprintf(fd, \
	"#\n"
	"#	Configuration for %s\n"
	"#\n"
	"#	This file is dynamically generated. If you edit this file, your\n"
	"#	changes may not be preserved by the manager. PLEASE keep a backup of\n"
	"#	the file before and after all manual changes.\n"
	"#\n"
	"#	The order of configuration directives matters as this file is parsed\n"
	"#	only once. You must put the server root and error log definitions\n"
	"#	first ensure configuration errors are logged.\n"
	"#\n\n", BLD_NAME);

	mprFprintf(fd, "ServerRoot \"%s\"\n", serverRoot);

#if BLD_FEATURE_LOG
	logService = mprGetMpr()->logService;
	logSpec = mprStrdup(logService->getLogSpec());
	if ((cp = strchr(logSpec, ':')) != 0) {
		*cp = '\0';
	}
	mprFprintf(fd, "ErrorLog \"%s\"\n", logSpec);
	mprFprintf(fd, "LogLevel %d\n", logService->getDefaultLevel());
#endif

#if BLD_FEATURE_LICENSE
	if (license) {
		mprPrintf("%010Ld %010Ld\n", license & 0xFFFFFFFF, license >> 32);
	}
#endif

	//
	//	Listen directives
	//
	lp = (MaListen*) listens.getFirst();
	while (lp) {
		flags = lp->getFlags();
		if (flags & MA_LISTEN_DEFAULT_PORT) {
			mprFprintf(fd, "Listen %s # %d\n", lp->getIpAddr(), lp->getPort());
		} else if (flags & MA_LISTEN_WILD_IP) {
			mprFprintf(fd, "Listen %d\n", lp->getPort());
		} else if (flags & MA_LISTEN_WILD_IP2) {
			//	Ignore
		} else {
			if (strchr(lp->getIpAddr(), '.') != 0) {
				mprFprintf(fd, "Listen %s:%d\n", lp->getIpAddr(), 
					lp->getPort());
			} else {
				mprFprintf(fd, "Listen [%s]:%d\n", lp->getIpAddr(), 
					lp->getPort());
			}
		}
		lp = (MaListen*) listens.getNext(lp);
	}

	//
	//	Global directives
	//
	mprFprintf(fd, "User %s\n", http->getUser());
	mprFprintf(fd, "Group %s\n", http->getGroup());

#if BLD_FEATURE_DLL
	MaModule	*mp;
	MprList		*modules;
	mprFprintf(fd, "\n#\n#  Loadable Modules\n#\n");
	mprFprintf(fd, "LoadModulePath %s\n", defaultHost->getModuleDirs());
	modules = http->getModules();
	mp = (MaModule*) modules->getFirst();
	while (mp) {
		mprFprintf(fd, "LoadModule %s lib%sModule\n", mp->name, mp->name);
		mp = (MaModule*) modules->getNext(mp);
	}
#endif

	//
	//	For clarity, always print the ThreadLimit even if default.
	//
	limits = http->getLimits();
	mprFprintf(fd, "ThreadLimit %d\n", limits->maxThreads);
	if (limits->threadStackSize != 0) {
		mprFprintf(fd, "ThreadStackSize %d\n", limits->threadStackSize);
	}
	if (limits->minThreads != 0) {
		mprFprintf(fd, "\nStartThreads %d\n", limits->minThreads);
	}
	if (limits->maxBody != MPR_HTTP_MAX_BODY) {
		mprFprintf(fd, "LimitRequestBody %d\n", limits->maxBody);
	}
	if (limits->maxResponseBody != MPR_HTTP_MAX_RESPONSE_BODY) {
		mprFprintf(fd, "LimitResponseBody %d\n", limits->maxResponseBody);
	}
	if (limits->maxScriptSize != MPR_HTTP_MAX_SCRIPT_SIZE) {
		mprFprintf(fd, "LimitScriptSize %d\n", limits->maxScriptSize);
	}
	if (limits->maxNumHeader != MPR_HTTP_MAX_NUM_HEADER) {
		mprFprintf(fd, "LimitRequestFields %d\n", limits->maxNumHeader);
	}
	if (limits->maxHeader != MPR_HTTP_MAX_HEADER) {
		mprFprintf(fd, "LimitRequestFieldSize %d\n", limits->maxHeader);
	}
	if (limits->maxFirstLine != MPR_HTTP_MAX_FIRST_LINE) {
		mprFprintf(fd, "LimitRequestLine %d\n", limits->maxFirstLine);
	}
	if (limits->maxUrl != MPR_HTTP_MAX_URL) {
		mprFprintf(fd, "LimitUrl %d\n", limits->maxUrl);
	}
	if (limits->maxUploadSize != MPR_HTTP_MAX_UPLOAD_SIZE) {
		mprFprintf(fd, "LimitUploadSize %d\n", limits->maxUploadSize);
	}
	if (limits->sendBufferSize != 0) {
		mprFprintf(fd, "SendBufferSize %d\n", limits->sendBufferSize);
	}
	//
	//	Virtual hosts. The first host is the default server
	//
	host = (MaHost*) hosts.getFirst();
	while (host) {
		mprFprintf(fd, "\n");
		if (host->isVhost()) {
			if (host->isNamedVhost()) {
				mprFprintf(fd, "NameVirtualHost %s\n", host->getIpSpec());
			}
			mprFprintf(fd, "<VirtualHost %s>\n", host->getIpSpec());
			indent++;
		}

		hostName = host->getName();
		if (strcmp(hostName, "default") != 0) {
			tabs(fd, indent);
			mprFprintf(fd, "ServerName http://%s\n", hostName);
		}
	
		tabs(fd, indent);
		mprFprintf(fd, "DocumentRoot %s\n", host->getDocumentRoot());

		//
		//	Handlers	
		//
		flags = host->getFlags();
		if (flags & MPR_HTTP_ADD_HANDLER) {
			mprFprintf(fd, "\n");
			if (flags & MPR_HTTP_RESET_HANDLERS) {
				tabs(fd, indent);
				mprFprintf(fd, "ResetHandlers\n");
			}
			hanp = (MaHandler*) host->getHandlers()->getFirst();
			while (hanp) {
				ext = (char*) (hanp->getExtensions() ? 
					hanp->getExtensions() : "");
				tabs(fd, indent);
				mprFprintf(fd, "AddHandler %s %s\n", hanp->getName(), ext);
				hanp = (MaHandler*) host->getHandlers()->getNext(hanp);
			}
		}
		
#if BLD_FEATURE_SSL_MODULE
		//
		//	SSL configuration
		//
		if (host->isSecure()) {
			MaSslConfig *sslConfig;
			MaSslModule	*sslModule;

			mprFprintf(fd, "\n");
			tabs(fd, indent);
			mprFprintf(fd, "SSLEngine on\n");
			sslModule = (MaSslModule*) http->findModule("ssl");
			if (sslModule != 0) {
				sslConfig = sslModule->getSslConfig(host->getName());
				if (sslConfig != 0) {

					tabs(fd, indent);
					mprFprintf(fd, "SSLCipherSuite %s\n",
						sslConfig->getCipherSuite());

					tabs(fd, indent);
					mprFprintf(fd, "SSLProtocol ");
					int protoMask = sslConfig->getSslProto();
					if (protoMask == MPR_HTTP_PROTO_ALL) {
						mprFprintf(fd, "ALL");
					} else if (protoMask == 
						(MPR_HTTP_PROTO_ALL & ~MPR_HTTP_PROTO_SSLV2)) {
						mprFprintf(fd, "ALL -SSLV2");
					} else {
						if (protoMask & MPR_HTTP_PROTO_SSLV2) {
							mprFprintf(fd, "SSLv2 ");
						}
						if (protoMask & MPR_HTTP_PROTO_SSLV3) {
							mprFprintf(fd, "SSLv3 ");
						}
						if (protoMask & MPR_HTTP_PROTO_TLSV1) {
							mprFprintf(fd, "TLSv1 ");
						}
					}
					mprFprintf(fd, "\n");

					if ((path = sslConfig->getCertFile()) != 0) {
						tabs(fd, indent);
						mprFprintf(fd, "SSLCertificateFile %s\n", path);
					}
					if ((path = sslConfig->getKeyFile()) != 0) {
						tabs(fd, indent);
						mprFprintf(fd, "SSLCertificateKeyFile %s\n", path);
					}
					if (sslConfig->getVerifyClient()) {
						tabs(fd, indent);
						mprFprintf(fd, "SSLVerifyClient require\n");
						if ((path = sslConfig->getCaFile()) != 0) {
							tabs(fd, indent);
							mprFprintf(fd, "SSLCaCertificateFile %s\n", path);
						}
						if ((path = sslConfig->getCaPath()) != 0) {
							tabs(fd, indent);
							mprFprintf(fd, "SSLCertificatePath %s\n", path);
						}
					}
				}
			}
		}
#endif
#if BLD_FEATURE_KEEP_ALIVE
		//
		//	General per-host directives
		//
		if (! host->getKeepAlive()) {
			tabs(fd, indent);
			mprFprintf(fd, "KeepAlive off\n");
		} else {
			if (host->getMaxKeepAlive() != defaultHost->getMaxKeepAlive()) {
				tabs(fd, indent);
				mprFprintf(fd, "MaxKeepAliveRequests %d\n", 
					host->getMaxKeepAlive());
			}
			if (host->getKeepAliveTimeout() !=
					defaultHost->getKeepAliveTimeout()) {
				tabs(fd, indent);
				mprFprintf(fd, "KeepAliveTimeout %d\n", 
					host->getKeepAliveTimeout() / 1000);
			}
		}
#endif
		mimeFile = host->getMimeFile();
		if (mimeFile && *mimeFile) {
			mprFprintf(fd, "TypesConfig %s\n", mimeFile);
		}
		if (host->getTimeout() != defaultHost->getTimeout()) {
			tabs(fd, indent);
			mprFprintf(fd, "Timeout %d\n", host->getTimeout() / 1000);
		}

#if BLD_FEATURE_SESSION
		if (host->getSessionTimeout() != defaultHost->getSessionTimeout()) {
			tabs(fd, indent);
			mprFprintf(fd, "SessionTimeout %d\n", host->getSessionTimeout());
		}
#endif
#if BLD_FEATURE_ACCESS_LOG && !BLD_FEATURE_ROMFS
		if (host->getLogHost() == host) {
			char	format[MPR_MAX_FNAME * 2];
			char	*fp;
			fp = format;
			format[0] = '\0';
			for (cp = host->getLogFormat(); 
					cp && *cp && fp < &format[sizeof(format) - 4]; cp++) {
				if (*cp == '\"') {
					*fp++ = '\\';
					*fp++ = *cp;
				} else {
					*fp++ = *cp;
				}
			}
			*fp++ = '\0';
			tabs(fd, indent);
			mprFprintf(fd, "CustomLog %s \"%s\"\n", host->getLogPath(), format);
		}
#endif

		//
		//	ActionPrograms. One mimeTypes table is shared among all hosts.
		//
		if (host == defaultHost) {
			mimeTypes = host->getMimeTypes();
			mt = (MaMimeHashEntry*) mimeTypes->getFirst();
			first = 1;
			while (mt) {
				actionProgram = mt->getActionProgram();
				if (actionProgram && *actionProgram) {
					if (first) {
						mprFprintf(fd, "\n");
						first = 0;
					}
					tabs(fd, indent);
					mprFprintf(fd, "Action %s %s\n", mt->getMimeType(),
						mt->getActionProgram());
				}
				mt = (MaMimeHashEntry*) mimeTypes->getNext(mt);
			}
		}

		//
		//	Aliases
		//
		aliases = host->getAliases();
		ap = (MaAlias*) aliases->getFirst();
		first = 1;
		while (ap) {
			//
			//	Must skip the catchall alias which has an empty prefix
			//
			if (ap->getPrefix()[0] != '\0' && !ap->isInherited()) {
				if (first) {
					mprFprintf(fd, "\n");
					first = 0;
				}
				tabs(fd, indent);
				code = ap->getRedirectCode();
				if (code != 0) {
					mprFprintf(fd, "Redirect %d %s \"%s\"\n", 
						code, ap->getPrefix(), ap->getName());
				} else {
					mprFprintf(fd, "Alias %s \"%s\"\n", 
						ap->getPrefix(), ap->getName());
				}
			}
			ap = (MaAlias*) aliases->getNext(ap);
		}

		//
		//	Directories -- Do in reverse order
		//
		defaultDp = dp = (MaDir*) host->getDirs()->getLast();
		first = 1;
		while (dp) {
			if (dp->isInherited()) {
				dp = (MaDir*) host->getDirs()->getPrev(dp);
				continue;
			}
			path = dp->getPath();
			if (*path) {
				if (!first) {
					mprFprintf(fd, "\n");
					tabs(fd, indent++);
					mprFprintf(fd, "<Directory %s>\n", dp->getPath());
				}
			}
			if (strcmp(dp->getIndex(), defaultDp->getIndex()) != 0) {
				tabs(fd, indent);
				mprFprintf(fd, "DirectoryIndex %s\n", dp->getIndex());
			}

			printAuth(fd, host, dp, indent);

			if (*path && !first) {
				tabs(fd, --indent);
				mprFprintf(fd, "</Directory>\n");
			}
			first = 0;
			dp = (MaDir*) host->getDirs()->getPrev(dp);
		}

		//
		//	Locations
		//
		loc = (MaLocation*) host->getLocations()->getLast();
		while (loc) {
			if (loc->isInherited()) {
				loc = (MaLocation*) host->getLocations()->getPrev(loc);
				continue;
			}
			mprFprintf(fd, "\n");
			tabs(fd, indent++);
			mprFprintf(fd, "<Location %s>\n", loc->getPrefix());

			if (loc->getHandlerName()) {
				tabs(fd, indent);
				mprFprintf(fd, "SetHandler %s\n", loc->getHandlerName());
			}

			printAuth(fd, host, loc, indent);
				
			tabs(fd, --indent);
			mprFprintf(fd, "</Location>\n");

			loc = (MaLocation*) host->getLocations()->getPrev(loc);
		}

		//
		//	Close out the VirtualHosts
		//
		if (host->isVhost()) {
			tabs(fd, --indent);
			mprFprintf(fd, "</VirtualHost>\n");
		}
		host = (MaHost*) hosts.getNext(host);
	}
	close(fd);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Print Authorization configuration
//

static void printAuth(int fd, MaHost *host, MaAuth *auth, int indent)
{
	MaAuthType		authType;
	MaAcl			acl;
	char			*users, *groups, *realm;
#if BLD_FEATURE_AUTH_MODULE
	MaAuthHandler	*handler;
#endif

	if (auth->isAuthInherited()) {
		return;
	}

#if BLD_FEATURE_AUTH_MODULE
	handler = (MaAuthHandler*) host->lookupHandler("auth");
	if (handler) {
		char	*path;
		path = handler->getGroupFile();
		if (path) {
			tabs(fd, indent);
			mprFprintf(fd, "AuthGroupFile %s\n", path);
		}
		path = handler->getUserFile();
		if (path) {
			tabs(fd, indent);
			mprFprintf(fd, "AuthUserFile %s\n", path);
		}
	}
#endif

	authType = auth->getType();
	if (authType == MPR_HTTP_AUTH_BASIC) {
		tabs(fd, indent);
		mprFprintf(fd, "AuthType basic\n");
	} else if (authType == MPR_HTTP_AUTH_DIGEST) {
		char *qop = auth->getQop();

		tabs(fd, indent);
		mprFprintf(fd, "AuthType digest\n");
		tabs(fd, indent);
		if (qop && *qop) {
			mprFprintf(fd, "AuthDigestQop %s\n", qop);
		}
	}

	realm = auth->getRealm();
	groups = auth->getRequiredGroups();
	users = auth->getRequiredUsers();
	acl = auth->getRequiredAcl();

	if (realm && *realm) {
		tabs(fd, indent);
		mprFprintf(fd, "AuthName \"%s\"\n", realm);
	}
	if (auth->getAnyValidUser()) {
		tabs(fd, indent);
		mprFprintf(fd, "Require valid-user\n");
	} else if (groups && *groups) {
		tabs(fd, indent);
		mprFprintf(fd, "Require group %s\n", groups);
	} else if (users && *users) {
		tabs(fd, indent);
		mprFprintf(fd, "Require user %s\n", users);
	} else if (acl) {
		tabs(fd, indent);
		mprFprintf(fd, "Require acl 0x%x\n", acl);
	}
}

////////////////////////////////////////////////////////////////////////////////

static void tabs(int fd, int indent)
{
	for (int i = 0; i < indent; i++) {
		write(fd, "\t", 1);
	}
}

#endif	// BLD_FEATURE_CONFIG_SAVE
////////////////////////////////////////////////////////////////////////////////

#else 	// BLD_FEATURE_CONFIG_PARSE
void mprServerConfigDummy() {};
#endif 	// BLD_FEATURE_CONFIG_PARSE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
