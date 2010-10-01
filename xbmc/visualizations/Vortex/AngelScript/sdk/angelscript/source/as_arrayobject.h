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



#ifndef AS_ARRAYOBJECT_H
#define AS_ARRAYOBJECT_H

#include "as_config.h"
#include "as_gcobject.h"

BEGIN_AS_NAMESPACE

#define asDEFAULT_ARRAY "#array"

class asCScriptEngine;
struct sArrayBuffer;
class asCObjectType;

class asCArrayObject : public asIScriptArray
{
public:
	asCArrayObject(asUINT length, asCObjectType *ot);
	virtual ~asCArrayObject();

	int AddRef();
	int Release();

	int    GetArrayTypeId();
	int    GetElementTypeId();

	void   Resize(asUINT numElements);
	asUINT GetElementCount();
	void  *GetElementPointer(asUINT index);
	void  *at(asUINT index);
	asCArrayObject &operator=(asCArrayObject&);

	int    CopyFrom(asIScriptArray *other);

	// GC methods
	void Destruct();
	void CountReferences();
	void AddUnmarkedReferences(asCArray<asCGCObject*> &unmarked);
	void ReleaseAllHandles();

protected:
	asSGCObject gc;
	sArrayBuffer *buffer;
	int arrayType;
	int elementSize;

	void CreateBuffer(sArrayBuffer **buf, asUINT numElements);
	void DeleteBuffer(sArrayBuffer *buf);
	void CopyBuffer(sArrayBuffer *dst, sArrayBuffer *src);

	void Construct(sArrayBuffer *buf, asUINT start, asUINT end);
	void Destruct(sArrayBuffer *buf, asUINT start, asUINT end);
};

void RegisterArrayObject(asCScriptEngine *engine);
void ArrayObjectConstructor(asCObjectType *ot, asCArrayObject *self);

END_AS_NAMESPACE

#endif
