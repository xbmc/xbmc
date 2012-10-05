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

#include "system.h"
#include "HDFile.h"
#include "Util.h"
#include "URL.h"
#include "utils/AliasShortcutUtils.h"
#ifdef _LINUX
#include "XHandle.h"
#endif

#include <sys/stat.h>
#ifdef _LINUX
#include <sys/ioctl.h>
#else
#include <io.h>
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#endif
#include "utils/log.h"


using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CHDFile::CHDFile()
    : m_hFile(INVALID_HANDLE_VALUE)
{}

//*********************************************************************************************
CHDFile::~CHDFile()
{
  if (m_hFile != INVALID_HANDLE_VALUE) Close();
}
//*********************************************************************************************
CStdString CHDFile::GetLocal(const CURL &url)
{
  CStdString path( url.GetFileName() );

  if( url.GetProtocol().Equals("file", false) )
  {
    // file://drive[:]/path
    // file:///drive:/path
    CStdString host( url.GetHostName() );

    if(host.size() > 0)
    {
      if(host.Right(1) == ":")
        path = host + "/" + path;
      else
        path = host + ":/" + path;
    }
  }

#ifndef _LINUX
  path.Replace('/', '\\');
#endif

  if (IsAliasShortcut(path))
    TranslateAliasShortcut(path);

  return path;
}

//*********************************************************************************************
bool CHDFile::Open(const CURL& url)
{
  CStdString strFile = GetLocal(url);

#ifdef _WIN32
  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  m_hFile.attach(CreateFileW(strWFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
#else
  m_hFile.attach(CreateFile(strFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
#endif
  if (!m_hFile.isValid()) return false;

  m_i64FilePos = 0;
  m_i64FileLen = 0;

  return true;
}

bool CHDFile::Exists(const CURL& url)
{
  struct __stat64 buffer;
  CStdString strFile = GetLocal(url);

#ifdef _WIN32
  CStdStringW strWFile;
  URIUtils::RemoveSlashAtEnd(strFile);
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  return (_wstat64(strWFile.c_str(), &buffer)==0);
#else
  return (_stat64(strFile.c_str(), &buffer)==0);
#endif
}

int CHDFile::Stat(struct __stat64* buffer)
{
#ifdef _LINUX
  return _fstat64((*m_hFile).fd, buffer);
#else
  // Duplicate the handle, as retrieving and closing a matching crt handle closes the crt handle AND the original Windows handle.
  HANDLE hFileDup;
  if (0 == DuplicateHandle(GetCurrentProcess(), (HANDLE)m_hFile, GetCurrentProcess(), &hFileDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - DuplicateHandle()");
    return -1;
  }

  int fd;
  fd = _open_osfhandle((intptr_t)((HANDLE)hFileDup), 0);
  if (fd == -1)
  {
    CLog::Log(LOGERROR, "Stat: fd == -1");
    return -1;
  }
  int result = _fstat64(fd, buffer);
  _close(fd);
  return result;
#endif
}

int CHDFile::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strFile = GetLocal(url);

#ifdef _WIN32
  CStdStringW strWFile;
  // win32 can only stat root drives with a slash at the end
  if(strFile.length() == 2 && strFile[1] ==':')
    URIUtils::AddSlashAtEnd(strFile);
  /* _wstat64 calls FindFirstFileEx. According to MSDN, the path should not end in a trailing backslash.
    Remove it before calling _wstat64 */
  if (strFile.length() > 3 && URIUtils::HasSlashAtEnd(strFile))
    URIUtils::RemoveSlashAtEnd(strFile);
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  return _wstat64(strWFile.c_str(), buffer);
#else
  return _stat64(strFile.c_str(), buffer);
#endif
}

bool CHDFile::SetHidden(const CURL &url, bool hidden)
{
#ifdef _WIN32
  CStdStringW path;
  g_charsetConverter.utf8ToW(GetLocal(url), path, false);
  DWORD attributes = hidden ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
  if (SetFileAttributesW(path.c_str(), attributes))
    return true;
#endif
  return false;
}

//*********************************************************************************************
bool CHDFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  // make sure it's a legal FATX filename (we are writing to the harddisk)
  CStdString strPath = GetLocal(url);

#ifdef _WIN32
  CStdStringW strWPath;
  g_charsetConverter.utf8ToW(strPath, strWPath, false);
  m_hFile.attach(CreateFileW(strWPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
#else
  m_hFile.attach(CreateFile(strPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
#endif
  if (!m_hFile.isValid())
    return false;

  m_i64FilePos = 0;
  Seek(0, SEEK_SET);

  return true;
}

//*********************************************************************************************
unsigned int CHDFile::Read(void *lpBuf, int64_t uiBufSize)
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
int CHDFile::Write(const void *lpBuf, int64_t uiBufSize)
{
  if (!m_hFile.isValid())
    return 0;

  DWORD nBytesWriten;
  if ( WriteFile((HANDLE)m_hFile, (void*) lpBuf, (DWORD)uiBufSize, &nBytesWriten, NULL) )
    return nBytesWriten;

  return 0;
}

//*********************************************************************************************
void CHDFile::Close()
{
  m_hFile.reset();
}

//*********************************************************************************************
int64_t CHDFile::Seek(int64_t iFilePosition, int iWhence)
{
  LARGE_INTEGER lPos, lNewPos;
  lPos.QuadPart = iFilePosition;
  int bSuccess;

  switch (iWhence)
  {
  case SEEK_SET:
    bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_BEGIN);
    break;

  case SEEK_CUR:
    bSuccess = SetFilePointerEx((HANDLE)m_hFile, lPos, &lNewPos, FILE_CURRENT);
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
int64_t CHDFile::GetLength()
{
  if(m_i64FileLen <= m_i64FilePos || m_i64FileLen == 0)
  {
    LARGE_INTEGER i64Size;
    if(GetFileSizeEx((HANDLE)m_hFile, &i64Size))
      m_i64FileLen = i64Size.QuadPart;
    else
      CLog::Log(LOGERROR, "CHDFile::GetLength - GetFileSizeEx failed with error %d", GetLastError());
  }
  return m_i64FileLen;
}

//*********************************************************************************************
int64_t CHDFile::GetPosition()
{
  return m_i64FilePos;
}

bool CHDFile::Delete(const CURL& url)
{
  CStdString strFile=GetLocal(url);

#ifdef _WIN32
  CStdStringW strWFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  return ::DeleteFileW(strWFile.c_str()) ? true : false;
#else
  return ::DeleteFile(strFile.c_str()) ? true : false;
#endif
}

bool CHDFile::Rename(const CURL& url, const CURL& urlnew)
{
  CStdString strFile=GetLocal(url);
  CStdString strNewFile=GetLocal(urlnew);

#ifdef _WIN32
  CStdStringW strWFile;
  CStdStringW strWNewFile;
  g_charsetConverter.utf8ToW(strFile, strWFile, false);
  g_charsetConverter.utf8ToW(strNewFile, strWNewFile, false);
  return ::MoveFileW(strWFile.c_str(), strWNewFile.c_str()) ? true : false;
#else
  return ::MoveFile(strFile.c_str(), strNewFile.c_str()) ? true : false;
#endif
}

void CHDFile::Flush()
{
  ::FlushFileBuffers(m_hFile);
}

int CHDFile::IoControl(EIoControl request, void* param)
{
#ifdef _LINUX
  if(request == IOCTRL_NATIVE && param)
  {
    SNativeIoControl* s = (SNativeIoControl*)param;
    return ioctl((*m_hFile).fd, s->request, s->param);
  }
#endif
  return -1;
}

int CHDFile::Truncate(int64_t size)
{
#ifdef _WIN32
  // Duplicate the handle, as retrieving and closing a matching crt handle closes the crt handle AND the original Windows handle.
  HANDLE hFileDup;
  if (0 == DuplicateHandle(GetCurrentProcess(), (HANDLE)m_hFile, GetCurrentProcess(), &hFileDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - DuplicateHandle()");
    return -1;
  }

  int fd;
  fd = _open_osfhandle((intptr_t)((HANDLE)hFileDup), 0);
  if (fd == -1)
  {
    CLog::Log(LOGERROR, "Stat: fd == -1");
    return -1;
  }
  int result = _chsize_s(fd, (long) size);
  _close(fd);
  return result;
#else
  return ftruncate((*m_hFile).fd, (off_t) size);
#endif
  return -1;
}