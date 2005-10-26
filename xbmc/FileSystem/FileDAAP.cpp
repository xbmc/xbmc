/*
* DAAP Support for XBox Media Center
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

#include "../stdafx.h"
#include "FileDAAP.h"
#include <sys/stat.h>

#define BUFFER_SIZE 32768

static UINT64 strtouint64(const char *s)
{
  UINT64 r = 0;

  while ((*s != 0) && (isspace(*s)))
    s++;
  if (*s == '+')
    s++;
  while ((*s != 0) && (isdigit(*s)))
  {
    r = r * ((UINT64)10);
    r += ((UINT64)(*s)) - ((UINT64)'0');
    s++;
  }
  return r;
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
      while( DAAP_Client_Release(m_pClient) != 0 );    
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
  //We need this section from now on
  if( !CSectionLoader::IsLoaded("LIBXDAAP") ) CSectionLoader::Load("LIBXDAAP");
  try
  {

    ITHOST it;
    it = m_mapHosts.find(strHost);
    if( it != m_mapHosts.end() )
      return it->second;


    if( !m_pClient )
      m_pClient = DAAP_Client_Create((DAAP_fnClientStatus)StatusCallback, (void*)this);

    DAAP_SClientHost* pHost = DAAP_Client_AddHost(m_pClient, (char *)strHost.c_str(), "A", "A");
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
    CLog::Log(LOGERROR, "CDaapClient::GetHost(%s) - Unknown Exception");
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
    case DAAP_STATUS_downloading:
      CLog::Log(LOGINFO, "CDaapClient::Callback - Downlading");
    case DAAP_STATUS_idle:
      CLog::Log(LOGINFO, "CDaapClient::Callback - Idle");
  }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileDAAP::CFileDAAP()
{  
  m_fileSize = 0;
  m_filePos = 0;
  m_httpHeaderLength = 0;
  m_httpContentLenght = 0;
  m_hStreamThread = NULL;

  m_thisHost = NULL;
  m_thisClient = NULL;

  m_bOpened = false;
  m_bInterupt = false;
  m_bStreaming = false;

}

CFileDAAP::~CFileDAAP()
{
  Close();  
}

//Callback function from XDAAP, if return value is less than 0, it will abort
int CFileDAAP::DAAPHttpCallback(const char *data, int size, int flag, int contentlength, void* context)
{
  CFileDAAP* pFile = (CFileDAAP*)context;
  
  if( !pFile->m_hStreamThread )
  {
    DuplicateHandle( NULL, GetCurrentThread(), NULL, &(pFile->m_hStreamThread), 0, FALSE, 0 );
  }

  int result = 0;
  if( flag == 1 )
  {
    //Header data
    pFile->m_httpHeaderLength = size;
    pFile->m_httpContentLenght = contentlength;

    //Enough to start streaming
    pFile->m_bStreaming = true;
    pFile->m_eventCallback.PulseEvent();
    return 0;
  }
  else if( flag == 2 )
  {    
    //Normal data
    pFile->m_httpContentLenght = contentlength;  


    //Good make sure we say we have started before writing as it can block
    pFile->m_bStreaming = true;
    pFile->m_eventCallback.PulseEvent();


    int iSizeLeft = size;
    int iWriteSize = 0;
    char *pData = (char*)data;

    while( 1 )
    {

      iWriteSize = pFile->m_DataBuffer.GetMaxWriteSize();
      if( iWriteSize > iSizeLeft ) iWriteSize = iSizeLeft;

      if( iWriteSize )
      {
        pFile->m_DataBuffer.WriteBinary(pData, iWriteSize);
        pFile->m_eventNewData.PulseEvent();
        pData+= iWriteSize;
        iSizeLeft -= iWriteSize;
      }

      if( iSizeLeft <= 0 ) break;

      if( pFile->m_bInterupt ) break;

      Sleep(1);
    }

    return 0;
  }
  else
  {
    //Either end of file, or failed to start
    pFile->m_bStreaming = false;
    pFile->m_eventCallback.PulseEvent();
    return -1;
  }  
}


//*********************************************************************************************
bool CFileDAAP::Open(const CURL& url, bool bBinary)
{
  CSingleLock lock(g_DaapClient);

  // only able to open mp3's or m4a's ...
  if (url.GetFileType() != "mp3" && url.GetFileType() != "m4a") return (false);
  // not protected drm'd iTunes songs yet
  // && url.GetFileType() != "m4p") return(false);
  if (url.GetFileName().length() == 0) return (false);

  if (m_bOpened) Close();

  m_DataBuffer.Create(BUFFER_SIZE*3, BUFFER_SIZE);

  m_filePos = 0;
  m_fileSize = 0;

  sscanf(url.GetFileName().c_str(), "%i.%s", &m_iFileID, m_sFileFormat);

  CLog::Log(LOGDEBUG, "CFileDAAP::Open(%s)", url.GetFileName().c_str());


  m_thisHost = g_DaapClient.GetHost(url.GetHostName());
  if (!m_thisHost)  
    return false;  
    
  if( !StartStreaming() )
  {
    CLog::Log(LOGERROR, "CFileDAAP::Open - Failed");
    return false;
  }

  //First callback should be header if exists, so it should give both contentlength and header length
  //we save this file lenght as it can change after a seek. I suppose we could get filesize from the database too.
  m_fileSize = (__int64) (m_httpContentLenght - m_httpHeaderLength);

  m_bOpened = true;
  return true;
}

bool CFileDAAP::StopStreaming()
{
  m_bInterupt = true;
  if( m_hStreamThread )
  {

    //Thread is still active for some reason.. this need to end before we can close
    //otherwise it might call the callback function on objects that doesn't exist
    if( WaitForSingleObject(m_hStreamThread, 5000) == WAIT_TIMEOUT )
    {
      CLog::Log(LOGERROR, "CFileDAAP::Open - Timeout waiting for stream to stop.");
    }

    CloseHandle( m_hStreamThread );
    m_hStreamThread = NULL;
  }
  m_bStreaming = false;

  return true;
}

bool CFileDAAP::StartStreaming()
{
  CSingleLock lock( g_DaapClient );

  try
  {

    if( m_bStreaming ) StopStreaming();

    m_bInterupt = false;
    m_httpContentLenght = 0;
    m_httpHeaderLength = 0;

    int iResult = DAAP_ClientHost_AsyncGetAudioFileCallback( m_thisHost, g_DaapClient.m_iDatabase, m_iFileID, m_sFileFormat, (int)m_filePos, DAAPHttpCallback, (void*)this);  

    if( iResult < 0 )
      throw("DAAP_ClientHost_AsyncGetAudioFileCallback failed.");
    
    if( !m_eventCallback.WaitMSec(5000) )
      throw("Timeout waiting for first callback.");
          
    return true;

  }
  catch(char* error)
  {
    CLog::Log(LOGERROR, "CFileDAAP::StartStreaming - %s", error);
    StopStreaming();

    return false;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CFileDAAP::StartStreaming - Unknown exception");
    StopStreaming();

    return false;
  }
  
}

bool CFileDAAP::Exists(const CURL& url)
{
  CStdString strType = url.GetFileType();
  if( strType.Equals("mp3") || strType.Equals("m4a") )
    return true;
  else
    return false;
}

int CFileDAAP::Stat(const CURL& url, struct __stat64* buffer)
{
  /*
  if (Open(url, true))
  {
   buffer->st_size = this->m_fileSize;
   buffer->st_mode = _S_IFREG;
   Close();
  }
  errno = ENOENT;
  */ 
  return -1;
}

//*********************************************************************************************
unsigned int CFileDAAP::Read(void *lpBuf, __int64 uiBufSize)
{
  char *pData = (char*)lpBuf;
  int iSizeLeft = (int)uiBufSize;
  int iReadSize = 0;
  
  while(1)
  {
    iReadSize = m_DataBuffer.GetMaxReadSize();
    if( iSizeLeft < iReadSize ) iReadSize = iSizeLeft;

    m_DataBuffer.ReadBinary(pData, iReadSize);

    pData += iReadSize;
    iSizeLeft -= iReadSize;

    //Check if we are done
    if( iSizeLeft <= 0 ) break;

    if( !m_bStreaming ) break;

    m_eventNewData.WaitMSec(100);
  }
  
  
  m_filePos += uiBufSize - iSizeLeft;
  return (int) (uiBufSize - iSizeLeft);
  
}

//*********************************************************************************************
void CFileDAAP::Close()
{
  StopStreaming();

  m_DataBuffer.Destroy();

  m_bOpened = false;
}

//*********************************************************************************************
__int64 CFileDAAP::Seek(__int64 iFilePosition, int iWhence)
{

  __int64 nextPos = 0;
  switch (iWhence)
  {
  case SEEK_SET:
    if (iFilePosition < 0 || iFilePosition > m_fileSize ) return -1;

    nextPos = iFilePosition;

    break;

  case SEEK_CUR:
    if ( (m_filePos+iFilePosition) < 0 || (m_filePos+iFilePosition) > m_fileSize ) return -1;

    nextPos = m_filePos + iFilePosition;

    break;

  case SEEK_END:
    if( iFilePosition > 0 || (m_fileSize+iFilePosition) > m_fileSize ) return -1;

    nextPos = m_fileSize+iFilePosition;
    break;
  }

  if( m_DataBuffer.SkipBytes((int)(nextPos - m_filePos)) )
  {
    return m_filePos;
  }
  else
  {
    StopStreaming();
    m_DataBuffer.Clear();

    m_filePos = nextPos;

    if( StartStreaming() )
      return m_filePos;
    else
      return -1;
  }
  
}

//*********************************************************************************************
__int64 CFileDAAP::GetLength()
{
  if (!m_bOpened) return 0;
  return m_fileSize;
}

//*********************************************************************************************
__int64 CFileDAAP::GetPosition()
{
  if (!m_bOpened) return 0;
  return m_filePos;
}


//*********************************************************************************************
bool CFileDAAP::ReadString(char *szLine, int iLineLength)
{
  if (!m_bOpened) return false;
  __int64 iFilePos = GetPosition();

  int iBytesRead = Read( (unsigned char*)szLine, iLineLength);
  if (iBytesRead <= 0)
  {
    return false;
  }

  szLine[iBytesRead] = 0;

  for (int i = 0; i < iBytesRead; i++)
  {
    if ('\n' == szLine[i])
    {
      if ('\r' == szLine[i + 1])
      {
        szLine[i + 2] = 0;
        Seek(iFilePos + i + 2, SEEK_SET);
      }
      else
      {
        // end of line
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 1, SEEK_SET);
      }
      break;
    }
    else if ('\r' == szLine[i])
    {
      if ('\n' == szLine[i + 1])
      {
        szLine[i + 2] = 0;
        Seek(iFilePos + i + 2, SEEK_SET);
      }
      else
      {
        // end of line
        szLine[i + 1] = 0;
        Seek(iFilePos + i + 1, SEEK_SET);
      }
      break;
    }
  }
  if (iBytesRead > 0)
  {
    return true;
  }
  return false;
}
