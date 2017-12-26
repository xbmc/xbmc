/*
 *      Copyright (C) 2014 Team XBMC
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

#include "LocalFile.h"

#if defined(TARGET_WINDOWS) || defined(TARGET_WINDOWS_STORE)
#include "platform/win32/LocalFileImpl.h"
#elif defined(TARGET_POSIX)
#include "platform/posix/LocalFileImpl.h"
#else
#error "No implementation yet"
#endif

namespace KODI
{
namespace PLATFORM
{

using DETAILS::CLocalFileImpl;

CLocalFile::CLocalFile()
    : m_pimpl(new CLocalFileImpl())
{
}

CLocalFile::~CLocalFile() = default;

bool CLocalFile::Open(const CURL &url)
{
  return m_pimpl->Open(url);
}

bool CLocalFile::Open(const std::string &url)
{
  return m_pimpl->Open(url);
}

bool CLocalFile::OpenForWrite(const CURL &url, bool bOverWrite /*= false*/)
{
  return m_pimpl->OpenForWrite(url, bOverWrite);
}

bool CLocalFile::OpenForWrite(const std::string &url, bool bOverWrite /*= false*/)
{
  return m_pimpl->OpenForWrite(url, bOverWrite);
}

void CLocalFile::Close()
{
  m_pimpl->Close();
}

int64_t CLocalFile::Read(void *lpBuf, size_t uiBufSize)
{
  return m_pimpl->Read(lpBuf, uiBufSize);
}

int64_t CLocalFile::Write(const void *lpBuf, size_t uiBufSize)
{
  return m_pimpl->Write(lpBuf, uiBufSize);
}

int64_t CLocalFile::Seek(int64_t iFilePosition, int iWhence /*= SEEK_SET*/)
{
  return m_pimpl->Seek(iFilePosition, iWhence);
}

int CLocalFile::Truncate(int64_t toSize)
{
  return m_pimpl->Truncate(toSize);
}

int64_t CLocalFile::GetPosition()
{
  return m_pimpl->GetPosition();
}

int64_t CLocalFile::GetLength()
{
  return m_pimpl->GetLength();
}

void CLocalFile::Flush()
{
  m_pimpl->Flush();
}

bool CLocalFile::Delete(const CURL &url)
{
  return CLocalFileImpl::Delete(url);
}

bool CLocalFile::Delete(const std::string &url)
{
  return CLocalFileImpl::Delete(url);
}

bool CLocalFile::Rename(const CURL &urlCurrentName, const CURL &urlNewName)
{
  return CLocalFileImpl::Rename(urlCurrentName, urlNewName);
}

bool CLocalFile::Rename(const std::string &urlCurrentName, const std::string &urlNewName)
{
  return CLocalFileImpl::Rename(urlCurrentName, urlNewName);
}

bool CLocalFile::SetHidden(const CURL &url, bool hidden)
{
  return CLocalFileImpl::SetHidden(url, hidden);
}

bool CLocalFile::SetHidden(const std::string &url, bool hidden)
{
  return CLocalFileImpl::SetHidden(url, hidden);
}

bool CLocalFile::Exists(const CURL &url)
{
  return CLocalFileImpl::Exists(url);
}

bool CLocalFile::Exists(const std::string &url)
{
  return CLocalFileImpl::Exists(url);
}

int CLocalFile::Stat(const CURL &url, struct __stat64 *buffer)
{
  return CLocalFileImpl::Stat(url, buffer);
}

int CLocalFile::Stat(const std::string &url, struct __stat64 *buffer)
{
  return CLocalFileImpl::Stat(url, buffer);
}

std::string CLocalFile::GetSystemTempFilename(std::string suffix)
{
  return CLocalFileImpl::GetSystemTempFilename(suffix);
}

int CLocalFile::Stat(struct __stat64 *buffer)
{
  return m_pimpl->Stat(buffer);
}
}
}
