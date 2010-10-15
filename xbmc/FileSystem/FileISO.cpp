/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
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

#include "FileISO.h"
#include "URL.h"
#include "iso9660.h"

#include <sys/stat.h>

using namespace std;
using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CFileISO::CFileISO()
{
  m_bOpened = false;
}

//*********************************************************************************************
CFileISO::~CFileISO()
{
  if (m_bOpened)
  {
    Close();
  }
}
//*********************************************************************************************
bool CFileISO::Open(const CURL& url)
{
  string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_isoReader.OpenFile((char*)strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
  {
    m_bOpened = false;
    return false;
  }

  m_bOpened = true;
  return true;
}

//*********************************************************************************************
unsigned int CFileISO::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_bOpened) return 0;
  char *pData = (char *)lpBuf;

  if (m_cache.getSize() > 0)
  {
    long lTotalBytesRead = 0;
    while (uiBufSize > 0)
    {
      if (m_cache.getMaxReadSize() )
      {
        long lBytes2Read = m_cache.getMaxReadSize();
        if (lBytes2Read > uiBufSize) lBytes2Read = (long)uiBufSize;
        m_cache.ReadData(pData, lBytes2Read );
        uiBufSize -= lBytes2Read ;
        pData += lBytes2Read;
        lTotalBytesRead += lBytes2Read ;
      }

      if (m_cache.getMaxWriteSize() > 5000)
      {
        byte buffer[5000];
        long lBytesRead = m_isoReader.ReadFile( m_hFile, buffer, sizeof(buffer));
        if (lBytesRead > 0)
          m_cache.WriteData((char*)buffer, lBytesRead);
        else
          return 0;
      }
    }
    return lTotalBytesRead;
  }
  int iResult = m_isoReader.ReadFile( m_hFile, (byte*)pData, (long)uiBufSize);
  if (iResult == -1)
    return 0;
  return iResult;
}

//*********************************************************************************************
void CFileISO::Close()
{
  if (!m_bOpened) return ;
  m_isoReader.CloseFile( m_hFile);
}

//*********************************************************************************************
int64_t CFileISO::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  int64_t lNewPos = m_isoReader.Seek(m_hFile, iFilePosition, iWhence);
  if(lNewPos >= 0)
    m_cache.Clear();
  return lNewPos;
}

//*********************************************************************************************
int64_t CFileISO::GetLength()
{
  if (!m_bOpened) return -1;
  return m_isoReader.GetFileSize(m_hFile);
}

//*********************************************************************************************
int64_t CFileISO::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_isoReader.GetFilePosition(m_hFile);
}

bool CFileISO::Exists(const CURL& url)
{
  string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_isoReader.OpenFile((char*)strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  m_isoReader.CloseFile(m_hFile);
  return true;
}

int CFileISO::Stat(const CURL& url, struct __stat64* buffer)
{
  string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_isoReader.OpenFile((char*)strFName.c_str());
  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    memset(buffer, 0, sizeof(struct __stat64));
    buffer->st_size = m_isoReader.GetFileSize(m_hFile);
    buffer->st_mode = _S_IFREG;
    m_isoReader.CloseFile(m_hFile);
    return 0;
  }
  errno = ENOENT;
  return -1;
}
