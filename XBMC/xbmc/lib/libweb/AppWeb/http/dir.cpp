///
///	@file 	dir.cpp
/// @brief 	Support for Directory directives
///	@overview Support authorization on a per-directory basis.
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
///////////////////////////////////// MaDir ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaDir::MaDir(MaHost *host)
{
	path = 0;
	pathLen = 0;
	indexName = mprStrdup("index.html");
	this->host = host;
	inherited = false;
}

////////////////////////////////////////////////////////////////////////////////

MaDir::MaDir(MaHost *host, char *path)
{
	this->path = mprStrdup(path);
	pathLen = mprStrlen(path, MPR_MAX_FNAME);
	indexName = mprStrdup("index.html");
	this->host = host;
	inherited = false;
}

////////////////////////////////////////////////////////////////////////////////

MaDir::MaDir(MaHost *host, MaDir *dp)
{
	this->host = host;
	indexName = mprStrdup(dp->indexName);
	inheritAuth(dp);
	pathLen = 0;
	path = 0;
	setPath(dp->path);
	inherited = true;
}

////////////////////////////////////////////////////////////////////////////////

MaDir::~MaDir()
{
	mprFree(indexName);
	mprFree(path);
}

////////////////////////////////////////////////////////////////////////////////

void MaDir::setPath(char *fileName)
{
	char	buf[MPR_MAX_FNAME];
	int		len;

	mprFree(path);
	mprGetFullPathName(buf, sizeof(buf) - 1, fileName);
	len = strlen(buf);
	if (buf[len - 1] != '/') {
		buf[len] = '/';
		buf[++len] = '\0';
	}
	path = mprStrdup(buf);
	pathLen = strlen(path);

#if WIN
	//
	//	Windows is case insensitive for file names. Always map to lower case.
	//
	mprStrLower(path);
#endif
	inherited = false;
}

////////////////////////////////////////////////////////////////////////////////

void MaDir::setIndex(char *name) 
{ 
	mprFree(indexName);
	indexName = mprStrdup(name); 
	inherited = false;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim:tw=78
// vim: sw=4 ts=4 
//
