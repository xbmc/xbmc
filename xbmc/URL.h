#pragma once
#ifdef _XBOX
	#include <xtl.h>
#else
	#include <windows.h>
#endif

#include <string>
#include "stdstring.h"
using namespace std;
class CURL
{
public:
	CURL(const CStdString& strURL);
	virtual ~CURL(void);
  void							SetFileName(const CStdString& strFileName);
	void							SetHostName(const CStdString& strHostName);
	bool							HasPort() const;
	int								GetPort() const;
	const CStdString& GetHostName() const;
	const CStdString& GetDomain() const;
	const CStdString& GetUserName() const;
	const CStdString& GetPassWord() const;
	const CStdString& GetFileName() const;
	const CStdString& GetProtocol() const;
	const CStdString& GetFileType() const;
  void							GetURL(CStdString& strURL) ;
	void							GetURLWithoutUserDetails(CStdString& strURL) ;

protected:
	int				 m_iPort;
	CStdString m_strHostName;
	CStdString m_strDomain;
	CStdString m_strUserName;
	CStdString m_strPassword;
	CStdString m_strFileName;
	CStdString m_strProtocol;
	CStdString m_strFileType;
};
