///
///	@file 	alias.cpp
/// @brief 	Alias service for aliasing URLs to file storage.
///	@overview This module supports the alias directives and mapping 
///		URLs to physical locations. It also performs redirections.
//
////////////////////////////////// Copyright ///////////////////////////////////
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
//////////////////////////////////// MaAlias ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	WARNING: Note that the aliasName is a URL if a code is specified
//	(non-zero). Otherwise, the aliasName should be a document path
//

MaAlias::MaAlias(char *prefix, char *aliasName, int code)
{
	this->prefix = mprStrdup(prefix);
	this->prefixLen = strlen(prefix);
	this->aliasName = mprStrdup(aliasName);
	redirectCode = code;
	inherited = false;

#if WIN
	//
	//	Windows is case insensitive for file names. Always map to lower case.
	//
	mprStrLower(this->prefix);
	mprStrLower(this->aliasName);
#endif
}

////////////////////////////////////////////////////////////////////////////////

MaAlias::MaAlias(MaAlias *ap)
{
	this->prefix = mprStrdup(ap->prefix);
	this->prefixLen = ap->prefixLen;
	this->aliasName = mprStrdup(ap->aliasName);
	redirectCode = ap->redirectCode;
	inherited = true;
}

////////////////////////////////////////////////////////////////////////////////

MaAlias::~MaAlias()
{
	mprFree(prefix);
	mprFree(aliasName);
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
