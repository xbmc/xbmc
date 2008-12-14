/*
 *	@file 	var.c
 *	@brief 	MPR Universal Variable Type
 *	@overview
 *
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
 *	This module is NOT multithreaded. 
 *
 *	Properties are variables that are stored in an object type variable.
 *	Properties can be primitive data types, other objects or functions.
 *	Properties are indexed by a character name.
 */

/********************************** Includes **********************************/

#include	"var.h"

/*********************************** Locals ***********************************/
#if VAR_DEBUG

static MprProperties	objectList;			/* Dummy head of objects list */
static int				objectCount = -1;	/* Count of objects */

#endif
/***************************** Forward Declarations ***************************/

static int 		adjustRefCount(MprProperties *pp, int adj);
static int		adjustVarRefCount(MprVar *vp, int adj);
static MprVar 	*allocProperty(char *propertyName);
static void 	copyVarCore(MprVar *dest, MprVar *src, int copyDepth);
static MprProperties 
				*createProperties(char *name, int hashSize);
static bool 	freeVar(MprVar *vp, int force);
static bool 	freeVarStorage(MprVar *vp, int force);
static MprVar	*getObjChain(MprProperties *pp, char *property);
static int		hash(MprProperties *pp, char *property);
static bool		releaseProperties(MprProperties *pp, int force);

/*********************************** Code *************************************/
/*
 *	Destroy a variable and all referenced variables. Release any referenced 
 *	object regardless of whether other users still have references. Be VERY
 *	careful using this routine. 
 *
 *	Return TRUE if the underlying data is freed. Objects may not be freed if 
 *	there are other users of the object.
 */

bool mprDestroyAllVars(MprVar *vp)
{
	mprAssert(vp);

	if (vp->trigger) {
		if ((vp->trigger)(MPR_VAR_DELETE, vp->parentProperties, vp, 0, 0) 
				== MPR_TRIGGER_ABORT) {
			return 0;
		}
	}

	/*
	 *	Free the actual value. If this var refers to an object, we will 
	 *	recurse through all the properties freeing all vars.
	 */
	return freeVar(vp, 1);
}

/******************************************************************************/
/*
 *	Destroy a variable. Release any referenced object (destroy if no other
 *	users are referencing).
 *
 *	Return TRUE if the underlying data is freed. Objects may not be freed if 
 *	there are other users of the object.
 */

bool mprDestroyVar(MprVar *vp)
{
	mprAssert(vp);

	if (vp->trigger) {
		if ((vp->trigger)(MPR_VAR_DELETE, vp->parentProperties, vp, 0, 0) 
				== MPR_TRIGGER_ABORT) {
			return 0;
		}
	}

	/*
	 *	Free the actual value. If this var refers to an object, we will 
	 *	recurse through all the properties freeing all that have no other
	 *	references.
	 */
	return freeVar(vp, 0);
}

/******************************************************************************/
/*
 *	Free the value in a variable for primitive types. Release objects.
 *
 *	Return TRUE if the underlying data is freed. Objects may not be freed if 
 *	there are other users of the object.
 */

static bool freeVar(MprVar *vp, int force)
{
	bool	freed;

	mprAssert(vp);

	freed = freeVarStorage(vp, force);

	mprFree(vp->name);
	mprFree(vp->fullName);

	if (vp->allocatedVar) {
		mprFree(vp);
	} else {
		vp->name = 0;
		vp->fullName = 0;
		vp->type = MPR_TYPE_UNDEFINED;
	}
	return freed;
}

/******************************************************************************/
/*
 *	Free the value in a variable for primitive types. Release objects.
 *
 *	Return TRUE if the underlying data is freed. Objects may not be freed if 
 *	there are other users of the object.
 */

static bool freeVarStorage(MprVar *vp, int force)
{
	MprArray	*argList;
	bool		freed;
	int			i;

	freed = 1;
	mprAssert(vp);

	switch (vp->type) {
	default:
		break;

	case MPR_TYPE_STRING:
		if (vp->allocatedData && vp->string != 0) {
			mprFree(vp->string);
			vp->string = 0;
			vp->allocatedData = 0;
		}
		break;

	case MPR_TYPE_OBJECT:
#if VAR_DEBUG
		/*
		 *	Recurse through all properties and release / delete. Release the 
		 *	properties hash table.
		 */
		if (vp->properties->refCount > 1) {
			mprLog(7, "freeVar: ACT \"%s\", 0x%x, ref %d, force %d\n", 
				vp->name, vp->properties, vp->properties->refCount, force);
		} else {
			mprLog(7, "freeVar: DEL \"%s\", 0x%x, ref %d, force %d\n", 
				vp->name, vp->properties, vp->properties->refCount, force);
		}
#endif
		if (vp->allocatedData) {
			freed = releaseProperties(vp->properties, force);
		}
		vp->properties = 0;
		break;

	case MPR_TYPE_FUNCTION:
		if (vp->allocatedData) {
			argList = vp->function.args;
			for (i = 0; i < argList->max; i++) {
				if (argList->handles[i] != 0) {
					mprFree(argList->handles[i]);
				}
			}
			mprDestroyArray(argList);
			vp->function.args = 0;
			mprFree(vp->function.body);
			vp->function.body = 0;
		}
		break;
	}

	vp->type = MPR_TYPE_UNDEFINED;
	return freed;
}

/******************************************************************************/
/*
 *	Adjust the object reference count and return the currrent count of 
 *	users.
 */

static int adjustVarRefCount(MprVar *vp, int adj)
{
	mprAssert(vp);

	if (vp->type != MPR_TYPE_OBJECT) {
		mprAssert(vp->type == MPR_TYPE_OBJECT);
		return 0;
	}
	return adjustRefCount(vp->properties, adj);
}

/******************************************************************************/
/*
 *	Get the object reference count 
 */

int mprGetVarRefCount(MprVar *vp)
{
	mprAssert(vp);

	if (vp->type != MPR_TYPE_OBJECT) {
		mprAssert(vp->type == MPR_TYPE_OBJECT);
		return 0;
	}
	return adjustRefCount(vp->properties, 0);
}

/******************************************************************************/
/*
 *	Update the variable's name
 */

void mprSetVarName(MprVar *vp, char *name)
{
	mprAssert(vp);

	mprFree(vp->name);
	vp->name = mprStrdup(name);
}

/******************************************************************************/
/*
 *	Append to the variable's full name
 */

void mprSetVarFullName(MprVar *vp, char *name)
{
#if VAR_DEBUG
	mprAssert(vp);

	mprFree(vp->fullName);
	vp->fullName = mprStrdup(name);
	if (vp->type == MPR_TYPE_OBJECT) {
		if (strcmp(vp->properties->name, "this") == 0) {
			mprStrcpy(vp->properties->name, sizeof(vp->properties->name), name);
		}
	}
#endif
}

/******************************************************************************/
/*
 *	Make a var impervious to recursive forced deletes. 
 */

void mprSetVarDeleteProtect(MprVar *vp, int deleteProtect)
{
	mprAssert(vp);

	if (vp->type == MPR_TYPE_OBJECT && vp->properties) {
		vp->properties->deleteProtect = deleteProtect;
	}
}

/******************************************************************************/
/*
 *	Make a variable readonly. Can still be deleted.
 */

void mprSetVarReadonly(MprVar *vp, int readonly)
{
	mprAssert(vp);

	vp->readonly = readonly;
}

/******************************************************************************/

MprVarTrigger mprAddVarTrigger(MprVar *vp, MprVarTrigger fn)
{
	MprVarTrigger oldTrigger;

	mprAssert(vp);
	mprAssert(fn);

	oldTrigger = vp->trigger;
	vp->trigger = fn;
	return oldTrigger;
}

/******************************************************************************/

MprType mprGetVarType(MprVar *vp)
{
	mprAssert(vp);

	return vp->type;
}

/******************************************************************************/
/********************************** Properties ********************************/
/******************************************************************************/
/*
 *	Create a property in an object with a defined value. It is an error if the
 *	property already exists.
 */

MprVar *mprCreateProperty(MprVar *obj, char *propertyName, MprVar *newValue)
{
	MprVar	*prop, *last;
	int		bucketIndex;

	mprAssert(obj);
	mprAssert(propertyName && *propertyName);

	if (obj->type != MPR_TYPE_OBJECT) {
		mprAssert(obj->type == MPR_TYPE_OBJECT);
		return 0;
	}

	/*
	 *	See if property already exists and locate the bucket to hold the
	 *	property reference.
	 */
	last = 0;
	bucketIndex = hash(obj->properties, propertyName);
	prop = obj->properties->buckets[bucketIndex];

	/*
	 *	Find the property in the hash chain if it exists
 	 */
	for (last = 0; prop; last = prop, prop = prop->forw) {
		if (prop->name[0] == propertyName[0] && 
				strcmp(prop->name, propertyName) == 0) {
			break;
		}
	}

	if (prop) {
		//	FUTURE -- remove. Just for debug.
		mprAssert(prop == 0);
		mprLog(0, "Attempting to create property %s in object %s\n",
			propertyName, obj->name);
		return 0;
	}

	if (obj->trigger) {
		if ((obj->trigger)(MPR_VAR_CREATE_PROPERTY, obj->properties, prop, 
				newValue, 0) == MPR_TRIGGER_ABORT) {
			return 0;
		}
	}

	/*
 	 *	Create a new property
	 */
	prop = allocProperty(propertyName);
	if (prop == 0) {
		mprAssert(prop);
		return 0;
	}

	copyVarCore(prop, newValue, MPR_SHALLOW_COPY);

	prop->bucketIndex = bucketIndex;
	if (last) {
		last->forw = prop;
	} else {
		obj->properties->buckets[bucketIndex] = prop;
	}
	prop->parentProperties = obj->properties;

	/*
 	 *	Update the item counts 
	 */
	obj->properties->numItems++;
	if (! mprVarIsFunction(prop->type)) {
		obj->properties->numDataItems++;
	}

	return prop;
}

/******************************************************************************/
/*
 *	Create a property in an object with a defined value. If the property 
 *	already exists, error, Same as mprCreateProperty except that the new value
 *	is passed by value rather than by pointer.
 */

MprVar *mprCreatePropertyValue(MprVar *obj, char *propertyName, MprVar newValue)
{
	return mprCreateProperty(obj, propertyName, &newValue);
}

/******************************************************************************/
/*
 *	Create a new property
 */

static MprVar *allocProperty(char *propertyName)
{
	MprVar		*prop;

	prop = (MprVar*) mprMalloc(sizeof(MprVar));
	if (prop == 0) {
		mprAssert(prop);
		return 0;
	}
	memset(prop, 0, sizeof(MprVar));
	prop->allocatedVar = 1;
	prop->name = mprStrdup(propertyName);
	prop->forw = (MprVar*) 0;

	return prop;
}

/******************************************************************************/
/*
 *	Update a property in an object with a defined value. Create the property
 *	if it doesn not already exist.
 */

MprVar *mprSetProperty(MprVar *obj, char *propertyName, MprVar *newValue)
{
	MprVar	*prop, triggerValue;
	int		rc;

	mprAssert(obj);
	mprAssert(propertyName && *propertyName);
	mprAssert(obj->type == MPR_TYPE_OBJECT);

	if (obj->type != MPR_TYPE_OBJECT) {
		mprAssert(0);
		return 0;
	}

	prop = mprGetProperty(obj, propertyName, 0);
	if (prop == 0) {
		return mprCreateProperty(obj, propertyName, newValue);
	}

	if (obj->trigger) {
		/* 
		 *	Call the trigger before the update and pass it the new value.
		 */
		triggerValue = *newValue;
		triggerValue.allocatedVar = 0;
		triggerValue.allocatedData = 0;
		rc = (obj->trigger)(MPR_VAR_WRITE, obj->properties, obj, 
				&triggerValue, 0);
		if (rc == MPR_TRIGGER_ABORT) {
			return 0;

		} else if (rc == MPR_TRIGGER_USE_NEW_VALUE) {
			/*
			 *	Trigger must copy to triggerValue a variable that is not
			 *	a structure copy of the existing data.
			 */
			copyVarCore(prop, &triggerValue, MPR_SHALLOW_COPY);
			mprDestroyVar(&triggerValue);
			return prop;
		}
	}
	copyVarCore(prop, newValue, MPR_SHALLOW_COPY);
	return prop;
}

/******************************************************************************/
/*
 *	Update a property in an object with a defined value. Create the property
 *	if it does not already exist. Same as mprSetProperty except that the 
 *	new value is passed by value rather than by pointer.
 */

MprVar *mprSetPropertyValue(MprVar *obj, char *propertyName, MprVar newValue)
{
	return mprSetProperty(obj, propertyName, &newValue);
}

/******************************************************************************/
/*
 *	Delete a property from this object
 */

int mprDeleteProperty(MprVar *obj, char *property)
{
	MprVar		*prop, *last;
	char		*cp;
	int			bucketIndex;

	mprAssert(obj);
	mprAssert(property && *property);
	mprAssert(obj->type == MPR_TYPE_OBJECT);

	if (obj->type != MPR_TYPE_OBJECT) {
		mprAssert(obj->type == MPR_TYPE_OBJECT);
		return 0;
	}

	last = 0;
	bucketIndex = hash(obj->properties, property);
	if ((prop = obj->properties->buckets[bucketIndex]) != 0) {
		for ( ; prop; prop = prop->forw) {
			cp = prop->name;
			if (cp[0] == property[0] && strcmp(cp, property) == 0) {
				break;
			}
			last = prop;
		}
	}
	if (prop == (MprVar*) 0) {
		mprAssert(prop);
		return MPR_ERR_NOT_FOUND;
	}
 	if (prop->readonly) {
		mprAssert(! prop->readonly);
		return MPR_ERR_READ_ONLY;
	}

	if (obj->trigger) {
		if ((obj->trigger)(MPR_VAR_DELETE_PROPERTY, obj->properties, prop, 0, 0)
				== MPR_TRIGGER_ABORT) {
			return MPR_ERR_ABORTED;
		}
	}

	if (last) {
		last->forw = prop->forw;
	} else {
		obj->properties->buckets[bucketIndex] = prop->forw;
	}

	obj->properties->numItems--;
	if (! mprVarIsFunction(prop->type)) {
		obj->properties->numDataItems--;
	}

	mprDestroyVar(prop);

	return 0;
}

/******************************************************************************/
/*
 *	Find a property in an object and return a pointer to it. If a value arg
 *	is supplied, then copy the data into the var. 
 */

MprVar *mprGetProperty(MprVar *obj, char *property, MprVar *value)
{
	MprVar	*prop, triggerValue;
	int		rc;

	if (obj == 0 || obj->type != MPR_TYPE_OBJECT || property == 0 || 
			*property == '\0') {
		if (value) {
			value->type = MPR_TYPE_UNDEFINED;
		}
		return 0;
	}

	for (prop = getObjChain(obj->properties, property); prop; 
			prop = prop->forw) {
		if (prop->name[0] == property[0] && strcmp(prop->name, property) == 0) {
			break;
		}
	}
	if (prop == 0) {
		if (value) {
			value->type = MPR_TYPE_UNDEFINED;
		}
		return 0;
	}
	if (value) {
		if (prop->trigger) {
			triggerValue = *prop;
			triggerValue.allocatedVar = 0;
			triggerValue.allocatedData = 0;
			/*
			 *	Pass the trigger the current read value and may receive
			 *	a new value.
			 */ 
			rc = (prop->trigger)(MPR_VAR_READ, prop->parentProperties, prop, 
				&triggerValue, 0);
			if (rc == MPR_TRIGGER_ABORT) {
				if (value) {
					value->type = MPR_TYPE_UNDEFINED;
				}
				return 0;

			} else if (rc == MPR_TRIGGER_USE_NEW_VALUE) {
				copyVarCore(prop, &triggerValue, MPR_SHALLOW_COPY);
				mprDestroyVar(&triggerValue);
			}
		}
		/*
		 *	Clone. No copy.
		 */
		*value = *prop;
	}
	return prop;
}

/******************************************************************************/
/*
 *	Read a properties value. This returns the property's value. It does not
 *	copy object/string data but returns a pointer directly into the variable.
 *	The caller does not and should not call mprDestroy on the returned value.
 *	If value is null, just read the property and run triggers.
 */

int mprReadProperty(MprVar *prop, MprVar *value)
{
	MprVar	triggerValue;
	int		rc;

	mprAssert(prop);

	if (prop->trigger) {
		triggerValue = *prop;
		triggerValue.allocatedVar = 0;
		triggerValue.allocatedData = 0;
		rc = (prop->trigger)(MPR_VAR_READ, prop->parentProperties, prop, 
			&triggerValue, 0);

		if (rc == MPR_TRIGGER_ABORT) {
			return MPR_ERR_ABORTED;

		} else if (rc == MPR_TRIGGER_USE_NEW_VALUE) {
			copyVarCore(prop, &triggerValue, MPR_SHALLOW_COPY);
			mprDestroyVar(&triggerValue);
			return 0;
		}
	}
	if (value) {
		*value = *prop;

		/*
		 *	Just so that if the user calls mprDestroyVar on value, it will do no
		 *	harm.
		 */
		value->allocatedVar = 0;
		value->allocatedData = 0;
	}
	return 0;
}

/******************************************************************************/
/*
 *	Read a properties value. This returns a copy of the property variable. 
 *	However, if the property is an object or string, it returns a copy of the
 *	reference to the underlying data. If copyDepth is set to MPR_DEEP_COPY,
 *	then the underlying objects and strings data will be copied as well. If 
 *	copyDepth is set to MPR_SHALLOW_COPY, then only strings will be copied. If
 *	it is set to MPR_NO_COPY, then no data will be copied. In all cases, the 
 *	user must call mprDestroyVar to free resources. This routine will run any 
 *	registered triggers which may modify the value the user receives (without 
 *	updating the properties real value).
 *
 *	WARNING: the args are reversed to most other APIs. This conforms to the
 *	strcpy(dest, src) standard instead.
 */

int mprCopyProperty(MprVar *dest, MprVar *prop, int copyDepth)
{
	MprVar	triggerValue;
	int		rc;

	mprAssert(prop);
	mprAssert(dest);

	if (prop->trigger) {
		triggerValue = *prop;
		triggerValue.allocatedVar = 0;
		triggerValue.allocatedData = 0;
		rc = (prop->trigger)(MPR_VAR_READ, prop->parentProperties, prop, 
			&triggerValue, copyDepth);

		if (rc == MPR_TRIGGER_ABORT) {
			return MPR_ERR_ABORTED;

		} else if (rc == MPR_TRIGGER_USE_NEW_VALUE) {
			copyVarCore(dest, &triggerValue, MPR_SHALLOW_COPY);
			mprDestroyVar(&triggerValue);
			return 0;
		}
	}
	mprCopyVar(dest, prop, copyDepth);
	return 0;
}

/******************************************************************************/
/*
 *	Write a new value into an existing property in an object.
 */

int mprWriteProperty(MprVar *vp, MprVar *value)
{
	MprVar	triggerValue;
	int		rc;

	mprAssert(vp);
	mprAssert(value);

	if (vp->readonly) {
		return MPR_ERR_READ_ONLY;
	}

	if (vp->trigger) {
		triggerValue = *value;

		rc = (vp->trigger)(MPR_VAR_WRITE, vp->parentProperties, vp, 
			&triggerValue, 0);

		if (rc == MPR_TRIGGER_ABORT) {
			return MPR_ERR_ABORTED;

		} else if (rc == MPR_TRIGGER_USE_NEW_VALUE) {
			copyVarCore(vp, &triggerValue, MPR_SHALLOW_COPY);
			mprDestroyVar(&triggerValue);
			return 0;
		}
		/* Fall through */
	}

	copyVarCore(vp, value, MPR_SHALLOW_COPY);
	return 0;
}

/******************************************************************************/
/*
 *	Write a new value into an existing property in an object.
 */

int mprWritePropertyValue(MprVar *vp, MprVar value)
{
	mprAssert(vp);

	return mprWriteProperty(vp, &value);
}

/******************************************************************************/
/*
 *	Get the count of properties.
 */

int mprGetPropertyCount(MprVar *vp, int includeFlags)
{
	mprAssert(vp);

	if (vp->type != MPR_TYPE_OBJECT) {
		return 0;
	}
	if (includeFlags == MPR_ENUM_DATA) {
		return vp->properties->numDataItems;
	} else {
		return vp->properties->numItems;
	}
}

/******************************************************************************/
/*
 *	Get the first property in an object. Used for walking all properties in an
 *	object.
 */

MprVar *mprGetFirstProperty(MprVar *obj, int includeFlags)
{
	MprVar		*prop;
	int			i;

	mprAssert(obj);
	mprAssert(obj->type == MPR_TYPE_OBJECT);

	if (obj->type != MPR_TYPE_OBJECT) {
		mprAssert(obj->type == MPR_TYPE_OBJECT);
		return 0;
	}

	for (i = 0; i < (int) obj->properties->hashSize; i++) {
		for (prop = obj->properties->buckets[i]; prop; prop = prop->forw) {
			if (prop) {
				if (mprVarIsFunction(prop->type)) {
					if (!(includeFlags & MPR_ENUM_FUNCTIONS)) {
						continue;
					}
				} else {
					if (!(includeFlags & MPR_ENUM_DATA)) {
						continue;
					}
				}
				return prop;
			}
			break;
		}
	}
	return 0;
}

/******************************************************************************/
/*
 *	Get the next property in sequence.
 */

MprVar *mprGetNextProperty(MprVar *obj, MprVar *last, int includeFlags)
{
	MprProperties	*properties;
	int				i;

	mprAssert(obj);
	mprAssert(obj->type == MPR_TYPE_OBJECT);

	if (obj->type != MPR_TYPE_OBJECT) {
		mprAssert(obj->type == MPR_TYPE_OBJECT);
		return 0;
	}
	properties = obj->properties;

	if (last->forw) {
		return last->forw;
	}

	for (i = last->bucketIndex + 1; i < (int) properties->hashSize; i++) {
		for (last = properties->buckets[i]; last; last = last->forw) {
			if (mprVarIsFunction(last->type)) {
				if (!(includeFlags & MPR_ENUM_FUNCTIONS)) {
					continue;
				}
			} else {
				if (!(includeFlags & MPR_ENUM_DATA)) {
					continue;
				}
			}
			return last;
		}
	}
	return 0;
}

/******************************************************************************/
/************************** Internal Support Routines *************************/
/******************************************************************************/
/*
 *	Create an hash table to hold and index properties. Properties are just 
 *	variables which may contain primitive data types, functions or other
 *	objects. The hash table is the essence of an object. HashSize specifies 
 *	the size of the hash table to use and should be a prime number.
 */

static MprProperties *createProperties(char *name, int hashSize)
{
	MprProperties	*pp;

	if (hashSize < 7) {
		hashSize = 7;
	}
	if ((pp = (MprProperties*) mprMalloc(sizeof(MprProperties))) == NULL) {
		mprAssert(0);
		return 0;
	}
	mprAssert(pp);
	memset(pp, 0, sizeof(MprProperties));

	pp->numItems = 0;
	pp->numDataItems = 0;
	pp->hashSize = hashSize;
	pp->buckets = (MprVar**) mprMalloc(pp->hashSize * sizeof(MprVar*));
	mprAssert(pp->buckets);
	memset(pp->buckets, 0, pp->hashSize * sizeof(MprVar*));
	pp->refCount = 1;

#if VAR_DEBUG
	if (objectCount == -1) {
		objectCount = 0;
		objectList.next = objectList.prev = &objectList;
	}

	mprStrcpy(pp->name, sizeof(pp->name), name);
	pp->next = &objectList;
	pp->prev = objectList.prev;
	objectList.prev->next = pp;
	objectList.prev = pp;
	objectCount++;
#endif
	return pp;
}

/******************************************************************************/
/*
 *	Release an object's properties hash table. If this is the last person 
 *	using it, free it. Return TRUE if the object is released.
 */

static bool releaseProperties(MprProperties *obj, int force)
{
	MprProperties	*pp;
	MprVar			*prop, *forw;
	int				i;

	mprAssert(obj);
	mprAssert(obj->refCount > 0);

#if VAR_DEBUG
	/*
	 *	Debug sanity check
	 */
	mprAssert(obj->refCount < 20);
#endif

	if (--obj->refCount > 0 && !force) {
		return 0;
	}

#if VAR_DEBUG
	mprAssert(obj->prev);
	mprAssert(obj->next);
	mprAssert(obj->next->prev);
	mprAssert(obj->prev->next);
	obj->next->prev = obj->prev;
	obj->prev->next = obj->next;
	objectCount--;
#endif

	for (i = 0; i < (int) obj->hashSize; i++) {
		for (prop = obj->buckets[i]; prop; prop = forw) {
			forw = prop->forw;
			if (prop->type == MPR_TYPE_OBJECT) {

				if (prop->properties == 0) {
					continue;
				}

				if (prop->properties == obj) {
					/* Self reference */
					continue;
				}
				pp = prop->properties;
				if (pp->visited) {
					continue;
				}

				pp->visited = 1;
				if (! freeVar(prop, pp->deleteProtect ? 0 : force)) {
					pp->visited = 0;
				}

			} else {
				freeVar(prop, force);
			}
		}
	}

	mprFree((void*) obj->buckets);
	mprFree((void*) obj);

	return 1;
}

/******************************************************************************/
/*
 *	Adjust the reference count
 */

static int adjustRefCount(MprProperties *pp, int adj)
{
	mprAssert(pp);

	/*
	 *	Debug sanity check
	 */
	mprAssert(pp->refCount < 40);

	return pp->refCount += adj;
}

/******************************************************************************/
#if VAR_DEBUG
/*
 *	Print objects held
 */

void mprPrintObjects(char *msg)
{
	MprProperties	*pp, *np;
	MprVar			*prop, *forw;
	char			*buf;
	int				i;

	mprLog(7, "%s: Object Store. %d objects.\n", msg, objectCount);
	pp = objectList.next;
	while (pp != &objectList) {
		mprLog(7, "%s: 0x%x, refCount %d, properties %d\n", 
			pp->name, pp, pp->refCount, pp->numItems);
		for (i = 0; i < (int) pp->hashSize; i++) {
			for (prop = pp->buckets[i]; prop; prop = forw) {
				forw = prop->forw;
				if (prop->properties == pp) {
					/* Self reference */
					continue;
				}
				mprVarToString(&buf, MPR_MAX_STRING, 0, prop);
				if (prop->type == MPR_TYPE_OBJECT) {
					np = objectList.next;
					while (np != &objectList) {
						if (prop->properties == np) {
							break;
						}
						np = np->next;
					}
					if (prop->properties == np) {
						mprLog(7, "    %s: OBJECT 0x%x, <%s>\n", 
							prop->name, prop->properties, prop->fullName);
					} else {
						mprLog(7, "    %s: OBJECT NOT FOUND, %s <%s>\n", 
							prop->name, buf, prop->fullName);
					}
				} else {
					mprLog(7, "    %s: <%s> = %s\n", prop->name, 
						prop->fullName, buf);
				}
				mprFree(buf);
			}
		}
		pp = pp->next;
	}
}

/******************************************************************************/

void mprPrintObjRefCount(MprVar *vp)
{
	mprLog(7, "OBJECT 0x%x, refCount %d\n", vp->properties,
		vp->properties->refCount);
}

#endif
/******************************************************************************/
/*
 *	Get the bucket chain containing a property.
 */

static MprVar *getObjChain(MprProperties *obj, char *property)
{
	mprAssert(obj);

	return obj->buckets[hash(obj, property)];
}

/******************************************************************************/
/*
 *	Fast hash. The history of this algorithm is part of lost computer science 
 *	folk lore.
 */

static int hash(MprProperties *pp, char *property)
{
	uint	sum;

	mprAssert(pp);
	mprAssert(property);

	sum = 0;
	while (*property) {
		sum += (sum * 33) + *property++;
	}

	return sum % pp->hashSize;
}

/******************************************************************************/
/*********************************** Constructors *****************************/
/******************************************************************************/
/*
 *	Initialize an undefined value.
 */

MprVar mprCreateUndefinedVar()
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_UNDEFINED;
	return v;
}

/******************************************************************************/
/*
 *	Initialize an null value.
 */

MprVar mprCreateNullVar()
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_NULL;
	return v;
}

/******************************************************************************/

MprVar mprCreateBoolVar(bool value)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_BOOL;
	v.boolean = value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a C function.
 */

MprVar mprCreateCFunctionVar(MprCFunction fn, void *thisPtr, int flags)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_CFUNCTION;
	v.cFunction.fn = fn;
	v.cFunction.thisPtr = thisPtr;
	v.flags = flags;

	return v;
}

/******************************************************************************/
/*
 *	Initialize a C function.
 */

MprVar mprCreateStringCFunctionVar(MprStringCFunction fn, void *thisPtr, int flags)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_STRING_CFUNCTION;
	v.cFunctionWithStrings.fn = fn;
	v.cFunctionWithStrings.thisPtr = thisPtr;
	v.flags = flags;

	return v;
}

/******************************************************************************/
#if BLD_FEATURE_FLOATING_POINT
/*
 *	Initialize a floating value.
 */

MprVar mprCreateFloatVar(double value)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_FLOAT;
	v.floating = value;
	return v;
}

#endif
/******************************************************************************/
/*
 *	Initialize an integer value.
 */

MprVar mprCreateIntegerVar(int value)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_INT;
	v.integer = value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize a 64-bit integer value.
 */

MprVar mprCreateInteger64Var(int64 value)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_INT64;
	v.integer64 = value;
	return v;
}

/******************************************************************************/
/*
 *	Initialize an number variable. Type is defined by configure.
 */

MprVar mprCreateNumberVar(MprNum value)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = BLD_FEATURE_NUM_TYPE_ID;
#if   BLD_FEATURE_NUM_TYPE_ID == MPR_TYPE_INT64
	v.integer64 = value;
#elif BLD_FEATURE_NUM_TYPE_ID == MPR_TYPE_FLOAT
	v.float = value;
#else
	v.integer = value;
#endif
	return v;
}

/******************************************************************************/
/*
 *	Initialize a (bare) JavaScript function. args and body can be null.
 */

MprVar mprCreateFunctionVar(char *args, char *body, int flags)
{
	MprVar		v;
	char		*cp, *arg, *last;
	int			aid;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_FUNCTION;
	v.flags = flags;

	v.function.args = mprCreateArray();

	if (args) {
		args = mprStrdup(args);
		arg = mprStrTok(args, ",", &last);
		while (arg) {
			while (isspace((int) *arg))
				arg++;
			for (cp = &arg[strlen(arg) - 1]; cp > arg; cp--) {
				if (!isspace((int) *cp)) {
					break;
				}
			}
			cp[1] = '\0';
					
			aid = mprAddToArray(v.function.args, mprStrdup(arg));
			arg = mprStrTok(0, ",", &last);
		}
		mprFree(args);
	}

	if (body) {
		v.function.body = mprStrdup(body);
	}
	v.allocatedData = 1;
	return v;
}

/******************************************************************************/
/*
 *	Initialize an object variable. Return type == MPR_TYPE_UNDEFINED if the
 *	memory allocation for the properties table failed.
 */

MprVar mprCreateObjVar(char *name, int hashSize)
{
	MprVar	v;

	mprAssert(name && *name);

	memset(&v, 0x0, sizeof(MprVar));
	v.type = MPR_TYPE_OBJECT;
	if (hashSize <= 0) {
		hashSize = MPR_DEFAULT_HASH_SIZE;
	}
	v.properties = createProperties(name, hashSize);
	if (v.properties == 0) {
		/* Indicate failed memory allocation */
		v.type = MPR_TYPE_UNDEFINED;
	}
	v.allocatedData = 1;
	v.name = mprStrdup(name);
	mprLog(7, "mprCreateObjVar %s, 0x%x\n", name, v.properties);
	return v;
}

/******************************************************************************/
/*
 *	Initialize a string value.
 */

MprVar mprCreateStringVar(char *value, bool allocate)
{
	MprVar	v;

	memset(&v, 0x0, sizeof(v));
	v.type = MPR_TYPE_STRING;
	if (value == 0) {
		v.string = "";
	} else if (allocate) {
		v.string = mprStrdup(value);
		v.allocatedData = 1;
	} else {
		v.string = value;
	}
	return v;
}

/******************************************************************************/
/*
 *	Copy an objects core value (only). This preserves the destination object's 
 *	name. This implements copy by reference for objects and copy by value for 
 *	strings and other types. Caller must free dest prior to calling.
 */

static void copyVarCore(MprVar *dest, MprVar *src, int copyDepth)
{
	MprVarTrigger	saveTrigger;
	MprVar			*srcProp, *destProp, *last;
	char			**srcArgs;
	int				i;

	mprAssert(dest);
	mprAssert(src);

	if (dest == src) {
		return;
	}

	/*
 	 *	FUTURE: we should allow read-only triggers where the value is never
	 *	stored in the object. Currently, triggers override the readonly
	 *	status.
	 */

	if (dest->type != MPR_TYPE_UNDEFINED && dest->readonly && !dest->trigger) {
		mprAssert(0);
		return;
	}

	if (dest->type != MPR_TYPE_UNDEFINED) {
		saveTrigger = dest->trigger;
		freeVarStorage(dest, 0);
	} else {
		saveTrigger = 0;
	}

	switch (src->type) {
	default:
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
		break;

	case MPR_TYPE_BOOL:
		dest->boolean = src->boolean;
		break;

	case MPR_TYPE_STRING_CFUNCTION:
		dest->cFunctionWithStrings = src->cFunctionWithStrings;
		break;

	case MPR_TYPE_CFUNCTION:
		dest->cFunction = src->cFunction;
		break;

#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
		dest->floating = src->floating;
		break;
#endif

	case MPR_TYPE_INT:
		dest->integer = src->integer;
		break;

	case MPR_TYPE_INT64:
		dest->integer64 = src->integer64;
		break;

	case MPR_TYPE_OBJECT:
		if (copyDepth == MPR_DEEP_COPY) {

			dest->properties = createProperties(src->name, 
				src->properties->hashSize);
			dest->allocatedData = 1;

			for (i = 0; i < (int) src->properties->hashSize; i++) {
				last = 0;
				for (srcProp = src->properties->buckets[i]; srcProp; 
						srcProp = srcProp->forw) {
					if (srcProp->visited) {
						continue;
					}
					destProp = allocProperty(srcProp->name);
					if (destProp == 0) {
						mprAssert(destProp);
						return;
					}

					destProp->bucketIndex = i;
					if (last) {
						last->forw = destProp;
					} else {
						dest->properties->buckets[i] = destProp;
					}
					destProp->parentProperties = dest->properties;

					/*
					 *	Recursively copy the object
					 */
					srcProp->visited = 1;
					copyVarCore(destProp, srcProp, copyDepth);
					srcProp->visited = 0;
					last = destProp;
				}
			}
			dest->properties->numItems = src->properties->numItems;
			dest->properties->numDataItems = src->properties->numDataItems;
			dest->allocatedData = 1;

		} else if (copyDepth == MPR_SHALLOW_COPY) {
			dest->properties = src->properties;
			adjustVarRefCount(src, 1);
			dest->allocatedData = 1;

		} else {
			dest->properties = src->properties;
			dest->allocatedData = 0;
		}
		break;

	case MPR_TYPE_FUNCTION:
		if (copyDepth != MPR_NO_COPY) {
			dest->function.args = mprCreateArray();
			srcArgs = (char**) src->function.args->handles;
			for (i = 0; i < src->function.args->max; i++) {
				if (srcArgs[i]) {
					mprAddToArray(dest->function.args, mprStrdup(srcArgs[i]));
				}
			}
			dest->function.body = mprStrdup(src->function.body);
			dest->allocatedData = 1;
		} else {
			dest->function.args = src->function.args;
			dest->function.body = src->function.body;
			dest->allocatedData = 0;
		}
		break;

	case MPR_TYPE_STRING:
		if (src->string && copyDepth != MPR_NO_COPY) {
			dest->string = mprStrdup(src->string);
			dest->allocatedData = 1;
		} else {
			dest->string = src->string;
			dest->allocatedData = 0;
		}
		break;
	}

	dest->type = src->type;
	dest->flags = src->flags;
	dest->trigger = saveTrigger;

	/*
	 *	Just for safety
	 */
	dest->spare = 0;
}

/******************************************************************************/
/*
 *	Copy an entire object including name.
 */

void mprCopyVar(MprVar *dest, MprVar *src, int copyDepth)
{
	mprAssert(dest);
	mprAssert(src);

	copyVarCore(dest, src, copyDepth);

	mprFree(dest->name);
	dest->name = mprStrdup(src->name);

#if VAR_DEBUG
	if (src->type == MPR_TYPE_OBJECT) {

		mprFree(dest->fullName);
		dest->fullName = mprStrdup(src->fullName);

		mprLog(7, "mprCopyVar: object \"%s\", FDQ \"%s\" 0x%x, refCount %d\n", 
			dest->name, dest->fullName, dest->properties, 
			dest->properties->refCount);
	}
#endif
}

/******************************************************************************/
/*
 *	Copy an entire object including name.
 */

void mprCopyVarValue(MprVar *dest, MprVar src, int copyDepth)
{
	mprAssert(dest);

	mprCopyVar(dest, &src, copyDepth); 
}

/******************************************************************************/
/*
 *	Copy an object. This implements copy by reference for objects and copy by
 *	value for strings and other types. Caller must free dest prior to calling.
 */

MprVar *mprDupVar(MprVar *src, int copyDepth)
{
	MprVar	*dest;

	mprAssert(src);

	dest = (MprVar*) mprMalloc(sizeof(MprVar));
	memset(dest, 0, sizeof(MprVar));

	mprCopyVar(dest, src, copyDepth);
	return dest;
}

/******************************************************************************/
/*
 *	Convert a value to a text based representation of its value
 *	FUTURE -- conver this to use the format string in all cases. Allow
 *	arbitrary format strings.
 */

void mprVarToString(char** out, int size, char *fmt, MprVar *obj)
{
	char	*src;

	mprAssert(out);

	*out = NULL;

	if (obj->trigger) {
		mprReadProperty(obj, 0);
	}

	switch (obj->type) {
	case MPR_TYPE_UNDEFINED:
		//	FUTURE -- spec says convert to "undefined"
		*out = mprStrdup("");
		break;

	case MPR_TYPE_NULL:
		*out = mprStrdup("null");
		break;

	case MPR_TYPE_BOOL:
		if (obj->boolean) {
			*out = mprStrdup("true");
		} else {
			*out = mprStrdup("false");
		}
		break;

#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
		if (fmt == NULL || *fmt == '\0') {
			mprAllocSprintf(out, size, "%f", obj->floating);
		} else {
			mprAllocSprintf(out, size, fmt, obj->floating);
		}
		break;
#endif

	case MPR_TYPE_INT:
		if (fmt == NULL || *fmt == '\0') {
			mprAllocSprintf(out, size, "%d", obj->integer);
		} else {
			mprAllocSprintf(out, size, fmt, obj->integer);
		}
		break;

	case MPR_TYPE_INT64:
		if (fmt == NULL || *fmt == '\0') {
			mprAllocSprintf(out, size, "%Ld", obj->integer64);
		} else {
			mprAllocSprintf(out, size, fmt, obj->integer64);
		}
		break;

	case MPR_TYPE_CFUNCTION:
		mprAllocSprintf(out, size, "[C Function]");
		break;

	case MPR_TYPE_STRING_CFUNCTION:
		mprAllocSprintf(out, size, "[C StringFunction]");
		break;

	case MPR_TYPE_FUNCTION:
		mprAllocSprintf(out, size, "[JavaScript Function]");
		break;

	case MPR_TYPE_OBJECT:
		//	FUTURE -- really want: [object class: name] 
		mprAllocSprintf(out, size, "[object %s]", obj->name);
		break;

	case MPR_TYPE_STRING:
		src = obj->string;

		mprAssert(src);
		if (fmt && *fmt) {
			mprAllocSprintf(out, size, fmt, src);

		} else if (src == NULL) {
			*out = mprStrdup("null");

		} else {
			*out = mprStrdup(src);
		}
		break;

	default:
		mprAssert(0);
	}
}

/******************************************************************************/
/*
 *	Parse a string based on formatting instructions and intelligently 
 *	create a variable.
 */

MprVar mprParseVar(char *buf, MprType preferredType)
{
	MprType		type;
	char		*cp;

	mprAssert(buf);

	type = preferredType;

	if (preferredType == MPR_TYPE_UNDEFINED) {
		if (*buf == '-') {
			type = MPR_NUM_VAR;

		} else if (!isdigit((int) *buf)) {
			if (strcmp(buf, "true") == 0 || strcmp(buf, "false") == 0) {
				type = MPR_TYPE_BOOL;
			} else {
				type = MPR_TYPE_STRING;
			}

		} else if (isdigit((int) *buf)) {
			type = MPR_NUM_VAR;
			cp = buf;
			if (*cp && tolower(((int) cp[1])) == 'x') {
				cp = &cp[2];
			}
			for (cp = buf; *cp; cp++) {
				if (! isdigit((int) *cp)) {
					break;
				}
			}

			if (*cp != '\0') {
#if BLD_FEATURE_FLOATING_POINT
				if (*cp == '.' || tolower((int) *cp) == 'e') {
					type = MPR_TYPE_FLOAT;
				} else
#endif
				{
					type = MPR_NUM_VAR;
				}
			}
		}
	}

	switch (type) {
	case MPR_TYPE_OBJECT:
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
	default:
		break;

	case MPR_TYPE_BOOL:
		return mprCreateBoolVar(buf[0] == 't' ? 1 : 0);

	case MPR_TYPE_INT:
		return mprCreateIntegerVar(mprParseInteger(buf));

	case MPR_TYPE_INT64:
		return mprCreateInteger64Var(mprParseInteger64(buf));

	case MPR_TYPE_STRING:
		if (strcmp(buf, "null") == 0) {
			return mprCreateNullVar();
		} else if (strcmp(buf, "undefined") == 0) {
			return mprCreateUndefinedVar();
		} 
			
		return mprCreateStringVar(buf, 1);

#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
		return mprCreateFloatVar(atof(buf));
#endif

	}
	return mprCreateUndefinedVar();
}

/******************************************************************************/
/*
 *	Convert the variable to a boolean. Only for primitive types.
 */

bool mprVarToBool(MprVar *vp)
{
	mprAssert(vp);

	switch (vp->type) {
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
	case MPR_TYPE_STRING_CFUNCTION:
	case MPR_TYPE_CFUNCTION:
	case MPR_TYPE_FUNCTION:
	case MPR_TYPE_OBJECT:
		return 0;

	case MPR_TYPE_BOOL:
		return vp->boolean;

#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
		return (vp->floating != 0 && !mprIsNan(vp->floating));
#endif

	case MPR_TYPE_INT:
		return (vp->integer != 0);

	case MPR_TYPE_INT64:
		return (vp->integer64 != 0);

	case MPR_TYPE_STRING:
		mprAssert(vp->string);
		return (vp->string[0] != '\0');
	}

	/* Not reached */
	return 0;
}

/******************************************************************************/
#if BLD_FEATURE_FLOATING_POINT
/*
 *	Convert the variable to a floating point number. Only for primitive types.
 */

double mprVarToFloat(MprVar *vp)
{
	mprAssert(vp);

	switch (vp->type) {
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
	case MPR_TYPE_STRING_CFUNCTION:
	case MPR_TYPE_CFUNCTION:
	case MPR_TYPE_FUNCTION:
	case MPR_TYPE_OBJECT:
		return 0;

	case MPR_TYPE_BOOL:
		return (vp->boolean) ? 1.0 : 0.0;

	case MPR_TYPE_FLOAT:
		return vp->floating;

	case MPR_TYPE_INT:
		return (double) vp->integer;

	case MPR_TYPE_INT64:
		return (double) vp->integer64;

	case MPR_TYPE_STRING:
		mprAssert(vp->string);
		return atof(vp->string);
	}

	/* Not reached */
	return 0;
}

#endif
/******************************************************************************/
/*
 *	Convert the variable to a number type. Only works for primitive types.
 */

MprNum mprVarToNumber(MprVar *vp)
{
#if BLD_FEATURE_NUM_TYPE_ID == MPR_TYPE_INT64
	return mprVarToInteger64(vp);
#elif BLD_FEATURE_NUM_TYPE_ID == MPR_TYPE_FLOAT
	return mprVarToFloat(vp);
#else 
	return mprVarToInteger(vp);
#endif
}

/******************************************************************************/
/*
 *	Convert the variable to a number type. Only works for primitive types.
 */

MprNum mprParseNumber(char *s)
{
#if BLD_FEATURE_NUM_TYPE_ID == MPR_TYPE_INT64
	return mprParseInteger64(s);
#elif BLD_FEATURE_NUM_TYPE_ID == MPR_TYPE_FLOAT
	return mprParseFloat(s);
#else 
	return mprParseInteger(s);
#endif
}

/******************************************************************************/
/*
 *	Convert the variable to an Integer64 type. Only works for primitive types.
 */

int64 mprVarToInteger64(MprVar *vp)
{
	mprAssert(vp);

	switch (vp->type) {
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
	case MPR_TYPE_STRING_CFUNCTION:
	case MPR_TYPE_CFUNCTION:
	case MPR_TYPE_FUNCTION:
	case MPR_TYPE_OBJECT:
		return 0;

	case MPR_TYPE_BOOL:
		return (vp->boolean) ? 1 : 0;

#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
		if (mprIsNan(vp->floating)) {
			return 0;
		}
		return (int64) vp->floating;
#endif

	case MPR_TYPE_INT:
		return vp->integer;

	case MPR_TYPE_INT64:
		return vp->integer64;

	case MPR_TYPE_STRING:
		return mprParseInteger64(vp->string);
	}

	/* Not reached */
	return 0;
}

/******************************************************************************/
/*
 *	Convert the string buffer to an Integer64.
 */

int64 mprParseInteger64(char *str)
{
	char	*cp;
	int64	num64;
	int		radix, c, negative;

	mprAssert(str);

	cp = str;
	num64 = 0;
	negative = 0;

	if (*cp == '-') {
		cp++;
		negative = 1;
	}

	/*
	 *	Parse a number. Observe hex and octal prefixes (0x, 0)
	 */
	if (*cp != '0') {
		/* 
		 *	Normal numbers (Radix 10)
		 */
		while (isdigit((int) *cp)) {
			num64 = (*cp - '0') + (num64 * 10);
			cp++;
		}
	} else {
		cp++;
		if (tolower((int) *cp) == 'x') {
			cp++;
			radix = 16;
			while (*cp) {
				c = tolower((int) *cp);
				if (isdigit(c)) {
					num64 = (c - '0') + (num64 * radix);
				} else if (c >= 'a' && c <= 'f') {
					num64 = (c - 'a') + (num64 * radix);
				} else {
					break;
				}
				cp++;
			}

		} else{
			radix = 8;
			while (*cp) {
				c = tolower((int) *cp);
				if (isdigit(c) && c < '8') {
					num64 = (c - '0') + (num64 * radix);
				} else {
					break;
				}
				cp++;
			}
		}
	}

	if (negative) {
		return 0 - num64;
	}
	return num64;
}

/******************************************************************************/
/*
 *	Convert the variable to an Integer type. Only works for primitive types.
 */

int mprVarToInteger(MprVar *vp)
{
	mprAssert(vp);

	switch (vp->type) {
	case MPR_TYPE_UNDEFINED:
	case MPR_TYPE_NULL:
	case MPR_TYPE_STRING_CFUNCTION:
	case MPR_TYPE_CFUNCTION:
	case MPR_TYPE_FUNCTION:
	case MPR_TYPE_OBJECT:
		return 0;

	case MPR_TYPE_BOOL:
		return (vp->boolean) ? 1 : 0;

#if BLD_FEATURE_FLOATING_POINT
	case MPR_TYPE_FLOAT:
		if (mprIsNan(vp->floating)) {
			return 0;
		}
		return (int) vp->floating;
#endif

	case MPR_TYPE_INT:
		return vp->integer;

	case MPR_TYPE_INT64:
		return (int) vp->integer64;

	case MPR_TYPE_STRING:
		return mprParseInteger(vp->string);
	}

	/* Not reached */
	return 0;
}

/******************************************************************************/
/*
 *	Convert the string buffer to an Integer.
 */

int mprParseInteger(char *str)
{
	char	*cp;
	int		num;
	int		radix, c, negative;

	mprAssert(str);

	cp = str;
	num = 0;
	negative = 0;

	if (*cp == '-') {
		cp++;
		negative = 1;
	}

	/*
	 *	Parse a number. Observe hex and octal prefixes (0x, 0)
	 */
	if (*cp != '0') {
		/* 
		 *	Normal numbers (Radix 10)
		 */
		while (isdigit((int) *cp)) {
			num = (*cp - '0') + (num * 10);
			cp++;
		}
	} else {
		cp++;
		if (tolower((int) *cp) == 'x') {
			cp++;
			radix = 16;
			while (*cp) {
				c = tolower((int) *cp);
				if (isdigit(c)) {
					num = (c - '0') + (num * radix);
				} else if (c >= 'a' && c <= 'f') {
					num = (c - 'a') + (num * radix);
				} else {
					break;
				}
				cp++;
			}

		} else{
			radix = 8;
			while (*cp) {
				c = tolower((int) *cp);
				if (isdigit(c) && c < '8') {
					num = (c - '0') + (num * radix);
				} else {
					break;
				}
				cp++;
			}
		}
	}

	if (negative) {
		return 0 - num;
	}
	return num;
}

/******************************************************************************/
#if BLD_FEATURE_FLOATING_POINT
/*
 *	Convert the string buffer to an Floating.
 */

double mprParseFloat(char *str)
{
	return atof(str);
}

/******************************************************************************/

bool mprIsNan(double f)
{
#if WIN
	return _isnan(f);
#elif VXWORKS
	return f != f;
#else
	return (f == FP_NAN);
#endif
}
/******************************************************************************/

bool mprIsInfinite(double f)
{
#if WIN
	return !_finite(f);
#elif VXWORKS
	return f == (1.0 / 0.0) || f == (-1.0 / 0.0);
#else
	return (f == FP_INFINITE);
#endif
}

#endif // BLD_FEATURE_FLOATING_POINT
/******************************************************************************/

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: sw=4 ts=4 
 */
