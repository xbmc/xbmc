/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
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
  // URL is of the form filereader://<foo>
  std::string strURL = url.Get();
  strURL = strURL.substr(13);
  return m_reader.Open(strURL,READ_CACHED);
}

bool CFileReaderFile::Exists(const CURL& url)
{
  return CFile::Exists(url.Get().substr(13));
}

int CFileReaderFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return CFile::Stat(url.Get().substr(13), buffer);
}


//*********************************************************************************************
bool CFileReaderFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  return false;
}

//*********************************************************************************************
ssize_t CFileReaderFile::Read(void *lpBuf, size_t uiBufSize)
{
  return m_reader.Read(lpBuf,uiBufSize);
}

//*********************************************************************************************
ssize_t CFileReaderFile::Write(const void *lpBuf, size_t uiBufSize)
{
  return -1;
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


