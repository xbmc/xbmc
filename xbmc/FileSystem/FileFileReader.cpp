
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
#include "FileFileReader.h"
#include "../Util.h"

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CFileFileReader::CFileFileReader()
{
}

//*********************************************************************************************
CFileFileReader::~CFileFileReader()
{
  Close();
}

//*********************************************************************************************
bool CFileFileReader::Open(const CURL& url, bool bBinary)
{
  CStdString strURL;
  url.GetURL(strURL);
  strURL = strURL.Mid(13);
  return m_reader.Open(strURL,false,true);
}

bool CFileFileReader::Exists(const CURL& url)
{
  CStdString strURL;
  url.GetURL(strURL);
  strURL = strURL.Mid(13);
  
  return CFile::Exists(strURL);
}

int CFileFileReader::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strURL;
  url.GetURL(strURL);
  strURL = strURL.Mid(13);
  
  return CFile::Stat(strURL,buffer);
}


//*********************************************************************************************
bool CFileFileReader::OpenForWrite(const CURL& url, bool bBinary, bool bOverWrite)
{
  return false;
}

//*********************************************************************************************
unsigned int CFileFileReader::Read(void *lpBuf, __int64 uiBufSize)
{
  return m_reader.Read(lpBuf,uiBufSize);
}

//*********************************************************************************************
int CFileFileReader::Write(const void *lpBuf, __int64 uiBufSize)
{
  return 0;
}

//*********************************************************************************************
void CFileFileReader::Close()
{
  m_reader.Close();
}

//*********************************************************************************************
__int64 CFileFileReader::Seek(__int64 iFilePosition, int iWhence)
{
  return m_reader.Seek(iFilePosition,iWhence);
}

//*********************************************************************************************
__int64 CFileFileReader::GetLength()
{
  return m_reader.GetLength();
}

//*********************************************************************************************
__int64 CFileFileReader::GetPosition()
{
  return m_reader.GetPosition();
}
