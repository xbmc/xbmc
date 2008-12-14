///
///	@file 	handler.cpp
/// @brief 	Support loadable URL handlers 
/// @overview URL handlers allow the extensible and modular processing 
///		of URLs. Handlers can be defined to run based on a URLs 
///		extension or leading URL prefix. This modules provides a base 
///		class used by all handlers.
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

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaHandlerService /////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaHandlerService::MaHandlerService(const char *name)
{
	handlerName = mprStrdup(name);
	maGetHttp()->insertHandlerService(this);
}

////////////////////////////////////////////////////////////////////////////////

MaHandlerService::~MaHandlerService()
{
	mprFree(handlerName);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual function -- must be overriddenn by handlerService super class
//

MaHandler *MaHandlerService::newHandler(MaServer *server, MaHost *host, 
	char *ext)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual function -- must be overriddenn by handlerService super class
//

int MaHandlerService::start()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual function -- must be overriddenn by handlerService super class
//

int MaHandlerService::stop()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Handler ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaHandler::MaHandler(char *name)
{
	extensions = 0;
	flags = 0;
	hitCount = 0;
	host = 0;
	this->name = mprStrdup(name);
}

////////////////////////////////////////////////////////////////////////////////

MaHandler::MaHandler(char *name, char *ext, int flags)
{
	this->name = mprStrdup(name);
	this->flags = flags;
	this->host = host;
	hitCount = 0;
	host = 0;

	if (ext) {
		extensions = mprStrdup(ext);
#if WIN
		mprStrLower(extensions);
#endif
		extList.parse(extensions);
	} else {
		extensions = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

MaHandler::~MaHandler()
{
	mprFree(name);
	mprFree(extensions);
}

////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_CONFIG_PARSE

int MaHandler::parseConfig(char *key, char *value, MaServer *server, 
	MaHost *host, MaAuth *auth, MaDir *dir, MaLocation *location)
{
	return 0;
}

#endif
////////////////////////////////////////////////////////////////////////////////
//
//	Virtual method, may be overridden by handlers that wish more complex
//	matching criteria (E.g. matching based on header values
//

int MaHandler::matchRequest(MaRequest *rq, char *uri, int uriLen)
{
	MprStringData	*sd;
	int				len;

	sd = (MprStringData*) extList.getFirst();
	while (sd) {
		len = strlen(sd->string);
		if (uriLen > len && strncmp(sd->string, &uri[uriLen - len], len) == 0) {
			return 1;
		}
		sd = (MprStringData*) extList.getNext(sd);
	}
	//
	//	No match, the next handler in the chain will match
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

MaHandler *MaHandler::cloneHandler()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaHandler::setup(MaRequest *rq)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaHandler::postData(MaRequest *rq, char *buf, int buflen)
{
}

////////////////////////////////////////////////////////////////////////////////

int MaHandler::run(MaRequest *rq)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
