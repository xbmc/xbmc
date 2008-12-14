/**
 *	@file 	esp.h
 *	@brief 	Header for Embedded Server Pages (ESP)
 */
/********************************* Copyright **********************************/
/*
 *	@copy	default
 *
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Mbedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.mbedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Mbedthis 
 *	Software at http://www.mbedthis.com 
 *	
 *	@end
 */
/********************************** Includes **********************************/

#ifndef _h_ESP_h
#define _h_ESP_h 1

#include	"buildConfig.h"
#include	"ejs.h"
#include	"espEnv.h"
#include	"var.h"
#include	"miniMpr.h"

/*********************************** Defines **********************************/

#if BLD_FEATURE_SQUEEZE
#define ESP_TOK_INCR 			1024
#define ESP_MAX_HEADER 			1024
#else
#define ESP_TOK_INCR 			4096
#define ESP_MAX_HEADER 			4096
#endif

/*
 *	ESP lexical analyser tokens
 */
#define ESP_TOK_ERR				-1			/* Any input error */
#define ESP_TOK_EOF				0			/* End of file */
#define ESP_TOK_START_ESP		1			/* <% */
#define ESP_TOK_END_ESP			2			/* %> */
#define ESP_TOK_ATAT			3			/* @@var */
#define ESP_TOK_LITERAL			4			/* literal HTML */
#define ESP_TOK_INCLUDE			5			/* include file.esp */
#define ESP_TOK_EQUALS			6			/* = var */

/*
 *	ESP parser states
 */
#define ESP_STATE_BEGIN			1			/* Starting state */
#define ESP_STATE_IN_ESP_TAG	2			/* Inside a <% %> group */

/*********************************** Types ************************************/

typedef void* EspHandle;					/* Opaque Web server handle type */

/*
 *	Per request control block
 */
typedef struct EspRequest {
	MprStr		docPath;					/* Physical path for ESP page */	
	EjsId		eid;						/* EJS instance handle */
	struct Esp	*esp;						/* Pointer to ESP control block */
	EspHandle	requestHandle;				/* Per request web server handle */
	MprStr		uri;						/* Request URI */		
	MprVar		*variables;					/* Pointer to variables */
} EspRequest;

/*
 *	Master ESP control block. This defines the function callbacks for a 
 *	web server handler to implement. ESP will call these functions as
 *	required.
 */
typedef struct Esp {
	int		maxScriptSize;
	void	(*createSession)(EspHandle handle, int timeout);
	void	(*destroySession)(EspHandle handle);
	char	*(*getSessionId)(EspHandle handle);
	int		(*mapToStorage)(EspHandle handle, char *path, int len, char *uri,
				int flags);
	int		(*readFile)(EspHandle handle, char **buf, int *len, char *path);
	void	(*redirect)(EspHandle handle, int code, char *url);
	void 	(*setCookie)(EspHandle handle, char *name, char *value, 
				int lifetime, char *path, bool secure);
	void	(*setHeader)(EspHandle handle, char *value, bool allowMultiple);
	void	(*setResponseCode)(EspHandle handle, int code);
	int		(*writeBlock)(EspHandle handle, char *buf, int size);
	int		(*writeFmt)(EspHandle handle, char *fmt, ...);
#if BLD_FEATURE_MULTITHREAD
	void 	(*lock)(void *lockData);
	void 	(*unlock)(void *lockData);
	void	*lockData;
#endif
} Esp;


/*
 *	ESP parse context
 */
typedef struct {
	char 	*inBuf;					/* Input data to parse */
	char 	*inp;					/* Next character for input */
	char	*endp;					/* End of storage (allow for null) */
	char	*tokp;					/* Pointer to current parsed token */
	char	*token;					/* Storage buffer for token */
	int		tokLen;					/* Length of buffer */
} EspParse;


/******************************** Private APIs ********************************/

extern void			espRegisterProcs();

/******************************** Published API *******************************/
#ifdef  __cplusplus
extern "C" {
#endif

/*
 *	Function callback signatures
 */
typedef int 		(*EspCFunction)(EspRequest *ep, int argc, 
						struct MprVar **argv);
typedef int 		(*EspStringCFunction)(EspRequest *ep, int argc, 
						char **argv);

/*
 *	APIs for those hosting the ESP module
 */
extern int 			espOpen(Esp *control);
extern void			espClose();
extern EspRequest	*espCreateRequest(EspHandle webServerRequestHandle, 
						char *uri, MprVar *envObj);
extern void			espDestroyRequest(EspRequest *ep);
extern int			espProcessRequest(EspRequest *ep, char *docPath, 
						char *docBuf, char **errMsg);

/*
 *	Method invocation
 */
extern void			espDefineCFunction(EspRequest *ep, char *functionName, 
						EspCFunction fn, void *thisPtr);
extern void 		espDefineFunction(EspRequest *ep, char *functionName, 
						char *args, char *body);
extern void			espDefineStringCFunction(EspRequest *ep, 
						char *functionName, EspStringCFunction fn, 
						void *thisPtr);
extern int 			espRunFunction(EspRequest *ep, MprVar *obj, 
						char *functionName, MprArray *args);
extern void			espSetResponseCode(EspRequest *ep, int code);
extern void			espSetReturn(EspRequest *ep, MprVar value);
extern void 		*espGetThisPtr(EspRequest *ep);

/*
 *	Utility routines to use in C methods
 */
extern void			espError(EspRequest *ep, char *fmt, ...);
extern int			espEvalFile(EspRequest *ep, char *path, MprVar *result, 
						char **emsg);
extern int			espEvalScript(EspRequest *ep, char *script, MprVar *result, 
						char **emsg);
extern MprVar		*espGetLocalObject(EspRequest *ep);
extern MprVar		*espGetGlobalObject(EspRequest *ep);
extern EspHandle 	espGetRequestHandle(EspRequest *ep);
extern MprVar		*espGetResult(EspRequest *ep);
extern EjsId 		espGetScriptHandle(EspRequest *ep);
extern void			espRedirect(EspRequest *ep, int code, char *url);
extern void			espSetHeader(EspRequest *ep, char *header, 
						bool allowMultiple);
extern void			espSetReturnString(EspRequest *ep, char *str);
extern int			espWrite(EspRequest *ep, char *buf, int size);
extern int			espWriteString(EspRequest *ep, char *buf);
extern int			espWriteFmt(EspRequest *ep, char *fmt, ...);

/*
 *	ESP array[] variable access (set will update/create)
 */
extern int			espGetVar(EspRequest *ep, EspEnvType oType, char *var,
						MprVar *value);
extern char			*espGetStringVar(EspRequest *ep, EspEnvType oType, 
						char *var, char *defaultValue);
extern void			espSetVar(EspRequest *ep, EspEnvType oType, char *var, 
						MprVar value);
extern void			espSetStringVar(EspRequest *ep, EspEnvType oType, 
						char *var, char *value);
extern int			espUnsetVar(EspRequest *ep, EspEnvType oType, char *var);

/*
 *	Object creation and management
 */
extern MprVar		espCreateObjVar(char *name, int hashSize);
extern MprVar		espCreateArrayVar(char *name, int size);
extern bool			espDestroyVar(MprVar *var);
extern MprVar		*espCreateProperty(MprVar *obj, char *property, 
						MprVar *newValue);
extern MprVar		*espCreatePropertyValue(MprVar *obj, char *property, 
						MprVar newValue);
extern int			espDeleteProperty(MprVar *obj, char *property);

/*
 *	JavaScript variable management. Set will create/update a property.
 *	All return a property reference. GetProperty will optionally return the
 *	property in value.
 */
extern MprVar		*espGetProperty(MprVar *obj, char *property, 
						MprVar *value);
extern MprVar		*espSetProperty(MprVar *obj, char *property, 
						MprVar *newValue);
extern MprVar		*espSetPropertyValue(MprVar *obj, char *property, 
						MprVar newValue);

#if 0
/*
 *	Low-level direct read and write of properties. 
 *	FUTURE:  -- Read is not (dest, src). MUST WARN IN DOC ABOUT COPY/READ
 *	Will still cause triggers to run.
 */
extern int	 		espReadProperty(MprVar *dest, MprVar *prop);
extern int			espWriteProperty(MprVar *prop, MprVar *newValue);
extern int			espWritePropertyValue(MprVar *prop, MprVar newValue);
#endif


/* 
 *	Access JavaScript variables by their full name. Can use "." or "[]". For
 *	example: "global.request['REQUEST_URI']"
 *	For Read/write, the variables must exist.
 */
extern int 			espCopyVar(EspRequest *ep, char *var, MprVar *value, 
						int copyDepth);
extern int			espDeleteVar(EspRequest *ep, char *var);
extern int 			espReadVar(EspRequest *ep, char *var, MprVar *value);
extern int	 		espWriteVar(EspRequest *ep, char *var, MprVar *value);
extern int	 		espWriteVarValue(EspRequest *ep, char *var, MprVar value);

/*
 *	Object property enumeration
 */
extern MprVar		*espGetFirstProperty(MprVar *obj, int includeFlags);
extern MprVar		*espGetNextProperty(MprVar *obj, MprVar *currentProperty,
						int includeFlags);
extern int			espGetPropertyCount(MprVar *obj, int includeFlags);

#ifdef  __cplusplus
}
#endif
/******************************************************************************/
#endif /* _h_ESP_h */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
