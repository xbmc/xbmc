/*
* DAAP Support for XBMC
* Copyright (c) 2004 Forza (Chris Barnett)
* Portions Copyright (c) by the authors of libOpenDAAP
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "DAAPFile.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include <sys/stat.h>

#include "lib/libXDAAP/private.h"

using namespace XFILE;

#define HEDAER_VERSION       "Client-DAAP-Version"
#define HEADER_REQUESTID     "Client-DAAP-Request-ID"
#define HEADER_VALIDATE      "Client-DAAP-Validation"
#define HEADER_ACCESS_INDEX  "Client-DAAP-Access-Index"
#define DAAP_USERAGENT       "iTunes/4.6 (Windows; N)"
#define DAAP_PORT            3689

extern "C"
{
/* prototype of function in LIBXDAAP to generate the has needed for requests */
void GenerateHash(short version_major,
                  const unsigned char *url, unsigned char hashSelect,
                  unsigned char *outhash,
                  int request_id);
}

CDaapClient g_DaapClient;

CDaapClient::CDaapClient()
{
  m_pClient = NULL;
  m_Status = DAAP_STATUS_error;

  m_pArtistsHead = NULL;
  m_iDatabase = 0;
}
CDaapClient::~CDaapClient()
{

}
void CDaapClient::Release()
{
  m_mapHosts.clear();

  if( m_pClient )
  {
    try
    {
      while( DAAP_Client_Release(m_pClient) != 0 ) {}
    }
    catch(...)
    {
      CLog::Log(LOGINFO, "CDaapClient::Disconnect - Unexpected exception");
    }

    m_pClient = NULL;
  }
}

DAAP_SClientHost* CDaapClient::GetHost(const CStdString &strHost)
{
  try
  {

    ITHOST it;
    it = m_mapHosts.find(strHost);
    if( it != m_mapHosts.end() )
      return it->second;


    if( !m_pClient )
      m_pClient = DAAP_Client_Create((DAAP_fnClientStatus)StatusCallback, (void*)this);

    DAAP_SClientHost* pHost = DAAP_Client_AddHost(m_pClient, (char *)strHost.c_str(), (char *)"A", (char *)"A");
    if( !pHost )
      throw("Unable to add host");

    if( DAAP_ClientHost_Connect(pHost) != 0 )
      throw("Unable to connect");

    m_mapHosts[strHost] = pHost;

    return pHost;

  }
  catch(char* err)
  {
    CLog::Log(LOGERROR, "CDaapClient::GetHost(%s) - %s", strHost.c_str(), err );
    return NULL;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CDaapClient::GetHost(%s) - Unknown Exception", strHost.c_str());
    return NULL;
  }

}

void CDaapClient::StatusCallback(DAAP_SClient *pClient, DAAP_Status status, int value, void* pContext)
{
  ((CDaapClient*)pContext)->m_Status = status;
  switch(status)
  {
    case DAAP_STATUS_connecting:
      CLog::Log(LOGINFO, "CDaapClient::Callback - Connecting");
      break;
    case DAAP_STATUS_downloading:
      CLog::Log(LOGINFO, "CDaapClient::Callback - Downlading");
      break;
    case DAAP_STATUS_idle:
      CLog::Log(LOGINFO, "CDaapClient::Callback - Idle");
      break;
    default:
      CLog::Log(LOGINFO, "CDaapClient::Callback - Status %d", status);
      break;
  }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDAAPFile::CDAAPFile()
{
  m_thisHost = NULL;
  m_thisClient = NULL;

  m_bOpened = false;
}

CDAAPFile::~CDAAPFile()
{
  Close();
}



//*********************************************************************************************
bool CDAAPFile::Open(const CURL& url)
{
  CSingleLock lock(g_DaapClient);

  if (m_bOpened) Close();

  m_url = url;

  CLog::Log(LOGDEBUG, "CDAAPFile::Open(%s)", url.GetFileName().c_str());
  CStdString host = url.GetHostName();
  if (url.HasPort())
    host.Format("%s:%i",url.GetHostName(),url.GetPort());
  m_thisHost = g_DaapClient.GetHost(host);
  if (!m_thisHost)
    return false;

  /* get us a new request id */
  int requestid = ++m_thisHost->request_id;

  m_hashurl = "/" + m_url.GetFileName();
  m_hashurl += m_url.GetOptions();

  char hash[33] = {0};
  GenerateHash(m_thisHost->version_major, (unsigned char*)(m_hashurl.c_str()), 2, (unsigned char*)hash, requestid);

  m_curl.SetUserAgent(DAAP_USERAGENT);

  //m_curl.SetRequestHeader(HEADER_VERSION, "3.0");
  m_curl.SetRequestHeader(HEADER_REQUESTID, requestid);
  m_curl.SetRequestHeader(HEADER_VALIDATE, CStdString(hash));
  m_curl.SetRequestHeader(HEADER_ACCESS_INDEX, 2);

  m_url.SetProtocol("http");
  if(!m_url.HasPort())
    m_url.SetPort(DAAP_PORT);


  m_bOpened = true;

  return m_curl.Open(m_url);
}


//*********************************************************************************************
unsigned int CDAAPFile::Read(void *lpBuf, int64_t uiBufSize)
{
  return m_curl.Read(lpBuf, uiBufSize);
}

//*********************************************************************************************
void CDAAPFile::Close()
{
  m_curl.Close();
  m_bOpened = false;
}

//*********************************************************************************************
int64_t CDAAPFile::Seek(int64_t iFilePosition, int iWhence)
{
  CSingleLock lock(g_DaapClient);

  int requestid = ++m_thisHost->request_id;

  char hash[33] = {0};
  GenerateHash(m_thisHost->version_major, (unsigned char*)(m_hashurl.c_str()), 2, (unsigned char*)hash, requestid);

  m_curl.SetRequestHeader(HEADER_REQUESTID, requestid);
  m_curl.SetRequestHeader(HEADER_VALIDATE, CStdString(hash));

  return m_curl.Seek(iFilePosition, iWhence);
}

//*********************************************************************************************
int64_t CDAAPFile::GetLength()
{
  return m_curl.GetLength();
}

//*********************************************************************************************
int64_t CDAAPFile::GetPosition()
{
  return m_curl.GetPosition();
}

bool CDAAPFile::Exists(const CURL& url)
{
  return false;
}

int CDAAPFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return -1;
}

int CDAAPFile::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
    return 1;

  return -1;
}
