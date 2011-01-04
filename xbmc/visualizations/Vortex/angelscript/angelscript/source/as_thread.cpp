/*
   AngelCode Scripting Library
   Copyright (c) 2003-2008 Andreas Jonsson

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
// as_thread.cpp
//
// Functions for multi threading support
//

#include "as_config.h"
#include "as_thread.h"
#include "as_atomic.h"

BEGIN_AS_NAMESPACE

// Singleton
asCThreadManager *threadManager = 0;

//======================================================================

extern "C"
{

AS_API int asThreadCleanup()
{
	// As this function can be called globally, 
	// we can't assume the threadManager exists
	if( threadManager )
		return threadManager->CleanupLocalData();

	return 0;
}

}

//======================================================================

asCThreadManager::asCThreadManager()
{
#ifdef AS_NO_THREADS
	tld = 0;
#endif
	refCount.set(1);
}

void asCThreadManager::AddRef()
{
	refCount.atomicInc();
}

void asCThreadManager::Release()
{
	if( refCount.atomicDec() == 0 )
	{
		// The last engine has been destroyed, so we 
		// need to delete the thread manager as well
		asDELETE(this,asCThreadManager);
		threadManager = 0;
	}
}

asCThreadManager::~asCThreadManager()
{
#ifndef AS_NO_THREADS
	ENTERCRITICALSECTION(criticalSection);

	// Delete all thread local datas
	asSMapNode<asDWORD,asCThreadLocalData*> *cursor = 0;
	if( tldMap.MoveFirst(&cursor) )
	{
		do
		{
			if( tldMap.GetValue(cursor) ) 
			{
				asDELETE(tldMap.GetValue(cursor),asCThreadLocalData);
			}
		} while( tldMap.MoveNext(&cursor, cursor) );
	}

	LEAVECRITICALSECTION(criticalSection);
#else
	if( tld ) 
	{
		asDELETE(tld,asCThreadLocalData);
	}
	tld = 0;
#endif
}

int asCThreadManager::CleanupLocalData()
{
#ifndef AS_NO_THREADS
	int r = 0;
#if defined AS_POSIX_THREADS
	asDWORD id = (asDWORD)pthread_self();
#elif defined AS_WINDOWS_THREADS
	asDWORD id = GetCurrentThreadId();
#endif

	ENTERCRITICALSECTION(criticalSection);

	asSMapNode<asDWORD,asCThreadLocalData*> *cursor = 0;
	if( tldMap.MoveTo(&cursor, id) )
	{
		asCThreadLocalData *tld = tldMap.GetValue(cursor);
		
		// Can we really remove it at this time?
		if( tld->activeContexts.GetLength() == 0 )
		{
			asDELETE(tld,asCThreadLocalData);
			tldMap.Erase(cursor);
			r = 0;
		}
		else
			r = asCONTEXT_ACTIVE;
	}

	LEAVECRITICALSECTION(criticalSection);

	return r;
#else
	if( tld )
	{
		if( tld->activeContexts.GetLength() == 0 )
		{
			asDELETE(tld,asCThreadLocalData);
			tld = 0;
		}
		else
			return asCONTEXT_ACTIVE;
	}
	return 0;
#endif
}

#ifndef AS_NO_THREADS
asCThreadLocalData *asCThreadManager::GetLocalData(asDWORD threadId)
{
	asCThreadLocalData *tld = 0;

	ENTERCRITICALSECTION(criticalSection);

	asSMapNode<asDWORD,asCThreadLocalData*> *cursor = 0;
	if( tldMap.MoveTo(&cursor, threadId) )
		tld = tldMap.GetValue(cursor);

	LEAVECRITICALSECTION(criticalSection);

	return tld;
}

void asCThreadManager::SetLocalData(asDWORD threadId, asCThreadLocalData *tld)
{
	ENTERCRITICALSECTION(criticalSection);

	tldMap.Insert(threadId, tld);

	LEAVECRITICALSECTION(criticalSection);
}
#endif

asCThreadLocalData *asCThreadManager::GetLocalData()
{
#ifndef AS_NO_THREADS
#if defined AS_POSIX_THREADS
	asDWORD id = (asDWORD)pthread_self();
#elif defined AS_WINDOWS_THREADS
	asDWORD id = GetCurrentThreadId();
#endif
		
	asCThreadLocalData *tld = GetLocalData(id);
	if( tld == 0 )
	{
		// Create a new tld
		tld = asNEW(asCThreadLocalData)();
		SetLocalData(id, tld);
	}

	return tld;
#else
	if( tld == 0 )
		tld = asNEW(asCThreadLocalData)();

	return tld;
#endif
}

//=========================================================================

asCThreadLocalData::asCThreadLocalData()
{
}

asCThreadLocalData::~asCThreadLocalData()
{
}

//=========================================================================

#ifndef AS_NO_THREADS
asCThreadCriticalSection::asCThreadCriticalSection()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_init(&criticalSection, 0);
#elif defined AS_WINDOWS_THREADS
	InitializeCriticalSection(&criticalSection);
#endif
}

asCThreadCriticalSection::~asCThreadCriticalSection()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_destroy(&criticalSection);
#elif defined AS_WINDOWS_THREADS
	DeleteCriticalSection(&criticalSection);
#endif
}

void asCThreadCriticalSection::Enter()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_lock(&criticalSection);
#elif defined AS_WINDOWS_THREADS
	EnterCriticalSection(&criticalSection);
#endif
}

void asCThreadCriticalSection::Leave()
{
#if defined AS_POSIX_THREADS
	pthread_mutex_unlock(&criticalSection);
#elif defined AS_WINDOWS_THREADS
	LeaveCriticalSection(&criticalSection);
#endif
}
#endif

//========================================================================

END_AS_NAMESPACE

