/*
 *	@file 	ejs.c
 *	@brief 	Embedded JavaScript (EJS) 
 *	@overview Main module interface logic.
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

#if BLD_FEATURE_EJS

/********************************** Local Data ********************************/

/*
 *	These fields must be locked before any access when multithreaded
 */
static MprVar	master;					/* Master object */
static MprArray	*ejsList;				/* List of ej handles */

#if BLD_FEATURE_MULTITHREAD
static EjsLock		lock;
static EjsUnlock	unlock;
static void			*lockData;
#define ejsLock()	if (lock) { (lock)(lockData); } else
#define ejsUnlock()	if (unlock) { (unlock)(lockData); } else
#else
#define ejsLock()		
#define ejsUnlock()	
#endif

/****************************** Forward Declarations **************************/

static char	*getNextVarToken(char **next, char *tokBuf, int tokBufLen);

/************************************* Code ***********************************/
/*
 *	Initialize the EJ subsystem
 */

int ejsOpen(EjsLock lockFn, EjsUnlock unlockFn, void *data)
{
	MprVar	*np;

#if BLD_FEATURE_MULTITHREAD
	if (lockFn) {
		lock = lockFn;
		unlock = unlockFn;
		lockData = data;
	}
#endif
	ejsLock();

	/*
	 *	Master is the top level object (above global). It is used to clone its
	 *	contents into the global scope for each. This is never visible to the
	 *	user, so don't use ejsCreateObj().
	 */
	master = mprCreateObjVar("master", EJS_SMALL_OBJ_HASH_SIZE);
	if (master.type == MPR_TYPE_UNDEFINED) {
		ejsUnlock();
		return MPR_ERR_CANT_ALLOCATE;
	}

	ejsList = mprCreateArray();
	ejsDefineStandardProperties(&master);

	/*
	 *	Make these objects immutable
	 */
	np = mprGetFirstProperty(&master, MPR_ENUM_FUNCTIONS | MPR_ENUM_DATA);
	while (np) {
		mprSetVarReadonly(np, 1);
		np = mprGetNextProperty(&master, np, MPR_ENUM_FUNCTIONS | 
			MPR_ENUM_DATA);
	}
	ejsUnlock();
	return 0;
}

/******************************************************************************/

void ejsClose()
{
	ejsLock();
	mprDestroyArray(ejsList);
	mprDestroyVar(&master);
	ejsUnlock();
}

/******************************************************************************/
/*
 *	Create and initialize an EJS engine
 */

EjsId ejsOpenEngine(EjsHandle primaryHandle, EjsHandle altHandle)
{
	MprVar	*np;
	Ejs		*ep;

	ep = (Ejs*) mprMalloc(sizeof(Ejs));
	if (ep == 0) {
		return (EjsId) -1;
	}
	memset(ep, 0, sizeof(Ejs));

	ejsLock();
	ep->eid = (EjsId) mprAddToArray(ejsList, ep);
	ejsUnlock();

	/*
	 *	Create array of local variable frames
	 */
	ep->frames = mprCreateArray();
	if (ep->frames == 0) {
		ejsCloseEngine(ep->eid);
		return (EjsId) -1;
	}
	ep->primaryHandle = primaryHandle;
	ep->altHandle = altHandle;

	/*
	 *	Create first frame: global variables
	 */
	ep->global = (MprVar*) mprMalloc(sizeof(MprVar));
	*ep->global = ejsCreateObj("global", EJS_OBJ_HASH_SIZE);
	if (ep->global->type == MPR_TYPE_UNDEFINED) {
		ejsCloseEngine(ep->eid);
		return (EjsId) -1;
	}
	mprAddToArray(ep->frames, ep->global);

	/*
	 *	Create first local variable frame
	 */
	ep->local = (MprVar*) mprMalloc(sizeof(MprVar));
	*ep->local = ejsCreateObj("local", EJS_OBJ_HASH_SIZE);
	if (ep->local->type == MPR_TYPE_UNDEFINED) {
		ejsCloseEngine(ep->eid);
		return (EjsId) -1;
	}
	mprAddToArray(ep->frames, ep->local);

	/*
	 *	Clone all master variables into the global frame. This does a
	 *	reference copy.
	 *
	 *		ejsDefineStandardProperties(ep->global);
	 */
	np = mprGetFirstProperty(&master, MPR_ENUM_FUNCTIONS | MPR_ENUM_DATA);
	while (np) {
		mprCreateProperty(ep->global, np->name, np);
		np = mprGetNextProperty(&master, np, MPR_ENUM_FUNCTIONS | 
			MPR_ENUM_DATA);
	}

	mprCreateProperty(ep->global, "global", ep->global);
	mprCreateProperty(ep->global, "this", ep->global);
	mprCreateProperty(ep->local, "local", ep->local);

	return ep->eid;
}

/******************************************************************************/
/*
 *	Close an EJS instance
 */

void ejsCloseEngine(EjsId eid)
{
	Ejs		*ep;
	MprVar	*vp;
	void	**handles;
	int		i;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return;
	}

	mprFree(ep->error);
	mprDestroyVar(&ep->result);
	mprDestroyVar(&ep->tokenNumber);

	mprDeleteProperty(ep->local, "local");
	mprDeleteProperty(ep->global, "this");
	mprDeleteProperty(ep->global, "global");

	handles = ep->frames->handles;
	for (i = 0; i < ep->frames->max; i++) {
		vp = (MprVar*) handles[i];
		if (vp) {
#if BLD_DEBUG
			if (vp->type == MPR_TYPE_OBJECT && vp->properties->refCount > 1) {
				mprLog(7, "ejsCloseEngine: %s has ref count %d\n",
					vp->name, vp->properties->refCount);
			}
#endif
			//	mprPrintObjects("");
			mprDestroyVar(vp);
			mprFree(vp);
			mprRemoveFromArray(ep->frames, i);
		}
	}
	mprDestroyArray(ep->frames);

	ejsLock();
	mprRemoveFromArray(ejsList, (int) ep->eid);
	ejsUnlock();

	mprFree(ep);
}

/******************************************************************************/
/*
 *	Evaluate an EJS script file
 */

int ejsEvalFile(EjsId eid, char *path, MprVar *result, char **emsg)
{
	struct stat	 sbuf;
	Ejs			*ep;
	char		*script;
	int			rc, fd;

	mprAssert(path && *path);

	if (emsg) {
		*emsg = NULL;
	}

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		goto error;
	}

	if ((fd = open(path, O_RDONLY | O_BINARY, 0666)) < 0) {
		ejsError(ep, "Can't open %s\n", path);
		goto error;
	}
	
	if (stat(path, &sbuf) < 0) {
		close(fd);
		ejsError(ep, "Cant stat %s", path);
		goto error;
	}
	
	if ((script = (char*) mprMalloc(sbuf.st_size + 1)) == NULL) {
		close(fd);
		ejsError(ep, "Cant malloc %d", sbuf.st_size);
		goto error;
	}
	
	if (read(fd, script, sbuf.st_size) != (int)sbuf.st_size) {
		close(fd);
		mprFree(script);
		ejsError(ep, "Error reading %s", path);
		goto error;
	}
	
	script[sbuf.st_size] = '\0';
	close(fd);

	rc = ejsEvalBlock(eid, script, result, emsg);
	mprFree(script);

	return rc;

/*
 *	Error return
 */
error:
	*emsg = mprStrdup(ep->error);
	return -1;
}

/******************************************************************************/
/*
 *	Create a new variable scope block. This pushes the old local frame down
 *	the stack and creates a new local variables frame.
 */

int ejsOpenBlock(EjsId eid)
{
	Ejs		*ep;

	if((ep = ejsPtr(eid)) == NULL) {
		return -1;
	}

	ep->local = (MprVar*) mprMalloc(sizeof(MprVar));
	*ep->local = ejsCreateObj("localBlock", EJS_OBJ_HASH_SIZE);

	mprCreateProperty(ep->local, "local", ep->local);

	return mprAddToArray(ep->frames, ep->local);
}

/******************************************************************************/
/*
 *	Close a variable scope block opened via ejsOpenBlock. Pop back the old
 *	local variables frame.
 */

int ejsCloseBlock(EjsId eid, int fid)
{
	Ejs		*ep;

	if((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return -1;
	}

	/*
 	 *	Must remove self-references before destroying "local"
	 */
	mprDeleteProperty(ep->local, "local");

	mprDestroyVar(ep->local);
	mprFree(ep->local);

	mprRemoveFromArray(ep->frames, fid);
	ep->local = (MprVar*) ep->frames->handles[ep->frames->used - 1];

	return 0;
}

/******************************************************************************/
/*
 *	Create a new variable scope block and evaluate a script. All frames
 *	created during this context will be automatically deleted when complete.
 *	vp and emsg are optional. i.e. created local variables will be discarded
 *	when this routine returns.
 */

int ejsEvalBlock(EjsId eid, char *script, MprVar *vp, char **emsg)
{
	int		rc, fid;

	mprAssert(script);

	fid = ejsOpenBlock(eid);
	rc = ejsEvalScript(eid, script, vp, emsg);
	ejsCloseBlock(eid, fid);

	return rc;
}

/******************************************************************************/
/*
 *	Parse and evaluate a EJS. Return the result in *vp. The result is "owned"
 *	by EJ and the caller must not free it. Returns -1 on errors and zero 
 *	for success. On errors, emsg will be set to the reason. The caller must 
 *	free emsg.
 */

int ejsEvalScript(EjsId eid, char *script, MprVar *vp, char **emsg)
{
	Ejs			*ep;
	EjsInput	*oldBlock;
	int			state;
	void		*endlessLoopTest;
	int			loopCounter;
	
	if (emsg) {
		*emsg = NULL;
	} 

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return -1;
	}

	mprDestroyVar(&ep->result);

	if (script == 0) {
		return 0;
	}

	/*
	 *	Allocate a new evaluation block, and save the old one
	 */
	oldBlock = ep->input;
	ejsLexOpenScript(ep, script);

	/*
	 *	Do the actual parsing and evaluation
	 */
	loopCounter = 0;
	endlessLoopTest = NULL;
	ep->exitStatus = 0;

	do {
		state = ejsParse(ep, EJS_STATE_BEGIN, EJS_FLAGS_EXE);

		if (state == EJS_STATE_RET) {
			state = EJS_STATE_EOF;
		}
		/*
		 *	Stuck parser and endless recursion protection.
		 */
		if (endlessLoopTest == ep->input->scriptServp) {
			if (loopCounter++ > 10) {
				state = EJS_STATE_ERR;
				ejsError(ep, "Syntax error");
			}
		} else {
			endlessLoopTest = ep->input->scriptServp;
			loopCounter = 0;
		}
	} while (state != EJS_STATE_EOF && state != EJS_STATE_ERR);

	ejsLexCloseScript(ep);

	/*
	 *	Return any error string to the user
	 */
	if (state == EJS_STATE_ERR && emsg) {
		*emsg = mprStrdup(ep->error);
	}

	/*
	 *	Restore the old evaluation block
	 */
	ep->input = oldBlock;

	if (state == EJS_STATE_ERR) {
		return -1;
	}

	if (vp) {
		*vp = ep->result;
	}

	return ep->exitStatus;
}

/******************************************************************************/
/*
 *	Core error handling
 */

void ejsErrorCore(Ejs* ep, char *fmt, va_list args)
{
	EjsInput	*ip;
	char		*errbuf, *msgbuf;

	mprAssert(ep);
	mprAssert(args);

	msgbuf = NULL;
	mprAllocVsprintf(&msgbuf, MPR_MAX_STRING, fmt, args);

	if (ep) {
		ip = ep->input;
		if (ip) {
			mprAllocSprintf(&errbuf, MPR_MAX_STRING,
				"%s\nError on line %d. Offending line: %s\n\n",
				msgbuf, ip->lineNumber, ip->line);
		} else {
			mprAllocSprintf(&errbuf, MPR_MAX_STRING, "%s\n", msgbuf);
		}
		mprFree(ep->error);
		ep->error = errbuf;
	}
	mprFree(msgbuf);
}

/******************************************************************************/
/*
 *	Internal use function to set the error message
 */

void ejsError(Ejs* ep, char* fmt, ...)
{
	va_list		args;

	va_start(args, fmt);
	ejsErrorCore(ep, fmt, args);
	va_end(args);
}

/******************************************************************************/
/*
 *	Public routine to set the error message
 */

void ejsSetErrorMsg(EjsId eid, char* fmt, ...)
{
	va_list		args;
	Ejs			*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return;
	}
	va_start(args, fmt);
	ejsErrorCore(ep, fmt, args);
	va_end(args);
}

/******************************************************************************/
/*
 *	Get the current line number
 */

int ejsGetLineNumber(EjsId eid)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return -1;
	}
	return ep->input->lineNumber;
}

/******************************************************************************/
/*
 *	Return the local object
 */

MprVar *ejsGetLocalObject(EjsId eid)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return 0; 
	}
	return ep->local;
}

/******************************************************************************/
/*
 *	Return the global object
 */

MprVar *ejsGetGlobalObject(EjsId eid)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return 0;
	}
	return ep->global;
}

/******************************************************************************/
/*
 *	Copy the value of an object property. Return value is in "value".
 *	If deepCopy is true, copy all object/strings. Otherwise, object reference
 *	counts are incremented. Callers must always call mprDestroyVar on the 
 *	return value to prevent leaks.
 *
 *	Returns: -1 on errors or if the variable is not found.
 */

int ejsCopyVar(EjsId eid, char *var, MprVar *value, bool deepCopy)
{
	Ejs			*ep;
	MprVar		*vp;

	mprAssert(var && *var);
	mprAssert(value);

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return -1;
	}

	if (ejsGetVarCore(ep, var, 0, &vp, 0) < 0) {
		return -1;
	}

	return mprCopyProperty(value, vp, deepCopy);
}

/******************************************************************************/
/*
 *	Return the value of an object property. Return value is in "value".
 *	Objects and strings are not copied and reference counts are not modified.
 *	Callers should NOT call mprDestroyVar. Returns: -1 on errors or if the 
 *	variable is not found.
 */

int ejsReadVar(EjsId eid, char *var, MprVar *value)
{
	Ejs			*ep;
	MprVar		*vp;

	mprAssert(var && *var);
	mprAssert(value);

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return -1;
	}

	if (ejsGetVarCore(ep, var, 0, &vp, 0) < 0) {
		return -1;
	}

	return mprReadProperty(vp, value);
}

/******************************************************************************/
/*
 *	Set a variable that may be an arbitrarily complex object or array reference.
 *	Will always define in the top most variable frame.
 */

int ejsWriteVar(EjsId eid, char *var, MprVar *value)
{
	Ejs			*ep;
	MprVar		*vp;

	mprAssert(var && *var);

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return -1;
	}

	if (ejsGetVarCore(ep, var, 0, &vp, EJS_FLAGS_CREATE) < 0) {
		return -1;
	}
	mprAssert(vp);

	/*
	 *	Only copy the value. Don't overwrite the object's name
	 */
	mprWriteProperty(vp, value);

	return 0;
}

/******************************************************************************/
/*
 *	Set a variable that may be an arbitrarily complex object or array reference.
 *	Will always define in the top most variable frame.
 */

int ejsWriteVarValue(EjsId eid, char *var, MprVar value)
{
	return ejsWriteVar(eid, var, &value);
}

/******************************************************************************/
/*
 *	Delete a variable
 */

int ejsDeleteVar(EjsId eid, char *var)
{
	Ejs			*ep;
	MprVar		*vp;
	MprVar		*obj;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return -1;
	}
	if (ejsGetVarCore(ep, var, &obj, &vp, 0) < 0) {
		return -1;
	}
	mprDeleteProperty(obj, vp->name);
	return 0;
}

/******************************************************************************/
/*
 *	Set the expression return value
 */

void ejsSetReturnValue(EjsId eid, MprVar value)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return;
	}
	mprCopyVar(&ep->result, &value, MPR_SHALLOW_COPY);
}

/******************************************************************************/
/*
 *	Set the expression return value to a string value
 */

void ejsSetReturnString(EjsId eid, char *str)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return;
	}
	mprCopyVarValue(&ep->result, mprCreateStringVar(str, 0), MPR_SHALLOW_COPY);
}

/******************************************************************************/
/*
 *	Get the expression return value
 */

MprVar *ejsGetReturnValue(EjsId eid)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return 0;
	}
	return &ep->result;
}

/******************************************************************************/
/*
 *	Define a C function. If eid < 0, then update the master object with this
 *	function. NOTE: in this case, functionName must be simple without any "." or
 *	"[]" elements. If eid >= 0, add to the specified script engine. In this
 *	case, functionName can be an arbitrary object reference and can contain "."
 *	or "[]".  
 */

void ejsDefineCFunction(EjsId eid, char *functionName, MprCFunction fn, 
	void *thisPtr, int flags)
{
	if (eid < 0) {
		ejsLock();
		mprSetPropertyValue(&master, functionName, 
			mprCreateCFunctionVar(fn, thisPtr, flags));
		ejsUnlock();
	} else {
		ejsWriteVarValue(eid, functionName, 
			mprCreateCFunctionVar(fn, thisPtr, flags));
	}
}

/******************************************************************************/
/*
 *	Define a C function with String arguments
 */

void ejsDefineStringCFunction(EjsId eid, char *functionName, 
	MprStringCFunction fn, void *thisPtr, int flags)
{
	if (eid < 0) {
		ejsLock();
		mprSetPropertyValue(&master, functionName, 
			mprCreateStringCFunctionVar(fn, thisPtr, flags));
		ejsUnlock();
	} else {
		ejsWriteVarValue(eid, functionName, 
			mprCreateStringCFunctionVar(fn, thisPtr, flags));
	}
}

/******************************************************************************/
/*
 *	Define a JavaScript function. Args should be comma separated.
 *	Body should not contain braces.
 */

void ejsDefineFunction(EjsId eid, char *functionName, char *args, char *body)
{
	MprVar		v;

	v = mprCreateFunctionVar(args, body, 0);
	if (eid < 0) {
		ejsLock();
		mprSetProperty(&master, functionName, &v);
		ejsUnlock();
	} else {
		ejsWriteVar(eid, functionName, &v);
	}
	mprDestroyVar(&v);
}

/******************************************************************************/

void *ejsGetThisPtr(EjsId eid)
{
	Ejs		*ep;

	if ((ep = ejsPtr(eid)) == NULL) {
		mprAssert(ep);
		return 0;
	}
	return ep->thisPtr;
}

/******************************************************************************/
/*
 *	Find a variable given a variable name and return the parent object and 
 *	the variable itself, the variable . This routine supports variable names
 *	that may be objects or arrays but may NOT have expressions in the array
 *	indicies. Returns -1 on errors or if the variable is not found.
 */

int ejsGetVarCore(Ejs *ep, char *varName, MprVar **obj, MprVar **varValue, 
	int flags)
{
	MprVar		*currentObj;
	MprVar		*currentVar;
	char		tokBuf[EJS_MAX_ID];
	char		*propertyName, *token, *next, *cp;

	if (obj) {
		*obj = 0;
	}
	if (varValue) {
		*varValue = 0;
	}
	currentObj = ejsFindObj(ep, 0, varName, flags);
	currentVar = 0;
	propertyName = 0;

	varName = mprStrdup(varName);
	next = varName;

	token = getNextVarToken(&next, tokBuf, sizeof(tokBuf));

	while (currentObj != 0 && token != 0 && *token) {
		
		if (*token == '[') {
			token = getNextVarToken(&next, tokBuf, sizeof(tokBuf));

			propertyName = token;
			if (*propertyName == '\"') {
				propertyName++;
				if ((cp = strchr(propertyName, '\"')) != 0) {
					*cp = '\0';
				}
			} else if (*propertyName == '\'') {
				propertyName++;
				if ((cp = strchr(propertyName, '\'')) != 0) {
					*cp = '\0';
				}
			}

			currentObj = currentVar;
			currentVar = ejsFindProperty(ep, 0, currentObj, propertyName, 0);

			token = getNextVarToken(&next, tokBuf, sizeof(tokBuf));
			if (*token != ']') {
				mprFree(varName);
				return -1;
			}

		} else if (*token == '.') {
			token = getNextVarToken(&next, tokBuf, sizeof(tokBuf));
			if (!isalpha((int) token[0]) && 
					token[0] != '_' && token[0] != '$') {
				mprFree(varName);
				return -1;
			}

			propertyName = token;
			currentObj = currentVar;
			currentVar = ejsFindProperty(ep, 0, currentObj, token, 0);

		} else {
			currentVar = ejsFindProperty(ep, 0, currentObj, token, 0);
		}
		token = getNextVarToken(&next, tokBuf, sizeof(tokBuf));
	}
	mprFree(varName);

	if (currentVar == 0 && currentObj >= 0 && flags & EJS_FLAGS_CREATE) {
		currentVar = mprCreatePropertyValue(currentObj, propertyName, 
			mprCreateUndefinedVar());
	}
	if (obj) {
		*obj = currentObj;
	}
	
	/*
 	 *	Don't use mprCopyVar as it will copy the data
 	 */
	if (varValue) {
		*varValue = currentVar;
	}
	return currentVar ? 0 : -1;
}

/******************************************************************************/
/*
 *	Get the next token as part of a variable specification. This will return
 *	a pointer to the next token and will return a pointer to the next token 
 *	(after this one) in "next". The tokBuf holds the parsed token.
 */
static char *getNextVarToken(char **next, char *tokBuf, int tokBufLen)
{
	char	*start, *cp;
	int		len;

	start = *next;
	while (isspace((int) *start) || *start == '\n' || *start == '\r') {
		start++;
	}
	cp = start;

	if (*cp == '.' || *cp == '[' || *cp == ']') {
		cp++;
	} else {
		while (*cp && *cp != '.' && *cp != '[' && *cp != ']' && 
				!isspace((int) *cp) && *cp != '\n' && *cp != '\r') {
			cp++;
		}
	}
	len = mprMemcpy(tokBuf, tokBufLen - 1, start, cp - start);
	tokBuf[len] = '\0';
	
	*next = cp;
	return tokBuf;
}

/******************************************************************************/
/*
 *	Get the EJS structure pointer
 */

Ejs *ejsPtr(EjsId eid)
{
	Ejs		*handle;
	int		intId;

	intId = (int) eid;

	ejsLock();
	mprAssert(0 <= intId && intId < ejsList->max);

	if (intId < 0 || intId >= ejsList->max || ejsList->handles[intId] == NULL) {
		mprAssert(0);
		ejsUnlock();
		return NULL;
	}
	handle = (Ejs*) ejsList->handles[intId];
	ejsUnlock();
	return handle;
}

/******************************************************************************/
/*
 *	Utility routine to crack JavaScript arguments. Return the number of args
 *	seen. This routine only supports %s and %d type args.
 *
 *	Typical usage:
 *
 *		if (ejsParseArgs(argc, argv, "%s %d", &name, &age) < 2) {
 *			mprError("Insufficient args\n");
 *			return -1;
 *		}
 */ 

int ejsParseArgs(int argc, char **argv, char *fmt, ...)
{
	va_list	vargs;
	bool	*bp;
	char	*cp, **sp, *s;
	int		*ip, argn;

	va_start(vargs, fmt);

	if (argv == 0) {
		return 0;
	}

	for (argn = 0, cp = fmt; cp && *cp && argn < argc && argv[argn]; ) {
		if (*cp++ != '%') {
			continue;
		}

		s = argv[argn];
		switch (*cp) {
		case 'b':
			bp = va_arg(vargs, bool*);
			if (bp) {
				if (strcmp(s, "true") == 0 || s[0] == '1') {
					*bp = 1;
				} else {
					*bp = 0;
				}
			} else {
				*bp = 0;
			}
			break;

		case 'd':
			ip = va_arg(vargs, int*);
			*ip = atoi(s);
			break;

		case 's':
			sp = va_arg(vargs, char**);
			*sp = s;
			break;

		default:
			mprAssert(0);
		}
		argn++;
	}

	va_end(vargs);
	return argn;
}

/******************************************************************************/
#if BLD_FEATURE_FLOATING_POINT
/*
 *	Just to suppress constant folding warnings
 */
double ejsValue(double d)
{
	return d;
}

#endif
/******************************************************************************/

#else
void ejsDummy() {}

/******************************************************************************/
#endif /* BLD_FEATURE_EJS */

/******************************************************************************/
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
