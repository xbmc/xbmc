/*
 * XBoxMediaPlayer
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
#include "Thread.h"
#include <process.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef _MT
#pragma message( "Please compile using multithreaded run-time libraries" )
#endif
typedef unsigned (WINAPI *PBEGINTHREADEX_THREADFUNC)(LPVOID lpThreadParameter);

CThread::CThread()
{
	m_bStop=false;
	
	m_bAutoDelete = false;
	m_dwThreadId = 0;
	m_ThreadHandle = NULL; 
}

CThread::~CThread()
{
	CloseHandle(m_ThreadHandle);
	m_ThreadHandle=NULL;
}


DWORD WINAPI CThread::staticThread(LPVOID* data)
{
	//DBG"thread start");

	CThread* pThread = (CThread*)(data);
	bool bDelete( pThread->IsAutoDelete() );
	pThread->OnStartup();
	pThread->Process();
	//DBG"thread:: call onexit");
	pThread->OnExit();
	//DBG"thread:: onexit done");
	pThread->m_eventStop.Set();
	if ( bDelete ) 
	{
		//DBG"thread:: delete");
		delete pThread;
		pThread = NULL;
	}
	//DBG"thread:: end thread");
	ExitThread(3);
	return 0;
}

void CThread::Create(bool bAutoDelete)
{
	//DBG"Create thread");
	m_bAutoDelete = bAutoDelete;
	m_eventStop.Reset();	
	m_bStop=false;
	//m_ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (PBEGINTHREADEX_THREADFUNC)staticThread, (void*)this, 0, (unsigned*)&m_dwThreadId);
	m_ThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)staticThread,(LPVOID)this,0,&m_dwThreadId);
}


bool CThread::IsAutoDelete() const 
{ 
  return m_bAutoDelete; 
}

void CThread::StopThread()
{
	m_bStop=true;
	if (m_ThreadHandle)
	{
		m_eventStop.Wait();
	}
	m_ThreadHandle=NULL;
}

unsigned long CThread::ThreadId() const 
{ 
  return m_dwThreadId; 
}


CThread::operator HANDLE() 
{ 
  return m_ThreadHandle; 
}

CThread::operator const HANDLE() const
{ 
  return m_ThreadHandle; 
}

bool CThread::SetPriority(const int iPriority)
// Set thread priority
// Return true for success
{
	if (m_ThreadHandle) 
  {
		return ( SetThreadPriority( m_ThreadHandle, iPriority ) == TRUE );
	}
	else 
  {
		return false;
	}
}

bool CThread::WaitForThreadExit(DWORD dwmsTimeOut)
// Waits for thread to exit, timeout in given number of msec.
// Returns true when thread ended
{
	DWORD	dwExitCode;
	DWORD	dwmsWait=0;

	GetExitCodeThread(m_ThreadHandle,&dwExitCode);
	if (dwExitCode==STILL_ACTIVE) 
  {
		// Wait for thread to end
		do 
    {
			Sleep(100);
			dwmsWait+=100;
			GetExitCodeThread(m_ThreadHandle,&dwExitCode);
		} while (dwExitCode==STILL_ACTIVE && dwmsWait<dwmsTimeOut);
		return (dwExitCode!=STILL_ACTIVE);
	}
	else 
  {
		// Thread has already ended
		return true;
	}
}

HANDLE   CThread::ThreadHandle()
{
	return m_ThreadHandle;
}