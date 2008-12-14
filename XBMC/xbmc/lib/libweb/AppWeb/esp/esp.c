/*
 *	@file 	esp.c
 *	@brief 	Embedded Server Pages (ESP) core processing.
 *	@overview Embedded Server Pages provides an efficient way to generate 
 *		dynamic pages using server-side Javascript. This code provides 
 *		core processing, and should be called by an associated web 
 *		server URL handler.
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

#if BLD_FEATURE_ESP_MODULE

/*********************************** Locals ***********************************/
/*
 *	Master ESP control interface with the web server
 */

static Esp *esp;

/***************************** Forward Declarations ***************************/

static int	buildScript(EspRequest *ep, char **jsBuf, char *input, char
				**errMsg);
static int	growTokenBuf(EspParse *parse, int nchar);

/************************************ Code ************************************/
/*
 *	Called at server initialization
 */

int espOpen(Esp *control)
{
	mprAssert(control);

#if BLD_FEATURE_MULTITHREAD
	ejsOpen(control->lock, control->unlock, control->lockData);
#else
	ejsOpen(0, 0, 0);
#endif

	/*
 	 *	Register the standard procedures
 	 */
	espRegisterProcs();

	/*
 	 *	Just for brain dead systems that don't zero global memory
 	 */
	esp = control;
	return 0;
}

/******************************************************************************/
/*
 *	Called at server termination
 */

void espClose()
{
	ejsClose();
}

/******************************************************************************/
/*
 *	Create for new ESP request. Assumed that this is called after all the 
 *	HTTP headers have been read but before POST data has been read. It is 
 *	expected that any session cookies have been read and that "variables" 
 *	contains references to all the environment objects including "session". 
 *	requestHandle is the web server request handle.
 */

EspRequest *espCreateRequest(EspHandle webServerRequestHandle, char *uri, 
	MprVar *variables)
{
	EspRequest	*ep;
	MprVar		*global;
#if BLD_FEATURE_LEGACY_API
	MprVar		*np;
	char		keyBuf[ESP_MAX_HEADER];
	int			i;
#endif

	mprAssert(variables);

	ep = (EspRequest*)mprMalloc(sizeof(EspRequest));
	if (ep == 0) {
		return 0;
	}
	memset(ep, 0, sizeof(EspRequest));
	ep->requestHandle = webServerRequestHandle;
	ep->esp = esp;
	ep->uri = mprStrdup(uri);
	ep->docPath = 0;
	ep->variables = variables;
	
	/*
 	 *	The handle passed to ejsOpenEngine is passed to every C function 
	 *	called by JavaScript.
	 */
	ep->eid = ejsOpenEngine((EjsHandle) ep, (EjsHandle) webServerRequestHandle);
	if (ep->eid < 0) {
		mprFree(ep);
		return 0;
	}

	/*
	 *	All these copies and SetProperties will only copy references 
	 *	They will increments the object ref counts.
	 */
	mprCopyVar(&variables[ESP_GLOBAL_OBJ], ejsGetGlobalObject(ep->eid), 
		MPR_SHALLOW_COPY);
	mprCopyVar(&variables[ESP_LOCAL_OBJ], ejsGetLocalObject(ep->eid), 
		MPR_SHALLOW_COPY);

	global = &variables[ESP_GLOBAL_OBJ];
	mprCreateProperty(global, "application", &variables[ESP_APPLICATION_OBJ]);
	mprCreateProperty(global, "cookies", &variables[ESP_COOKIES_OBJ]);
	mprCreateProperty(global, "files", &variables[ESP_FILES_OBJ]);
	mprCreateProperty(global, "form", &variables[ESP_FORM_OBJ]);
	mprCreateProperty(global, "headers", &variables[ESP_HEADERS_OBJ]);
	mprCreateProperty(global, "request", &variables[ESP_REQUEST_OBJ]);

	//
	//	FUTURE -- could server be shared across all requests for a given host
	//	and be made read-only.
	//
	mprCreateProperty(global, "server", &variables[ESP_SERVER_OBJ]);

#if BLD_FEATURE_SESSION
	mprCreateProperty(global, "session", &variables[ESP_SESSION_OBJ]);
#endif

#if BLD_FEATURE_LEGACY_API
	/*
	 *	DEPRECATED: 2.0
	 *	Define variables as globals. headers[] are prefixed with "HTTP_".
 	 *	NOTE: MaRequest::setVar does not copy into globals, whereas espSetVar
	 *	does if legacy_api is defined. So variables pre-defined by MaRequest 
 	 *	must be copied here into globals[].
	 *
	 *	NOTE: if a variable is in session[] and in form[], the form[] will
	 *	override being later in the variables[] list. Use mprSetProperty 
	 *	instead of mprCreateProperty to cover for this case.
	 */
	for (i = 0; i < ESP_OBJ_MAX; i++) {
		if (i == ESP_GLOBAL_OBJ || i == ESP_LOCAL_OBJ) {
			continue;
		}
		if (variables[i].type != MPR_TYPE_OBJECT) {
			continue;
		}
		np = mprGetFirstProperty(&variables[i], MPR_ENUM_DATA);
		while (np) {
			if (i == ESP_HEADERS_OBJ) {
				mprSprintf(keyBuf, sizeof(keyBuf) - 1, "HTTP_%s", np->name);
				mprSetProperty(global, keyBuf, np);
			} else {
				mprSetProperty(global, np->name, np);
			}
			np = mprGetNextProperty(&variables[i], np, MPR_ENUM_DATA);
		}
	}
#endif
	return ep;
}

/******************************************************************************/
 
void espDestroyRequest(EspRequest *ep)
{
	mprAssert(ep);
	mprAssert(ep->eid >= 0);

	mprFree(ep->uri);
	mprFree(ep->docPath);

	mprDestroyVar(&ep->variables[ESP_GLOBAL_OBJ]);
	mprDestroyVar(&ep->variables[ESP_LOCAL_OBJ]);

	ejsCloseEngine(ep->eid);
	mprFree(ep);
}

/******************************************************************************/
/*
 *	The callback function will be called:
 *
 *		(fn)(EjsId eid, EspRequest *ep, argc, argv);
 *
 *	Callers can get their web server handle by calling:
 *
 *		rq = (requiredCast) espGetHandle(ep);
 */

void espDefineCFunction(EspRequest *ep, char *functionName, EspCFunction fn, 
	void *thisPtr)
{
	mprAssert(functionName && *functionName);
	mprAssert(fn);

	if (ep) {
		ejsDefineCFunction(ep->eid, functionName, (MprCFunction) fn, 
			thisPtr, 0);
	} else {
		ejsDefineCFunction(-1, functionName, (MprCFunction) fn, thisPtr, 0);
	}
}

/******************************************************************************/

void espDefineStringCFunction(EspRequest *ep, char *functionName, 
	EspStringCFunction fn, void *thisPtr)
{
	mprAssert(functionName && *functionName);
	mprAssert(fn);

	if (ep) {
		ejsDefineStringCFunction(ep->eid, functionName, (MprStringCFunction) fn,
			thisPtr, 0);
	} else {
		ejsDefineStringCFunction(-1, functionName, (MprStringCFunction) fn, 
			thisPtr, 0);
	}
}

/******************************************************************************/

void *espGetRequestHandle(EspRequest *ep)
{
	return ep->requestHandle;
}

/******************************************************************************/

EjsId espGetScriptHandle(EspRequest *ep)
{
	return ep->eid;
}

/******************************************************************************/

char *espGetStringVar(EspRequest *ep, EspEnvType oType, char *var, 
	char *defaultValue)
{
	MprVar	value;

	if (espGetVar(ep, oType, var, &value) < 0 || 
			value.type != MPR_TYPE_STRING) {
		return defaultValue;
	} 
	return value.string;
}

/******************************************************************************/

int espGetVar(EspRequest *ep, EspEnvType oType, char *var, MprVar *value)
{
	MprVar		*vp;

	mprAssert(ep);
	mprAssert(var);

	vp = mprGetProperty(&ep->variables[oType], var, 0);
	if (vp == 0) {
		return -1;
	}
	*value = *vp;
	return 0;
}

/******************************************************************************/
/*
 *	Process the ESP page. docBuf holds the page already. We expect that
 *	ep->variables holds all the pertinent environment variables.
 */

int espProcessRequest(EspRequest *ep, char *docPath, char *docBuf, 
	char **errMsg)
{
	char	*jsBuf;

	mprAssert(ep);

	ep->docPath = mprStrdup(docPath);

	jsBuf = 0;
	if (buildScript(ep, &jsBuf, docBuf, errMsg) < 0) {
		return MPR_ERR_CANT_COMPLETE;
	}

	if (jsBuf) {
		mprLog(7, "esp: script is:\n%s\n", jsBuf);

		/*
		 *	Now evaluate the entire escript
		 */
		if (ejsEvalScript(ep->eid, jsBuf, 0, errMsg) < 0) {
			mprFree(jsBuf);
			return MPR_ERR_ABORTED;
		}

		mprFree(jsBuf);
	}
	return 0;
}

/******************************************************************************/

void espRedirect(EspRequest *ep, int code, char *url)
{
	mprAssert(ep);
	mprAssert(url);

	ep->esp->redirect(ep->requestHandle, code, url);
}

/******************************************************************************/

void espError(EspRequest *ep, char *fmt, ...)
{
	va_list		args;
	char		*buf;

	mprAssert(ep);
	mprAssert(fmt);

	va_start(args, fmt);
	mprAllocVsprintf(&buf, MPR_MAX_HEAP_SIZE, fmt, args);
	ejsSetErrorMsg(ep->eid, buf);
	mprFree(buf);
	va_end(args);
}

/******************************************************************************/

void espSetHeader(EspRequest *ep, char *header, bool allowMultiple)
{
	mprAssert(ep);

	ep->esp->setHeader(ep->requestHandle, header, allowMultiple);
}

/******************************************************************************/
/*
 *	Caller does not need to destroy the var
 */

MprVar *espGetResult(EspRequest *ep)
{
	mprAssert(ep);

	return ejsGetReturnValue(ep->eid);
}

/******************************************************************************/

void espSetReturn(EspRequest *ep, MprVar value)
{
	mprAssert(ep);

	ejsSetReturnValue(ep->eid, value);
}

/******************************************************************************/

void espSetReturnString(EspRequest *ep, char *str)
{
	mprAssert(ep);

	ejsSetReturnValue(ep->eid, mprCreateStringVar(str, 0));
}

/******************************************************************************/

void espSetResponseCode(EspRequest *ep, int code)
{
	mprAssert(ep);

	ep->esp->setResponseCode(ep->requestHandle, code);
}

/******************************************************************************/

void espSetVar(EspRequest *ep, EspEnvType oType, char *var, MprVar value)
{
	mprSetPropertyValue(&ep->variables[oType], var, value);
}

/******************************************************************************/

void espSetStringVar(EspRequest *ep, EspEnvType oType, char *var, char *value)
{
	/*
 	 *	Will create or update if already existing
 	 */
	mprSetPropertyValue(&ep->variables[oType], var, 
		mprCreateStringVar(value, 0));
}

/******************************************************************************/

int espUnsetVar(EspRequest *ep, EspEnvType oType, char *var)
{
	return mprDeleteProperty(&ep->variables[oType], var);
}

/******************************************************************************/

int espWrite(EspRequest *ep, char *buf, int size)
{
	mprAssert(ep);
	mprAssert(buf);
	mprAssert(size >= 0);

	return ep->esp->writeBlock(ep->requestHandle, buf, size);
}

/******************************************************************************/

int espWriteString(EspRequest *ep, char *buf)
{
	mprAssert(ep);
	mprAssert(buf);

	return ep->esp->writeBlock(ep->requestHandle, buf, strlen(buf));
}

/******************************************************************************/

int espWriteFmt(EspRequest *ep, char *fmt, ...)
{
	va_list		args;
	char		*buf;
	int			rc, len;

	mprAssert(ep);
	mprAssert(fmt);

	va_start(args, fmt);
	len = mprAllocVsprintf(&buf, MPR_MAX_HEAP_SIZE, fmt, args);
	rc = ep->esp->writeBlock(ep->requestHandle, buf, len);
	mprFree(buf);
	va_end(args);
	return rc;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/*
 *	Get a javascript identifier. Must allow x.y['abc'] or x.y["abc"].
 *	Must be careful about quoting and only allow quotes inside []. 
 */

static int getIdentifier(EspParse *parse)
{
	int		quote, bracket, prevC, c;

	mprAssert(parse);

	quote = 0;
	bracket = 0;
	prevC = 0;
	c = *parse->inp++;

	while (isalnum(c) || c == '_' || c == '.' || c == '[' || 
			c == ']' || c == '\'' || c == '\"') {
		if (c == '\'' || c == '\"') {
			if (c == quote) {
				quote = 0;
			} else if (prevC == '[') {
				quote = c;
			} else {
				break;
			}

		} else if (c == '[') {
			bracket = c;

		} else if (c == ']') {
			if (bracket == 0) {
				break;
			}
			bracket = 0;
		}
		if (growTokenBuf(parse, 2) < 0) {
			return ESP_TOK_ERR;
		}
		*parse->tokp++ = c;
		prevC = c;
		c = *parse->inp++;
	}

	parse->inp--;
	*parse->tokp = '\0';

	return 0;
}

/******************************************************************************/
/*
 *	Get the next ESP input token. input points to the next input token.
 *	parse->token will hold the parsed token. The function returns the token id.
 */

static int getEspToken(int state, EspParse *parse)
{
	char	*cp;
	int		tid, done, c, quoted;

	tid = ESP_TOK_LITERAL;
	parse->tokp = parse->token;
	parse->tokp[0] = '\0';
	quoted = 0;

	c = *parse->inp++;
	for (done = 0; !done; c = *parse->inp++) {

		/*
		 *	Get room for 3 more characters in the token buffer
		 */
		if (growTokenBuf(parse, 3) < 0) {
			return ESP_TOK_ERR;
		}

		switch (c) {
		case 0:
			if (*parse->token) {
				done++;
				parse->inp--;
				break;
			}
			return ESP_TOK_EOF;

		default:
			if (c == '\"' && state != ESP_STATE_IN_ESP_TAG) {
				*parse->tokp++ = '\\';
			}
			*parse->tokp++ = c;
			quoted = 0;
			break;

		case '\\':
			if (state != ESP_STATE_IN_ESP_TAG) {
				*parse->tokp++ = c;
			}
			quoted = 1;
			*parse->tokp++ = c;
			break;

		case '@':
			if (*parse->inp == '@' && state != ESP_STATE_IN_ESP_TAG) {
				if (quoted) {
					parse->tokp--;
					quoted = 0;
				} else {
					if (*parse->token) {
						parse->inp--;
					} else {
						parse->inp++;
						tid = ESP_TOK_ATAT;
						if (getIdentifier(parse) < 0) {
							return ESP_TOK_ERR;
						}
					}
					done++;
					break;
				}
			}
			*parse->tokp++ = c;
			break;

		case '<':
			if (*parse->inp == '%' && state != ESP_STATE_IN_ESP_TAG) {
				if (quoted) {
					parse->tokp--;
					quoted = 0;
					*parse->tokp++ = c;
					break;
				}
				if (*parse->token) {
					parse->inp--;
					done++;
					break;
				}
				parse->inp++;
				while (isspace((int) *parse->inp)) {
					parse->inp++;
				}
				if (*parse->inp == '=') {
					parse->inp++;
					while (isspace((int) *parse->inp)) {
						parse->inp++;
					}
					tid = ESP_TOK_EQUALS;
					if (getIdentifier(parse) < 0) {
						return ESP_TOK_ERR;
					}
					done++;
					break;
				}
				if (*parse->inp == 'i' && 
						strncmp(parse->inp, "include", 7) == 0 &&
						isspace((int) parse->inp[7])) {
					tid = ESP_TOK_INCLUDE;
					parse->inp += 7;
					while (isspace((int) *parse->inp)) {
						parse->inp++;
					}
					while (*parse->inp && !isspace((int) *parse->inp) && 
							*parse->inp != '%' && parse->tokp < parse->endp) {
						if (growTokenBuf(parse, 2) < 0) {
							return ESP_TOK_ERR;
						}
						*parse->tokp++ = *parse->inp++;
					}
					*parse->tokp = '\0';
					if (parse->token[0] == '"') {
						parse->tokp = parse->token;
						for (cp = &parse->token[1]; *cp; ) {
							if (growTokenBuf(parse, 2) < 0) {
								return ESP_TOK_ERR;
							}
							*parse->tokp++ = *cp++;
						}
						if (cp[-1] == '"') {
							parse->tokp--;
						}
						*parse->tokp = '\0';
					}
					
				} else {
					tid = ESP_TOK_START_ESP;
				}
				done++;
				break;
			}
			*parse->tokp++ = c;
			break;

		case '%':
			if (*parse->inp == '>' && state == ESP_STATE_IN_ESP_TAG) {
				if (quoted) {
					parse->tokp--;
					quoted = 0;
				} else {
					if (*parse->token) {
						parse->inp--;
					} else {
						tid = ESP_TOK_END_ESP;
						parse->inp++;
					}
					done++;
					break;
				}
			}
			*parse->tokp++ = c;
			break;
		}
	}

	*parse->tokp = '\0';
	parse->inp--;
	return tid;
}

/******************************************************************************/
/*
 *	Get room for more (nchar) characters in the token buffer
 */

static int growTokenBuf(EspParse *parse, int nchar)
{
	char	*newBuf;
	int		extra;

	mprAssert(parse);
	mprAssert(parse->tokp);
	mprAssert(parse->endp);
	mprAssert(nchar > 0);

	if (parse->tokp >= &parse->endp[- nchar]) {
		extra = max(nchar, ESP_TOK_INCR);
		newBuf = (char*) mprRealloc(parse->token, 
			parse->tokLen + extra);
		if (newBuf == 0) {
			return ESP_TOK_ERR;
		}

		parse->tokp += (newBuf - parse->token);
		parse->endp += (newBuf - parse->token);
		parse->tokLen += extra;
		parse->endp += extra;
		parse->token = newBuf;
		*parse->tokp = '\0';
	}
	return 0;
}

/******************************************************************************/
/*
 *	Convert an ESP page into a JavaScript. We also expand include files.
 */

static int buildScript(EspRequest *ep, char **jsBuf, char *input, char **errMsg)
{
	EspParse	parse;
	char		path[MPR_MAX_FNAME], dir[MPR_MAX_FNAME], incPath[MPR_MAX_FNAME];
	char		*incBuf, *incText;
	int			state, tid, len, rc, maxScriptSize, incSize;

	mprAssert(ep);
	mprAssert(jsBuf);
	mprAssert(input);

	rc = 0;
	len = 0;
	state = ESP_STATE_BEGIN;
	if (errMsg) {
		*errMsg = 0;
	}

	memset(&parse, 0, sizeof(parse));
	parse.token = (char*) mprMalloc(ESP_TOK_INCR);
	if (parse.token == 0) {
		return MPR_ERR_CANT_ALLOCATE;
	}
	parse.token[0] = '\0';
	parse.tokLen = ESP_TOK_INCR;
	parse.endp = &parse.token[parse.tokLen - 1];
	parse.tokp = parse.token;
	parse.inBuf = input;
	parse.inp = parse.inBuf;

	maxScriptSize = esp->maxScriptSize;

	tid = getEspToken(state, &parse);
	while (tid != ESP_TOK_EOF && len >= 0) {

		switch (tid) {
		default:
		case ESP_TOK_ERR:
			mprFree(parse.token);
			return MPR_ERR_BAD_SYNTAX;
			
		case ESP_TOK_LITERAL:
			len = mprReallocStrcat(jsBuf, maxScriptSize, len, 0, 
				"write(\"", parse.token, "\");\n", (void*) 0);
			break;

		case ESP_TOK_ATAT:
			/*
			 *	Trick to get undefined variables to evaluate to "".
			 *	Catenate with "" to cause toString to run. 
			 */
			len = mprReallocStrcat(jsBuf, maxScriptSize, len, 0, 
				"write(\"\" + ", parse.token, ");\n", (void*) 0);
			break;

		case ESP_TOK_EQUALS:
			len = mprReallocStrcat(jsBuf, maxScriptSize, len, 0, 
				"write(\"\" + ", parse.token, ");\n", (void*) 0);
			state = ESP_STATE_IN_ESP_TAG;
			break;

		case ESP_TOK_START_ESP:
			state = ESP_STATE_IN_ESP_TAG;
			tid = getEspToken(state, &parse);
			while (tid != ESP_TOK_EOF && tid != ESP_TOK_EOF && 
					tid != ESP_TOK_END_ESP && len >= 0) {
				len = mprReallocStrcat(jsBuf, maxScriptSize, len, 0, 
					parse.token, (void*) 0);
				tid = getEspToken(state, &parse);
			}
			state = ESP_STATE_BEGIN;
			break;

		case ESP_TOK_END_ESP:
			state = ESP_STATE_BEGIN;
			break;

		case ESP_TOK_INCLUDE:
			if (parse.token[0] == '/') {
				mprStrcpy(incPath, sizeof(incPath), parse.token);
			} else {
				mprGetDirName(dir, sizeof(dir), ep->uri);
				mprSprintf(incPath, sizeof(incPath), "%s/%s", 
					dir, parse.token);
			}
			if (esp->mapToStorage(ep->requestHandle, path, sizeof(path),
					incPath, 0) < 0) {
				mprAllocSprintf(errMsg, MPR_MAX_STRING, 
					"Can't find include file: %s", path);
				rc = MPR_ERR_CANT_OPEN;
				break;
			}
			if (esp->readFile(ep->requestHandle, &incText, &incSize, 
					path) < 0) {
				mprAllocSprintf(errMsg, MPR_MAX_STRING, 
					"Can't read include file: %s", path);
				rc = MPR_ERR_CANT_READ;
				break;
			}
			incText[incSize] = '\0';

			/*
			 *	Recurse and process the include script
			 */
			incBuf = 0;
			if ((rc = buildScript(ep, &incBuf, incText, errMsg)) < 0) {
				mprFree(incText);
				mprFree(parse.token);
				return rc;
			}

			len = mprReallocStrcat(jsBuf, maxScriptSize, len, 0, incBuf, 
				(void*) 0);
			mprFree(incText);
			mprFree(incBuf);
			state = ESP_STATE_IN_ESP_TAG;
			break;
		}
		tid = getEspToken(state, &parse);
	}
	mprFree(parse.token);
	if (len < 0) {
		mprAllocSprintf(errMsg, MPR_MAX_STRING, 
			"Script token is too big in %s.\nConfigured maximum is %d.", 
			path, maxScriptSize);
		return MPR_ERR_WONT_FIT;
	}
	return rc;
}

/******************************************************************************/
/******************************* Wrapped Routines *****************************/
/******************************************************************************/

int espCopyVar(EspRequest *ep, char *var, MprVar *value, int copyDepth)
{
	return ejsCopyVar(ep->eid, var, value, copyDepth);
}

/******************************************************************************/

MprVar espCreateObjVar(char *name, int hashSize)
{
	return ejsCreateObj(name, hashSize);
}

/******************************************************************************/

MprVar espCreateArrayVar(char *name, int size)
{
	return ejsCreateArray(name, size);
}

/******************************************************************************/

bool espDestroyVar(MprVar *obj)
{
	return ejsDestroyVar(obj);
}

/******************************************************************************/

MprVar *espCreateProperty(MprVar *obj, char *property, MprVar *newValue)
{
	return mprCreateProperty(obj, property, newValue);
}

/******************************************************************************/

MprVar *espCreatePropertyValue(MprVar *obj, char *property, MprVar newValue)
{
	return mprCreatePropertyValue(obj, property, newValue);
}

/******************************************************************************/

void espDefineFunction(EspRequest *ep, char *functionName, char *args, char *body)
{
	ejsDefineFunction(ep->eid, functionName, args, body);
}

/******************************************************************************/

int espDeleteProperty(MprVar *obj, char *property)
{
	return mprDeleteProperty(obj, property);
}

/******************************************************************************/

int espDeleteVar(EspRequest *ep, char *var)
{
	return ejsDeleteVar(ep->eid, var);
}

/******************************************************************************/
int espEvalFile(EspRequest *ep, char *path, MprVar *result, char **emsg)
{
	return ejsEvalFile(ep->eid, path, result, emsg);
}

/******************************************************************************/

int espEvalScript(EspRequest *ep, char *script, MprVar *result, char **emsg)
{
	return ejsEvalScript(ep->eid, script, result, emsg);
}

/******************************************************************************/

int espGetPropertyCount(MprVar *obj, int includeFlags)
{
	if (obj->type != MPR_TYPE_OBJECT) {
		return MPR_ERR_BAD_STATE;
	}
	return mprGetPropertyCount(obj, includeFlags);
}

/******************************************************************************/

MprVar *espGetFirstProperty(MprVar *obj, int includeFlags)
{
	return mprGetFirstProperty(obj, includeFlags);
}

/******************************************************************************/

MprVar *espGetGlobalObject(EspRequest *ep)
{
	return ejsGetGlobalObject(ep->eid);
}

/******************************************************************************/

MprVar *espGetLocalObject(EspRequest *ep)
{
	return ejsGetLocalObject(ep->eid);
}

/******************************************************************************/

MprVar *espGetNextProperty(MprVar *obj, MprVar *currentProperty, 
	int includeFlags)
{
	return mprGetNextProperty(obj, currentProperty, includeFlags);
}

/******************************************************************************/

MprVar *espGetProperty(MprVar *obj, char *property, MprVar *value)
{
	return mprGetProperty(obj, property, value);
}

/******************************************************************************/

void *espGetThisPtr(EspRequest *ep)
{
	return ejsGetThisPtr(ep->eid);
}

/******************************************************************************/
#if UNUSED
 
int espReadProperty(MprVar *dest, MprVar *prop)
{
	mprAssert(prop);
	mprAssert(dest);

	*dest = *prop;
	return 0;
}

#endif
/******************************************************************************/

int espReadVar(EspRequest *ep, char *var, MprVar *value)
{
	return ejsReadVar(ep->eid, var, value);
}

/******************************************************************************/

int espRunFunction(EspRequest *ep, MprVar *obj, char *functionName, 
	MprArray *args)
{
	return ejsRunFunction(ep->eid, obj, functionName, args);
}

/******************************************************************************/

MprVar *espSetProperty(MprVar *obj, char *property, MprVar *newValue)
{
	return mprSetProperty(obj, property, newValue);
}

/******************************************************************************/

MprVar *espSetPropertyValue(MprVar *obj, char *property, MprVar newValue)
{
	return mprSetPropertyValue(obj, property, newValue);
}

/******************************************************************************/

int espWriteVar(EspRequest *ep, char *var, MprVar *value)
{
	return ejsWriteVar(ep->eid, var, value);
}

/******************************************************************************/

int espWriteVarValue(EspRequest *ep, char *var, MprVar value)
{
	return ejsWriteVarValue(ep->eid, var, value);
}

/******************************************************************************/
#if UNUSED

int espWriteProperty(MprVar *prop, MprVar *newValue)
{
	return mprWriteProperty(prop, newValue);
}

/******************************************************************************/

int espWritePropertyValue(MprVar *prop, MprVar newValue)
{
	return mprWritePropertyValue(prop, newValue);
}

#endif
/******************************************************************************/

#else	/* !BLD_FEATURE_ESP_MODULE */
void espDummy() {}

/******************************************************************************/
#endif /* BLD_FEATURE_ESP_MODULE */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4
 */
