///
///	@file 	appwebStaticLink.cpp
/// @brief 	Statically link code between appweb and winAppweb
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

#include	"appweb/appweb.h"

#if BLD_FEATURE_PHP5_MODULE_BUILTIN
#include	"http/modules/php5/php5Handler.h"
#endif
#if BLD_FEATURE_MATRIXSSL_MODULE_BUILTIN
#include	"http/modules/matrixSsl/matrixSslModule.h"
#endif
#if BLD_FEATURE_OPENSSL_MODULE_BUILTIN
#include	"http/modules/openSsl/openSslModule.h"
#endif

//////////////////////////////////// Locals ////////////////////////////////////

static MaModule *staticModules[64];		// List of static modules 
static int maxStaticModules;			// Max static modules

//////////////////////////////////// Code //////////////////////////////////////

void maLoadStaticModules()
{
	int		index = 0;

	staticModules[index] = 0;
#if BLD_FEATURE_ADMIN_MODULE_BUILTIN
	staticModules[index++] = new MaAdminModule(0);
#endif
#if BLD_FEATURE_AUTH_MODULE_BUILTIN
	staticModules[index++] = new MaAuthModule(0);
#endif
#if BLD_FEATURE_COPY_MODULE_BUILTIN
	staticModules[index++] = new MaCopyModule(0);
#endif
#if BLD_FEATURE_DIR_MODULE_BUILTIN
	staticModules[index++] = new MaDirModule(0);
#endif
#if BLD_FEATURE_ESP_MODULE_BUILTIN
	staticModules[index++] = new MaEspModule(0);
#endif
#if BLD_FEATURE_CGI_MODULE_BUILTIN
	staticModules[index++] = new MaCgiModule(0);
#endif
#if BLD_FEATURE_GACOMPAT_MODULE_BUILTIN
	staticModules[index++] = new MaCompatModule(0);
#endif
#if BLD_FEATURE_C_API_MODULE_BUILTIN
	staticModules[index++] = new MaCapiModule(0);
#endif
#if BLD_FEATURE_EGI_MODULE_BUILTIN
	staticModules[index++] = new MaEgiModule(0);
#endif
#if BLD_FEATURE_SSL_MODULE_BUILTIN
	staticModules[index++] = new MaSslModule(0);
#endif
#if BLD_FEATURE_MATRIXSSL_MODULE_BUILTIN
	staticModules[index++] = new MaMatrixSslModule(0);
#endif
#if BLD_FEATURE_OPENSSL_MODULE_BUILTIN
	staticModules[index++] = new MaOpenSslModule(0);
#endif
#if BLD_FEATURE_PHP5_MODULE_BUILTIN
	staticModules[index++] = new MaPhp5Module(0);
#endif
#if BLD_FEATURE_PUT_MODULE_BUILTIN
	staticModules[index++] = new MaPutModule(0);
#endif
#if BLD_FEATURE_UPLOAD_MODULE_BUILTIN
	staticModules[index++] = new MaUploadModule(0);
#endif
	maxStaticModules = index;
}

////////////////////////////////////////////////////////////////////////////////

void maUnloadStaticModules()
{
	int		i;

	for (i = 0; i < maxStaticModules; i++) {
		delete staticModules[i];
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
