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
	if (m_ThreadHandle!=NULL)
	{
		CloseHandle(m_ThreadHandle);
	}
	m_ThreadHandle=NULL;
}


DWORD WINAPI CThread::staticThread(LPVOID* data)
{
	//DBG"thread start");

	CThread* pThread = (CThread*)(data);
	bool bDelete( pThread->IsAutoDelete() );
	pThread->OnStartup();
	pThread->Process();
	pThread->OnExit();
	pThread->m_eventStop.Set();
	if ( bDelete ) 
	{
		delete pThread;
		pThread = NULL;
	}
	_endthreadex(123);
	return 0;
}

void CThread::Create(bool bAutoDelete)
{
	if (m_ThreadHandle!=NULL)
	{
		throw 1;//ERROR should not b possible!!!
	}
	m_bAutoDelete = bAutoDelete;
	m_eventStop.Reset();	
	m_bStop=false;
	m_ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (PBEGINTHREADEX_THREADFUNC)staticThread, (void*)this, 0, (unsigned*)&m_dwThreadId);
	//m_ThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)staticThread,(LPVOID)this,0,&m_dwThreadId);
}


bool CThread::IsAutoDelete() const 
{ 
  return m_bAutoDelete; 
}

void CThread::StopThread()
{
	m_bStop=true;
	if(m_ThreadHandle)
	{
		WaitForSingleObject(m_ThreadHandle,INFINITE);
		CloseHandle(m_ThreadHandle);
		m_ThreadHandle=NULL;
	}
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

  if (!m_ThreadHandle) return true;
  DWORD dwExitCode;
  WaitForSingleObject(m_ThreadHandle,dwmsTimeOut);

	GetExitCodeThread(m_ThreadHandle,&dwExitCode);
  if (dwExitCode!=STILL_ACTIVE)
  {
    CloseHandle(m_ThreadHandle);
    m_ThreadHandle=NULL;
    return true;
  }
  return false;
}

HANDLE   CThread::ThreadHandle()
{
	return m_ThreadHandle;
}