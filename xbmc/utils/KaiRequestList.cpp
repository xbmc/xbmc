
#include "../stdafx.h"
#include "KaiRequestList.h"

CKaiRequestList::CKaiRequestList(void)
{
	InitializeCriticalSection(&m_critical);
}

CKaiRequestList::~CKaiRequestList(void)
{
	DeleteCriticalSection(&m_critical);
}

void CKaiRequestList::QueueRequest(CStdString& aRequest)
{
	EnterCriticalSection(&m_critical);
	
	m_requests.push(aRequest);

	LeaveCriticalSection(&m_critical);
}


bool CKaiRequestList::GetNext(CStdString& aRequest)
{
	EnterCriticalSection(&m_critical);
	
	bool bPending = m_requests.size()>0;
	if (bPending)
	{
		aRequest = m_requests.front();
		m_requests.pop();
	}

	LeaveCriticalSection(&m_critical);

	return bPending;
}