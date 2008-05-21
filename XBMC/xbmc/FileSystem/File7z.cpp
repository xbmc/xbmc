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
  return m_FilePosition;
}
__int64 CFile7z::GetLength()
{
  return m_FileLength;
}

bool CFile7z::Open(const CURL& url, bool bBinary)
{
  if (!bBinary)
    return false;

  m_strArchive = url.GetHostName();
  CStdString strOptions = url.GetOptions();
  m_strPathInArchive = url.GetFileName();

  m_FilePosition = 0;
  m_FileLength   = 0;

  m_Processed = 0;

  m_Open         = false;
  const CCache *File = NULL;
  if (g_7zManager.Cache(m_strArchive) && (File = g_7zManager.GetFileIn7z(m_strArchive, m_strPathInArchive)) != NULL)
  {
    m_FileLength       = File->Size();
    Byte    *Buffer    = 0;
    size_t  BufferSize = 0;
    m_ExtractInfo      = new CFile7zExtractThread(m_strArchive, m_strPathInArchive, &Buffer, &BufferSize);
    m_ExtractThread    = new CThread(m_ExtractInfo);
    if (m_ExtractThread != NULL)
    {
      m_Open = true;
      m_ExtractThread->Create();
    }
    m_CacheEntry = g_CacheManager.GetCacheEntry(m_strArchive);
  }

  if (m_Open)
  {
    while (!m_ExtractInfo->m_Ready);
    { }
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

  if ((m_ExtractInfo->m_TempOut->GetLength() - m_FilePosition) < CRITICAL_BUFFER)
  {
    while ((m_ExtractInfo->m_TempOut->GetLength() - m_FilePosition) < MINIMUM_BUFFER)
      sleep(1);
  }
  int ret = m_ExtractInfo->m_TempOut->Read(lpBuf, uiBufSize);
  m_FilePosition = m_FilePosition + ret;
  return ret;
}

int	CFile7z::Write(const void* lpBuf, __int64 uiBufSize)
{
  return -1;
}

__int64	CFile7z::Seek(__int64 iFilePosition, int iWhence)
{
  return m_ExtractInfo->m_TempOut->Seek(iFilePosition, iWhence);
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
