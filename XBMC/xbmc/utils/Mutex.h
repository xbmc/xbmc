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

	void Wait();
	void WaitMSec(DWORD dwMillSeconds);

protected:
	HANDLE	m_hMutex;
};