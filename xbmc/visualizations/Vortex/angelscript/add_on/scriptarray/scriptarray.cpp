#include <new>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "scriptarray.h"

BEGIN_AS_NAMESPACE

struct SArrayBuffer
{
	asDWORD numElements;
	asBYTE  data[1];
};

static CScriptArray* ScriptArrayFactory2(asIObjectType *ot, asUINT length)
{
	CScriptArray *a = new CScriptArray(length, ot);

	// It's possible the constructor raised a script exception, in which case we 
	// need to free the memory and return null instead, else we get a memory leak.
	asIScriptContext *ctx = asGetActiveContext();
	if( ctx && ctx->GetState() == asEXECUTION_EXCEPTION )
	{
		delete a;
		return 0;
	}

	return a;
}

static CScriptArray* ScriptArrayFactory(asIObjectType *ot)
{
	return ScriptArrayFactory2(ot, 0);
}

// This optional callback is called when the template type is first used by the compiler.
// It allows the application to validate if the template can be instanciated for the requested 
// subtype at compile time, instead of at runtime.
static bool ScriptArrayTemplateCallback(asIObjectType *ot)
{
	// Make sure the subtype can be instanciated with a default factory/constructor, 
	// otherwise we won't be able to instanciate the elements
	int typeId = ot->GetSubTypeId();
	if( (typeId & asTYPEID_MASK_OBJECT) && !(typeId & asTYPEID_OBJHANDLE) )
	{
		asIObjectType *subtype = ot->GetEngine()->GetObjectTypeById(typeId);
		asDWORD flags = subtype->GetFlags();
		if( (flags & asOBJ_VALUE) && !(flags & asOBJ_POD) )
		{
			// Verify that there is a default constructor
			for( int n = 0; n < subtype->GetBehaviourCount(); n++ )
			{
				asEBehaviours beh;
				int funcId = subtype->GetBehaviourByIndex(n, &beh);
				if( beh != asBEHAVE_CONSTRUCT ) continue;

				asIScriptFunction *func = ot->GetEngine()->GetFunctionDescriptorById(funcId);
				if( func->GetParamCount() == 0 )
				{
					// Found the default constructor
					return true;
				}
			}

			// There is no default constructor
			return false;
		}
		else if( (flags & asOBJ_REF) )
		{
			// Verify that there is a default factory
			for( int n = 0; n < subtype->GetFactoryCount(); n++ )
			{
				int funcId = subtype->GetFactoryIdByIndex(n);
				asIScriptFunction *func = ot->GetEngine()->GetFunctionDescriptorById(funcId);
				if( func->GetParamCount() == 0 )
				{
					// Found the default factory
					return true;
				}
			}	

			// No default factory
			return false;
		}
	}

	// The type is ok
	return true;
}

// Registers the template array type
void RegisterScriptArray(asIScriptEngine *engine)
{
	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") == 0 )
		RegisterScriptArray_Native(engine);
	else
		RegisterScriptArray_Generic(engine);
}

void RegisterScriptArray_Native(asIScriptEngine *engine)
{
	int r;
	
	// Register the array type as a template
	r = engine->RegisterObjectType("array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE); assert( r >= 0 );

	// Register a callback for validating the subtype before it is used
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in)", asFUNCTION(ScriptArrayTemplateCallback), asCALL_CDECL); assert( r >= 0 );

	// Templates receive the object type as the first parameter. To the script writer this is hidden
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in)", asFUNCTIONPR(ScriptArrayFactory, (asIObjectType*), CScriptArray*), asCALL_CDECL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint)", asFUNCTIONPR(ScriptArrayFactory2, (asIObjectType*, asUINT), CScriptArray*), asCALL_CDECL); assert( r >= 0 );

	// The memory management methods
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ADDREF, "void f()", asMETHOD(CScriptArray,AddRef), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASE, "void f()", asMETHOD(CScriptArray,Release), asCALL_THISCALL); assert( r >= 0 );

	// The index operator returns the template subtype
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_INDEX, "T &f(uint)", asMETHOD(CScriptArray, At), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_INDEX, "const T &f(uint) const", asMETHOD(CScriptArray, At), asCALL_THISCALL); assert( r >= 0 );
	
	// The assignment operator
	r = engine->RegisterObjectMethod("array<T>", "array<T> &opAssign(const array<T>&in)", asMETHOD(CScriptArray, operator=), asCALL_THISCALL); assert( r >= 0 );

	// Other methods
	r = engine->RegisterObjectMethod("array<T>", "uint length() const", asMETHOD(CScriptArray, GetSize), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void resize(uint)", asMETHOD(CScriptArray, Resize), asCALL_THISCALL); assert( r >= 0 );

	// Register GC behaviours in case the array needs to be garbage collected
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETREFCOUNT, "int f()", asMETHOD(CScriptArray, GetRefCount), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_SETGCFLAG, "void f()", asMETHOD(CScriptArray, SetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETGCFLAG, "bool f()", asMETHOD(CScriptArray, GetFlag), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(CScriptArray, EnumReferences), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(CScriptArray, ReleaseAllHandles), asCALL_THISCALL); assert( r >= 0 );
}

CScriptArray &CScriptArray::operator=(const CScriptArray &other)
{
	// Only perform the copy if the array types are the same
	if( &other != this && 
		other.GetArrayObjectType() == GetArrayObjectType() )
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

CScriptArray::CScriptArray(asUINT length, asIObjectType *ot)
{
	refCount = 1;
	gcFlag = false;
	objType = ot;
	objType->AddRef();
	buffer = 0;

	// Determine element size
	// TODO: Should probably store the template sub type id as well
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

CScriptArray::~CScriptArray()
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}
	if( objType ) objType->Release();
}

asUINT CScriptArray::GetSize()
{
	return buffer->numElements;
}

void CScriptArray::Resize(asUINT numElements)
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

	SArrayBuffer *newBuffer;
	int typeId = objType->GetSubTypeId();
	if( typeId & asTYPEID_MASK_OBJECT )
	{
		// Allocate memory for the buffer
		newBuffer = (SArrayBuffer*)new asBYTE[sizeof(SArrayBuffer)-1+sizeof(void*)*numElements];
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
		newBuffer = (SArrayBuffer*)new asBYTE[sizeof(SArrayBuffer)-1+elementSize*numElements];
		newBuffer->numElements = numElements;

		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		memcpy(newBuffer->data, buffer->data, c*elementSize);
	}

	// Release the old buffer
	delete[] (asBYTE*)buffer;

	buffer = newBuffer;
}

// internal
bool CScriptArray::CheckMaxSize(asUINT numElements)
{
	// This code makes sure the size of the buffer that is allocated 
	// for the array doesn't overflow and becomes smaller than requested

	asUINT maxSize = 0xFFFFFFFFul - sizeof(SArrayBuffer) + 1;
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

asIObjectType *CScriptArray::GetArrayObjectType() const
{
	return objType;
}

int CScriptArray::GetArrayTypeId() const
{
	return objType->GetTypeId();
}

int CScriptArray::GetElementTypeId() const
{
	return objType->GetSubTypeId();
}

// Return a pointer to the array element. Returns 0 if the index is out of bounds
void *CScriptArray::At(asUINT index)
{
	if( index >= buffer->numElements )
	{
		// If this is called from a script we raise a script exception
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException("Index out of bounds");
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

// internal
void CScriptArray::CreateBuffer(SArrayBuffer **buf, asUINT numElements)
{
	int typeId = objType->GetSubTypeId();
	if( typeId & asTYPEID_MASK_OBJECT )
	{
		*buf = (SArrayBuffer*)new asBYTE[sizeof(SArrayBuffer)-1+sizeof(void*)*numElements];
		(*buf)->numElements = numElements;
	}
	else
	{
		*buf = (SArrayBuffer*)new asBYTE[sizeof(SArrayBuffer)-1+elementSize*numElements];
		(*buf)->numElements = numElements;
	}

	Construct(*buf, 0, numElements);
}

// internal
void CScriptArray::DeleteBuffer(SArrayBuffer *buf)
{
	Destruct(buf, 0, buf->numElements);

	// Free the buffer
	delete[] (asBYTE*)buf;
}

// internal
void CScriptArray::Construct(SArrayBuffer *buf, asUINT start, asUINT end)
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

// internal
void CScriptArray::Destruct(SArrayBuffer *buf, asUINT start, asUINT end)
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

// internal
void CScriptArray::CopyBuffer(SArrayBuffer *dst, SArrayBuffer *src)
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

// GC behaviour
void CScriptArray::EnumReferences(asIScriptEngine *engine)
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

// GC behaviour
void CScriptArray::ReleaseAllHandles(asIScriptEngine *engine)
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

void CScriptArray::AddRef()
{
	// Clear the GC flag then increase the counter
	gcFlag = false;
	refCount++;
}

void CScriptArray::Release()
{
	// Now do the actual releasing (clearing the flag set by GC)
	gcFlag = false;
	if( --refCount == 0 )
	{
		delete this;
	}
}

// GC behaviour
int CScriptArray::GetRefCount()
{
	return refCount;
}

// GC behaviour
void CScriptArray::SetFlag()
{
	gcFlag = true;
}

// GC behaviour
bool CScriptArray::GetFlag()
{
	return gcFlag;
}

//--------------------------------------------
// Generic calling conventions

static void ScriptArrayFactory_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);

	*(CScriptArray**)gen->GetAddressOfReturnLocation() = ScriptArrayFactory(ot);
}

static void ScriptArrayFactory2_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);
	asUINT length = gen->GetArgDWord(1);

	*(CScriptArray**)gen->GetAddressOfReturnLocation() = ScriptArrayFactory2(ot, length);
}

static void ScriptArrayTemplateCallback_Generic(asIScriptGeneric *gen)
{
	asIObjectType *ot = *(asIObjectType**)gen->GetAddressOfArg(0);
	*(bool*)gen->GetAddressOfReturnLocation() = ScriptArrayTemplateCallback(ot);
}

static void ScriptArrayAssignment_Generic(asIScriptGeneric *gen)
{
	CScriptArray *other = (CScriptArray*)gen->GetArgObject(0);
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void ScriptArrayAt_Generic(asIScriptGeneric *gen)
{
	asUINT index = gen->GetArgDWord(0);
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	gen->SetReturnAddress(self->At(index));
}

static void ScriptArrayLength_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	gen->SetReturnDWord(self->GetSize());
}

static void ScriptArrayResize_Generic(asIScriptGeneric *gen)
{
	asUINT size = gen->GetArgDWord(0);
	CScriptArray *self = (CScriptArray*)gen->GetObject();

	self->Resize(size);
}

static void ScriptArrayAddRef_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	self->AddRef();
}

static void ScriptArrayRelease_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	self->Release();
}

static void ScriptArrayGetRefCount_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	*(int*)gen->GetAddressOfReturnLocation() = self->GetRefCount();
}

static void ScriptArraySetFlag_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	self->SetFlag();
}

static void ScriptArrayGetFlag_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	*(bool*)gen->GetAddressOfReturnLocation() = self->GetFlag();
}

static void ScriptArrayEnumReferences_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->EnumReferences(engine);
}

static void ScriptArrayReleaseAllHandles_Generic(asIScriptGeneric *gen)
{
	CScriptArray *self = (CScriptArray*)gen->GetObject();
	asIScriptEngine *engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);
	self->ReleaseAllHandles(engine);
}

void RegisterScriptArray_Generic(asIScriptEngine *engine)
{
	int r;
	
	r = engine->RegisterObjectType("array<class T>", 0, asOBJ_REF | asOBJ_GC | asOBJ_TEMPLATE); assert( r >= 0 );

	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in)", asFUNCTION(ScriptArrayFactory_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_TEMPLATE_CALLBACK, "bool f(int&in)", asFUNCTION(ScriptArrayTemplateCallback_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_FACTORY, "array<T>@ f(int&in, uint)", asFUNCTION(ScriptArrayFactory2_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ADDREF, "void f()", asFUNCTION(ScriptArrayAddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASE, "void f()", asFUNCTION(ScriptArrayRelease_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_INDEX, "T &f(uint)", asFUNCTION(ScriptArrayAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_INDEX, "const T &f(uint) const", asFUNCTION(ScriptArrayAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "array<T> &opAssign(const array<T>&in)", asFUNCTION(ScriptArrayAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "uint length() const", asFUNCTION(ScriptArrayLength_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectMethod("array<T>", "void resize(uint)", asFUNCTION(ScriptArrayResize_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETREFCOUNT, "int f()", asFUNCTION(ScriptArrayGetRefCount_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_SETGCFLAG, "void f()", asFUNCTION(ScriptArraySetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_GETGCFLAG, "bool f()", asFUNCTION(ScriptArrayGetFlag_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_ENUMREFS, "void f(int&in)", asFUNCTION(ScriptArrayEnumReferences_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("array<T>", asBEHAVE_RELEASEREFS, "void f(int&in)", asFUNCTION(ScriptArrayReleaseAllHandles_Generic), asCALL_GENERIC); assert( r >= 0 );
}

END_AS_NAMESPACE
