///
///	@file 	copyHandler.h
/// @brief 	Header for the copyHandler
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
////////////////////////////////// Includes ////////////////////////////////////

#ifndef _h_COPY_MODULE
#define _h_COPY_MODULE 1

#include	"http.h"

/////////////////////////////// Forward Definitions ////////////////////////////

class MaCopyHandler;
class MaCopyHandlerService;
class MaCopyModule;

extern "C" {
	extern int mprCopyInit(void *handle);
}

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// MaCopyModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaCopyModule : public MaModule {
  private:
	MaCopyHandlerService 
					*copyHandlerService;
  public:
					MaCopyModule(void *handle);
					~MaCopyModule();
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaCopyHandler ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaCopyHandlerService : public MaHandlerService {
  public:
					MaCopyHandlerService();
					~MaCopyHandlerService();
	MaHandler		*newHandler(MaServer *server, MaHost *host, char *ex);
};

class MaCopyHandler : public MaHandler {
  public:
					MaCopyHandler();
					~MaCopyHandler();
	MaHandler		*cloneHandler();
	int				matchRequest(MaRequest *rq, char *uri, int uriLen);
	int				run(MaRequest *rq);
};

////////////////////////////////////////////////////////////////////////////////
#endif // _h_COPY_MODULE 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
