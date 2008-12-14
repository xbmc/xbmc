///
///	@file 	simpleModule.h
/// @brief 	Header for the simpleModule
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
#ifndef _h_SIMPLE_MODULE
#define _h_SIMPLE_MODULE 1

#include	"appweb/appweb.h"

////////////////////////// Forward Definitions /////////////////////////

class SimpleModule;

extern "C" {
	extern int mprSimpleModuleInit(void *handle);
};

////////////////////////////////////////////////////////////////////////
///////////////////////////// SimpleModule /////////////////////////////
////////////////////////////////////////////////////////////////////////

class SimpleModule : public MaModule {
  public:
					SimpleModule(void *handle);
					~SimpleModule();
	int				parseConfig(char *key, char *value, MaServer *server, 
						MaHost *host, MaAuth *auth, MaDir* dir, 
						MaLocation *location);
	int				start();
	void			stop();
};

////////////////////////////////////////////////////////////////////////
#endif // _h_SIMPLE_MODULE 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
