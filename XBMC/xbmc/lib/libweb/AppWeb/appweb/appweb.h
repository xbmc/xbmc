///
///	@file 	appweb.h
/// @brief 	Primary header for the Mbedthis Appweb Library
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

#ifndef _h_APP_WEB
#define _h_APP_WEB 1

#ifndef  	__cplusplus
//
//	C includes
//
#include	"capi.h"
#if BLD_FEATURE_GACOMPAT_MODULE
#include	"compatApi.h"
#endif
#if BLD_FEATURE_ESP_MODULE
#include	"esp.h"
#endif

#else // !__cplusplus
//
//	C++ includes
//
#include	"mpr.h"
#include	"http.h"
#include	"client.h"

//
//	FUTURE -- remove from here. Menu definitions.
//
#if WIN
#define MPR_HTTP_MENU_ABOUT		1
#define MPR_HTTP_MENU_STOP		2
#define MPR_HTTP_MENU_HELP		3
#define MPR_HTTP_MENU_CONSOLE	4
#endif

#if BLD_FEATURE_ADMIN_MODULE
#include	"adminHandler.h"
#endif
#if BLD_FEATURE_AUTH_MODULE
#include	"authHandler.h"
#endif
#if BLD_FEATURE_COPY_MODULE
#include	"copyHandler.h"
#endif
#if BLD_FEATURE_CGI_MODULE
#include	"cgiHandler.h"
#endif
#if BLD_FEATURE_DIR_MODULE
#include	"dirHandler.h"
#endif
#if BLD_FEATURE_EGI_MODULE
#include	"egiHandler.h"
#endif
#if BLD_FEATURE_ESP_MODULE
#include	"espHandler.h"
#endif
#if BLD_FEATURE_SSL_MODULE
#include	"sslModule.h"
#endif
#if BLD_FEATURE_UPLOAD_MODULE
#include	"uploadHandler.h"
#endif
#if BLD_FEATURE_PUT_MODULE
#include	"putHandler.h"
#endif
#if BLD_FEATURE_C_API_MODULE
#include	"capi.h"
#endif

#if BLD_FEATURE_GACOMPAT_MODULE
	#include	"gaCompatModule.h"
#endif
//
//	Internal use only by appweb.cpp
//
void maLoadStaticModules();
void maUnloadStaticModules();

extern "C" 
{
    int appWebMain(int argc, char *argv[]);
}
#endif // __cplusplus

#endif // _h_HTTP 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
