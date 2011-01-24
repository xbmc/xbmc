/*
 *      Copyright (C) 2010 Team Boxee
 *      http://www.boxee.tv
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

#include "FileUDF.h"
#include "URL.h"
#include "Util.h"

#include <sys/stat.h>
#include <errno.h>

using namespace std;
using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//*********************************************************************************************
CFileUDF::CFileUDF()
{
  m_bOpened = false;
}

//*********************************************************************************************
CFileUDF::~CFileUDF()
{
  if (m_bOpened)
  {
    Close();
  }
}
//*********************************************************************************************
bool CFileUDF::Open(const CURL& url)
{
  CStdString strFName = url.GetHostName();

  CURL::Decode(strFName);

  m_hFile = m_udfIsoReaderLocal.OpenFile((char*)strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
  {
    m_bOpened = false;
    return false;
  }

  m_bOpened = true;
  return true;
}

//*********************************************************************************************
unsigned int CFileUDF::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_bOpened) return 0;
  char *pData = (char *)lpBuf;

  int iResult = m_udfIsoReaderLocal.ReadFile( m_hFile, (unsigned char*)pData, (long)uiBufSize);
  if (iResult == -1)
    return 0;
  return iResult;
}

//*********************************************************************************************
void CFileUDF::Close()
{
  if (!m_bOpened) return ;
  m_udfIsoReaderLocal.CloseFile( m_hFile);
}

//*********************************************************************************************
int64_t CFileUDF::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  int64_t lNewPos = m_udfIsoReaderLocal.Seek(m_hFile, iFilePosition, iWhence);
  return lNewPos;
}

//*********************************************************************************************
int64_t CFileUDF::GetLength()
{
  if (!m_bOpened) return -1;
  return m_udfIsoReaderLocal.GetFileSize(m_hFile);
}

//*********************************************************************************************
int64_t CFileUDF::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_udfIsoReaderLocal.GetFilePosition(m_hFile);
}

bool CFileUDF::Exists(const CURL& url)
{
  string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_udfIsoReaderLocal.OpenFile((char*)strFName.c_str());
  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;

  m_udfIsoReaderLocal.CloseFile(m_hFile);
  return true;
}

int CFileUDF::Stat(const CURL& url, struct __stat64* buffer)
{
  string strFName = "\\";
  strFName += url.GetFileName();
  for (int i = 0; i < (int)strFName.size(); ++i )
  {
    if (strFName[i] == '/') strFName[i] = '\\';
  }
  m_hFile = m_udfIsoReaderLocal.OpenFile((char*)strFName.c_str());
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
