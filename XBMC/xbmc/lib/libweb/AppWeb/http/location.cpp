///
///	@file 	location.cpp
/// @brief 	Implement Location directives.
/// @overview Location directives provide authorization and handler 
///		matching based on URL prefixes.
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
////////////////////////////////// MaLocation //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaLocation::MaLocation()
{
	flags = 0;
	prefix = 0;
	prefixLen = 0;
	handlerName = 0;
}

////////////////////////////////////////////////////////////////////////////////

MaLocation::MaLocation(MaAuth *auth)
{
	prefix = 0;
	prefixLen = 0;
	handlerName = 0;
	flags = 0;
	inherited = 0;
	inheritAuth(auth);
}

////////////////////////////////////////////////////////////////////////////////

MaLocation::MaLocation(MaLocation *lp)
{
	prefix = mprStrdup(lp->prefix);
	prefixLen = lp->prefixLen;
	handlerName = mprStrdup(lp->handlerName);
	flags = lp->flags;
	inherited = true;
	inheritAuth(lp);
}

////////////////////////////////////////////////////////////////////////////////

MaLocation::~MaLocation()
{
	mprFree(prefix);
	mprFree(handlerName);
}

////////////////////////////////////////////////////////////////////////////////

void MaLocation::setHandler(char *name)
{
	mprFree(handlerName);
	handlerName = mprStrdup(name);
	inherited = false;
}

////////////////////////////////////////////////////////////////////////////////

void MaLocation::setPrefix(char *uri)
{
	mprFree(prefix);
	prefix = mprStrdup(uri);
	prefixLen = strlen(prefix);

#if WIN
	//
	//	Windows is case insensitive for file names. Always map to lower case.
	//
	mprStrLower(prefix);
#endif
	inherited = false;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
