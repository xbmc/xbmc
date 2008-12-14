///
///	@file 	simpleHandler.h
/// @brief 	Header for the simpleHandler
///
////////////////////////////////////////////////////////////////////////////////
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
#ifndef _h_SIMPLE_HANDLER
#define _h_SIMPLE_HANDLER 1

#include	"appweb/appweb.h"

/////////////////////////// Extern Definitions /////////////////////////

class SimpleHandler;
class SimpleHandlerService;

extern "C" {
	extern int mprSimpleHandlerInit(void *handle);
};

////////////////////////////////////////////////////////////////////////
///////////////////////// SimpleHandlerModule //////////////////////////
////////////////////////////////////////////////////////////////////////

class SimpleHandlerModule : public MaModule {
  private:
	SimpleHandlerService *simpleService;
  public:
					SimpleHandlerModule(void *handle);
					~SimpleHandlerModule();
};

////////////////////////////////////////////////////////////////////////
///////////////////////////// SimpleHandler ////////////////////////////
////////////////////////////////////////////////////////////////////////

class SimpleHandlerService : public MaHandlerService {
  public:
					SimpleHandlerService();
					~SimpleHandlerService();
	MaHandler		*newHandler(MaServer *server, MaHost *host, 
						char *ext);
	int				setup();
};

class SimpleHandler : public MaHandler {
  private:
  public:
					SimpleHandler(char *extensions);
					~SimpleHandler();
	MaHandler		*cloneHandler();
	int				matchRequest(MaRequest *rq, char *uri, 
						int uriLen);
	void			postData(MaRequest *rq, char *buf, int buflen);
	int				run(MaRequest *rq);
	int				setup(MaRequest *rq);
};

////////////////////////////////////////////////////////////////////////
#endif // _h_SIMPLE_HANDLER 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
