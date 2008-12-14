/*
 *	@file 	var.h
 *	@brief 	MPR Universal Variable Type
 *	@copy	default.m
 *	
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1994-2007. All Rights Reserved.
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

/******************************* Documentation ********************************/
/*
 *	Variables can efficiently store primitive types and can hold references to
 *	objects. Objects can store properties which are themselves variables.
 *	Properties can be primitive data types, other objects or functions. 
 *	Properties are indexed by a character name. A variable may store one of 
 *	the following types: 
 *
 *		string, integer, integer-64bit, C function, C function with string args,
 *		 Javascript function, Floating point number, boolean value, Undefined 
 *		value and the Null value. 
 *
 *	Variables have names while objects may be referenced by multiple variables.
 *	Objects use reference counting for garbage collection.
 *
 *	This module is not thread safe for performance and compactness. It relies
 *	on upper modules to provide thread synchronization as required. The API
 *	provides primitives to get variable/object references or to get copies of 
 *	variables which will help minimize required lock times.
 */

#ifndef _h_MPR_VAR
#define _h_MPR_VAR 1

/********************************* Includes ***********************************/

#include	"buildConfig.h"
#include	"miniMpr.h"

/********************************** Defines ***********************************/

/*
 *	Define VAR_DEBUG if you want to track objects. However, this code is not
 *	thread safe and you need to run the server single threaded.
 *
 *		#define VAR_DEBUG 1
 */

#ifdef __cplusplus
extern "C" {
#endif
/*
 *	Forward declare types
 */
struct MprProperties;
struct MprVar;

/*
 *	Possible variable types. Don't use enum because we need to be able to
 *	do compile time conditional compilation on BLD_FEATURE_NUM_TYPE_ID.
 */
typedef int MprType;
#define MPR_TYPE_UNDEFINED 			0 	///< Undefined. No value has been set.
#define MPR_TYPE_NULL 				1	///< Value defined to be null.
#define MPR_TYPE_BOOL 				2	///< Boolean type.
#define MPR_TYPE_CFUNCTION 			3	///< C function or C++ method
#define MPR_TYPE_FLOAT 				4	///< Floating point number
#define MPR_TYPE_INT 				5	///< Integer number
#define MPR_TYPE_INT64 				6	///< 64-bit Integer number
#define MPR_TYPE_OBJECT 			7	///< Object reference
#define MPR_TYPE_FUNCTION 			8	///< JavaScript function
#define MPR_TYPE_STRING 			9	///< String (immutable)
#define MPR_TYPE_STRING_CFUNCTION 	10	///< C/C++ function with string args

/*
 *	Create a type for the default number type
 *	Config.h will define the default number type. For example:
 *
 *		BLD_FEATURE_NUM_TYPE=int
 *		BLD_FEATURE_NUM_TYPE_ID=MPR_TYPE_INT
 */

/**
 *	Set to the type used for MPR numeric variables. Will equate to int, int64 
 *	or double. 
 */
typedef BLD_FEATURE_NUM_TYPE MprNum;

/**
 *	Set to the MPR_TYPE used for MPR numeric variables. Will equate to 
 *	MPR_TYPE_INT, MPR_TYPE_INT64 or MPR_TYPE_FLOAT.
 */
#define MPR_NUM_VAR BLD_FEATURE_NUM_TYPE_ID
#define MPR_TYPE_NUM BLD_FEATURE_NUM_TYPE_ID

/*
 *	Return TRUE if a variable is a function type
 */
#define mprVarIsFunction(type) \
	(type == MPR_TYPE_FUNCTION || type == MPR_TYPE_STRING_CFUNCTION || \
	 type == MPR_TYPE_CFUNCTION)

/*
 *	Return TRUE if a variable is a numeric type
 */
#define mprVarIsNumber(type) \
	(type == MPR_TYPE_INT || type == MPR_TYPE_INT64 || type == MPR_TYPE_FLOAT)

/*
 *	Return TRUE if a variable is a boolean
 */
#define mprVarIsBoolean(type) \
	(type == MPR_TYPE_BOOL)
#define mprVarIsString(type) \
	(type == MPR_TYPE_STRING)
#define mprVarIsObject(type) \
	(type == MPR_TYPE_OBJECT)
#define mprVarIsFloating(type) \
	(type == MPR_TYPE_FLOAT)
#define mprVarIsUndefined(var) \
	((var)->type == MPR_TYPE_UNDEFINED)
#define mprVarIsNull(var) \
	((var)->type == MPR_TYPE_NULL)
#define mprVarIsValid(var) \
	(((var)->type != MPR_TYPE_NULL) && ((var)->type != MPR_TYPE_UNDEFINED))

#define MPR_VAR_MAX_RECURSE		5				/* Max object loops */

#if BLD_FEATURE_SQUEEZE
#define MPR_MAX_VAR				64				/* Max var full name */
#else
#define MPR_MAX_VAR				512
#endif

/*
 *	Function signatures
 */
typedef void* MprVarHandle;
typedef int (*MprCFunction)(MprVarHandle userHandle, int argc, 
	struct MprVar **argv);
typedef int (*MprStringCFunction)(MprVarHandle userHandle, int argc, 
	char **argv);

/*
 *	Triggers
 */
typedef enum {
	MPR_VAR_WRITE,						/* This property is being updated */
	MPR_VAR_READ,						/* This property is being read */
	MPR_VAR_CREATE_PROPERTY,			/* A property is being created */
	MPR_VAR_DELETE_PROPERTY,			/* A property is being deleted */
	MPR_VAR_DELETE						/* This object is being deleted */
} MprVarTriggerOp;

/*
 *	Trigger function return codes.
 */
typedef enum {
	MPR_TRIGGER_ABORT,					/* Abort the current operation */
	MPR_TRIGGER_USE_NEW_VALUE,			/* Proceed and use the newValue */
	MPR_TRIGGER_PROCEED					/* Proceed with the operation */
} MprVarTriggerStatus;

/*
 *	The MprVarTrigger arguments have the following meaning:
 *
 *		op					The operation being performed. See MprVarTriggerOp.
 *		parentProperties	Pointer to the MprProperties structure.
 *		vp					Pointer to the property that registered the trigger.
 *		newValue			New value (see below for more details).
 *		copyDepth			Specify what data items to copy.
 *
 *	For VAR_READ, newVar is set to a temporary variable that the trigger 
 *		function may assign a value to be returned instead of the actual 
 *		property value. 
 *	For VAR_WRITE, newValue holds the new value. The old existing value may be
 *		accessed via vp.
 *	For DELETE_PROPERTY, vp is the property being deleted. newValue is null.
 *	For ADD_PROPERTY, vp is set to the property being added and newValue holds 
 *		the new value.
 */
typedef MprVarTriggerStatus (*MprVarTrigger)(MprVarTriggerOp op, 
	struct MprProperties *parentProperties, struct MprVar *vp, 
	struct MprVar *newValue, int copyDepth);

/*
 *	mprCreateFunctionVar flags
 */
/** Use the alternate handle on function callbacks */
#define MPR_VAR_ALT_HANDLE		0x1

/** Use the script handle on function callbacks */
#define MPR_VAR_SCRIPT_HANDLE	0x2

/*
 *	Useful define for the copyDepth argument
 */
/** Don't copy any data. Copy only the variable name */
#define MPR_NO_COPY			0

/** Copy strings. Increment object reference counts. */
#define MPR_SHALLOW_COPY	1

/** Copy strings and do complete object copies. */
#define MPR_DEEP_COPY		2

/*
 *	GetFirst / GetNext flags
 */
/** Step into data properties. */
#define MPR_ENUM_DATA		0x1

/** Step into functions properties. */
#define MPR_ENUM_FUNCTIONS	0x2

/*
 *	Collection type to hold properties in an object
 */
typedef struct MprProperties {					/* Collection of properties */
#if VAR_DEBUG
	struct MprProperties *next;					/* Linked list */
	struct MprProperties *prev;					/* Linked list */
	char				name[32];				/* Debug name */
#endif
	struct MprVar		**buckets;				/* Hash chains */
	int					numItems;				/* Total count of items */
	/* FUTURE - Better way of doing this */
	int					numDataItems;			/* Enumerable data items */
	uint				hashSize		: 8;	/* Size of the hash table */
	/* FUTURE -- increase size of refCount */
	uint				refCount		: 8; 	/* References to this property*/
	/* FUTURE - make these flags */
	uint				deleteProtect	: 8;	/* Don't recursively delete */
	uint				visited			: 8;	/* Node has been processed */
} MprProperties;

/*
 *	Universal Variable Type
 */
typedef struct MprVar {
	/* FUTURE - remove name to outside reference */
	MprStr				name;					/* Property name */
	/* FUTURE - remove */
	MprStr				fullName;				/* Full object name */
	/* FUTURE - make part of the union */
	MprProperties		*properties;			/* Pointer to properties */

	/*
	 *	Packed bit field
	 */
	MprType				type			: 8;	/* Selector into union */
	uint				bucketIndex		: 8;	/* Copy of bucket index */

	uint				flags			: 5;	/* Type specific flags */
	uint				allocatedData 	: 1;	/* Data needs freeing */
	uint				readonly		: 1;	/* Unmodifiable */
	uint				deleteProtect	: 1;	/* Don't recursively delete */

	uint				visited			: 1;	/* Node has been processed */
	uint				allocatedVar	: 1;	/* Var needs freeing */
	uint				spare			: 6;	/* Unused */

	struct MprVar		*forw;					/* Hash table linkage */
	MprVarTrigger		trigger;				/* Trigger function */

#if UNUSED && KEEP
	struct MprVar		*baseClass;				/* Pointer to class object */
#endif
	MprProperties		*parentProperties;		/* Pointer to parent object */

	/*
	 *	Union of primitive types. When debugging on Linux, don't use unions 
	 *	as the gdb debugger can't display them.
	 */
#if (!BLD_DEBUG && !VXWORKS) || WIN
	union {
#endif
		int				boolean;				/* Use int for speed */
#if BLD_FEATURE_FLOATING_POINT
		double			floating;
#endif
		int				integer;
		int64			integer64;
		struct {								/* Javascript functions */
			MprArray	*args;					/* Null terminated */
			char		*body;
		} function;
		struct {								/* Function with MprVar args */
			MprCFunction fn;
			void		*thisPtr;
		} cFunction;
		struct {								/* Function with string args */
			MprStringCFunction fn;
			void		*thisPtr;
		} cFunctionWithStrings;
		MprStr			string;					/* Allocated string */
#if (!BLD_DEBUG && !VXWORKS) || WIN
	};
#endif
} MprVar;

/*
 *	Define a field macro so code an use numbers in a "generic" fashion.
 */
#if MPR_NUM_VAR == MPR_TYPE_INT || DOXYGEN
//*	Default numeric type */
#define mprNumber integer
#endif
#if MPR_NUM_VAR == MPR_TYPE_INT64
//*	Default numeric type */
#define mprNumber integer64
#endif
#if MPR_NUM_VAR == MPR_TYPE_FLOAT
//*	Default numeric type */
#define mprNumber floating
#endif

typedef BLD_FEATURE_NUM_TYPE MprNumber;

/********************************* Prototypes *********************************/
/*
 *	Variable constructors and destructors
 */
extern MprVar	mprCreateObjVar(char *name, int hashSize);
extern MprVar 	mprCreateBoolVar(bool value);
extern MprVar 	mprCreateCFunctionVar(MprCFunction fn, void *thisPtr, 
					int flags);
#if BLD_FEATURE_FLOATING_POINT
extern MprVar 	mprCreateFloatVar(double value);
#endif
extern MprVar 	mprCreateIntegerVar(int value);
extern MprVar 	mprCreateInteger64Var(int64 value);
extern MprVar 	mprCreateFunctionVar(char *args, char *body, int flags);
extern MprVar	mprCreateNullVar();
extern MprVar 	mprCreateNumberVar(MprNumber value);
extern MprVar 	mprCreateStringCFunctionVar(MprStringCFunction fn, 
					void *thisPtr, int flags);
extern MprVar	mprCreateStringVar(char *value, bool allocate);
extern MprVar	mprCreateUndefinedVar();
extern bool 	mprDestroyVar(MprVar *vp);
extern bool 	mprDestroyAllVars(MprVar* vp);
extern MprType	mprGetVarType(MprVar *vp);

/*
 *	Copy
 */
extern void		mprCopyVar(MprVar *dest, MprVar *src, int copyDepth);
extern void		mprCopyVarValue(MprVar *dest, MprVar src, int copyDepth);
extern MprVar	*mprDupVar(MprVar *src, int copyDepth);

/*
 *	Manage vars
 */
extern MprVarTrigger 
				mprAddVarTrigger(MprVar *vp, MprVarTrigger fn);
extern int		mprGetVarRefCount(MprVar *vp);
extern void 	mprSetVarDeleteProtect(MprVar *vp, int deleteProtect);
extern void 	mprSetVarFullName(MprVar *vp, char *name);
extern void 	mprSetVarReadonly(MprVar *vp, int readonly);
extern void 	mprSetVarName(MprVar *vp, char *name);

/*
 *	Create properties and return a reference to the property.
 */
extern MprVar	*mprCreateProperty(MprVar *obj, char *property, 
					MprVar *newValue);
extern MprVar	*mprCreatePropertyValue(MprVar *obj, char *property, 
					MprVar newValue);
extern int		mprDeleteProperty(MprVar *obj, char *property);

/*
 *	Get/Set properties. Set will update/create.
 */
extern MprVar	*mprGetProperty(MprVar *obj, char *property, MprVar *value);
extern MprVar	*mprSetProperty(MprVar *obj, char *property, MprVar *value);
extern MprVar	*mprSetPropertyValue(MprVar *obj, char *property, MprVar value);

/*
 *	Directly read/write property values (the property must already exist)
 *	For mprCopyProperty, mprDestroyVar must always called on the var.
 */
extern int	 	mprReadProperty(MprVar *prop, MprVar *value);
extern int		mprWriteProperty(MprVar *prop, MprVar *newValue);
extern int		mprWritePropertyValue(MprVar *prop, MprVar newValue);

/*
 *	Copy a property. NOTE: reverse of most other args: (dest, src)
 */
extern int	 	mprCopyProperty(MprVar *dest, MprVar *prop, int copyDepth);

/*
 *	Enumerate properties
 */
extern MprVar	*mprGetFirstProperty(MprVar *obj, int includeFlags);
extern MprVar	*mprGetNextProperty(MprVar *obj, MprVar *currentProperty, 
					int includeFlags);

/*
 *	Query properties characteristics
 */
extern int		mprGetPropertyCount(MprVar *obj, int includeFlags);

/*
 *	Conversion routines
 */
extern MprVar 	mprParseVar(char *str, MprType prefType);
extern MprNum 	mprVarToNumber(MprVar *vp);
extern int	 	mprVarToInteger(MprVar *vp);
extern int64 	mprVarToInteger64(MprVar *vp);
extern bool 	mprVarToBool(MprVar *vp);
#if BLD_FEATURE_FLOATING_POINT
extern double 	mprVarToFloat(MprVar *vp);
#endif
extern void 	mprVarToString(char** buf, int size, char *fmt, MprVar *vp);

/*
 *	Parsing and utility routines
 */
extern MprNum 	mprParseNumber(char *str);
extern int	 	mprParseInteger(char *str);

extern int64 	mprParseInteger64(char *str);

#if BLD_FEATURE_FLOATING_POINT
extern double 	mprParseFloat(char *str);
extern bool 	mprIsInfinite(double f);
extern bool 	mprIsNan(double f);
#endif

#if VAR_DEBUG
extern void 	mprPrintObjects(char *msg);
extern void 	mprPrintObjRefCount(MprVar *vp);
#endif

#ifdef __cplusplus
}
#endif

/*****************************************************************************/
#endif /* _h_MPR_VAR */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
