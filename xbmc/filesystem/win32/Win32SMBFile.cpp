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

#ifdef TARGET_WINDOWS
#include "Win32SMBFile.h"
#include "Win32SMBDirectory.h"
#include "URL.h"
#include "win32/WIN32Util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>

using namespace XFILE;

// local helper
static bool worthTryToConnect(const DWORD lastErr)
{
  return lastErr != ERROR_INVALID_DATA && // used to indicate internal errors
         lastErr != ERROR_FILE_NOT_FOUND      && lastErr != ERROR_BAD_NET_NAME  &&
         lastErr != ERROR_NO_NET_OR_BAD_PATH  && lastErr != ERROR_NO_NETWORK    &&
         lastErr != ERROR_BAD_NETPATH;
}


bool CWin32SMBFile::ConnectAndAuthenticate(const CURL& url)
{
  CURL authUrl(url);
  CWin32SMBDirectory smbDir;
  return smbDir.ConnectAndAuthenticate(authUrl, false);
}

CWin32SMBFile::CWin32SMBFile() : CWin32File(true)
{ }

CWin32SMBFile::~CWin32SMBFile()
{ /* cleanup by CWin32File destructor */ }

bool CWin32SMBFile::Open(const CURL& url)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files
  if (CWin32File::Open(url))
    return true;

  return worthTryToConnect(m_lastSMBFileErr) && ConnectAndAuthenticate(url) && CWin32File::Open(url);
}

bool CWin32SMBFile::OpenForWrite(const CURL& url, bool bOverWrite /*= false*/)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files
  if (CWin32File::OpenForWrite(url, bOverWrite))
    return true;

  return worthTryToConnect(m_lastSMBFileErr) && ConnectAndAuthenticate(url) && CWin32File::OpenForWrite(url, bOverWrite);
}

bool CWin32SMBFile::Delete(const CURL& url)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files

  if (CWin32File::Delete(url))
    return true;

  return worthTryToConnect(m_lastSMBFileErr) && ConnectAndAuthenticate(url) && CWin32File::Delete(url);
}

bool CWin32SMBFile::Rename(const CURL& urlCurrentName, const CURL& urlNewName)
{
  assert(urlCurrentName.IsProtocol("smb")); // function suitable only for SMB files
  assert(urlNewName.IsProtocol("smb")); // function suitable only for SMB files

  if (CWin32File::Rename(urlCurrentName, urlNewName))
    return true;

  return worthTryToConnect(m_lastSMBFileErr) && ConnectAndAuthenticate(urlCurrentName) && ConnectAndAuthenticate(urlNewName) &&
    CWin32File::Rename(urlCurrentName, urlNewName);
}

bool CWin32SMBFile::SetHidden(const CURL& url, bool hidden)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files

  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  DWORD attrs = GetFileAttributesW(pathnameW.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES)
  {
    if (!worthTryToConnect(GetLastError()) || !ConnectAndAuthenticate(url))
      return false;
    attrs = GetFileAttributesW(pathnameW.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
      return false;
  }
  
  if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
    return false;

  // check whether attribute is already set/cleared
  if (((attrs & FILE_ATTRIBUTE_HIDDEN) != 0) == hidden)
    return true;

  if (hidden)
    return SetFileAttributesW(pathnameW.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN) != 0;
  return SetFileAttributesW(pathnameW.c_str(), attrs & ~FILE_ATTRIBUTE_HIDDEN) != 0;
}

bool CWin32SMBFile::Exists(const CURL& url)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files
  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  DWORD attrs = GetFileAttributesW(pathnameW.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES)
  {
    if (!worthTryToConnect(GetLastError()) || !ConnectAndAuthenticate(url))
      return false;
    attrs = GetFileAttributesW(pathnameW.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
      return false;
  }
  return (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int CWin32SMBFile::Stat(const CURL& url, struct __stat64* statData)
{
  assert(url.IsProtocol("smb")); // function suitable only for SMB files

  if (CWin32File::Stat(url, statData) == 0)
    return 0;

  if (!worthTryToConnect(m_lastSMBFileErr) || !ConnectAndAuthenticate(url))
    return -1;

  return CWin32File::Stat(url, statData);
}


#endif // TARGET_WINDOWS
