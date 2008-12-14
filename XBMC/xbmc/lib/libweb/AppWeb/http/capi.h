///
///	@file 	capi.h
/// @brief 	C language API
///	@overview See capi.dox for additional documentation.
//
////////////////////////////////// Copyright ///////////////////////////////////
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
////////////////////////////////// Includes ////////////////////////////////////

#ifndef _h_CAPI
#define _h_CAPI 1

#include "mpr.h"
#include "var.h"
#include "httpEnv.h"

#if BLD_FEATURE_C_API_MODULE || DOXYGEN
#ifdef  __cplusplus
extern "C" {
#endif

/////////////////////////////////// Types //////////////////////////////////////

#ifdef  __cplusplus
	extern "C" int mprCapiInit(void *handle);
	
	class MaCapiModule : public MaModule {
	  private:
	  public:
						MaCapiModule(void *handle);
						~MaCapiModule();
	};
	#include "client.h"

#else // !__cplusplus
	typedef struct { void *x; } Mpr;
	typedef struct { void *x; } MaHttp;
	typedef struct { void *x; } MaHost;
	typedef struct { void *x; } MaServer;
	typedef struct { void *x; } MaRequest;
	typedef struct { void *x; } MaClient;
#endif

#if BLD_FEATURE_LEGACY_API && BLD_FEATURE_ESP_MODULE
	//
	//	DEPRECATED 2.0
	//
	typedef int  (*MaEspCb)(MaRequest *rq, int argc, char **argv);
#endif

//////////////////////////////////// API ///////////////////////////////////////

#if BLD_FEATURE_CONFIG_PARSE
extern int		maConfigureServer(MaServer *server, char *configFile);
#endif

extern void 	maCreateEnvVars(MaRequest *rq, char *buf, int len);
extern MaHttp 	*maCreateHttp();
extern MaServer	*maCreateServer(MaHttp *http, char *name, char *serverRoot);

#if BLD_FEATURE_LEGACY_API && BLD_FEATURE_ESP_MODULE
extern int 		maDefineEsp(char *name, MaEspCb fn);
#endif

#if BLD_FEATURE_EGI_MODULE
//
//	Almost DEPRECATED 2.0 but back by user demand
//
typedef void (*MaEgiCb)(MaRequest *rq, char *script, char *uri, char *query,
	char *postData, int postLen);
extern int 		maDefineEgiForm(char *name, MaEgiCb fn);
#endif

extern void 	maDeleteHttp(MaHttp *http);
extern void 	maDeleteServer(MaServer *server);
extern void 	maRequestError(MaRequest *rq, int code, char *fmt, ...);
extern int		maGetConfigErrorLine(MaServer *server);

#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
extern char 	*maGetCookie(MaRequest *rq);
extern void		maGetCrackedCookie(MaRequest *rq, char **name, char **value,
					char **path);
#endif

extern char 	*maGetFileName(MaRequest *rq);
extern char 	*maGetVar(MaRequest *rq, MaEnvType objType, char *var, 
					char *defaultValue);
extern void 	maRedirect(MaRequest *rq, int code, char *url);

#if BLD_FEATURE_COOKIE || BLD_FEATURE_SESSION
extern void		maSetCookie(MaRequest *rq, char *name, char *value, 
					int lifetime, char *path, bool secure);
#endif

extern int 		maSetFileName(MaRequest *rq, char *fileName);
extern void		maSetHeader(MaRequest *rq, char *value, int allowMultiple);
extern void		maSetResponseCode(MaRequest *rq, int code);

#if BLD_FEATURE_LEGACY_API
extern void 	maSetResult(MaRequest *rq, char *s);
#endif

extern void 	maSetVar(MaRequest *rq, MaEnvType objType, char *var, 
					char *value);
extern int		maStartServers(MaHttp *http);
extern void	 	maStopServers(MaHttp *http);
extern int 		maWrite(MaRequest *rq, char *buf, int size);
extern int 		maWriteFmt(MaRequest *rq, char* fmt, ...);
extern int 		maWriteStr(MaRequest *rq, char *s);

#if BLD_FEATURE_C_API_CLIENT
extern MaClient	*maCreateClient();
extern void		maDestroyClient(MaClient *cp);

extern void		maClientSetTimeout(MaClient *cp, int timeout);
extern void		maClientSetRetries(MaClient *cp, int count);
extern void		maClientSetKeepAlive(MaClient *cp, int on);
extern char		*maClientGetHeaderVar(MaClient *cp, char *key);
extern int		maClientDoGetRequest(MaClient *cp, char *url);
extern int		maClientDoPostRequest(MaClient *cp, char *url, char *postData, 
					int postLen);
extern int		maClientDoHeadRequest(MaClient *cp, char *url);
extern int		maClientDoOptionsRequest(MaClient *cp, char *url);
extern int		maClientDoTraceRequest(MaClient *cp, char *url);
extern int		maClientGetResponseCode(MaClient *cp);
extern char		*maClientGetResponseContent(MaClient *cp, int *contentLen);
extern char		*maClientGetResponseHeader(MaClient *cp);
extern char		*maClientGetResponseMessage(MaClient *cp);
extern int		maClientGetFd(MaClient *cp);
extern char		*maClientGetHost(MaClient *cp);
extern int		maClientGetPort(MaClient *cp);
extern void		maClientResetAuth(MaClient *cp);
extern void		maClientSetAuth(MaClient *cp, char *realm, char *user, 
					char *password);
extern void		maClientSetHost(MaClient *cp, char *host);
extern void		maClientSetPort(MaClient *cp, int num);
extern void		maClientSetProxy(MaClient *cp, char *host, int port);
extern void		maClientLock(MaClient *cp);
extern void		maClientUnlock(MaClient *cp);
#endif // BLD_FEATURE_C_API_CLIENT

/*
 *	MPR APIs
 */
#if BLD_FEATURE_LOG
extern void		mprAddLogFileListener();
#else
#define 		mprAddLogFileListener()
#endif
extern int		mprCreateMpr(char *appName);
extern void		mprDeleteMpr();
extern int		mprGetAsyncSelectMode();
extern int		mprGetFds(fd_set* readInterest, fd_set* writeInterest, 
						fd_set* exceptInterest, int *maxFd, int *lastGet);
extern int		mprGetIdleTime();
extern int		mprIsExiting();
extern int		mprRunTasks();
extern int		mprRunTimers();
extern void		mprServiceEvents(int loopOnce, int maxTimeout);
extern void		mprServiceIO(int readyFds, fd_set* readFds, fd_set* writeFds, 
						fd_set* exceptFds);
extern void 	mprSetThreads(int low, int high);

#if WIN
extern void		mprServiceWinIO(int sock, int winMask);
extern void		mprSetAsyncSelectMode(int on);
extern void 	mprSetHwnd(HWND appHwnd);
#endif

#if BLD_FEATURE_LOG
extern void	 	mprSetLogSpec(char *logSpec);
#else
#define			mprSetLogSpec(x)
#endif

#if WIN
extern void 	mprSetSocketHwnd(HWND socketHwnd);
extern void 	mprSetSocketMessage(int  msgId);
#endif

extern int 		mprStartMpr(int startFlags);
extern void		mprStopMpr();
extern void		mprTerminate(int graceful);
extern void		mprTrace(int level, char *fmt, ...);

/* */
extern char 	*maGetRequestUserName(MaRequest *rq);
extern char 	*maGetRequestCookies(MaRequest *rq);
extern MprVar 	*maGetRequestVars(MaRequest *rq);
extern char 	*maGetRequestIpAddr(MaRequest *rq);
extern char 	*maGetRequestUriExt(MaRequest *rq);
extern char 	*maGetRequestMimeType(MaRequest *rq);
extern char 	*maGetRequestUriQuery(MaRequest *rq);
extern int 		 maGetRequestPort(MaRequest *rq);
extern void 	maFlushResponse(MaRequest *rq, int background, 
						int completeRequired);
extern MaHost	*maGetDefaultHost(MaServer *server);
extern MaServer	*maGetDefaultServer();
extern void 	maSetServerDefaultPage(MaServer *server, char *path, 
						char *fileName);
extern void 	maSetHostDefaultPage(MaHost *host, char *path, 
					char *fileName);
extern void		maSetServerRoot(MaServer *server, char *root);
extern char 	*maGetServerRoot(MaServer *server);
extern int 		maGetIntVar(MaRequest *rq, MaEnvType objType, char *var, int
					defaultValue);
extern void 	maSetIntVar(MaRequest *rq, MaEnvType objType, char *var, int
					value);

#if BLD_FEATURE_SESSION
extern char 	*maGetSessionId(MaRequest *rq);
extern void 	maSetSessionExpiryCallback(MaRequest *rq, 
					void (*callback)(void *arg), void *arg);
extern void 	maCreateSession(MaRequest *rq, int timeout);
extern void 	maDestroySession(MaRequest *rq);
#endif

///////////////////////////////// UnPublished API //////////////////////////////

#if BLD_FEATURE_LEGACY_API
/*
 *	Just for the A&L DMF
 */
extern char 	*maGetUserName(MaRequest *rq);
extern char 	*maGetUri(MaRequest *rq);
extern int 		maIsKeepAlive(MaRequest *rq);
#if BLD_FEATURE_ESP_MODULE
extern int 		maIsEsp(MaRequest *rq);
#endif
#endif

//
//	DLL initialization modules
//
#if BLD_FEATURE_ADMIN_MODULE
extern int mprAdminInit(void *handle);
#endif
#if BLD_FEATURE_AUTH_MODULE
extern int mprAuthInit(void *handle);
#endif
#if BLD_FEATURE_CGI_MODULE
extern int mprCgiInit(void *handle);
#endif
#if BLD_FEATURE_GACOMPAT_MODULE
extern int mprCompatInit(void *handle);
#endif
#if BLD_FEATURE_COPY_MODULE
extern int mprCopyInit(void *handle);
#endif
#if BLD_FEATURE_DIR_MODULE
extern int mprDirInit(void *handle);
#endif
#if BLD_FEATURE_EGI_MODULE
extern int mprEgiInit(void *handle);
#endif
#if BLD_FEATURE_ESP_MODULE
extern int mprEspInit(void *handle);
#endif
#if BLD_FEATURE_SSL_MODULE
extern int mprSslInit(void *handle);
#endif
#if BLD_FEATURE_UPLOAD_MODULE
extern int mprUploadInit(void *handle);
#endif
#if BLD_FEATURE_OPENSSL_MODULE
extern int mprOpenSslInit(void *handle);
#endif
#if BLD_FEATURE_PHP_MODULE
extern int mprPhp4Init(void *handle);
#endif

////////////////////////////////////////////////////////////////////////////////
#ifdef  __cplusplus
} 	// extern "C" 
#endif

#endif // BLD_FEATURE_C_API_MODULE
#endif // _h_CAPI 

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
