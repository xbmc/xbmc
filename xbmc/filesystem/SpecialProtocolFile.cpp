/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SpecialProtocolFile.h"
#include "SpecialProtocol.h"
#include "URL.h"

#include <sys/stat.h>

using namespace XFILE;

CSpecialProtocolFile::CSpecialProtocolFile(void)
{
}

CSpecialProtocolFile::~CSpecialProtocolFile(void)
{
  Close();
}

bool CSpecialProtocolFile::Open(const CURL& url)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url);

  return m_file.Open(strFileName);
}

bool CSpecialProtocolFile::OpenForWrite(const CURL& url, bool bOverWrite /*=false */)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url);

  return m_file.OpenForWrite(strFileName,bOverWrite);
}

bool CSpecialProtocolFile::Delete(const CURL& url)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url);
  
  return m_file.Delete(strFileName);
}

bool CSpecialProtocolFile::Exists(const CURL& url)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url);

  return m_file.Exists(strFileName);
}

int CSpecialProtocolFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url);

  return m_file.Stat(strFileName, buffer);
}

bool CSpecialProtocolFile::Rename(const CURL& url, const CURL& urlnew)
{
  CStdString strFileName=CSpecialProtocol::TranslatePath(url);
  CStdString strFileName2=CSpecialProtocol::TranslatePath(urlnew);

  return m_file.Rename(strFileName,strFileName2);
}

int CSpecialProtocolFile::Stat(struct __stat64* buffer)
{
  return m_file.Stat(buffer);
}

unsigned int CSpecialProtocolFile::Read(void* lpBuf, int64_t uiBufSize)
{
  return m_file.Read(lpBuf, uiBufSize);
}
  
int CSpecialProtocolFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  return m_file.Write(lpBuf,uiBufSize);
}

int64_t CSpecialProtocolFile::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
{
  return m_file.Seek(iFilePosition, iWhence);
}

void CSpecialProtocolFile::Close()
{
  m_file.Close();
}

int64_t CSpecialProtocolFile::GetPosition()
{
  return m_file.GetPosition();
}

int64_t CSpecialProtocolFile::GetLength()
{
  return m_file.GetLength();
}



