///
///	@file 	gaCompatModule.h
/// @brief 	Header for the GoAhead Compatibility module
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

#ifndef _h_COMPAT_MODULE
#define _h_COMPAT_MODULE 1

#define	UNSAFE_FUNCTIONS_OK 1

#include	"http.h"
#include	"compatApi.h"

#if BLD_FEATURE_EJS
#include	"ejs.h"
#endif

#if BLD_FEATURE_EGI_MODULE
#include	"egiHandler.h"
#endif

#if BLD_FEATURE_ESP_MODULE
#include	"espHandler.h"
#endif

/////////////////////////////// Extern Definitions /////////////////////////////
#if BLD_FEATURE_GACOMPAT_MODULE

class MaCompatModule;

extern "C" {
	extern int mprCompatInit(void *handle);
}

#ifdef __cplusplus
////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// CompatModule /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaCompatModule : public MaModule {
  public:
					MaCompatModule(void *handle);
					~MaCompatModule();
};

////////////////////////////////////////////////////////////////////////////////

class GaCompatHandler : public MaHandler {
  public:
					GaCompatHandler(char *extensions);
					~GaCompatHandler();
	MaHandler 		*cloneHandler();
	int 			matchRequest(MaRequest *rq, char *uri, int uriLen);
	int 			run(MaRequest *rq);

  private:
	int 			securityChecks(MaRequest *rq);
	void 			formatAuthResponse(MaRequest *rq, MaAuth *auth, 
						char *realm, int authType, int code, 
						char *userMsg);
};

////////////////////////////////////////////////////////////////////////////////
#endif /* __cpluscplus */
#endif // BLD_FEATURE_GACOMPAT_MODULE
#endif // _h_COMPAT_MODULE

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
