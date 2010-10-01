/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/



//
// as_arrayobject.h
//
// A class for storing arrays of any object type in the scripts
//



#ifndef AS_ANYOBJECT_H
#define AS_ANYOBJECT_H

#include "as_config.h"
#include "as_gcobject.h"

BEGIN_AS_NAMESPACE

class asCScriptEngine;
class asCObjectType;
class asCDataType;

class asCAnyObject : public asIScriptAny
{
public:
	asCAnyObject(asCObjectType *ot);
	asCAnyObject(void *ref, int refTypeId, asCObjectType *ot);
	virtual ~asCAnyObject();

	int AddRef();
	int Release();

	asCAnyObject &operator=(asCAnyObject&);

	void Store(void *ref, int refTypeId);
	int  Retrieve(void *ref, int refTypeId);
	int  GetTypeId();
	int  CopyFrom(asIScriptAny *other);

	// GC methods
	void Destruct();
	void CountReferences();
	void AddUnmarkedReferences(asCArray<asCGCObject*> &unmarked);
	void ReleaseAllHandles();

protected:
	void FreeObject();

	asSGCObject gc;
	int valueTypeId;
	void *value;
};

void RegisterAnyObject(asCScriptEngine *engine);
void AnyObjectConstructor(asCObjectType *ot, asCAnyObject *self);
void AnyObjectConstructor2(void *ref, int refTypeId, asCObjectType *ot, asCAnyObject *self);

END_AS_NAMESPACE

#endif
