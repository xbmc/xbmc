///
///	@file 	espHandler.h
/// @brief 	Header for the espHandler
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

#ifndef _h_ESP_MODULE
#define _h_ESP_MODULE 1

#include	"http.h"
#include	"httpEnv.h"
#include	"esp.h"

//////////////////////////////////// Types /////////////////////////////////////

class MaEspProc;

extern "C" {
	extern int mprEspInit(void *handle);
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// EspModule //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class MaEspModule : public MaModule {
  private:
  public:
					MaEspModule(void *handle);
					~MaEspModule();
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// EspHandlerService ///////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Flags
//
#define ESP_FLAGS_ERRORS_TO_BROWSER	0x1		// Send errors to the browser
#define ESP_FLAGS_ERRORS_TO_LOG		0x2		// Send errors only to the logfile

class MaEspHandlerService : public MaHandlerService {
  private:
	MprList			handlerHeaders;			// List of handler headers
	int				serviceFlags;

#if BLD_FEATURE_MULTITHREAD
	MprMutex		*mutex;
#endif
  public:
					MaEspHandlerService();
					~MaEspHandlerService();
	inline int		getFlags() { return serviceFlags; };
	MaHandler		*newHandler(MaServer *server, MaHost *host, char *ext);
	void			setErrors(int where);
	int				start();

#if BLD_FEATURE_MULTITHREAD
	inline void		lock() { mutex->lock(); };
	inline void		unlock() { mutex->unlock(); };
#else
	inline void		lock() { };
	inline void		unlock() { };
#endif
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// EspHandler //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	A master instance of the EspHandler is create for each referencing host.
//	A request instance is cloned from this for each request.
//

class MaEspHandler : public MaHandler {
  private:
	MprBuf			*postBuf;
	EspRequest		*espRequest;
	MaEspHandlerService *espHandlerService;

  public:
					MaEspHandler(MaEspHandlerService *hs, char *extensions);
					~MaEspHandler();
	MaHandler		*cloneHandler();
	int 			parseConfig(char *key, char *value, MaServer *server, 
						MaHost *host, MaAuth *auth, MaDir *dir, 
						MaLocation *location);
	MprBuf			*getPostBuf();
	void			postData(MaRequest *rq, char *buf, int buflen);
	int				process(MaRequest *rq);
	int				run(MaRequest *rq);
	int				setup(MaRequest *rq);
};

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// EspProc ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if BLD_FEATURE_LEGACY_API
//
//	DEPRECATED in 2.X -- Use espDefineCMethod instead
//
class MaEspProc {
  private:
	char			*name;
  public:
					MaEspProc(char *procName, char *scriptName = "javascript");
					MaEspProc(MaServer *server, MaHost *host, char *procName);
	virtual			~MaEspProc();
	char			*getName();
	virtual int		run(MaRequest *rq, int argc, char **argv) = 0;
	void			setErrorMsg(MaRequest *rq, char *fmt, ...);
	void			setReturnValue(MaRequest *rq, MprVar result);
};

#endif // BLD_FEATURE_LEGACY_API
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// C API ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifdef  __cplusplus
extern "C" {
#endif

extern MaRequest *espGetRequest();

#ifdef  __cplusplus
} 	// extern "C" 
#endif

////////////////////////////////////////////////////////////////////////////////
#endif // _h_ESP_MODULE 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
