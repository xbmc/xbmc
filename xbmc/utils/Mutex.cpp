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
}

void CMutex::Wait()
{
	if (m_hMutex) WaitForSingleObject(m_hMutex,INFINITE);
}

void CMutex::Release()
{
	if (m_hMutex) ReleaseMutex(m_hMutex);
}

HANDLE CMutex::GetHandle()
{
	return m_hMutex;
}

void CMutex::WaitMSec(DWORD dwMillSeconds)
{
	if (m_hMutex) WaitForSingleObject(m_hMutex,dwMillSeconds);
}
