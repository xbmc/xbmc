// Thread.h: interface for the CThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_)
#define AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif

class CThread  
{
public:
	CThread();
	virtual ~CThread();
	void						Create(bool bAutoDelete = false);
	unsigned long		ThreadId() const;
	bool						WaitForThreadExit(DWORD dwTimeOutSec);
	bool						SetPriority(const int iPriority);
	HANDLE          ThreadHandle();
	operator				HANDLE();
	operator const	HANDLE() const;
	bool						IsAutoDelete() const;
	virtual void		StopThread();

protected:
	virtual void		OnStartup(){};
	virtual void		OnExit(){};
	virtual void		Process(){};

	bool            m_bAutoDelete;
	bool						m_bStop;
	bool						m_bStopped;
	HANDLE					m_ThreadHandle;
	DWORD						m_dwThreadId;
private:
	static DWORD WINAPI CThread::staticThread(LPVOID* data);
};

#endif // !defined(AFX_THREAD_H__ACFB7357_B961_4AC1_9FB2_779526219817__INCLUDED_)
