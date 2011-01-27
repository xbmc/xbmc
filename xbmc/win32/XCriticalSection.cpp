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

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

//////////////////////////////////////////////////////////////////////
XCriticalSection::XCriticalSection()
	: m_isDestroyed(false)
	, m_isInitialized(false)
{
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Initialize()
{
	if (m_isDestroyed)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to initialize destroyed section.", (void *)this);
		return;
	}
	
	if (m_isInitialized)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to initialze initialized section.", (void *)this);
		return;
	}
    
    InitializeCriticalSection(&m_criticalSection);    
    m_isInitialized = true;
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Destroy()
{
	if (m_isDestroyed)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to destroy destroyed section.", (void *)this);
		return;
	}
	
	if (m_isInitialized == false)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to destroy uninitialized section.", (void *)this);
		return;
	}
	
	DeleteCriticalSection(&m_criticalSection);
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
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to enter destroyed section.", (void *)this);
		return;
	}
	
	if (m_isInitialized == false)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to enter uninitialized section.", (void *)this);
		return;
	}
	
	EnterCriticalSection(&m_criticalSection);
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Leave()
{
	if (Owning() == false)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p] Some other thread trying to leave our critical section.", (void *)this);
		return;
	}
	
	if (m_isDestroyed)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to leave destroyed section.", (void *)this);
		return;
	}
	
	if (m_isInitialized == false)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to leave initialized section.", (void *)this);
		return;
	}
	
	LeaveCriticalSection(&m_criticalSection);
}

//////////////////////////////////////////////////////////////////////
DWORD XCriticalSection::Exit()
{
	if (m_isDestroyed)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to exit destroyed section.", (void *)this);
		return 0;
	}
	
	if (m_isInitialized == false)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to exit uninitialized section.", (void *)this);
		return 0;
	}
	
	// Only an owning thread can get out of a critical section.
	if (Owning() == false)
	    return 0;

	// As many times as we've entered, leave.
	DWORD count = m_criticalSection.RecursionCount;
    for (DWORD i=0; i<count; i++)
    	Leave();

	return count;
}

//////////////////////////////////////////////////////////////////////
BOOL XCriticalSection::Owning()
{
	return CThread::IsCurrentThread((ThreadIdentifier)m_criticalSection.OwningThread);
}

//////////////////////////////////////////////////////////////////////
void XCriticalSection::Restore(DWORD count)
{
	if (m_isDestroyed)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to restore destroyed section.", (void *)this);
		return;
	}
	
	if (m_isInitialized == false)
	{
		//CLog::Log(LOGWARNING, "CRITSEC[%p]: Trying to restore uninitialized section.", (void *)this);
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
