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




#include "as_config.h"
#include "as_property.h"

BEGIN_AS_NAMESPACE

asCGlobalProperty::asCGlobalProperty() 
{ 
	memory = 0; 
	memoryAllocated = false; 
	realAddress = 0; 
	initFunc = 0; 

	refCount.set(1);
}

asCGlobalProperty::~asCGlobalProperty()
{ 
	if( memoryAllocated ) { asDELETEARRAY(memory); } 
	if( initFunc )
		initFunc->Release();
}

void asCGlobalProperty::AddRef()
{
	refCount.atomicInc();
}

void asCGlobalProperty::Release()
{
	// The property doesn't delete itself. The  
	// engine will do that at a later time
	if( refCount.atomicDec() == 1 && initFunc )
	{
		// Since the initFunc holds a reference to the property,
		// we'll release it when we reach refCount 1. This will
		// break the circle and allow the engine to free the property
		// without the need for a GC run.
		initFunc->Release();
		initFunc = 0;
	}
}

void *asCGlobalProperty::GetAddressOfValue()
{ 
	return (memoryAllocated || realAddress) ? memory : &storage; 
}

// The global property structure is responsible for allocating the storage
// method for script declared variables. Each allocation is independent of
// other global properties, so that variables can be added and removed at
// any time.
void asCGlobalProperty::AllocateMemory() 
{ 
	if( type.GetSizeOnStackDWords() > 2 ) 
	{ 
		memory = asNEWARRAY(asDWORD, type.GetSizeOnStackDWords()); 
		memoryAllocated = true; 
	} 
}

void asCGlobalProperty::SetRegisteredAddress(void *p) 
{ 
	realAddress = p; 	
	if( type.IsObject() && !type.IsReference() && !type.IsObjectHandle() )
	{
		// The global property is a pointer to a pointer 
		memory = &realAddress;
	} 
	else
		memory = p; 
}


END_AS_NAMESPACE
