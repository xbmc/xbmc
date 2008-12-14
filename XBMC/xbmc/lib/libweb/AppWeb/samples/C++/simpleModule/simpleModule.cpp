///
///	@file 	simpleModule.cpp
/// @brief 	Create a simple AppWeb dynamically loadable module
///
////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	The latest version of this code is available at http://www.mbedthis.com
//
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version.
//
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	See the GNU General Public License for more details at:
//	http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This General Public License does NOT permit incorporating this software 
//	into proprietary programs. If you are unable to comply with the GPL, a 
//	commercial license for this software and support services are available
//	from Mbedthis Software at http://www.mbedthis.com
//
////////////////////////////// Includes ////////////////////////////////

#include	"simpleModule.h"

////////////////////////////////////////////////////////////////////////
///////////////////////////// SimpleModule /////////////////////////////
////////////////////////////////////////////////////////////////////////

///
///	This function is called when the module is dynamically loaded
///

int mprSimpleModuleInit(void *handle)
{
	mprLog(0, "In mprSimpleModuleInit()\n");
	new SimpleModule(handle);
	return 0;
}

////////////////////////////////////////////////////////////////////////

///
///	The constructor is called by either the DLL load entry point 
///	above, or if not using DLLs, it should be called directly from 
///	your application at initialization time.
///

SimpleModule::SimpleModule(void *handle) : MaModule("simpleModule", handle)
{
	mprLog(0, "In SimpleModule()\n");
}

////////////////////////////////////////////////////////////////////////

SimpleModule::~SimpleModule()
{
	//
	//	Put cleanup code here for when AppWeb is exiting
	//
	mprLog(0, "In ~SimpleModule()\n");
}

////////////////////////////////////////////////////////////////////////

int SimpleModule::parseConfig(char *key, char *value, MaServer *server, 
	MaHost *host, MaAuth *auth, MaDir* dir, MaLocation *location)
{
	if (mprStrCmpAnyCase(key, "simpleDirective") == 0) {
		//
		//	Put code here to parse the "value". Return 1 to indicate
		//	we have processed this directive.
		//
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
//	AppWeb is starting up
//

int SimpleModule::start()
{
	//
	//	Put startup code here
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
//	AppWeb is shutting down
//

void SimpleModule::stop()
{
	//
	//	Put shutdown code here
	//
}

////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
