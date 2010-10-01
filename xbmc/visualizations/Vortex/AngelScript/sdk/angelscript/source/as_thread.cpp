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
// as_thread.cpp
//
// Functions for multi threading support
//

#include <assert.h>

#include "as_config.h"
#include "as_thread.h"

BEGIN_AS_NAMESPACE

#ifdef USE_THREADS
// From windows.h
extern "C"
{
	void __stdcall InitializeCriticalSection(CRITICAL_SECTION *);
	void __stdcall DeleteCriticalSection(CRITICAL_SECTION *);
	void __stdcall EnterCriticalSection(CRITICAL_SECTION *);
	void __stdcall LeaveCriticalSection(CRITICAL_SECTION *);
	unsigned long __stdcall GetCurrentThreadId();
}
#endif

// Singleton
asCThreadManager threadManager;

//======================================================================

AS_API int asThreadCleanup()
{
#ifdef USE_THREADS
	return threadManager.CleanupLocalData();
#else
	return asERROR;
#endif
}

//======================================================================

asCThreadManager::asCThreadManager()
{
#ifndef USE_THREADS
	tld = 0;
#endif
}

asCThreadManager::~asCThreadManager()
{
#ifdef USE_THREADS
	ENTERCRITICALSECTION(criticalSection);

	// Delete all thread local datas
	if( tldMap.MoveFirst() )
	{
		do
		{
			if( tldMap.GetValue() ) 
				delete tldMap.GetValue();
		} while( tldMap.MoveNext() );
	}

	LEAVECRITICALSECTION(criticalSection);
#else
	if( tld ) delete tld;
	tld = 0;
#endif
}

#ifdef USE_THREADS
int asCThreadManager::CleanupLocalData()
{
	asDWORD id = GetCurrentThreadId();
	int r = 0;

	ENTERCRITICALSECTION(criticalSection);

	if( tldMap.MoveTo(id) )
	{
		asCThreadLocalData *tld = tldMap.GetValue();
		
		// Can we really remove it at this time?
		if( tld->activeContexts.GetLength() == 0 )
		{
			delete tld;
			tldMap.Erase(true);
			r = 0;
		}
		else
			r = asCONTEXT_ACTIVE;
	}

	LEAVECRITICALSECTION(criticalSection);

	return r;
}

asCThreadLocalData *asCThreadManager::GetLocalData(asDWORD threadId)
{
	asCThreadLocalData *tld = 0;

	ENTERCRITICALSECTION(criticalSection);

	if( tldMap.MoveTo(threadId) )
		tld = tldMap.GetValue();

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
#ifdef USE_THREADS
	asDWORD id = GetCurrentThreadId();
		
	asCThreadLocalData *tld = GetLocalData(id);
	if( tld == 0 )
	{
		// Create a new tld
		tld = new asCThreadLocalData();
		SetLocalData(id, tld);
	}

	return tld;
#else
	if( tld == 0 )
		tld = new asCThreadLocalData();

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

#ifdef USE_THREADS
asCThreadCriticalSection::asCThreadCriticalSection()
{
	InitializeCriticalSection(&criticalSection);
}

asCThreadCriticalSection::~asCThreadCriticalSection()
{
	DeleteCriticalSection(&criticalSection);
}

void asCThreadCriticalSection::Enter()
{
	EnterCriticalSection(&criticalSection);
}

void asCThreadCriticalSection::Leave()
{
	LeaveCriticalSection(&criticalSection);
}
#endif

//========================================================================

END_AS_NAMESPACE

