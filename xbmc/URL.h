#pragma once

class CURL
{
public:
	CURL(const CStdString& strURL);
	CURL(const CURL& url);
	virtual ~CURL(void);
  void							SetFileName(const CStdString& strFileName);
	void							SetHostName(const CStdString& strHostName);
	void SetUserName(const CStdString& strUserName);
	void SetPassword(const CStdString& strPassword);
	bool							HasPort() const;
	int								GetPort() const;
	const CStdString& GetHostName() const;
	const CStdString& GetDomain() const;
	const CStdString& GetUserName() const;
	const CStdString& GetPassWord() const;
	const CStdString& GetFileName() const;
	const CStdString& GetProtocol() const;
	const CStdString& GetFileType() const;
  void							GetURL(CStdString& strURL) const;
  void              GetURLPath(CStdString& strPath) const;
	void							GetURLWithoutUserDetails(CStdString& strURL) const;
	void							GetURLWithoutFilename(CStdString& strURL) const;
	CURL& operator= (const CURL& source);


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
