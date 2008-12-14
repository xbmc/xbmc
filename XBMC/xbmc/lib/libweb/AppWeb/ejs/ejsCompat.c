/*
 *	@file 	ejsCompat.c
 *	@brief 	Legacy WebServer API Compatibility
 *	@overview This module provides compatibility with the legacy 
 *		WebServer APIs. These APIs are deprecated. Only use them 
 *		for legacy applications. They will be deleted in a future release.
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
/********************************** Includes **********************************/

#include	"ejsInternal.h"
#include	"ejsCompat.h"

#if BLD_FEATURE_EJS && (BLD_FEATURE_COMPAT_MODULE || BLD_FEATURE_LEGACY_API)

/*********************************** Locals ***********************************/
/*
 *	WARNING: !!!!!!!!!!!! This code is certainly not multithreaded !!!!!!
 *
 *	It is included only as part of the legacy API and will be deleted in the
 *	future.
 */
static char	resultBuf[MPR_MAX_STRING];

/************************************* Code ***********************************/
/*
 *	Utility routine to crack EJS arguments. Return the number of args
 *	seen. This routine only supports %s and %d type args.
 *
 *	Typical usage:
 *
 *		if (ejArgs(argc, argv, "%s %d", &name, &age) < 2) {
 *			error("Insufficient args\n");
 *			return -1;
 *		}
 */

int ejArgs(int argc, char **argv, char *fmt, ...)
{
	va_list	vargs;
	char	*cp, **sp;
	int		*ip;
	int		argn;

	va_start(vargs, fmt);

	if (argv == NULL) {
		return 0;
	}

	for (argn = 0, cp = fmt; cp && *cp && argv[argn]; ) {
		if (*cp++ != '%') {
			continue;
		}

		switch (*cp) {
		case 'd':
			ip = va_arg(vargs, int*);
			*ip = atoi(argv[argn]);
			break;

		case 's':
			sp = va_arg(vargs, char**);
			*sp = argv[argn];
			break;

		default:
/*
 *			Unsupported
 */
			mprAssert(0);
		}
		argn++;
	}

	va_end(vargs);
	return argn;
}

/******************************************************************************/

Ejs *ejPtr(int eid)
{
	return ejsPtr((EjsId) eid);
}

/******************************************************************************/
/*
 *	Get the result. This only supports string results.
 */

char *ejGetResult(int eid)
{
	Ejs		*ep;

	if ((ep = ejPtr(eid)) == NULL) {
		return NULL;
	}
	/*	FUTURE -- coerce to a string */
	return ep->result.string;
}

/******************************************************************************/
/*
 *	Get a simple variable that is not an object or array
 */

int ejGetVar(int eid, char *var, char **value)
{
	MprVar		v;
	char		*result;

	mprAssert(var && *var);
	mprAssert(value);

	if (ejsReadVar((EjsId) eid, var, &v) < 0) {
		return -1;
	}
	if (v.type == MPR_TYPE_STRING) {
		*value = v.string;
		return 0;
	}
	mprVarToString(&result, MPR_MAX_STRING, 0, &v);

	/*	WARNING -- not multithreaded */
	mprStrcpy(resultBuf, sizeof(resultBuf), result);
	*value = resultBuf;

	/*
	 *	This function used to return the variable stack frame. That is not
	 *	meaningful with the new EJS. So return return global. (0)
	 */
	return 0;
}

/******************************************************************************/
#if UNUSED

void ejError(Ejs *ep, char *fmt, ...)
{
	va_list		args;
	char		*buf;

	va_start(args, fmt);
	mprAllocVsprintf(&buf, MPR_MAX_HEAP_SIZE, fmt, args);
	ejsError(ep, buf);
	mprFree(buf);
	va_end(args);
}

#endif
/******************************************************************************/
/*
 *	Evaluate a script
 */

char *ejEval(int eid, char *script, char **emsg)
{
	MprVar		v;
	char		*result;

	/*	WARNING -- not multithreaded */
	static char	resultBuf[MPR_MAX_STRING];

	if (ejsEvalScript((EjsId) eid, script, &v, emsg) < 0) {
		return 0;
	}
	if (v.type == MPR_TYPE_STRING) {
		return v.string;
	}
	mprVarToString(&result, MPR_MAX_STRING, 0, &v);
	mprStrcpy(resultBuf, sizeof(resultBuf), result);
	return resultBuf;
}

/******************************************************************************/
/*
 *	Evaluate a script file
 */

#ifndef __NO_EJ_FILE
char_t *ejEvalFile(int eid, char_t *path, char_t **emsg)
{
	MprVar	v;
	char	*result;

	if (ejsEvalFile((EjsId) eid, path, &v, emsg) < 0) {
		return 0;
	}
	if (v.type == MPR_TYPE_STRING) {
		return v.string;
	}
	mprVarToString(&result, MPR_MAX_STRING, 0, &v);

	/*	WARNING -- not multithreaded */
	mprStrcpy(resultBuf, sizeof(resultBuf), result);
	return resultBuf;
}

#endif
/******************************************************************************/
/*
 *	Set a simple local variable that is not an object or array. 
 *	Note: a variable with a value of NULL means declared but undefined. 
 *	The value is defined in the top-most variable frame.
 */

void ejSetLocalVar(int eid, char *var, char *value)
{
	Ejs		*ep;

	if ((ep = ejPtr(eid)) == NULL) {
		return;
	}
	mprCreatePropertyValue(ep->local, var, mprCreateStringVar(value, 0));
}

/******************************************************************************/
/*
 *	Set a simple global variable that is not an object or array. 
 *	Note: a variable with a value of NULL means declared but undefined. 
 *	The value is defined in the global variable frame.
 */

void ejSetGlobalVar(int eid, char *var, char *value)
{
	Ejs		*ep;

	if ((ep = ejPtr(eid)) == NULL) {
		return;
	}
	mprCreatePropertyValue(ep->global, var, mprCreateStringVar(value, 1));
}

/******************************************************************************/
/*
 *	Set the result
 */

void ejSetResult(int eid, char *s)
{
	Ejs		*ep;
	MprVar	v;

	if ((ep = ejPtr(eid)) == NULL) {
		return;
	}
	/* No need to free as we don't allocate */
	v = mprCreateStringVar(s, 0);
	mprCopyVar(&ep->result, &v, MPR_SHALLOW_COPY);
}

/******************************************************************************/
/*
 *	Set a variable that is not an object or array. Note: a variable with a 
 *	value of NULL means declared but undefined. The value is defined in the 
 *	top-most variable frame.
 */

void ejSetVar(int eid, char *var, char *value)
{
	Ejs		*ep;

	mprAssert(var && *var);

	if ((ep = ejPtr(eid)) == NULL) {
		return;
	}

	mprCreatePropertyValue(ep->local, var, mprCreateStringVar(value, 0));
}

/******************************************************************************/
/*
 *	Define a function
 */

int ejSetGlobalFunction(int eid, char *name,
	int (*fn)(int eid, void *handle, int argc, char **argv))
{
	Ejs					*ep;
	MprStringCFunction	sfn = (MprStringCFunction) fn;

	if ((ep = ejPtr(eid)) == NULL) {
		return -1;
	}
	/* Must use the web server handle */
	ejsDefineStringCFunction((EjsId) eid, name, sfn, 0, MPR_VAR_ALT_HANDLE);
	return 0;
}

/******************************************************************************/

#else
void ejsCompatDummy() {}

/******************************************************************************/
#endif /* BLD_FEATURE_EJS && ... */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
