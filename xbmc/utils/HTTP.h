/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// HTTP.h: interface for the CHTTP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HTTP_H__A368CB6F_3D08_4966_9F9F_961A59CB4EC7__INCLUDED_)
#define AFX_HTTP_H__A368CB6F_3D08_4966_9F9F_961A59CB4EC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AutoPtrHandle.h"

#include <map>

class CHTTP
{
public:
  CHTTP();
  CHTTP(const std::string& strProxyServer, int iProxyPort);
  virtual ~CHTTP();

  bool Post(const std::string& strURL, const std::string& strPostData, std::string& strHTML);
  void SetCookie(const std::string& strCookie);
  void SetReferer(const std::string& strCookie);
  bool Get(const std::string& strURL, std::string& strHTML);
  bool Head(std::string& strURL);
  bool Download(const std::string &strURL, const std::string &strFileName, LPDWORD pdwSize = NULL);
  bool GetHeader(CStdString strName, CStdString& strValue) const;
  void Cancel();
  void SetUserAgent(std::string strUserAgent);
  void SetContentType(const std::string& strContentType);

  std::string m_redirectedURL;
  bool IsInternet(bool checkDNS = true);
  static bool BreakURL(const std::string& strURL, std::string& strHostName, std::string &strUsername, std::string &strPassword, int& iPort, std::string& Page);
  int Open(const std::string& strURL, const char* verb, const char* pData);
  void Close();

  void Reset();

protected:
  bool Send(char* pBuffer, int iLen);
  bool Connect();
  bool Recv(int iLen);
  bool ReadData(std::string& strData);
  void ParseHeaders();

private:
  void ParseHeader(std::string::size_type start, std::string::size_type colon, std::string::size_type end);
  CStdString ConstructAuthorization(const CStdString &auth, const CStdString &username, const CStdString &password);

  AUTOPTR::CAutoPtrSocket m_socket;
#ifndef _LINUX
  HANDLE hEvent;
#endif
  std::string m_strProxyServer;
  std::string m_strProxyUsername;
  std::string m_strProxyPassword;
  std::string m_strHostName;
  std::string m_strCookie;
  std::string m_strReferer;
  std::string m_strHeaders;
  std::string m_strUsername;
  std::string m_strPassword;
  std::string m_strUserAgent;
  std::string m_strContentType;
  std::map<CStdString, CStdString> m_mapHeaders;

  bool m_bProxyEnabled;
  int m_iProxyPort;
  int m_iPort;

  char* m_RecvBuffer;
  int m_RecvBytes;
  
  bool m_cancelled;
};

#endif // !defined(AFX_HTTP_H__A368CB6F_3D08_4966_9F9F_961A59CB4EC7__INCLUDED_)


