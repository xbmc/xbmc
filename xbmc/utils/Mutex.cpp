#include "../stdafx.h"
#include "Mutex.h"

CMutex::CMutex()
{
	m_hMutex=CreateMutex( NULL, FALSE, NULL);
}

CMutex::CMutex( char* pName )
{
	m_hMutex=CreateMutex( NULL, FALSE, pName);
}

CMutex::~CMutex()
{
	CloseHandle(m_hMutex);
	m_hMutex=NULL;
}

bool CMutex::Wait()
{
	if (m_hMutex) 
	{
		if (WAIT_OBJECT_0==WaitForSingleObject(m_hMutex,INFINITE)) return true;
	}
	return false;
}

void CMutex::Release()
{
	if (m_hMutex) 
	{
		ReleaseMutex(m_hMutex);
	}
}

CMutexWait::CMutexWait(CMutex& mutex) 
:m_mutex(mutex)
{ 
	m_bLocked=m_mutex.Wait();
}

CMutexWait::~CMutexWait()
{ 
	if (m_bLocked)
	{
		m_mutex.Release();
	}
}


HANDLE CMutex::GetHandle()
{
	return m_hMutex;
}

void CMutex::WaitMSec(DWORD dwMillSeconds)
{
	if (m_hMutex) WaitForSingleObject(m_hMutex,dwMillSeconds);
}
