
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
#include "WINFileSMB.h"
#include "URL.h"
#include "settings/GUISettings.h"

#include <sys/stat.h>
#include <io.h>
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#include "WINSMBDirectory.h"

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CWINFileSMB::CWINFileSMB()
    : m_hFile(INVALID_HANDLE_VALUE)
{}

//*********************************************************************************************
CWINFileSMB::~CWINFileSMB()
{
  if (m_hFile != INVALID_HANDLE_VALUE) Close();
}
//*********************************************************************************************
CStdString CWINFileSMB::GetLocal(const CURL &url)
{
  CStdString path( url.GetFileName() );

  if( url.GetProtocol().Equals("smb", false) )
  {
    CStdString host( url.GetHostName() );

    if(host.size() > 0)
    {
      path = "//" + host + "/" + path;
    }
  }

  path.Replace('/', '\\');

  return path;
}

//*********************************************************************************************
bool CWINFileSMB::Open(const CURL& url)
{
  CStdString strFile = GetLocal(url);

  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  m_hFile.attach(CreateFileW(strWFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));

  if (!m_hFile.isValid())
  {
    if(GetLastError() == ERROR_FILE_NOT_FOUND)
      return false;

    XFILE::CWINSMBDirectory smb;
    smb.ConnectToShare(url);
    m_hFile.attach(CreateFileW(strWFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
    if (!m_hFile.isValid())
    {
      CLog::Log(LOGERROR,"CWINFileSMB: Unable to open file %s Error: %d", strFile.c_str(), GetLastError());
      return false;
    }
  }

  m_i64FilePos = 0;
  Seek(0, SEEK_SET);

  return true;
}

bool CWINFileSMB::Exists(const CURL& url)
{
  struct __stat64 buffer;
  if(url.GetFileName() == url.GetShareName())
    return false;
  CStdString strFile = GetLocal(url);
  URIUtils::RemoveSlashAtEnd(strFile);
  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  if(_wstat64(strWFile.c_str(), &buffer) == 0)
    return true;

  if(errno == ENOENT)
    return false;

  XFILE::CWINSMBDirectory smb;
  if(smb.ConnectToShare(url) == false)
    return false;

  return (_wstat64(strWFile.c_str(), &buffer) == 0);
}

int CWINFileSMB::Stat(struct __stat64* buffer)
{
  int fd;
  HANDLE hFileDup;
  if (0 == DuplicateHandle(GetCurrentProcess(), (HANDLE)m_hFile, GetCurrentProcess(), &hFileDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - DuplicateHandle()");
    return -1;
  }

  fd = _open_osfhandle((intptr_t)((HANDLE)hFileDup), 0);
  if (fd == -1)
  {
    CLog::Log(LOGERROR, "CWINFileSMB Stat: fd == -1");
    return -1;
  }

  int result = _fstat64(fd, buffer);
  _close(fd);
  return result;
}

int CWINFileSMB::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFile = GetLocal(url);
  /* _wstat64 calls FindFirstFileEx. According to MSDN, the path should not end in a trailing backslash.
    Remove it before calling _wstat64 */
  if (strFile.length() > 3 && URIUtils::HasSlashAtEnd(strFile))
    URIUtils::RemoveSlashAtEnd(strFile);
  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  if(_wstat64(strWFile.c_str(), buffer) == 0)
    return 0;

  if(errno == ENOENT)
    return -1;

  XFILE::CWINSMBDirectory smb;
  if(smb.ConnectToShare(url) == false)
    return -1;

  return _wstat64(strWFile.c_str(), buffer);
}


//*********************************************************************************************
bool CWINFileSMB::OpenForWrite(const CURL& url, bool bOverWrite)
{
  CStdString strPath = GetLocal(url);

  CStdStringW strWPath;
  g_charsetConverter.utf8ToW(strPath, strWPath, false);
  m_hFile.attach(CreateFileW(strWPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));

  if (!m_hFile.isValid())
  {
    if(GetLastError() == ERROR_FILE_NOT_FOUND)
      return false;

    XFILE::CWINSMBDirectory smb;
    smb.ConnectToShare(url);
    m_hFile.attach(CreateFileW(strWPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
    if (!m_hFile.isValid())
    {
      CLog::Log(LOGERROR,"CWINFileSMB: Unable to open file for writing '%s' Error '%d%",strWPath.c_str(), GetLastError());
      return false;
    }
  }

  m_i64FilePos = 0;
  Seek(0, SEEK_SET);

  return true;
}

//*********************************************************************************************
unsigned int CWINFileSMB::Read(void *lpBuf, int64_t uiBufSize)
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
int CWINFileSMB::Write(const void *lpBuf, int64_t uiBufSize)
{
  if (!m_hFile.isValid())
    return 0;

  DWORD nBytesWriten;
  if ( WriteFile((HANDLE)m_hFile, (void*) lpBuf, (DWORD)uiBufSize, &nBytesWriten, NULL) )
    return nBytesWriten;

  return 0;
}

//*********************************************************************************************
void CWINFileSMB::Close()
{
  m_hFile.reset();
}

//*********************************************************************************************
int64_t CWINFileSMB::Seek(int64_t iFilePosition, int iWhence)
{
  LARGE_INTEGER lPos, lNewPos;
  lPos.QuadPart = iFilePosition;
  int bSuccess;

  int64_t length = GetLength();

  switch (iWhence)
  {
  case SEEK_SET:
    if (iFilePosition <= length || length == 0)
      bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_BEGIN);
    else
      bSuccess = false;
    break;

  case SEEK_CUR:
    if ((GetPosition()+iFilePosition) <= length || length == 0)
      bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_CURRENT);
    else
      bSuccess = false;
    break;

  case SEEK_END:
    bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_END);
    break;

  default:
    return -1;
  }
  if (bSuccess)
  {
    m_i64FilePos = lNewPos.QuadPart;
    return m_i64FilePos;
  }
  else
    return -1;
}

//*********************************************************************************************
int64_t CWINFileSMB::GetLength()
{
  LARGE_INTEGER i64Size;
  GetFileSizeEx((HANDLE)m_hFile, &i64Size);
  return i64Size.QuadPart;
}

//*********************************************************************************************
int64_t CWINFileSMB::GetPosition()
{
  return m_i64FilePos;
}

bool CWINFileSMB::Delete(const CURL& url)
{
  CStdString strFile=GetLocal(url);

  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  return ::DeleteFileW(strWFile.c_str()) ? true : false;
}

bool CWINFileSMB::Rename(const CURL& url, const CURL& urlnew)
{
  CStdString strFile=GetLocal(url);
  CStdString strNewFile=GetLocal(urlnew);

  CStdStringW strWFile;
  CStdStringW strWNewFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  g_charsetConverter.utf8ToW(strNewFile, strWNewFile, false);
  return ::MoveFileW(strWFile.c_str(), strWNewFile.c_str()) ? true : false;
}

bool CWINFileSMB::SetHidden(const CURL &url, bool hidden)
{
  CStdStringW path;
  g_charsetConverter.utf8ToW(GetLocal(url), path, false);
  DWORD attributes = hidden ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
  if (SetFileAttributesW(path.c_str(), attributes))
    return true;
  return false;
}

void CWINFileSMB::Flush()
{
  ::FlushFileBuffers(m_hFile);
}

int CWINFileSMB::IoControl(EIoControl request, void* param)
{
  return -1;
}

int CWINFileSMB::Truncate(int64_t size)
{
  int fd;
  HANDLE hFileDup;
  if (0 == DuplicateHandle(GetCurrentProcess(), (HANDLE)m_hFile, GetCurrentProcess(), &hFileDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - DuplicateHandle()");
    return -1;
  }

  fd = _open_osfhandle((intptr_t)((HANDLE)hFileDup), 0);
  if (fd == -1)
  {
    CLog::Log(LOGERROR, "CWINFileSMB Stat: fd == -1");
    return -1;
  }
  int result = _chsize_s(fd, (long) size);
  _close(fd);
  return result;
}