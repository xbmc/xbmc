/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "system.h"
#include "CriticalSection.h"
#include "utils/log.h"
#include "Thread.h"
#define SAFELY(expr)                                   \
{                                                      \
	int err = 0;                                         \
	if ((err=expr) != 0)                                 \
	    { CLog::Log(LOGERROR, "(%s): [%s:%d] %d", #expr, __FILE__, __LINE__, err); } \
}

//////////////////////////////////////////////////////////////////////
XCriticalSection::XCriticalSection()
	: m_ownerThread(0)
	, m_count(0)
	, m_isDestroyed(false)
	, m_isInitialized(false)
{
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Initialize()
{
	if (m_isDestroyed)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to initialize destroyed section.", (void *)this);
		return;
	}
	
	if (m_isInitialized)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to initialze initialized section.", (void *)this);
		return;
	}
	
	// Setup for recursive locks.
	pthread_mutexattr_t attr;
	SAFELY(pthread_mutexattr_init(&attr));
	SAFELY(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE));

	// Create the mutexes
	SAFELY(pthread_mutex_init(&m_mutex, &attr));
	SAFELY(pthread_mutex_init(&m_countMutex, &attr));
	
	SAFELY(pthread_mutexattr_destroy(&attr));
	m_isInitialized = true;
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Destroy()
{
	if (m_isDestroyed)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to destroy destroyed section.", (void *)this);
		return;
	}
	
	if (!m_isInitialized)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to destroy uninitialized section.", (void *)this);
		return;
	}
	
	SAFELY(pthread_mutex_destroy(&m_mutex));
	SAFELY(pthread_mutex_destroy(&m_countMutex));
	m_isDestroyed = true;
}

//////////////////////////////////////////////////////////////////////
XCriticalSection::~XCriticalSection()
{
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Enter()
{	
	if (m_isDestroyed)
	{
#ifdef _DEBUG
		// This has to be a printf, otherwise we may recurse.
		printf("CRITSEC[%p]: Trying to enter destroyed section.\n", (void *)this);
#endif
		return;
	}
	
	if (!m_isInitialized)
	{
#ifdef _DEBUG
		// This has to be a printf, otherwise we may recurse.
		printf("CRITSEC[%p]: Trying to enter uninitialized section.\n", (void *)this);
#endif
		return;
	}
	
	// Lock the mutex, bump the count.
	SAFELY(pthread_mutex_lock(&m_mutex));
	
	pthread_mutex_lock(&m_countMutex);
	
	// Save the owner, bump the count.
	m_count++;
	m_ownerThread = CThread::GetCurrentThreadId();
	
	pthread_mutex_unlock(&m_countMutex);
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Leave()
{	
	if (m_isDestroyed)
	{
#ifdef _DEBUG
		printf("CRITSEC[%p]: Trying to leave destroyed section.\n", (void *)this);
#endif
		return;
	}
	
	if (!m_isInitialized)
	{
#ifdef _DEBUG
		printf("CRITSEC[%p]: Trying to leave uninitialized section.\n", (void *)this);
#endif
		return;
	}

	if (!Owning())
	{
#ifdef _DEBUG
		printf("CRITSEC[%p]: Some other thread trying to leave our critical section.\n", (void *)this);
#endif
		return;
	}
	
	pthread_mutex_lock(&m_countMutex);
	
	// Decrease the count, unlock mutex.
	if (m_count > 0)
	{
		m_count--;
		if(m_count == 0)
			m_ownerThread = 0;
		SAFELY(pthread_mutex_unlock(&m_mutex));
	}
	else
	{
#ifdef _DEBUG
		printf("RITSEC[%p]: Trying to leave, already left.\n", (void *)this);
#endif
	}
	
	pthread_mutex_unlock(&m_countMutex);
}

//////////////////////////////////////////////////////////////////////
DWORD XCriticalSection::Exit()
{
	if (m_isDestroyed)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to exit destroyed section.", (void *)this);
		return 0;
	}
	
	if (!m_isInitialized)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to exit uninitialized section.", (void *)this);
		return 0;
	}
	
	// Only an owning thread can get out of a critical section.
	if (!Owning())
	    return 0;

	pthread_mutex_lock(&m_countMutex);
	
	// As many times as we've entered, leave.
    DWORD count = m_count;
    for (DWORD i=0; i<count; i++)
    	Leave();

    pthread_mutex_unlock(&m_countMutex);

	return count;
}

//////////////////////////////////////////////////////////////////////
BOOL XCriticalSection::Owning()
{
	return CThread::IsCurrentThread(m_ownerThread);
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Restore(DWORD count)
{
	if (m_isDestroyed)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to restore destroyed section.", (void *)this);
		return;
	}
	
	if (!m_isInitialized)
	{
		CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to restore uninitialized section.", (void *)this);
		return;
	}

	// Restore the specified count.
	for (DWORD i=0; i<count; i++)
		Enter();	
}

// The C API.
void  InitializeCriticalSection(XCriticalSection* section)             { section->Initialize(); }
void  DeleteCriticalSection(XCriticalSection* section)                 { section->Destroy(); }
BOOL  OwningCriticalSection(XCriticalSection* section)                 { return section->Owning(); }
DWORD ExitCriticalSection(XCriticalSection* section)                   { return section->Exit(); }
void  RestoreCriticalSection(XCriticalSection* section, DWORD count)   { return section->Restore(count); }
void  EnterCriticalSection(XCriticalSection* section)                  { section->Enter(); }
void  LeaveCriticalSection(XCriticalSection* section)                  { section->Leave(); }

