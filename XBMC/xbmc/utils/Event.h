// Event.h: interface for the CEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENT_H__724ADE14_0F5C_4836_B995_08FFAA97D6B9__INCLUDED_)
#define AFX_EVENT_H__724ADE14_0F5C_4836_B995_08FFAA97D6B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CEvent  
{
public:
	void PulseEvent();
	bool WaitMSec(DWORD dwMillSeconds);
	HANDLE GetHandle();
	void Reset();
	void Set();
	void Wait();
	CEvent();
	virtual ~CEvent();

protected:
	CEvent(const CEvent& event);
  CEvent& operator=(const CEvent& src);
	HANDLE m_hEvent;
};

#endif // !defined(AFX_EVENT_H__724ADE14_0F5C_4836_B995_08FFAA97D6B9__INCLUDED_)
