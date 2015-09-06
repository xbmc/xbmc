/*
*      Copyright (C) 2005-2008 Team XBMC
*      http://www.xbmc.org
*
*      Copyright (C) 2015 Romank
*      https://github.com/Romank1
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


#ifdef HAS_DS_PLAYER

#include "DSPVRBackend.h"

CDSPVRBackend::CDSPVRBackend(const CStdString& strBackendBaseAddress, const CStdString& strBackendName)
  : m_strBaseURL(strBackendBaseAddress)
  , m_strBackendName(strBackendName)
  , m_tcpclient(NULL) 
{ 
  if (m_strBaseURL.length() > 7 && !(m_strBaseURL.Left(7).Equals("http://", false)))
    m_strBaseURL = "http://" + m_strBaseURL;
}

CDSPVRBackend::~CDSPVRBackend()
{
  TCPClientDisconnect();
  SAFE_DELETE(m_tcpclient);
}

bool CDSPVRBackend::JSONRPCSendCommand(HttpRequestMethod requestType, const CStdString& strCommand, const CStdString& strArguments, CVariant &json_response)
{
  CStdString strResponse;
  bool bReturn = false;

  switch (requestType)
  {
  case REQUEST_GET:
    bReturn = HttpRequestGET(strCommand, strResponse);
    break;
  case REQUEST_POST:
    bReturn = HttpRequestPOST(strCommand, strArguments, strResponse);
    break;
  default:
   return false;
  }

  if (bReturn)
  {
#ifdef _DEBUG
    // Print only the first 512 bytes, otherwise XBMC will crash...
	CLog::Log(LOGDEBUG, "%s Response: %s", __FUNCTION__, strResponse.substr(0, 512).c_str());
#endif
    if (strResponse.length() != 0)
	{
	  json_response = CJSONVariantParser::Parse(reinterpret_cast<const unsigned char*>(strResponse.c_str()), strResponse.size());
	  if (json_response.isNull())
	  {
		CLog::Log(LOGDEBUG, "%s Failed to parse %s:", __FUNCTION__, strResponse.c_str());
		return false;
	  }
	}
	else
	{
	  CLog::Log(LOGDEBUG, "%s Empty response", __FUNCTION__);
	  return false;
    }
#ifdef _DEBUG
	  //printValueTree(stdout, json_response);
#endif
  }

  return bReturn;
}

bool CDSPVRBackend::TCPClientConnect()
{
  bool bReturn = false;

  if (m_strBaseURL.empty())
  {
    CLog::Log(LOGERROR, "%s Base URL is not valid.", __FUNCTION__);
    return false;
  }

  if (!m_tcpclient)
  {
    m_tcpclient = new CDSSocket(af_inet, pf_inet, sock_stream, tcp);
  }
  
  if (!m_tcpclient->create())
  {
    CLog::Log(LOGERROR, "%s Could not connect create socket", __FUNCTION__);
    return false;
  }

  const CURL pathToUrl(m_strBaseURL);
  if (!m_tcpclient->connect(pathToUrl.GetHostName(), (unsigned short)pathToUrl.GetPort()))
  {
    CLog::Log(LOGERROR, "%s Could not connect to PVR Backend server", __FUNCTION__);
    return false;
  }

  m_tcpclient->set_non_blocking(1);
  CLog::Log(LOGINFO, "%s Connected to %s", __FUNCTION__, m_strBaseURL.c_str());
  
  return m_tcpclient->is_valid();
}

bool CDSPVRBackend::TCPClientDisconnect()
{
  if (!m_tcpclient)
    return false;

  CLog::Log(LOGINFO, "%s TCP Client Disconnect", __FUNCTION__);
  return m_tcpclient->close();
}

bool CDSPVRBackend::TCPClientSendCommand(const CStdString& strCommand, CStdString& strResponse)
{
  CSingleLock lock(m_ObjectLock);

  if (!m_tcpclient)
    return false;

  CStdString strCommandCopy = strCommand;
  strCommandCopy.Replace("\n","");
  CLog::Log(LOGDEBUG, "%s Sending Command: '%s'", __FUNCTION__, strCommandCopy.c_str());

  strResponse.clear();
  if (!m_tcpclient->send(strCommand))
  {
    CLog::Log(LOGERROR, "%s Send Command '%s' failed.", __FUNCTION__, strCommandCopy.c_str());
    return false;
  }

  string strResult;
  if (!m_tcpclient->ReadLine(strResult))
  {
    CLog::Log(LOGERROR, "%s Send Command - Failed.", __FUNCTION__);
    return false;
  }
  
  CLog::Log(LOGDEBUG, "%s Response from PVR Backend:'%s'", __FUNCTION__, strResult.c_str());

  strResponse = strResult;
  return true;
}

/*
* Check whether we still have a connection with the Backend Server.
* return True when a connection is available, otherwise False.
*/
bool CDSPVRBackend::TCPClientIsConnected()
{
  if (!m_tcpclient || !m_tcpclient->is_valid())
    return false;

  return true;
}

bool CDSPVRBackend::IsFileExistAndAccessible(const CStdString& strFilePath)
{
  long errCode;

  // Check if we can access the file
  bool bReturn = CDSFile::Exists(strFilePath, &errCode);
  if (!bReturn)
  {
    switch(errCode)
    {
      case ERROR_FILE_NOT_FOUND:
        CLog::Log(LOGERROR, "%s File not found: %s.", __FUNCTION__, strFilePath.c_str());
        break;
      case ERROR_ACCESS_DENIED:
      {
        char strUserName[256];
        DWORD lLength = 256;

        if (GetUserName(strUserName, &lLength))
          CLog::Log(LOGERROR, "%s Access denied on %s. Check share access rights for user '%s'.\n", __FUNCTION__, strFilePath.c_str(), strUserName);
        else
          CLog::Log(LOGERROR, "%s Access denied on %s. Check share access rights.", __FUNCTION__, strFilePath.c_str());
        
        CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, "DSPlayer", "Access denied: " + strFilePath, TOAST_DISPLAY_TIME, false);
        break;
      }
      default:
        CLog::Log(LOGERROR, "%s Cannot find or access file: %s. Check share access rights.", __FUNCTION__, strFilePath.c_str());
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s File found: %s", __FUNCTION__, strFilePath.c_str());
  }

  return bReturn;
}

bool CDSPVRBackend::ResolveHostName(const CStdString& strUrl, CStdString& strResolvedUrl)
{
  bool bReturn = false;
  strResolvedUrl.clear();

  CURL url(strUrl);
  CStdString strHostName = url.GetHostName();
  hostent* remoteHost = gethostbyname(strHostName.c_str());
  if (remoteHost != NULL)
  {
    in_addr * address = (in_addr *)remoteHost->h_addr;
    CStdString ip_address = inet_ntoa(*address);
    strResolvedUrl = strUrl;
    if (ip_address != strHostName)
    {
      strResolvedUrl.Replace(strHostName, ip_address);
      CLog::Log(LOGDEBUG, "%s Successfully resolved host name: %s to ip: %s", __FUNCTION__, strHostName.c_str(), ip_address.c_str());
    }
    bReturn = true;
  }

  if (!bReturn)
    CLog::Log(LOGERROR, "%s Failed to resolve hostname: %s", __FUNCTION__, strHostName.c_str());

  return bReturn;
}

bool CDSPVRBackend::HttpRequestGET(const CStdString& strCommand, CStdString& strResponse)
{
  if (m_strBaseURL.empty())
  {
    CLog::Log(LOGERROR, "%s Base URL is not valid.", __FUNCTION__);
    return false;
  }

  CSingleLock lock(m_ObjectLock);

  bool bReturn = false;
  CStdString strUrl = m_strBaseURL + strCommand;
  CLog::Log(LOGDEBUG, "%s URL: %s", __FUNCTION__, strUrl.c_str());
  CFile file;
  if (file.Open(strUrl.c_str(), 0))
  {
    char buffer[1024];
    while (file.ReadString(buffer, 1024))
      strResponse.append(buffer);
    bReturn = true;
  }
  else
  {
    CLog::Log(LOGERROR, "%s Can not open url: %s", __FUNCTION__, strUrl.c_str());
  }

  file.Close();

  if (!bReturn)
    CLog::Log(LOGERROR, "%s Request failed, url:", __FUNCTION__, strUrl.c_str());
  
  return bReturn;
}

bool CDSPVRBackend::HttpRequestPOST(const CStdString& strCommand, const CStdString& strArguments, CStdString& strResponse)
{
  if (m_strBaseURL.empty())
  {
    CLog::Log(LOGERROR, "%s Base URL is not valid.", __FUNCTION__);
    return false;
  }

  CSingleLock lock(m_ObjectLock);
  
  bool bReturn = false;
  CStdString strUrl = m_strBaseURL + strCommand;
  CLog::Log(LOGDEBUG, "%s URL: %s", __FUNCTION__, strUrl.c_str());
  CFile file;
  if (file.OpenForWrite(strUrl, false))
  {
	  int rc = file.Write(strArguments.c_str(), strArguments.length());
  	if (rc >= 0)
  	{
  	  CStdString result;
  	  result.clear();
  	  char buffer[1024];
	    while (file.ReadString(buffer, 1023))
  	    result.append(buffer);
      strResponse = result;
  	  bReturn = true;
  	}
    else
    {
      CLog::Log(LOGERROR, "%s Can not write to %s", __FUNCTION__, strUrl.c_str());
    }

	  file.Close();
  }
  else
  {
    CLog::Log(LOGERROR, "%s Can not open %s for write", __FUNCTION__, strUrl.c_str());
  }

  return bReturn;
}

#endif
