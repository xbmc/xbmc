//  CMutex - Wrapper for xbox Mutex API
//
//	by Bobbin007 in 2003

#pragma once
#include <xtl.h>

class CMutex 
{
public:
	CMutex();
	CMutex( char* pName );
	virtual ~CMutex();

	HANDLE GetHandle();

	void Release();

	bool Wait();
	void WaitMSec(DWORD dwMillSeconds);

protected:
	HANDLE	m_hMutex;
};

class CMutexWait
{
public:
	CMutexWait(CMutex& mutex);
	virtual ~CMutexWait();
private:
	CMutex& m_mutex;
	bool    m_bLocked;
};