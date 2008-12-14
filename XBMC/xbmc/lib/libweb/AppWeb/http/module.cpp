///
///	@file 	module.cpp
/// @brief 	Dynamic and static module management
///	@overview This file support the dynamic loading of DLLs (shared 
///		libraries). It also provides and abstraction so that staticly 
///		linked modules can be treated similarly to dynamically loaded 
///		modules.
///	@remarks This module is thread-safe.
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
/////////////////////////////////// Includes ///////////////////////////////////

#include	"http.h"

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// MaModule ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MaModule::MaModule(char *name, void *handle)
{
#if BLD_FEATURE_MULTITHREAD
	mutex = new MprMutex();
#endif
	this->name = mprStrdup(name);
	this->handle = handle;
	flags = 0;
	maGetHttp()->insertModule(this);
}

////////////////////////////////////////////////////////////////////////////////

MaModule::~MaModule()
{
	lock();
	mprFree(name);
	maGetHttp()->removeModule(this);
#if BLD_FEATURE_MULTITHREAD
	delete mutex;
#endif
}

////////////////////////////////////////////////////////////////////////////////
#if UNUSED

void MaModule::setLoaded(bool isDll)
{
	lock();
	flags |= MPR_MODULE_LOADED;
#if BLD_FEATURE_DLL
	if (isDll) {
		flags |= MPR_MODULE_DLL;
	}
#endif
	unlock();
}

#endif
////////////////////////////////////////////////////////////////////////////////

void MaModule::setHandle(void *h)
{
	handle = h;
}

////////////////////////////////////////////////////////////////////////////////

void *MaModule::getHandle()
{
	return handle;
}

////////////////////////////////////////////////////////////////////////////////

char *MaModule::getName()
{
	return name;
}

////////////////////////////////////////////////////////////////////////////////

int MaModule::parseConfig(char *key, char *value, MaServer *server, 
		MaHost *host, MaAuth *auth, MaDir* dir, MaLocation *location)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MaModule::start()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void MaModule::stop()
{
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
