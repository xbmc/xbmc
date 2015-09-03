/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
 *
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "UDFFile.h"
#include "URL.h"

#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

using namespace std;
using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CUDFFile::CUDFFile()
  : m_bOpened(false)
  , m_hFile(INVALID_HANDLE_VALUE)
{
}

//*********************************************************************************************
CUDFFile::~CUDFFile()
{
  if (m_bOpened)
  {
    Close();
  }
}
//*********************************************************************************************
bool CUDFFile::Open(const CURL& url)
{
  if(!m_udfIsoReaderLocal.Open(url.GetHostName().c_str()) || url.GetFileName().empty())
     return false;

  m_hFile = m_udfIsoReaderLocal.OpenFile(url.GetFileName().c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
  {
    m_bOpened = false;
    return false;
  }

  m_bOpened = true;
  return true;
}

//*********************************************************************************************
ssize_t CUDFFile::Read(void *lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;
  if (uiBufSize > LONG_MAX)
    uiBufSize = LONG_MAX;

  if (!m_bOpened)
    return -1;
  char *pData = (char *)lpBuf;

  return m_udfIsoReaderLocal.ReadFile( m_hFile, (unsigned char*)pData, (long)uiBufSize);
}

//*********************************************************************************************
void CUDFFile::Close()
{
  if (!m_bOpened) return ;
  m_udfIsoReaderLocal.CloseFile( m_hFile);
  m_bOpened = false;
}

//*********************************************************************************************
int64_t CUDFFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  int64_t lNewPos = m_udfIsoReaderLocal.Seek(m_hFile, iFilePosition, iWhence);
  return lNewPos;
}

//*********************************************************************************************
int64_t CUDFFile::GetLength()
{
  if (!m_bOpened) return -1;
  return m_udfIsoReaderLocal.GetFileSize(m_hFile);
}

//*********************************************************************************************
int64_t CUDFFile::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_udfIsoReaderLocal.GetFilePosition(m_hFile);
}

bool CUDFFile::Exists(const CURL& url)
{
  if(!m_udfIsoReaderLocal.Open(url.GetHostName().c_str()))
     return false;

  m_hFile = m_udfIsoReaderLocal.OpenFile(url.GetFileName().c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  m_udfIsoReaderLocal.CloseFile(m_hFile);
  m_hFile = INVALID_HANDLE_VALUE;
  return true;
}

int CUDFFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if(!m_udfIsoReaderLocal.Open(url.GetHostName().c_str()))
     return -1;

  if (url.GetFileName().empty())
  {
    buffer->st_mode = _S_IFDIR;
    return 0;
  }

  m_hFile = m_udfIsoReaderLocal.OpenFile(url.GetFileName().c_str());
  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    buffer->st_size = m_udfIsoReaderLocal.GetFileSize(m_hFile);
    buffer->st_mode = _S_IFREG;
    m_udfIsoReaderLocal.CloseFile(m_hFile);
    return 0;
  }
  errno = ENOENT;
  return -1;
}
