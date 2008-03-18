#include "stdafx.h" 
/*
 * XBoxMediaPlayer
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
#include "FileSndtrk.h"


using namespace XFILE;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CFileSndtrk::CFileSndtrk()
    : m_hFile(INVALID_HANDLE_VALUE)
{}

//*********************************************************************************************
CFileSndtrk::~CFileSndtrk()
{
}
//*********************************************************************************************
bool CFileSndtrk::Open(const CURL& url, bool bBinary)
{
  m_hFile.attach( CreateFile(url.GetFileName(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));
  if ( !m_hFile.isValid() ) return false;

  m_i64FilePos = 0;
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;
  Seek(0, SEEK_SET);

  return true;
}
//*********************************************************************************************
bool CFileSndtrk::OpenForWrite(const char* strFileName)
{
  m_hFile.attach(CreateFile(strFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
  if (!m_hFile.isValid()) return false;

  m_i64FilePos = 0;
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  m_i64FileLength = i64Size.QuadPart;
  Seek(0, SEEK_SET);

  return true;
}

//*********************************************************************************************
unsigned int CFileSndtrk::Read(void *lpBuf, __int64 uiBufSize)
{
  if (!m_hFile.isValid()) return 0;
  DWORD nBytesRead;
  if ( ReadFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &nBytesRead, NULL) )
  {
    m_i64FilePos += nBytesRead;
    return nBytesRead;
  }
  return 0;
}

//*********************************************************************************************
unsigned int CFileSndtrk::Write(void *lpBuf, __int64 uiBufSize)
{
  if (!m_hFile.isValid()) return 0;
  DWORD nBytesWriten;
  if ( WriteFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &nBytesWriten, NULL) )
  {
    return nBytesWriten;
  }
  return 0;
}

//*********************************************************************************************
void CFileSndtrk::Close()
{
  m_hFile.reset();
}

//*********************************************************************************************
__int64 CFileSndtrk::Seek(__int64 iFilePosition, int iWhence)
{
  LARGE_INTEGER lPos, lNewPos;
  lPos.QuadPart = iFilePosition;
  switch (iWhence)
  {
  case SEEK_SET:
    SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_BEGIN);
    break;

  case SEEK_CUR:
    SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_CURRENT);
    break;

  case SEEK_END:
    SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_END);
    break;
  default:
    return -1;
  }
  m_i64FilePos = lNewPos.QuadPart;
  return (lNewPos.QuadPart);
}

//*********************************************************************************************
__int64 CFileSndtrk::GetLength()
{
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  return i64Size.QuadPart;

  // return m_i64FileLength;
}

//*********************************************************************************************
__int64 CFileSndtrk::GetPosition()
{
  return m_i64FilePos;
}


int CFileSndtrk::Write(const void* lpBuf, __int64 uiBufSize)
{
  if (!m_hFile.isValid()) return -1;
  DWORD dwNumberOfBytesWritten = 0;
  WriteFile((HANDLE)m_hFile, lpBuf, (DWORD)uiBufSize, &dwNumberOfBytesWritten, NULL);
  return (int)dwNumberOfBytesWritten;
}
