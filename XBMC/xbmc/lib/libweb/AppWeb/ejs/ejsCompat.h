/*
 *	@file 	ejsCompat.h
 *	@brief 	Compatability with the legacy WebServer APIs
 *	@overview This module provides compatibility with the legacy 
 *		WebServer APIs. These APIs are deprecated. Only use them 
 *		for legacy applications. They will be deleted in a future release.
 *	@deprecated Deprecated in AppWeb 2.0
 */
/********************************* Copyright **********************************/
/*
 *	@copy	default.g
 *	
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	Portions Copyright (c) GoAhead Software, 1995-2000. All Rights Reserved.
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
/********************************* Includes ***********************************/
#ifndef _h_EJS_WEBS
#define _h_EJS_WEBS 1

#if BLD_FEATURE_EJS && (BLD_FEATURE_COMPAT_MODULE || BLD_FEATURE_LEGACY_API)

/*************************** WebServer Compatibility API **********************/
#ifdef __cplusplus
extern "C" {
#endif

/*
 *	These APIs are deprecated. Only use them to support legacy applications.
 *	They will be removed in a future release.
 */

#ifndef CHAR_T_DEFINED
typedef char char_t;
#endif

/*
 *	Some functions can just be renamed
 */
#define ejCloseBlock 	ejsCloseBlock
#define ejGetLineNumber ejsGetLineNumber
#define ejGetUserHandle ejsGetUserHandle
#define ejOpenBlock		ejsOpenBlock
#define ejSetUserHandle	ejsSetUserHandle

/*
 *	Some need to be wrapped. See ejsCompat.c
 */
extern int 		ejArgs(int argc, char_t **argv, char_t *fmt, ...);
extern char_t	*ejGetResult(int eid);
extern int		ejGetVar(int eid, char_t *var, char_t **value);
extern char_t	*ejEvalBlock(int eid, char_t *script, char_t **emsg);
extern char_t	*ejEval(int eid, char_t *script, char_t **emsg);
#ifndef __NO_EJ_FILE
extern char_t	*ejEvalFile(int eid, char_t *path, char_t **emsg);
#endif
extern int 		ejSetGlobalFunction(int eid, char_t *name, 
					int (*fn)(int eid, void *handle, int argc, char_t **argv));
extern void		ejSetGlobalVar(int eid, char_t *var, char_t *value);
extern void		ejSetLocalVar(int eid, char_t *var, char_t *value);
extern void		ejSetResult(int eid, char_t *s);
extern void		ejSetVar(int eid, char_t *var, char_t *value);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/
#endif /* BLD_FEATURE_EJS && ... */
#endif /* _h_EJS_WEBS */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
