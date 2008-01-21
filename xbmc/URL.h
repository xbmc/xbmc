#pragma once

#include "../guilib/StdString.h"

class CURL
{
public:
  CURL(const CStdString& strURL);
  CURL(const CURL& url);
  CURL();
  virtual ~CURL(void);
  void SetFileName(const CStdString& strFileName);
  void SetHostName(const CStdString& strHostName);
  void SetUserName(const CStdString& strUserName);
  void SetPassword(const CStdString& strPassword);
  void SetProtocol(const CStdString& strProtocol);
  void SetOptions(const CStdString& strOptions);
  void SetPort(int port);
  bool HasPort() const;
  int GetPort() const;
  const CStdString& GetHostName() const;
  const CStdString& GetDomain() const;
  const CStdString& GetUserName() const;
  const CStdString& GetPassWord() const;
  const CStdString& GetFileName() const;
  const CStdString& GetProtocol() const;
  const CStdString& GetFileType() const;
  const CStdString& GetShareName() const;
  const CStdString& GetOptions() const;
  const CStdString  GetFileNameWithoutPath() const; /* return the filename excluding path */

  inline const char GetDirectorySeparator() const;

  void GetURL(CStdString& strURL) const;
  void GetURLPath(CStdString& strPath) const;
  void GetURLWithoutUserDetails(CStdString& strURL) const;
  void GetURLWithoutFilename(CStdString& strURL) const;
  CURL& operator= (const CURL& source);
  bool IsLocal() const;
  static bool IsFileOnly(const CStdString &url);

protected:
  int m_iPort;
  CStdString m_strHostName;
  CStdString m_strShareName;
  CStdString m_strDomain;
  CStdString m_strUserName;
  CStdString m_strPassword;
  CStdString m_strFileName;
  CStdString m_strProtocol;
  CStdString m_strFileType;
  CStdString m_strOptions;
};
