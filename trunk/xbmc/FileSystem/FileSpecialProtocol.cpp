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

bool CFileSpecialProtocol::Open(const CURL& url)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url.Get());

  return m_file.Open(strFileName);
}

bool CFileSpecialProtocol::OpenForWrite(const CURL& url, bool bOverWrite /*=false */)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url.Get());

  return m_file.OpenForWrite(strFileName,bOverWrite);
}

bool CFileSpecialProtocol::Delete(const CURL& url)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url.Get());
  
  return m_file.Delete(strFileName);
}

bool CFileSpecialProtocol::Exists(const CURL& url)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url.Get());

  return m_file.Exists(strFileName);
}

int CFileSpecialProtocol::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url.Get());

  return m_file.Stat(strFileName, buffer);
}

bool CFileSpecialProtocol::Rename(const CURL& url, const CURL& urlnew)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url.Get());
  CStdString strFileName2=CSpecialProtocol::TranslatePath(urlnew.Get());

  return m_file.Rename(strFileName,strFileName2);
}

int CFileSpecialProtocol::Stat(struct __stat64* buffer)
{
  return m_file.Stat(buffer);
}

unsigned int CFileSpecialProtocol::Read(void* lpBuf, int64_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}
  
int CFileSpecialProtocol::Write(const void* lpBuf, int64_t uiBufSize)
{
  return m_file.Write(lpBuf,uiBufSize);
}

int64_t CFileSpecialProtocol::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CFileSpecialProtocol::Close()
{
  m_file.Close();
}

int64_t CFileSpecialProtocol::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CFileSpecialProtocol::GetLength()
{
  return m_file.GetLength();
}



