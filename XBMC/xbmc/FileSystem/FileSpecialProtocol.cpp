/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "stdafx.h"
#include "FileSpecialProtocol.h"
#include "SpecialProtocol.h"
#include "URL.h"

#include <sys/stat.h>

using namespace XFILE;

CFileSpecialProtocol::CFileSpecialProtocol(void)
{
}

CFileSpecialProtocol::~CFileSpecialProtocol(void)
{
  Close();
}

bool CFileSpecialProtocol::Open(const CURL& url, bool bBinary /*=true*/)
{
  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CSpecialProtocol::TranslatePath(strPath);

  return m_file.Open(strFileName);
}

bool CFileSpecialProtocol::OpenForWrite(const CURL& url, bool bBinary /*=true*/, bool bOverWrite /*=false */)
{
  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CSpecialProtocol::TranslatePath(strPath);

  return m_file.OpenForWrite(strFileName,bBinary,bOverWrite);
}

bool CFileSpecialProtocol::Delete(const CURL& url)
{
  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CSpecialProtocol::TranslatePath(strPath);
  
  return m_file.Delete(strFileName);
}

bool CFileSpecialProtocol::Exists(const CURL& url)
{
  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CSpecialProtocol::TranslatePath(strPath);

  return m_file.Exists(strFileName);
}

int CFileSpecialProtocol::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strPath;
  url.GetURL(strPath);
  CStdString strFileName=CSpecialProtocol::TranslatePath(strPath);

  return m_file.Stat(strFileName, buffer);
}

int CFileSpecialProtocol::Stat(struct __stat64* buffer)
{
  return m_file.Stat(buffer);
}

unsigned int CFileSpecialProtocol::Read(void* lpBuf, __int64 uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}
  
int CFileSpecialProtocol::Write(const void* lpBuf, __int64 uiBufSize)
{
  return m_file.Write(lpBuf,uiBufSize);
}

__int64 CFileSpecialProtocol::Seek(__int64 iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CFileSpecialProtocol::Close()
{
  m_file.Close();
}

__int64 CFileSpecialProtocol::GetPosition()
{
  return m_file.GetPosition();
}

__int64 CFileSpecialProtocol::GetLength()
{
  return m_file.GetLength();
}


