/*
 *	@file 	ejsProc.c
 *	@brief 	EJS support functions
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

/****************************** Forward Declarations **************************/
/*
 *	Object constructors
 */
static int 		objectConsProc(EjsHandle eid, int argc, MprVar **argv);
static int 		arrayConsProc(EjsHandle eid, int argc, MprVar **argv);
static int 		booleanConsProc(EjsHandle eid, int argc, MprVar **agv);
static int 		numberConsProc(EjsHandle eid, int argc, MprVar **argv);
static int 		stringConsProc(EjsHandle eid, int argc, MprVar **argv);

/*
 *	Core functions
 */
static int 		toStringProc(EjsHandle eid, int argc, MprVar **argv);
static int 		valueOfProc(EjsHandle eid, int argc, MprVar **argv);

/*
 *	Triggers
 */
static MprVarTriggerStatus lengthTrigger(MprVarTriggerOp op, 
	MprProperties *parentProperties, MprVar *prop, MprVar *newValue,
	int copyRef);

/******************************************************************************/
/*
 *	Routine to create the base common to all object types
 */

MprVar ejsCreateObj(char *name, int hashSize)
{
	MprVar	o;

	o = mprCreateObjVar(name, hashSize);
	if (o.type == MPR_TYPE_UNDEFINED) {
		mprAssert(0);
		return o;
	}

	mprCreatePropertyValue(&o, "toString", 
		mprCreateCFunctionVar(toStringProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(&o, "valueOf", 
		mprCreateCFunctionVar(valueOfProc, 0, MPR_VAR_SCRIPT_HANDLE));
	return o;
}

/******************************************************************************/
/*
 *	Routine to destroy a variable
 */

bool ejsDestroyVar(MprVar *obj)
{
	return mprDestroyVar(obj);
}

/******************************************************************************/
/*
 *	Routine to create the base array type
 */

MprVar ejsCreateArray(char *name, int size)
{
	MprVar	obj, *lp, undef;
	char	index[16];
	int		i;

	/*	Sanity limit for size of hash table */

	obj = ejsCreateObj(name, max(size, 503));
	if (obj.type == MPR_TYPE_UNDEFINED) {
		mprAssert(0);
		return obj;
	}

	undef = mprCreateUndefinedVar();
	for (i = 0; i < size; i++) {
		mprItoa(i, index, sizeof(index));
		mprCreateProperty(&obj, index, &undef);
	}

	lp = mprCreatePropertyValue(&obj, "length", mprCreateIntegerVar(size));
	mprAssert(lp);

	mprSetVarReadonly(lp, 1);
	mprAddVarTrigger(lp, lengthTrigger);

	return obj;
}

/******************************************************************************/
/******************************** Constructors ********************************/
/******************************************************************************/
/*
 *	Object constructor. Nothing really done here. For future expansion.
 */

static int objectConsProc(EjsHandle eid, int argc, MprVar **argv)
{
#if UNUSED
	MprVar	*obj;
	Ejs		*ep;

	if((ep = ejsPtr((EjsId) eid)) == NULL) {
		return -1;
	}

	obj = mprGetProperty(ep->local, "this", 0);
	mprAssert(obj);
#endif
	return 0;
}

/******************************************************************************/
/*
 *	Array constructor
 */

static int arrayConsProc(EjsHandle eid, int argc, MprVar **argv)
{
	MprVar	*obj, *lp, undef;
	Ejs		*ep;
	char	index[16];
	int		i, max;

	objectConsProc(eid, argc, argv);

	if ((ep = ejsPtr((EjsId) eid)) == NULL) {
		return -1;
	}
	obj = mprGetProperty(ep->local, "this", 0);
	mprAssert(obj);


	if (argc == 1) {
		/*
		 *	x = new Array(size);
		 */
		undef = mprCreateUndefinedVar();
		max = (int) mprVarToInteger(argv[0]);
		for (i = 0; i < max; i++) {
			mprItoa(i, index, sizeof(index));
			mprCreateProperty(obj, index, &undef);
		}
	} else if (argc > 1) {
		/*
		 *	x = new Array(element0, element1, ..., elementN):
		 */
		max = argc;
		for (i = 0; i < max; i++) {
			mprItoa(i, index, sizeof(index));
			mprCreateProperty(obj, index, argv[i]);
		}

	} else {
		max = 0;
	}

	lp = mprCreatePropertyValue(obj, "length", mprCreateIntegerVar(max));
	mprAssert(lp);

	mprSetVarReadonly(lp, 1);
	mprAddVarTrigger(lp, lengthTrigger);

	return 0;
}

/******************************************************************************/
/*
 *	Boolean constructor
 */

static int booleanConsProc(EjsHandle eid, int argc, MprVar **argv)
{
	objectConsProc(eid, argc, argv);
	return 0;
}

/******************************************************************************/
#if FUTURE
/*
 *	Date constructor
 */

static int dateConsProc(EjsHandle eid, int argc, MprVar **argv)
{
	objectConsProc(eid, argc, argv);
	return 0;
}

#endif
/******************************************************************************/
/*
 *	Number constructor
 */

static int numberConsProc(EjsHandle eid, int argc, MprVar **argv)
{
	objectConsProc(eid, argc, argv);
	return 0;
}

/******************************************************************************/
/*
 *	String constructor
 */

static int stringConsProc(EjsHandle eid, int argc, MprVar **argv)
{
	objectConsProc(eid, argc, argv);
	return 0;
}

/******************************************************************************/
/********************************** Functions *********************************/
/******************************************************************************/

static int toStringProc(EjsHandle eid, int argc, MprVar **argv)
{
	MprVar	*obj;
	Ejs		*ep;
	char	*buf;
	int		radix;

	if (argc == 0) {
		radix = 10;

	} else if (argc == 1) {
		radix = (int) mprVarToInteger(argv[0]);

	} else {
		mprAssert(0);
		return -1;
	}

	if((ep = ejsPtr((EjsId) eid)) == NULL) {
		return -1;
	}

	obj = mprGetProperty(ep->local, "this", 0);
	mprAssert(obj);

	mprVarToString(&buf, MPR_MAX_STRING, 0, obj);
	mprCopyVarValue(&ep->result, mprCreateStringVar(buf, 0), MPR_SHALLOW_COPY);
	mprFree(buf);

	return 0;
}

/******************************************************************************/

static int valueOfProc(EjsHandle eid, int argc, MprVar **argv)
{
	MprVar	*obj;
	Ejs		*ep;

	if (argc != 0) {
		mprAssert(0);
		return -1;
	}

	if((ep = ejsPtr((EjsId) eid)) == NULL) {
		return -1;
	}

	obj = mprGetProperty(ep->local, "this", 0);
	mprAssert(obj);

	switch (obj->type) {
	default:
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
	case MPR_TYPE_CFUNCTION:
	case MPR_TYPE_OBJECT:
	case MPR_TYPE_FUNCTION:
	case MPR_TYPE_STRING_CFUNCTION:
		mprCopyVar(&ep->result, obj, MPR_SHALLOW_COPY);
		break;

	case MPR_TYPE_STRING:
		mprCopyVarValue(&ep->result, mprCreateIntegerVar(atoi(obj->string)), 0);
		break;

	case MPR_TYPE_BOOL:
	case MPR_TYPE_INT:
	case MPR_TYPE_INT64:
#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
#endif
		mprCopyVar(&ep->result, obj, 0);
		break;
	} 
	return 0;
}

/******************************************************************************/
/*
 *	Var access trigger on the Array.length property. Return the count of
 *	enumerable properties (don't count functions).
 */

static MprVarTriggerStatus lengthTrigger(MprVarTriggerOp op, 
	MprProperties *parentProperties, MprVar *prop, MprVar *newValue, 
	int copyRef)
{
	switch (op) {
	case MPR_VAR_READ:
		/*
		 *	Subtract one for the length property
	 	 *	FUTURE -- need an API to access parentProperties
		 *	FUTURE -- contradiction to be read-only yet allow USE_NEW_VALUE.
		 *		API needs finer control.
		 */
		*newValue = mprCreateIntegerVar(parentProperties->numDataItems - 1);
		return MPR_TRIGGER_USE_NEW_VALUE;

	case MPR_VAR_WRITE:
		return MPR_TRIGGER_ABORT;

	case MPR_VAR_CREATE_PROPERTY:
	case MPR_VAR_DELETE_PROPERTY:
	case MPR_VAR_DELETE:
	default:
		break;
	}
	return MPR_TRIGGER_PROCEED;
}

/******************************************************************************/
/**************************** Extension Functions *****************************/
/******************************************************************************/
/*
 *	Assert 
 */

static int assertProc(EjsHandle eid, int argc, MprVar **argv)
{
	bool	b;

	if (argc < 1) {
		ejsSetErrorMsg((EjsId) eid, "usage: assert(condition)\n");
		return -1;
	}
	b = mprVarToBool(argv[0]);
	if (b == 0) {
		ejsSetErrorMsg((EjsId) eid, "Assertion failure\n");
		return -1;
	}
	ejsSetReturnValue((EjsId) eid, mprCreateBoolVar(b));
	return 0;
}

/******************************************************************************/
/*
 *	Exit 
 */

static int exitProc(EjsHandle eid, int argc, MprVar **argv)
{
	int			status;

	if (argc < 1) {
		ejsSetErrorMsg((EjsId) eid, "usage: exit(status)\n");
		return -1;
	}
	status = (int) mprVarToInteger(argv[0]);
	ejsSetExitStatus((EjsId) eid, status);

	ejsSetReturnValue((EjsId) eid, mprCreateStringVar("", 0));
	return 0;
}

/******************************************************************************/

static void printVar(MprVar *vp, int recurseCount, int indent)
{
	MprVar	*np;
	char	*buf;
	int		i;

	if (recurseCount > 5) {
		write(1, "Skipping - recursion too deep\n", 29);
		return;
	}

	for (i = 0; i < indent; i++) {
		write(1, "  ", 2);
	}

	if (vp->type == MPR_TYPE_OBJECT) {
		if (vp->name) {
			write(1, vp->name, strlen(vp->name));
		} else {
			write(1, "unknown", 7);
		}
		write(1, ": {\n", 4);
		np = mprGetFirstProperty(vp, MPR_ENUM_DATA);
		while (np) {
			if (strcmp(np->name, "local") == 0 ||
					strcmp(np->name, "global") == 0 ||
					strcmp(np->name, "this") == 0) {
				np = mprGetNextProperty(vp, np, MPR_ENUM_DATA);
				continue;
			}
			printVar(np, recurseCount + 1, indent + 1);
			np = mprGetNextProperty(vp, np, MPR_ENUM_DATA);
			if (np) {
				write(1, ",\n", 2);
			}
		}
		write(1, "\n", 1);
		for (i = 0; i < indent; i++) {
			write(1, "  ", 2);
		}
		write(1, "}", 1);

	} else {
		if (vp->name) {
			write(1, vp->name, strlen(vp->name));
		} else {
			write(1, "unknown", 7);
		}
		write(1, ": ", 2);

		/*	FUTURE -- other types ? */
		mprVarToString(&buf, MPR_MAX_STRING, 0, vp);
		if (vp->type == MPR_TYPE_STRING) {
			write(1, "\"", 1);
		}
		write(1, buf, strlen(buf));
		if (vp->type == MPR_TYPE_STRING) {
			write(1, "\"", 1);
		}
		mprFree(buf);
	}
}

/******************************************************************************/
/*
 *	Print the args to stdout
 */

static int printVarsProc(EjsHandle eid, int argc, MprVar **argv)
{
	MprVar	*vp;
	char	*buf;
	int		i;

	for (i = 0; i < argc; i++) {
		vp = argv[i];
		switch (vp->type) {
		case MPR_TYPE_OBJECT:
			printVar(vp, 0, 0);
			break;
		default:
			mprVarToString(&buf, MPR_MAX_STRING, 0, vp);
			write(1, buf, strlen(buf));
			mprFree(buf);
			break;
		}
	}
	write(1, "\n", 1);

	ejsSetReturnValue((EjsId) eid, mprCreateStringVar("", 0));
	return 0;
}

/******************************************************************************/
/*
 *	Print the args to stdout
 */

static int printProc(EjsHandle eid, int argc, MprVar **argv)
{
	char	*buf;
	int		i;

	for (i = 0; i < argc; i++) {
		mprVarToString(&buf, MPR_MAX_STRING, 0, argv[i]);
		write(1, buf, strlen(buf));
		mprFree(buf);
	}
	return 0;
}

/******************************************************************************/
/*
 *	println
 */

static int printlnProc(EjsHandle eid, int argc, MprVar **argv)
{
	printProc(eid, argc, argv);
	write(1, "\n", 1);
	return 0;
}

/******************************************************************************/
/*
 *	Trace 
 */

static int traceProc(EjsHandle eid, int argc, char **argv)
{
	if (argc == 1) {
		mprLog(0, argv[0]);

	} else if (argc == 2) {
		mprLog(atoi(argv[0]), argv[1]);

	} else {
		ejsSetErrorMsg((EjsId) eid, "Usage: trace([level], message)");
		return -1;
	}
	ejsSetReturnString((EjsId) eid, "");
	return 0;
}

/******************************************************************************/
/*
 *	Return the object reference count
 */

static int refCountProc(EjsHandle eid, int argc, MprVar **argv)
{
	MprVar		*vp;
	int			count;

	vp = argv[0];
	if (vp->type == MPR_TYPE_OBJECT) {
		count = mprGetVarRefCount(vp);
		ejsSetReturnValue((EjsId) eid, mprCreateIntegerVar(count));
	} else {
		ejsSetReturnValue((EjsId) eid, mprCreateIntegerVar(0));
	}

	return 0;
}

/******************************************************************************/
/*
 *	Evaluate a sub-script. It is evaluated in the same variable scope as
 *	the calling script / function.
 */

static int evalScriptProc(EjsHandle eid, int argc, MprVar **argv)
{
	MprVar		*arg;
	char		*emsg;
	int			i;

	ejsSetReturnValue((EjsId) eid, mprCreateUndefinedVar());

	for (i = 0; i < argc; i++) {
		arg = argv[i];
		if (arg->type != MPR_TYPE_STRING) {
			continue;
		}
		if (ejsEvalScript((EjsId) eid, arg->string, 0, &emsg) < 0) {
			ejsSetErrorMsg((EjsId) eid, "%s", emsg);
			mprFree(emsg);
			return -1;
		}
	}
	/*
	 *	Return with the value of the last expression
 	 */
	return 0;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/*
 *	Define the standard properties and functions inherited by all script engines.
 */

int ejsDefineStandardProperties(MprVar *obj)
{
#if BLD_FEATURE_FLOATING_POINT
	mprCreatePropertyValue(obj, "NaN", mprCreateFloatVar(0.0 / ejsValue(0.0)));
	mprCreatePropertyValue(obj, "Infinity", 
		mprCreateFloatVar(ejsValue(MAX_FLOAT) * ejsValue(MAX_FLOAT)));
#endif
	mprCreatePropertyValue(obj, "null", mprCreateNullVar());
	mprCreatePropertyValue(obj, "undefined", mprCreateUndefinedVar());
	mprCreatePropertyValue(obj, "true", mprCreateBoolVar(1));
	mprCreatePropertyValue(obj, "false", mprCreateBoolVar(0));

#if BLD_FEATURE_LEGACY_API
	/*
 	 *	DEPRECATED: 2.0.
 	 *	So that ESP/ASP can ignore "language=javascript" statements
	 */
	mprCreatePropertyValue(obj, "javascript", mprCreateIntegerVar(0));
#endif

	/*
 	 *	Extension functions
 	 */
	mprCreatePropertyValue(obj, "assert", 
		mprCreateCFunctionVar(assertProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "eval", 
		mprCreateCFunctionVar(evalScriptProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "exit", 
		mprCreateCFunctionVar(exitProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "refCount", 
		mprCreateCFunctionVar(refCountProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "print", 
		mprCreateCFunctionVar(printProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "println", 
		mprCreateCFunctionVar(printlnProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "printVars", 
		mprCreateCFunctionVar(printVarsProc,0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "trace", 
		mprCreateStringCFunctionVar(traceProc, 0, MPR_VAR_SCRIPT_HANDLE));

	/*
	 *	Constructors
	 */
	mprCreatePropertyValue(obj, "Array", 
		mprCreateCFunctionVar(arrayConsProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "Boolean",
		mprCreateCFunctionVar(booleanConsProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "Object", 
		mprCreateCFunctionVar(objectConsProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "Number", 
		mprCreateCFunctionVar(numberConsProc, 0, MPR_VAR_SCRIPT_HANDLE));
	mprCreatePropertyValue(obj, "String", 
		mprCreateCFunctionVar(stringConsProc, 0, MPR_VAR_SCRIPT_HANDLE));

	/*	mprCreatePropertyValue(obj, "Date", 
	 *		mprCreateCFunctionVar(dateConsProc, 0, MPR_VAR_SCRIPT_HANDLE));
	 *	mprCreatePropertyValue(obj, "Regexp", 
	 *		mprCreateCFunctionVar(regexpConsProc, 0, MPR_VAR_SCRIPT_HANDLE));
	 */

	/*
	 *	Can we use on var x = "string text";
	 */
	return 0;
}

/******************************************************************************/

#else
void ejsProcsDummy() {}

/******************************************************************************/
#endif /* BLD_FEATURE_EJS */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
