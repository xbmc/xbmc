// HTTP.h: interface for the CHTTP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HTTP_H__A368CB6F_3D08_4966_9F9F_961A59CB4EC7__INCLUDED_)
#define AFX_HTTP_H__A368CB6F_3D08_4966_9F9F_961A59CB4EC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#ifdef _XBOX
	#include <xtl.h>
#else
	#include <windows.h>
#endif
#include <string>
using namespace std;
#include "../autoptrhandle.h"
using namespace AUTOPTR;


class CHTTP  
{
public:
	CHTTP();
	CHTTP(const string& strProxyServer, int iProxyPort);
	virtual ~CHTTP();

	bool 		Open(const string& strURL);
	bool 		Recv(char* pBuffer, int iLen, int& iRead, bool bFill);
	void 		Close();
	bool 		Post(const string& strURL, const string& strPostData, string& strHTML);
	void 		SetCookie(const string& strCookie);
	bool 		Get(string& strURL, string& strHTML);
	bool 		Download(const string &strURL, const string &strFileName);
	void		SetHTTPVer(unsigned int iVer);
protected:
	bool   BreakURL(const string& strURL, string& strHostName, int& iPort, string& Page);
	bool	 Send(char* pBuffer, int iLen);
	bool	 Connect();

	string m_strProxyServer;
	int    m_iProxyPort;
	CAutoPtrSocket m_socket;
	WSAEVENT hEvent;
	
	string m_strHostName;
	int    m_iPort;
	int			m_iHTTPver;
private:
	string m_strCookie;
};

#endif // !defined(AFX_HTTP_H__A368CB6F_3D08_4966_9F9F_961A59CB4EC7__INCLUDED_)
