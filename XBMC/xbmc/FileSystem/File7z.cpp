/*
* XBMC
* 7z Filesystem
* Copyright (c) 2008 topfs2
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

#include "File7z.h"
#include "../utils/log.h"

#define MINIMUM_BUFFER 4096

#define CRITICAL_BUFFER 1024



using namespace XFILE;
//using namespace DIRECTORY;

CFile7z::CFile7z()
{ }

CFile7z::~CFile7z()
{ }

__int64 CFile7z::GetPosition()
{

}
__int64 CFile7z::GetLength()
{

}

bool CFile7z::Open(const CURL& url, bool bBinary)
{
  if (!bBinary)
    return false;

  CStdString strArchive = url.GetHostName();
  CStdString strOptions = url.GetOptions();
  CStdString strPathInArchive = url.GetFileName();

  m_FilePosition = 0;
  m_FileLength   = 0;

  m_Processed = 0;

  m_Open         = false;
  const CCache *File = NULL;
  if (g_7zManager.Cache(strArchive) && (File = g_7zManager.GetFileIn7z(strArchive, strPathInArchive)) != NULL)
  {
    m_FileLength = File->Size();
//    printf("Size: %lld", m_FileLength);
    Byte    *Buffer    = 0;
    size_t  BufferSize = 0;
    m_ExtractInfo = new CFile7zExtractThread(strArchive, strPathInArchive, &Buffer, &BufferSize);
    m_ExtractThread = new CThread(m_ExtractInfo);
    if (m_ExtractThread != NULL)
    {
      m_Open = true;
      m_ExtractThread->Create();
      //m_ExtractInfo->Run();
    }
  }

  if (m_Open)
  {
    while (!m_ExtractInfo->m_Ready);
     // printf("Not Ready yet, needs to wait\n");
    while (m_ExtractInfo->m_NowPos < MINIMUM_BUFFER && m_ExtractInfo->m_NowPos != m_FileLength && !m_ExtractInfo->m_Error)
    {
      sleep(1);
      CLog::Log(LOGDEBUG, "7z: To little buffer (%i) - Wait for it to fill up\n", m_ExtractInfo->m_NowPos);
    }
    if (m_ExtractInfo->m_Error)
      m_Open = false;
  }
  return m_Open;
}

bool CFile7z::Exists(const CURL& url)
{
  return false;
}
int	CFile7z::Stat(const CURL& url, struct __stat64* buffer)
{
  return -1;
}

unsigned int CFile7z::Read(void* lpBuf, __int64 uiBufSize)
{
  if (m_FilePosition >= m_FileLength) // we are done
    return 0;

  if (m_ExtractInfo->m_Error)
  {
    CLog::Log(LOGERROR, "7z: Extraction error");
    return 0;
  }

  return m_ExtractInfo->m_TempOut->Read(lpBuf, uiBufSize);




  __int64 Copy;
/*  if ((m_FileLength - m_FilePosition) < uiBufSize)
    Copy = (m_FileLength - m_FilePosition);*/
  if ((m_FilePosition + uiBufSize) > m_ExtractInfo->m_NowPos)
    Copy = (m_ExtractInfo->m_NowPos - m_FilePosition);
  else
    Copy = uiBufSize;
//  printf("uiBufSize  %lld\n", uiBufSize);
//  printf("FilePos    %lld\n", m_FilePosition);
//  printf("NowPos     %lld\n", m_ExtractInfo->m_NowPos);
//  printf("Copy       %lld\n", Copy);
//  printf("blockIndex %lld\n", m_ExtractInfo->m_blockIndex);

//  lpBuf = m_ExtractInfo->m_Buffer + m_Processed;
  memcpy(lpBuf, (m_ExtractInfo->m_Buffer + m_FilePosition), size_t(Copy));

  while ((m_ExtractInfo->m_NowPos - m_FilePosition) < CRITICAL_BUFFER)
  {
    CLog::Log(LOGDEBUG, "7z: BELOW CRITICALBUFFER - NowPos (%i) Buffer (%i)", m_ExtractInfo->m_NowPos, (m_ExtractInfo->m_NowPos - m_FilePosition));
    sleep(1);
  }
  m_FilePosition += Copy;

  return Copy;
}

int	CFile7z::Write(const void* lpBuf, __int64 uiBufSize)
{
  return -1;
}

__int64	CFile7z::Seek(__int64 iFilePosition, int iWhence)
{
  return m_ExtractInfo->m_TempOut->Seek(iFilePosition, iWhence);
  
  switch (iWhence)
  {


    case SEEK_CUR:
//      printf("SEEK_CUR %lld\n", iFilePosition);
      if (iFilePosition == 0)
        return m_FilePosition;
      else if ((m_FilePosition + iFilePosition) <= m_ExtractInfo->m_NowPos)
        m_FilePosition += iFilePosition;
      else
        return -1;
      break;
    case SEEK_END:
//      printf("SEEK_END %lld\n", iFilePosition);
/*      if (m_ExtractInfo->m_NowPos == m_FileLength)
      {
        m_FilePosition = m_FileLength;
        return m_FilePosition;
      }
      else*/
        return -1;
      break;
    case SEEK_SET:
//      printf("SEEK_SET %lld\n", iFilePosition);
      if (iFilePosition > m_FileLength)
        return -1;
      if (iFilePosition > m_ExtractInfo->m_NowPos)
        return -1;
//      printf("SEEK_SET APPROVED\n");
      m_FilePosition = iFilePosition;
      return m_FilePosition;
      break;
    default:
      //printf("DEFAULT #%i | %lld\n", iWhence, iFilePosition);
      return -1;
      break;
  }
}

void CFile7z::Close()
{
//  printf("CLOSE\n");
}
void CFile7z::Flush()
{
//  printf("FLUSH\n");
}

bool CFile7z::OpenForWrite(const CURL& url, bool bBinary)
{
//  printf("OpenForWrite\n");
  return false;
}

unsigned int	CFile7z::Write(void *lpBuf, __int64 uiBufSize)
{
//  printf("Write\n");
  return -1;
}
