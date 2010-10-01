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
// as_gcobject.h
//
// A garbage collectable object
//



#ifndef AS_GCOBJECT_H
#define AS_GCOBJECT_H

#include "as_config.h"
#include "as_array.h"

BEGIN_AS_NAMESPACE

class asCObjectType;

// Do not inherit from this struct, instead place it as the 
// first member in your class. Also make sure your class has
// virtual methods so that the vftable is there.
struct asSGCObject
{
	asSGCObject();
	~asSGCObject();

	int AddRef();
	int Release();

	void Init(asCObjectType *objType);

//protected:
	int refCount;
	asCObjectType *objType;
	int gcCount;
};

// This class is implemented so that we can treat the garbage
// collected objects equally without actually inheriting from 
// the GCObject. We must do this since we cannot use multiple  
// inheritance when storing anonymous pointers.
class asCGCObject
{
public:
	asCGCObject();
	virtual ~asCGCObject();

	int AddRef();
	int Release();

	// Necessary GC methods
	void Destruct();
	void CountReferences();
	void AddUnmarkedReferences(asCArray<asCGCObject*> &unmarked);
	void ReleaseAllHandles();

//protected:
	asSGCObject gc;
};

// To find the pointer of a asCGCObject from the asSGCObject 
// we need to compensate for the virtual function table
#define CONV2GCCLASS(x) ((asCGCObject*)((char*)(x) - sizeof(void*)))

void GCObject_AddRef(asCGCObject *self);
void GCObject_Release(asCGCObject *self);
void GCObject_AddRef_Generic(asIScriptGeneric *gen);
void GCObject_Release_Generic(asIScriptGeneric *gen);

END_AS_NAMESPACE

#endif
