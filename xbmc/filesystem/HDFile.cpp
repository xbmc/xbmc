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

#include "system.h"
#include "HDFile.h"
#include "Util.h"
#include "URL.h"
#include "utils/AliasShortcutUtils.h"
#ifdef TARGET_POSIX
#include "XHandle.h"
#endif

#include <sys/stat.h>
#ifdef TARGET_POSIX
#include <fcntl.h>
#include <sys/ioctl.h>
#else
#include <io.h>
#include "utils/CharsetConverter.h"
#include "utils/URIUtils.h"
#include "win32/WIN32Util.h"
#endif
#include "utils/log.h"

#include <algorithm>

using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CHDFile::CHDFile()
    : m_hFile(INVALID_HANDLE_VALUE),
      m_i64LastDropPos(0)
{}

//*********************************************************************************************
CHDFile::~CHDFile()
{
  if (m_hFile != INVALID_HANDLE_VALUE) Close();
}
//*********************************************************************************************
std::string CHDFile::GetLocal(const CURL &url)
{
  std::string path(url.GetFileName());

  if(url.GetProtocol() == "file")
  {
    // file://drive[:]/path
    // file:///drive:/path
    std::string host(url.GetHostName());

    if(!host.empty())
    {
      if(host[host.length()-1] == ':')
        path = host + "/" + path;
      else
        path = host + ":/" + path;
    }
  }

  if (IsAliasShortcut(path))
    TranslateAliasShortcut(path);

  return path;
}

//*********************************************************************************************
bool CHDFile::Open(const CURL& url)
{
  std::string strFile(GetLocal(url));

#ifdef TARGET_WINDOWS
  m_hFile.attach(CreateFileW(CWIN32Util::ConvertPathToWin32Form(strFile).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
#else
  m_hFile.attach(CreateFile(strFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
#endif
  if (!m_hFile.isValid()) return false;

  m_i64FilePos = 0;
  m_i64FileLen = 0;
  m_i64LastDropPos = 0;

  return true;
}

bool CHDFile::Exists(const CURL& url)
{
  std::string strFile(GetLocal(url));

#ifdef TARGET_WINDOWS
  URIUtils::RemoveSlashAtEnd(strFile);
  DWORD attributes = GetFileAttributesW(CWIN32Util::ConvertPathToWin32Form(strFile).c_str());
  if(attributes == INVALID_FILE_ATTRIBUTES)
    return false;
  return true;
#else
  struct __stat64 buffer;
  return (_stat64(strFile.c_str(), &buffer)==0);
#endif
}

int CHDFile::Stat(struct __stat64* buffer)
{
#ifdef TARGET_POSIX
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
  std::string strFile(GetLocal(url));

#ifdef TARGET_WINDOWS
  std::wstring strWFile(CWIN32Util::ConvertPathToWin32Form(strFile));
  /* _wstat64 can't handle long paths therefore we remove the \\?\ */
  CWIN32Util::RemoveExtraLongPathPrefix(strWFile);
  // win32 can only stat root drives with a slash at the end
  if(strWFile.length() == 2 && strWFile[1] == L':')
    strWFile.push_back(L'\\');
  /* _wstat64 calls FindFirstFileEx. According to MSDN, the path should not end in a trailing backslash.
    Remove it before calling _wstat64 */
  else if (strWFile.length() > 3 && URIUtils::HasSlashAtEnd(strFile))
    strWFile.pop_back();
  return _wstat64(strWFile.c_str(), buffer);
#else
  return _stat64(strFile.c_str(), buffer);
#endif
}

bool CHDFile::SetHidden(const CURL &url, bool hidden)
{
#ifdef TARGET_WINDOWS
  DWORD attributes = hidden ? FILE_ATTRIBUTE_HIDDEN : FILE_ATTRIBUTE_NORMAL;
  if (SetFileAttributesW(CWIN32Util::ConvertPathToWin32Form(GetLocal(url)).c_str(), attributes))
    return true;
#endif
  return false;
}

//*********************************************************************************************
bool CHDFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  // make sure it's a legal FATX filename (we are writing to the harddisk)
  std::string strPath(GetLocal(url));

#ifdef TARGET_WINDOWS
  m_hFile.attach(CreateFileW(CWIN32Util::ConvertPathToWin32Form(strPath).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
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
#if defined(HAVE_POSIX_FADVISE)
    // Drop the cache between where we last seeked and 16 MB behind where
    // we are now, to make sure the file doesn't displace everything else.
    // However, we never throw out the first 16 MB of the file, as we might
    // want the header etc., and we never ask the OS to drop in chunks of
    // less than 1 MB.
    int64_t start_drop = std::max<int64_t>(m_i64LastDropPos, 16 << 20);
    int64_t end_drop = std::max<int64_t>(m_i64FilePos - (16 << 20), 0);
    if (end_drop - start_drop >= (1 << 20))
    {
      posix_fadvise((*m_hFile).fd, start_drop, end_drop - start_drop, POSIX_FADV_DONTNEED);
      m_i64LastDropPos = end_drop;
    }
#endif
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
    if (m_i64FilePos != lNewPos.QuadPart)
    {
      // If we seek, disable the cache drop heuristic until we
      // have played sequentially for a while again from here.
      m_i64LastDropPos = lNewPos.QuadPart;
    }
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
  std::string strFile(GetLocal(url));

#ifdef TARGET_WINDOWS
  return ::DeleteFileW(CWIN32Util::ConvertPathToWin32Form(strFile).c_str()) ? true : false;
#else
  return ::DeleteFile(strFile.c_str()) ? true : false;
#endif
}

bool CHDFile::Rename(const CURL& url, const CURL& urlnew)
{
  std::string strFile(GetLocal(url));
  std::string strNewFile(GetLocal(urlnew));

#ifdef TARGET_WINDOWS
  return ::MoveFileW(CWIN32Util::ConvertPathToWin32Form(strFile).c_str(), CWIN32Util::ConvertPathToWin32Form(strNewFile).c_str()) ? true : false;
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
#ifdef TARGET_POSIX
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
#ifdef TARGET_WINDOWS
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
