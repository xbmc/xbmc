/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

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

   Andreas Jonsson
   andreas@angelcode.com
*/


#include <new>
#include <stdlib.h>

#include "as_config.h"
#include "as_arrayobject.h"
#include "as_texts.h"

BEGIN_AS_NAMESPACE

struct sArrayBuffer
{
	asDWORD numElements;
	asBYTE  data[1];
};

static asCArrayObject* ArrayObjectFactory2(asIObjectType *ot, asUINT length)
{
	asCArrayObject *a = asNEW(asCArrayObject)(length, ot);

	// It's possible the constructor raised a script exception, in which case we 
	// need to free the memory and return null instead, else we get a memory leak.
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && ctx->GetState() == asEXECUTION_EXCEPTION )
	{
		asDELETE(a, asCArrayObject);
		return 0;
	}

	return a;
}

asCArrayObject* ArrayObjectFactory(asIObjectType *ot)
{
	return ArrayObjectFactory2(ot, 0);
}

#ifndef AS_MAX_PORTABILITY

static asCArrayObject &ArrayObjectAssignment(asCArrayObject *other, asCArrayObject *self)
{
	return *self = *other;
}

static void *ArrayObjectAt(asUINT index, asCArrayObject *self)
{
	return self->at(index);
}

static asUINT ArrayObjectLength(asCArrayObject *self)
{
	return self->GetElementCount();
}
static void ArrayObjectResize(asUINT size, asCArrayObject *self)
{
	self->Resize(size);
}

#else

static void ArrayObjectFactory_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);

	*(asCArrayObject**)gen->GetAddressOfReturnLocation() = ArrayObjectFactory(ot);
}

static void ArrayObjectFactory2_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);
	asUINT length = gen->GetArgDWord(1);

	*(asCArrayObject**)gen->GetAddressOfReturnLocation() = ArrayObjectFactory2(ot, length);
}

static void ArrayObjectAssignment_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *other = (asCArrayObject*)gen->GetArgObject(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void ArrayObjectAt_Generic(asIScriptGeneric *gen)
{
	asUINT index = gen->GetArgDWord(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	gen->SetReturnAddress(self->at(index));
}

static void ArrayObjectLength_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	gen->SetReturnDWord(self->GetElementCount());
}

static void ArrayObjectResize_Generic(asIScriptGeneric *gen)
{
	asUINT size = gen->GetArgDWord(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	self->Resize(size);
}

static void ArrayObject_AddRef_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	self->AddRef();
}

static void ArrayObject_Release_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	self->Release();
}

static void ArrayObject_GetRefCount_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = self->GetRefCount();
}

static void ArrayObject_SetFlag_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	self->SetFlag();
}

static void ArrayObject_GetFlag_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	*(bool*)gen->GetAddressOfReturnLocation() = self->GetFlag();
}

static void ArrayObject_EnumReferences_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->EnumReferences(engine);
}

static void ArrayObject_ReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->ReleaseAllHandles(engine);
}

#endif

void RegisterArrayObject(asIScriptEngine *engine)
{
	int r;

	r = engine->RegisterObjectType("_builtin_array_<class T>", sizeof(asCArrayObject), asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE); asASSERT( r >= 0 );
#ifndef AS_MAX_PORTABILITY
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_FACTORY, "_builtin_array_<T>@ f(int&in)", asFUNCTIONPR(ArrayObjectFactory, (asIObjectType*), asCArrayObject*), asCALL_CDECL); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_FACTORY, "_builtin_array_<T>@ f(int&in, uint)", asFUNCTIONPR(ArrayObjectFactory2, (asIObjectType*, asUINT), asCArrayObject*), asCALL_CDECL); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(asCArrayObject,AddRef), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(asCArrayObject,Release), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterObjectMethod("_builtin_array_<T>", "_builtin_array_<T> &opAssign(const _builtin_array_<T>&in)", asFUNCTION(ArrayObjectAssignment), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_INDEX, "T &f(uint)", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_INDEX, "const T &f(uint) const", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );
	r = engine->RegisterObjectMethod("_builtin_array_<T>", "uint length() const", asFUNCTION(ArrayObjectLength), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );
	r = engine->RegisterObjectMethod("_builtin_array_<T>", "void resize(uint)", asFUNCTION(ArrayObjectResize), asCALL_CDECL_OBJLAST); asASSERT( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(asCArrayObject,GetRefCount), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(asCArrayObject,SetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(asCArrayObject,GetFlag), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(asCArrayObject,EnumReferences), asCALL_THISCALL); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(asCArrayObject,ReleaseAllHandles), asCALL_THISCALL); asASSERT( r >= 0 );
#else
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_FACTORY, "_builtin_array_<T>@ f(int&in)", asFUNCTION(ArrayObjectFactory_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_FACTORY, "_builtin_array_<T>@ f(int&in, uint)", asFUNCTION(ArrayObjectFactory2_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(ArrayObject_AddRef_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(ArrayObject_Release_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectMethod("_builtin_array_<T>", "_builtin_array_<T> &opAssign(const _builtin_array_<T>&in)", asFUNCTION(ArrayObjectAssignment_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_INDEX, "T &f(uint)", asFUNCTION(ArrayObjectAt_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_INDEX, "const T &f(uint) const", asFUNCTION(ArrayObjectAt_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectMethod("_builtin_array_<T>", "uint length() const", asFUNCTION(ArrayObjectLength_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectMethod("_builtin_array_<T>", "void resize(uint)", asFUNCTION(ArrayObjectResize_Generic), asCALL_GENERIC); asASSERT( r >= 0 );

	// Register GC behaviours
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ArrayObject_GetRefCount_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ArrayObject_SetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ArrayObject_GetFlag_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ArrayObject_EnumReferences_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
	r = engine->RegisterObjectBehaviour("_builtin_array_<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ArrayObject_ReleaseAllHandles_Generic), asCALL_GENERIC); asASSERT( r >= 0 );
#endif 
}

asCArrayObject &asCArrayObject::operator=(asCArrayObject &other)
{
	if( &other != this )
	{
		if( buffer )
		{
			DeleteBuffer(buffer);
			buffer = 0;
		}

		// Copy all elements from the other array
		CreateBuffer(&buffer, other.buffer->numElements);
		CopyBuffer(buffer, other.buffer);
	}

	return *this;
}

int asCArrayObject::CopyFrom(asIScriptArray *other)
{
	if( other == 0 ) return asINVALID_ARG;

	// Verify that the types are equal
	if( GetArrayTypeId() != other->GetArrayTypeId() )
		return asINVALID_TYPE;

	*this = *(asCArrayObject*)other;

	return 0;
}

asCArrayObject::asCArrayObject(asUINT length, asIObjectType *ot)
{
	refCount.set(1);
	gcFlag = false;
	objType = ot;
	objType->AddRef();
	buffer = 0;

	// Determine element size
	int typeId = objType->GetSubTypeId();
	if( typeId & asTYPEID_MASK_OBJECT )
	{
		elementSize = sizeof(asPWORD);
	}
	else
	{
		elementSize = objType->GetEngine()->GetSizeOfPrimitiveType(typeId);
	}

	isArrayOfHandles = typeId & asTYPEID_OBJHANDLE ? true : false;

	// Make sure the array size isn't too large for us to handle
	if( !CheckMaxSize(length) )
	{
		// Don't continue with the initialization
		return;
	}

	CreateBuffer(&buffer, length);

	// Notify the GC of the successful creation
	if( objType->GetFlags() & asOBJ_GC )
		objType->GetEngine()->NotifyGarbageCollectorOfNewObject(this, objType->GetTypeId());
}

asCArrayObject::~asCArrayObject()
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}
	if( objType ) objType->Release();
}

asIScriptEngine *asCArrayObject::GetEngine() const
{
	return objType->GetEngine();
}

asUINT asCArrayObject::GetElementCount()
{
	return buffer->numElements;
}

void asCArrayObject::Resize(asUINT numElements)
{
	// Don't do anything if the size is already correct
	if( numElements == buffer->numElements )
		return;

	// Make sure the array size isn't too large for us to handle
	if( !CheckMaxSize(numElements) )
	{
		// Don't resize the array
		return;
	}

	sArrayBuffer *newBuffer;
	int typeId = objType->GetSubTypeId();
	if( typeId & asTYPEID_MASK_OBJECT )
	{
		// Allocate memory for the buffer
		newBuffer = (sArrayBuffer*)asNEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
		newBuffer->numElements = numElements;

		// Copy the elements from the old buffer
		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		asDWORD **d = (asDWORD**)newBuffer->data;
		asDWORD **s = (asDWORD**)buffer->data;
		for( int n = 0; n < c; n++ )
			d[n] = s[n];
		
		if( numElements > buffer->numElements )
		{
			Construct(newBuffer, buffer->numElements, numElements);
		}
		else if( numElements < buffer->numElements )
		{
			Destruct(buffer, numElements, buffer->numElements);
		}
	}
	else
	{
		// Allocate memory for the buffer
		newBuffer = (sArrayBuffer*)asNEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+elementSize*numElements);
		newBuffer->numElements = numElements;

		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		memcpy(newBuffer->data, buffer->data, c*elementSize);
	}

	// Release the old buffer
	userFree(buffer);

	buffer = newBuffer;
}

// internal
bool asCArrayObject::CheckMaxSize(asUINT numElements)
{
	// This code makes sure the size of the buffer that is allocated 
	// for the array doesn't overflow and becomes smaller than requested

	asUINT maxSize = 0xFFFFFFFFul - sizeof(sArrayBuffer) + 1;
	if( objType->GetSubTypeId() & asTYPEID_MASK_OBJECT )
	{
		maxSize /= sizeof(void*);
	}
	else
	{
		maxSize /= elementSize;
	}

	if( numElements > maxSize )
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
		{
			// Set a script exception
			ctx->SetException("Too large array size");
		}

		return false;
	}

	// OK
	return true;
}

int asCArrayObject::GetArrayTypeId()
{
	return objType->GetTypeId();
}

int asCArrayObject::GetElementTypeId()
{
	return objType->GetSubTypeId();
}

void *asCArrayObject::GetElementPointer(asUINT index)
{
	if( index >= buffer->numElements ) return 0;

	int typeId = objType->GetSubTypeId();
	if( (typeId & asTYPEID_MASK_OBJECT) && !isArrayOfHandles )
		return (void*)((size_t*)buffer->data)[index];
	else
		return buffer->data + elementSize*index;
}

void *asCArrayObject::at(asUINT index)
{
	if( index >= buffer->numElements )
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException(TXT_OUT_OF_BOUNDS);
		return 0;
	}
	else
	{
		int typeId = objType->GetSubTypeId();
		if( (typeId & asTYPEID_MASK_OBJECT) && !isArrayOfHandles )
			return (void*)((size_t*)buffer->data)[index];
		else
			return buffer->data + elementSize*index;
	}
}

void asCArrayObject::CreateBuffer(sArrayBuffer **buf, asUINT numElements)
{
	int typeId = objType->GetSubTypeId();
	if( typeId & asTYPEID_MASK_OBJECT )
	{
		*buf = (sArrayBuffer*)asNEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
		(*buf)->numElements = numElements;
	}
	else
	{
		*buf = (sArrayBuffer*)asNEWARRAY(asBYTE, sizeof(sArrayBuffer)-1+elementSize*numElements);
		(*buf)->numElements = numElements;
	}

	Construct(*buf, 0, numElements);
}

void asCArrayObject::DeleteBuffer(sArrayBuffer *buf)
{
	Destruct(buf, 0, buf->numElements);

	// Free the buffer
	asDELETEARRAY(buf);
}

void asCArrayObject::Construct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	int typeId = objType->GetSubTypeId();
	if( isArrayOfHandles )
	{
		// Set all object handles to null
		asDWORD *d = (asDWORD*)(buf->data + start * sizeof(void*));
		memset(d, 0, (end-start)*sizeof(void*));
	}
	else if( typeId & asTYPEID_MASK_OBJECT )
	{
		asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
		asDWORD **d = (asDWORD**)(buf->data + start * sizeof(void*));

		asIScriptEngine *engine = objType->GetEngine();

		for( ; d < max; d++ )
			*d = (asDWORD*)engine->CreateScriptObject(typeId);
	}
}

void asCArrayObject::Destruct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	int typeId = objType->GetSubTypeId();
	if( typeId & asTYPEID_MASK_OBJECT )
	{
		asIScriptEngine *engine = objType->GetEngine();

		asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
		asDWORD **d   = (asDWORD**)(buf->data + start * sizeof(void*));

		for( ; d < max; d++ )
		{
			if( *d )
				engine->ReleaseScriptObject(*d, typeId);
		}
	}
}


void asCArrayObject::CopyBuffer(sArrayBuffer *dst, sArrayBuffer *src)
{
	asIScriptEngine *engine = objType->GetEngine();
	if( isArrayOfHandles )
	{
		// Copy the references and increase the reference counters
		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int typeId = objType->GetSubTypeId();
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;

			asDWORD **max = (asDWORD**)(dst->data + count * sizeof(void*));
			asDWORD **d   = (asDWORD**)dst->data;
			asDWORD **s   = (asDWORD**)src->data;
			
			for( ; d < max; d++, s++ )
			{
				*d = *s;
				if( *d )
					engine->AddRefScriptObject(*d, typeId);
			}
		}
	}
	else
	{
		int typeId = objType->GetSubTypeId();

		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;
			if( typeId & asTYPEID_MASK_OBJECT )
			{
				// Call the assignment operator on all of the objects
				asDWORD **max = (asDWORD**)(dst->data + count * sizeof(void*));
				asDWORD **d   = (asDWORD**)dst->data;
				asDWORD **s   = (asDWORD**)src->data;

				for( ; d < max; d++, s++ )
					engine->CopyScriptObject(*d, *s, typeId);
			}
			else
			{
				// Primitives are copied byte for byte
				memcpy(dst->data, src->data, count*elementSize);
			}
		}
	}
}

void asCArrayObject::Destruct()
{
	// Call destructor and free the memory
	asDELETE(this, asCArrayObject);
}

void asCArrayObject::EnumReferences(asIScriptEngine *engine)
{
	// If the array is holding handles, then we need to notify the GC of them
	int typeId = objType->GetSubTypeId();
	if( typeId & asTYPEID_MASK_OBJECT )
	{
		void **d = (void**)buffer->data;
		for( asUINT n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] )
				engine->GCEnumCallback(d[n]);
		}
	}
}

void asCArrayObject::ReleaseAllHandles(asIScriptEngine *engine)
{
	int subTypeId = objType->GetSubTypeId();
	asIObjectType *subType = engine->GetObjectTypeById(subTypeId);
	if( subType && subType->GetFlags() & asOBJ_GC )
	{
		void **d = (void**)buffer->data;
		for( asUINT n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] )
			{
				engine->ReleaseScriptObject(d[n], subTypeId);
				d[n] = 0;
			}
		}
	}
}


int asCArrayObject::AddRef()
{
	// Clear the GC flag then increase the counter
	gcFlag = false;
	return refCount.atomicInc();
}

int asCArrayObject::Release()
{
	// Now do the actual releasing (clearing the flag set by GC)
	gcFlag = false;
	int r = refCount.atomicDec();
	if( r == 0 )
	{
		Destruct();
		return 0;
	}

	return r;
}

int asCArrayObject::GetRefCount()
{
	return refCount.get();
}

void asCArrayObject::SetFlag()
{
	gcFlag = true;
}

bool asCArrayObject::GetFlag()
{
	return gcFlag;
}

END_AS_NAMESPACE
