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


//
// as_gc.h
//
// The garbage collector is used to resolve cyclic references
//



#ifndef AS_GC_H
#define AS_GC_H

#include "as_config.h"
#include "as_array.h"
#include "as_map.h"
#include "as_thread.h"

BEGIN_AS_NAMESPACE

class asCScriptEngine;
class asCObjectType;

class asCGarbageCollector
{
public:
	asCGarbageCollector();

	int  GarbageCollect(asDWORD flags);
	void GetStatistics(asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected);
	void GCEnumCallback(void *reference);
	void AddScriptObjectToGC(void *obj, asCObjectType *objType);

	asCScriptEngine *engine;

protected:
	struct asSObjTypePair {void *obj; asCObjectType *type;};
	struct asSIntTypePair {int i; asCObjectType *type;};

	enum egcDestroyState
	{
		destroyGarbage_init = 0,
		destroyGarbage_loop,
		destroyGarbage_haveMore
	};

	enum egcDetectState
	{
		clearCounters_init = 0,
		clearCounters_loop,
		countReferences_init,
		countReferences_loop,
		detectGarbage_init,
		detectGarbage_loop1,
		detectGarbage_loop2,
		verifyUnmarked,
		breakCircles_init,
		breakCircles_loop,
		breakCircles_haveGarbage
	};

	int            DestroyGarbage();
	int            IdentifyGarbageWithCyclicRefs();
	void           ClearMap();
	asSObjTypePair GetObjectAtIdx(int idx);
	void           RemoveObjectAtIdx(int idx);

	// Holds all the objects known by the garbage collector
	asCArray<asSObjTypePair>           gcObjects;

	// This array temporarily holds references to objects known to be live objects
	asCArray<void*>                    liveObjects;

	// This map holds objects currently being searched for cyclic references, it also holds a 
	// counter that gives the number of references to the object that the GC can't reach
	asCMap<void*, asSIntTypePair>      gcMap;

	// State variables
	egcDestroyState                    destroyState;
	asUINT                             destroyIdx;
	asUINT                             numDestroyed;
	egcDetectState                     detectState;
	asUINT                             detectIdx;
	asUINT                             numDetected;
	asSMapNode<void*, asSIntTypePair> *gcMapCursor;

	// Critical section for multithreaded access
	DECLARECRITICALSECTION(gcCritical);
};

END_AS_NAMESPACE

#endif
