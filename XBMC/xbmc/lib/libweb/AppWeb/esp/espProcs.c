/*
 *	@file 	espProcs.c
 *	@brief 	Embedded Server Pages (ESP) Procedures.
 *	@overview These ESP procedures can be used in ESP pages for common tasks.
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

#include	"esp.h"

/************************************ Code ************************************/
#if BLD_FEATURE_ESP_MODULE
#if BLD_FEATURE_SESSION
/*
 *	destroySession
 */

static int destroySessionProc(EspRequest *ep, int argc, char **argv)
{
	ep->esp->destroySession(ep->requestHandle);
	return 0;
}

#endif	/* BLD_FEATURE_SESSION */

/******************************************************************************/
/*
 *	include
 *
 *	This includes javascript libraries. For example:
 *
 *		<% include("file", ...); %> 
 *
 *	Don't confuse with ESP includes:
 *
 *		<% include file.esp %>
 *
 *	Filenames are relative to the base document including the file.
 *	FUTURE -- move back to EJS. Only here now because we need ep->readFile.
 */ 

static int includeProc(EspRequest *ep, int argc, char **argv)
{
	Esp		*esp;
	char	path[MPR_MAX_FNAME], dir[MPR_MAX_FNAME];
	char	*emsg, *buf;
	int		size, i;

	esp = ep->esp;
	mprAssert(argv);
	for (i = 0; i < argc; i++) {
		mprGetDirName(dir, sizeof(dir), ep->docPath);
		mprSprintf(path, sizeof(path), "%s/%s", dir, argv[i]);
		
		if (esp->readFile(ep->requestHandle, &buf, &size, path) < 0) {
			espError(ep, "Can't read include file: %s", path);
			return MPR_ERR_CANT_ACCESS;
		}
		buf[size] = '\0';

		if (ejsEvalScript(espGetScriptHandle(ep), buf, 0, &emsg) < 0) {
			espError(ep, "Cant evaluate script");
			mprFree(buf);
			return -1;
		}
		mprFree(buf);
	}
	return 0;
}

/******************************************************************************/
/*
 *	redirect
 *
 *	This implemements <% redirect(url, code); %> command. The redirection 
 *	code is optional.
 */ 

static int redirectProc(EspRequest *ep, int argc, char **argv)
{
	char	*url;
	int		code;

	if (argc < 1) {
		espError(ep, "Bad args");
		return MPR_ERR_BAD_ARGS;
	}
	url = argv[0];
	if (argc == 2) {
		code = atoi(argv[1]);
	} else {
		code = 302;
	}
	espRedirect(ep, code, url);
	return 0;
}

/******************************************************************************/
#if BLD_FEATURE_SESSION
/*
 *	useSession
 */

static int useSessionProc(EspRequest *ep, int argc, char **argv)
{
	int			timeout;

	if (argc > 1) {
		espError(ep, "Bad args");
		return MPR_ERR_BAD_ARGS;

	} else if (argc == 1) {
		timeout = atoi(argv[0]);
	} else {
		timeout = 0;
	}
	
	ep->esp->createSession(ep->requestHandle, timeout);
	espSetReturnString(ep, ep->esp->getSessionId(ep->requestHandle));
	return 0;
}

#endif /* BLD_FEATURE_SESSION */
/******************************************************************************/
/*
 *	setHeader
 *
 *	This implemements <% setHeader("key: value", allowMultiple); %> command.
 */ 

static int setHeaderProc(EspRequest *ep, int argc, char **argv)
{
	mprAssert(argv);
	if (argc != 2) {
		espError(ep, "Bad args");
		return MPR_ERR_BAD_ARGS;
	}
	ep->esp->setHeader(ep->requestHandle, argv[0], atoi(argv[1]));
	return 0;
}

/******************************************************************************/
/*
 *	write
 *
 *	This implemements <% write("text"); %> command.
 */ 

static int writeProc(EspRequest *ep, int argc, char **argv)
{
	char	*s;
	int		i, len;

	mprAssert(argv);
	for (i = 0; i < argc; i++) {
		s = argv[i];
		len = strlen(s);
		if (len > 0) {
			if (espWrite(ep, s, len) != len) {
				espError(ep, "Can't write to client");
				return -1;
			}
		}
	}
	return 0;
}

/******************************************************************************/
/*
 *	rename
 *
 *	This implemements <% rename(oldFile, newFile); %> command.
 */ 

static int renameProc(EspRequest *ep, int argc, char **argv)
{
	if (argc != 2) {
		espError(ep, "Bad args");
		return -1;
	}
	if (rename(argv[0], argv[1]) < 0) {
		espError(ep, "Can't rename uploaded file");
	}
	return 0;
}

/******************************************************************************/

void espRegisterProcs()
{
	espDefineStringCFunction(0, "rename", renameProc, 0);
	espDefineStringCFunction(0, "write", writeProc, 0);
	espDefineStringCFunction(0, "setHeader", setHeaderProc, 0);
	espDefineStringCFunction(0, "redirect", redirectProc, 0);
	espDefineStringCFunction(0, "include", includeProc, 0);

#if BLD_FEATURE_SESSION
	/*
	 *	Create and use are synonomous
	 */
	espDefineStringCFunction(0, "useSession", useSessionProc, 0);
	espDefineStringCFunction(0, "createSession", useSessionProc, 0);
	espDefineStringCFunction(0, "destroySession", destroySessionProc, 0);
#endif
}

/******************************************************************************/

#else
void mprEspControlsDummy() {}

#endif /* BLD_FEATURE_ESP_MODULE */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
