///
///	@file 	php5Handler.h
/// @brief 	Header for the phpHandler
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

#ifndef _h_PHP5_MODULE
#define _h_PHP5_MODULE 1

//
//	PHP includes crtdbg.h which messes up _delete definitions
//
#define _INC_CRTDBG

#ifndef UNSAFE_FUNCTIONS_OK
#define UNSAFE_FUNCTIONS_OK 1
#endif

#include	"http.h"

#if BLD_FEATURE_MULTITHREAD
#define ZTS 1
#define PTHREADS 1
#endif

//
//	Workaround for VS 2005 and PHP5 headers. Need to include before PHP headers
//	include it.
//
#if _MSC_VER >= 1400
#include	<sys/utime.h>
#endif


#if BLD_FEATURE_PHP5_MODULE

#if WIN
#define PHP_WIN32
#define ZEND_WIN32
#endif

#include	<math.h>

//
//	Windows binary build does not define this
//
#ifndef ZEND_DEBUG
#define ZEND_DEBUG 0
#endif

#define MA_PHP_MODULE_NAME	"php5"
#define MA_PHP_HANDLER_NAME	"php5Handler"
#define MA_PHP_LOG_NAME		"php5"

extern "C" {

#if BLD_FEATURE_DLL == 0
//
//	Need this to prevent crtdbg.h defining "delete()" when linking statically
//
#define _MFC_OVERRIDES_NEW
#endif

#undef chdir

#include <main/php.h>
#include <main/php_globals.h>
#include <main/php_variables.h>
#include <Zend/zend_modules.h>
#include <main/SAPI.h>

#ifdef PHP_WIN32
	#include <win32/time.h>
	#include <win32/signal.h>
	#include <process.h>
#else
	#include <main/build-defs.h>
#endif

#include <Zend/zend.h>
#include <Zend/zend_extensions.h>
#include <main/php_ini.h>
#include <main/php_globals.h>
#include <main/php_main.h>
#include <TSRM/TSRM.h>
}

/////////////////////////////// Forward Definitions ////////////////////////////

class MaPhp5Handler;
class MaPhp5HandlerService;
class MaPhp5Module;

extern "C" {
#if PHP5
	extern int mprPhp5Init(void *handle);
#else
	extern int mprPhp5Init(void *handle);
#endif
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaPhp5Module /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaPhp5Module : public MaModule {
  private:
	MaPhp5HandlerService 
					*phpHandlerService;
  public:
					MaPhp5Module(void *handle);
					~MaPhp5Module();
	void			unload();
};

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MaPhp5Handler ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaPhp5HandlerService : public MaHandlerService {
  private:
	MprLogModule	*log;					// Mpr log handle

  public:
					MaPhp5HandlerService();
					~MaPhp5HandlerService();
	MaHandler		*newHandler(MaServer *server, MaHost *host, char *ex);
	int				start();
	int				stop();
};


class MaPhp5Handler : public MaHandler {
  public:
	MprHashTable	*env;
	void 			*func_data;				// function data
	MprLogModule	*log;					// Pointer to Php5HandlerServer log
	int 			phpInitialized;			// Can execute
	zval 			*var_array;				// Track var array

  public:
					MaPhp5Handler(MprLogModule *serviceLog, char *extensions);
					~MaPhp5Handler();
	MaHandler		*cloneHandler();
	int				execScript(MaRequest *rq);
	int				run(MaRequest *rq);
	void			setVar(MaRequest *rq, MaEnvType t, char *var, char *value);
};

////////////////////////////////////////////////////////////////////////////////
#endif // BLD_FEATURE_PHP5_MODULE
#endif // _h_PHP5_MODULE 
