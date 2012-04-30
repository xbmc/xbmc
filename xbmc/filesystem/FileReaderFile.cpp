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
#include "FileReaderFile.h"
#include "URL.h"

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CFileReaderFile::CFileReaderFile()
{
}

//*********************************************************************************************
CFileReaderFile::~CFileReaderFile()
{
  Close();
}

//*********************************************************************************************
bool CFileReaderFile::Open(const CURL& url)
{
  CStdString strURL = url.Get();
  strURL = strURL.Mid(13);
  return m_reader.Open(strURL,READ_CACHED);
}

bool CFileReaderFile::Exists(const CURL& url)
{
  return CFile::Exists(url.Get().Mid(13));
}

int CFileReaderFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return CFile::Stat(url.Get().Mid(13),buffer);
}


//*********************************************************************************************
bool CFileReaderFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  return false;
}

//*********************************************************************************************
unsigned int CFileReaderFile::Read(void *lpBuf, int64_t uiBufSize)
{
  return m_reader.Read(lpBuf,uiBufSize);
}

//*********************************************************************************************
int CFileReaderFile::Write(const void *lpBuf, int64_t uiBufSize)
{
  return 0;
}

//*********************************************************************************************
void CFileReaderFile::Close()
{
  m_reader.Close();
}

//*********************************************************************************************
int64_t CFileReaderFile::Seek(int64_t iFilePosition, int iWhence)
{
  return m_reader.Seek(iFilePosition,iWhence);
}

//*********************************************************************************************
int64_t CFileReaderFile::GetLength()
{
  return m_reader.GetLength();
}

//*********************************************************************************************
int64_t CFileReaderFile::GetPosition()
{
  return m_reader.GetPosition();
}


