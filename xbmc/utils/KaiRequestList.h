#pragma once

class CKaiRequestList
{
public:
	CKaiRequestList(void);
	virtual ~CKaiRequestList(void);
	
	void QueueRequest(CStdString& aRequest);
	bool GetNext(CStdString& aRequest);

protected:

	typedef std::queue<CStdString> REQUESTQUEUE;
	REQUESTQUEUE m_requests;

	CRITICAL_SECTION m_critical;
};
