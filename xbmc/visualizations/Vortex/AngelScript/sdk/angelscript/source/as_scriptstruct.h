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
// as_scriptstruct.h
//
// A generic class for handling script declared structures
//



#ifndef AS_SCRIPTSTRUCT_H
#define AS_SCRIPTSTRUCT_H

#include "as_config.h"
#include "as_gcobject.h"

BEGIN_AS_NAMESPACE

class asCObjectType;

class asCScriptStruct : public asIScriptStruct
{
public:
	asCScriptStruct(asCObjectType *objType);
	virtual ~asCScriptStruct();

	int AddRef();
	int Release();

	int GetStructTypeId();

	int GetPropertyCount();
	int GetPropertyTypeId(asUINT prop);
	const char *GetPropertyName(asUINT prop);
	void *GetPropertyPointer(asUINT prop);

	asCScriptStruct &operator=(const asCScriptStruct &other);

	int CopyFrom(asIScriptStruct *other);

	// GC methods
	void Destruct();
	void CountReferences();
	void AddUnmarkedReferences(asCArray<asCGCObject*> &unmarked);
	void ReleaseAllHandles();

protected:
	// Used for properties
	void *AllocateObject(asCObjectType *objType, asCScriptEngine *engine);
	void FreeObject(void *ptr, asCObjectType *objType, asCScriptEngine *engine);
	void CopyObject(void *src, void *dst, asCObjectType *objType, asCScriptEngine *engine);
	void CopyHandle(asDWORD *src, asDWORD *dst, asCObjectType *objType, asCScriptEngine *engine);

	asSGCObject gc;
};

void ScriptStruct_Construct(asCObjectType *objType, asCScriptStruct *self);
asCScriptStruct &ScriptStruct_Assignment(asCScriptStruct *other, asCScriptStruct *self);

void ScriptStruct_Construct_Generic(asIScriptGeneric *gen);
void ScriptStruct_Assignment_Generic(asIScriptGeneric *gen);

END_AS_NAMESPACE

#endif
