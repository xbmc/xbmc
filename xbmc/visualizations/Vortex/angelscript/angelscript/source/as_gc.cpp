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
// as_gc.cpp
//
// The implementation of the garbage collector
//


#include <stdlib.h>

#include "as_gc.h"
#include "as_scriptengine.h"
#include "as_scriptobject.h"

BEGIN_AS_NAMESPACE

asCGarbageCollector::asCGarbageCollector()
{
	engine       = 0;
	detectState  = clearCounters_init;
	destroyState = destroyGarbage_init;
	numDestroyed = 0;
	numDetected  = 0;
}

void asCGarbageCollector::AddScriptObjectToGC(void *obj, asCObjectType *objType)
{
	engine->CallObjectMethod(obj, objType->beh.addref);
	asSObjTypePair ot = {obj, objType};

	// Add the data to the gcObjects array in a critical section as
	// another thread might be calling this method at the same time
	ENTERCRITICALSECTION(gcCritical);
	gcObjects.PushLast(ot);
	LEAVECRITICALSECTION(gcCritical);
}

int asCGarbageCollector::GarbageCollect(asDWORD flags)
{
	// The application is responsible for making sure
	// the gc is only executed by one thread at a time.

	bool doDetect  = (flags & asGC_DETECT_GARBAGE)  || !(flags & asGC_DESTROY_GARBAGE);
	bool doDestroy = (flags & asGC_DESTROY_GARBAGE) || !(flags & asGC_DETECT_GARBAGE);

	if( flags & asGC_FULL_CYCLE )
	{
		// Reset the state
		if( doDetect )
			detectState  = clearCounters_init;
		if( doDestroy )
			destroyState = destroyGarbage_init;

		int r = 1;
		unsigned int count = (unsigned int)gcObjects.GetLength();
		for(;;)
		{
			// Detect all garbage with cyclic references
			if( doDetect )
				while( (r = IdentifyGarbageWithCyclicRefs()) == 1 );

			// Now destroy all known garbage
			if( doDestroy )
				while( (r = DestroyGarbage()) == 1 );

			// Run another iteration if any garbage was destroyed
			if( count != gcObjects.GetLength() )
				count = (unsigned int)gcObjects.GetLength();
			else
				break;
		}

		// Take the opportunity to clear unused types as well
		engine->ClearUnusedTypes();

		return 0;
	}
	else
	{
		// Destroy the garbage that we know of
		if( doDestroy )
			DestroyGarbage();

		// Run another incremental step of the identification of cyclic references
		if( doDetect )
			IdentifyGarbageWithCyclicRefs();
	}

	// Return 1 to indicate that the cycle wasn't finished
	return 1;
}

void asCGarbageCollector::GetStatistics(asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected)
{
	// It's not necessary to protect this access, as
	// it doesn't matter if another thread is currently
	// appending a new object.
	if( currentSize )
		*currentSize = (asUINT)gcObjects.GetLength();

	if( totalDestroyed )
		*totalDestroyed = numDestroyed;

	if( totalDetected )
		*totalDetected = numDetected;
}

void asCGarbageCollector::ClearMap()
{
	// Decrease reference counter for all objects removed from the map
	asSMapNode<void*, asSIntTypePair> *cursor = 0;
	gcMap.MoveFirst(&cursor);
	while( cursor )
	{
		void *obj = gcMap.GetKey(cursor);
		asSIntTypePair it = gcMap.GetValue(cursor);

		engine->CallObjectMethod(obj, it.type->beh.release);

		gcMap.MoveNext(&cursor, cursor);
	}

	gcMap.EraseAll();
}

asCGarbageCollector::asSObjTypePair asCGarbageCollector::GetObjectAtIdx(int idx)
{
	// We need to protect this access with a critical section as
	// another thread might be appending an object at the same time
	ENTERCRITICALSECTION(gcCritical);
	asSObjTypePair gcObj = gcObjects[idx];
	LEAVECRITICALSECTION(gcCritical);

	return gcObj;
}

void asCGarbageCollector::RemoveObjectAtIdx(int idx)
{
	// We need to protect this update with a critical section as
	// another thread might be appending an object at the same time
	ENTERCRITICALSECTION(gcCritical);
	if( idx == (int)gcObjects.GetLength() - 1)
		gcObjects.PopLast();
	else
		gcObjects[idx] = gcObjects.PopLast();
	LEAVECRITICALSECTION(gcCritical);
}

int asCGarbageCollector::DestroyGarbage()
{
	for(;;)
	{
		switch( destroyState )
		{
		case destroyGarbage_init:
		{
			// If there are no objects to be freed then don't start
			if( gcObjects.GetLength() == 0 )
				return 0;

			destroyIdx = (asUINT)-1;
			destroyState = destroyGarbage_loop;
		}
		break;

		case destroyGarbage_loop:
		case destroyGarbage_haveMore:
		{
			// If the refCount has reached 1, then only the GC still holds a
			// reference to the object, thus we don't need to worry about the
			// application touching the objects during collection.

			// Destroy all objects that have refCount == 1. If any objects are
			// destroyed, go over the list again, because it may have made more
			// objects reach refCount == 1.
			while( ++destroyIdx < gcObjects.GetLength() )
			{
				asSObjTypePair gcObj = GetObjectAtIdx(destroyIdx);
				if( engine->CallObjectMethodRetInt(gcObj.obj, gcObj.type->beh.gcGetRefCount) == 1 )
				{
					// Release the object immediately

					// Make sure the refCount is really 0, because the
					// destructor may have increased the refCount again.
					bool addRef = false;
					if( gcObj.type->flags & asOBJ_SCRIPT_OBJECT )
					{
						// Script objects may actually be resurrected in the destructor
						int refCount = ((asCScriptObject*)gcObj.obj)->Release();
						if( refCount > 0 ) addRef = true;
					}
					else
						engine->CallObjectMethod(gcObj.obj, gcObj.type->beh.release);

					// Was the object really destroyed?
					if( !addRef )
					{
						numDestroyed++;
						RemoveObjectAtIdx(destroyIdx);
						destroyIdx--;
					}
					else
					{
						// Since the object was resurrected in the
						// destructor, we must add our reference again
						engine->CallObjectMethod(gcObj.obj, gcObj.type->beh.addref);
					}

					destroyState = destroyGarbage_haveMore;

					// Allow the application to work a little
					return 1;
				}
			}

			// Only move to the next step if no garbage was detected in this step
			if( destroyState == destroyGarbage_haveMore )
				destroyState = destroyGarbage_init;
			else
				return 0;
		}
		break;
		}
	}

	// Shouldn't reach this point
	UNREACHABLE_RETURN;
}

int asCGarbageCollector::IdentifyGarbageWithCyclicRefs()
{
	for(;;)
	{
		switch( detectState )
		{
		case clearCounters_init:
		{
			ClearMap();
			detectState = clearCounters_loop;
			detectIdx = 0;
		}
		break;

		case clearCounters_loop:
		{
			// Build a map of objects that will be checked, the map will
			// hold the object pointer as key, and the gcCount and the
			// object's type as value. As objects are added to the map the
			// gcFlag must be set in the objects, so we can be verify if
			// the object is accessed during the GC cycle.

			// If an object is removed from the gcObjects list during the
			// iteration of this step, it is possible that an object won't
			// be used during the analyzing for cyclic references. This
			// isn't a problem, as the next time the GC cycle starts the
			// object will be verified.
			while( detectIdx < gcObjects.GetLength() )
			{
				// Add the gc count for this object
				asSObjTypePair gcObj = GetObjectAtIdx(detectIdx);
				int refCount = engine->CallObjectMethodRetInt(gcObj.obj, gcObj.type->beh.gcGetRefCount);
				if( refCount > 1 )
				{
					asSIntTypePair it = {refCount-1, gcObj.type};
					gcMap.Insert(gcObj.obj, it);

					// Increment the object's reference counter when putting it in the map
					engine->CallObjectMethod(gcObj.obj, gcObj.type->beh.addref);

					// Mark the object so that we can
					// see if it has changed since read
					engine->CallObjectMethod(gcObj.obj, gcObj.type->beh.gcSetFlag);

					detectIdx++;

					// Let the application work a little
					return 1;
				}
				else
					detectIdx++;
			}

			detectState = countReferences_init;
		}
		break;

		case countReferences_init:
		{
			detectIdx = (asUINT)-1;
			gcMap.MoveFirst(&gcMapCursor);
			detectState = countReferences_loop;
		}
		break;

		case countReferences_loop:
		{
			// Call EnumReferences on all objects in the map to count the number
			// of references reachable from between objects in the map. If all
			// references for an object in the map is reachable from other objects
			// in the map, then we know that no outside references are held for
			// this object, thus it is a potential dead object in a circular reference.

			// If the gcFlag is cleared for an object we consider the object alive
			// and referenced from outside the GC, thus we don't enumerate its references.

			// Any new objects created after this step in the GC cycle won't be
			// in the map, and is thus automatically considered alive.
			while( gcMapCursor )
			{
				void *obj = gcMap.GetKey(gcMapCursor);
				asCObjectType *type = gcMap.GetValue(gcMapCursor).type;
				gcMap.MoveNext(&gcMapCursor, gcMapCursor);

				if( engine->CallObjectMethodRetBool(obj, type->beh.gcGetFlag) )
				{
					engine->CallObjectMethod(obj, engine, type->beh.gcEnumReferences);

					// Allow the application to work a little
					return 1;
				}
			}

			detectState = detectGarbage_init;
		}
		break;

		case detectGarbage_init:
		{
			detectIdx = (asUINT)-1;
			gcMap.MoveFirst(&gcMapCursor);
			liveObjects.SetLength(0);
			detectState = detectGarbage_loop1;
		}
		break;

		case detectGarbage_loop1:
		{
			// All objects that are known not to be dead must be removed from the map,
			// along with all objects they reference. What remains in the map after
			// this pass is sure to be dead objects in circular references.

			// An object is considered alive if its gcFlag is cleared, or all the
			// references were not found in the map.

			// Add all alive objects from the map to the liveObjects array
			while( gcMapCursor )
			{
				asSMapNode<void*, asSIntTypePair> *cursor = gcMapCursor;
				gcMap.MoveNext(&gcMapCursor, gcMapCursor);

				void *obj = gcMap.GetKey(cursor);
				asSIntTypePair it = gcMap.GetValue(cursor);

				bool gcFlag = engine->CallObjectMethodRetBool(obj, it.type->beh.gcGetFlag);
				if( !gcFlag || it.i > 0 )
				{
					liveObjects.PushLast(obj);

					// Allow the application to work a little
					return 1;
				}
			}

			detectState = detectGarbage_loop2;
		}
		break;

		case detectGarbage_loop2:
		{
			// In this step we are actually removing the alive objects from the map.
			// As the object is removed, all the objects it references are added to the
			// liveObjects list, by calling EnumReferences. Only objects still in the map
			// will be added to the liveObjects list.
			while( liveObjects.GetLength() )
			{
				void *gcObj = liveObjects.PopLast();
				asCObjectType *type = 0;

				// Remove the object from the map to mark it as alive
				asSMapNode<void*, asSIntTypePair> *cursor = 0;
				if( gcMap.MoveTo(&cursor, gcObj) )
				{
					type = gcMap.GetValue(cursor).type;
					gcMap.Erase(cursor);

					// We need to decrease the reference count again as we remove the object from the map
					engine->CallObjectMethod(gcObj, type->beh.release);

					// Enumerate all the object's references so that they too can be marked as alive
					engine->CallObjectMethod(gcObj, engine, type->beh.gcEnumReferences);
				}

				// Allow the application to work a little
				return 1;
			}

			detectState = verifyUnmarked;
		}
		break;

		case verifyUnmarked:
		{
			// In this step we must make sure that none of the objects still in the map
			// has been touched by the application. If they have then we must run the
			// detectGarbage loop once more.
			gcMap.MoveFirst(&gcMapCursor);
			while( gcMapCursor )
			{
				void *gcObj = gcMap.GetKey(gcMapCursor);
				asCObjectType *type = gcMap.GetValue(gcMapCursor).type;

				bool gcFlag = engine->CallObjectMethodRetBool(gcObj, type->beh.gcGetFlag);
				if( !gcFlag )
				{
					// The unmarked object was touched, rerun the detectGarbage loop
					detectState = detectGarbage_init;
					return 1;
				}

				gcMap.MoveNext(&gcMapCursor, gcMapCursor);
			}

			// No unmarked object was touched, we can now be sure
			// that objects that have gcCount == 0 really is garbage
			detectState = breakCircles_init;
		}
		break;

		case breakCircles_init:
		{
			detectIdx = (asUINT)-1;
			gcMap.MoveFirst(&gcMapCursor);
			detectState = breakCircles_loop;
		}
		break;

		case breakCircles_loop:
		case breakCircles_haveGarbage:
		{
			// All objects in the map are now known to be dead objects
			// kept alive through circular references. To be able to free
			// these objects we need to force the breaking of the circle
			// by having the objects release their references.
			while( gcMapCursor )
			{
				numDetected++;
				void *gcObj = gcMap.GetKey(gcMapCursor);
				asCObjectType *type = gcMap.GetValue(gcMapCursor).type;
				engine->CallObjectMethod(gcObj, engine, type->beh.gcReleaseAllReferences);

				gcMap.MoveNext(&gcMapCursor, gcMapCursor);

				detectState = breakCircles_haveGarbage;

				// Allow the application to work a little
				return 1;
			}

			// If no garbage was detected we can finish now
			if( detectState != breakCircles_haveGarbage )
			{
				// Restart the GC
				detectState = clearCounters_init;
				return 0;
			}
			else
			{
				// Restart the GC
				detectState = clearCounters_init;
				return 1;
			}
		}
		break;
		} // switch
	}

	// Shouldn't reach this point
	UNREACHABLE_RETURN;
}

void asCGarbageCollector::GCEnumCallback(void *reference)
{
	if( detectState == countReferences_loop )
	{
		// Find the reference in the map
		asSMapNode<void*, asSIntTypePair> *cursor = 0;
		if( gcMap.MoveTo(&cursor, reference) )
		{
			// Decrease the counter in the map for the reference
			gcMap.GetValue(cursor).i--;
		}
	}
	else if( detectState == detectGarbage_loop2 )
	{
		// Find the reference in the map
		asSMapNode<void*, asSIntTypePair> *cursor = 0;
		if( gcMap.MoveTo(&cursor, reference) )
		{
			// Add the object to the list of objects to mark as alive
			liveObjects.PushLast(reference);
		}
	}
}

END_AS_NAMESPACE

