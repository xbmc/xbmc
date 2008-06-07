// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// Thread.cpp: Implementierung der Klasse CThread.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Thread.h"

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CThread::CThread()
{
	m_hThread = 0;
	m_dwThreadId = NULL;
	m_hEventStarted = CreateEvent(0, TRUE, TRUE, NULL);
}

CThread::~CThread()
{
	CloseHandle(m_hEventStarted);
	CloseHandle(m_hThread);
}

BOOL CThread::Create(int nPriority /*=THREAD_PRIORITY_NORMAL*/, DWORD dwCreateFlags /*=0*/)
{  
	m_hThread=CreateThread(0, 0x10000, ThreadProc, this, dwCreateFlags, &m_dwThreadId);
	if (!m_hThread)
	{
		delete this;
		return FALSE;
	}
  ResetEvent(m_hEventStarted);
	::SetThreadPriority(m_hThread, nPriority);  
	return TRUE;
}

BOOL CThread::PostThreadMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL res=::PostThreadMessage(m_dwThreadId, message, wParam, lParam);;
	ASSERT(res);
	return res;
}

#ifdef _XBOX
DWORD CThread::SuspendThread()
{
  return ::SuspendThread(m_hThread);
}
#endif

DWORD CThread::ResumeThread()
{
	BOOL res=::ResumeThread(m_hThread);
	if (res)
	{
		WaitForSingleObject(m_hEventStarted, INFINITE);
	}
	return res;
}

DWORD WINAPI CThread::ThreadProc(LPVOID lpParameter)
{
	return ((CThread *)lpParameter)->Run();
}

DWORD CThread::Run()
{
	InitInstance();
	SetEvent(m_hEventStarted);
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		if (!msg.hwnd)
			OnThreadMessage(msg.message, msg.wParam, msg.lParam);
		DispatchMessage(&msg);
	}
	DWORD res=ExitInstance();
	delete this;
	return res;
}

int CThread::OnThreadMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

BOOL CThread::InitInstance()
{
	return TRUE;
}

DWORD CThread::ExitInstance()
{
	return 0;
}
