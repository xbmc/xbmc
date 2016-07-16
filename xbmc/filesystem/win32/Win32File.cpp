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
#include "Win32File.h"
#include "platform/win32/WIN32Util.h"
#include "utils/win32/Win32Log.h"
#include "utils/SystemInfo.h"
#include "utils/auto_buffer.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif // WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sys/stat.h>

#include <intsafe.h>
#include <wchar.h>
#include <cassert>


using namespace XFILE;


CWin32File::CWin32File() : m_smbFile(false)
{
  m_hFile = INVALID_HANDLE_VALUE;
  m_filePos = -1;
  m_allowWrite = false;
  m_lastSMBFileErr = ERROR_SUCCESS;
}

CWin32File::CWin32File(bool asSmbFile) : m_smbFile(asSmbFile)
{
  m_hFile = INVALID_HANDLE_VALUE;
  m_filePos = -1;
  m_allowWrite = false;
  m_lastSMBFileErr = ERROR_SUCCESS;
}


CWin32File::~CWin32File()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);
}

bool CWin32File::Open(const CURL& url)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    CLog::LogF(LOGERROR, "Attempt to open file without closing opened file object first");
    return false;
  }

  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.length() <= 6) // 6 is length of "\\?\x:"
    return false; // pathnameW is empty or points to device ("\\?\x:")

  assert((pathnameW.compare(4, 4, L"UNC\\", 4) == 0 && m_smbFile) || !m_smbFile);

  m_filepathnameW = pathnameW;
  m_hFile = CreateFileW(pathnameW.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return m_hFile != INVALID_HANDLE_VALUE;
}

bool CWin32File::OpenForWrite(const CURL& url, bool bOverWrite /*= false*/)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  if (m_hFile != INVALID_HANDLE_VALUE)
    return false;

  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.length() <= 6) // 6 is length of "\\?\x:"
    return false; // pathnameW is empty or points to device ("\\?\x:")

  assert((pathnameW.compare(4, 4, L"UNC\\", 4) == 0 && m_smbFile) || !m_smbFile);

  m_hFile = CreateFileW(pathnameW.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                        NULL, bOverWrite ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  if (m_hFile == INVALID_HANDLE_VALUE)
    return false;
    
  const bool newlyCreated = (GetLastError() != ERROR_ALREADY_EXISTS);
  m_filepathnameW = pathnameW;

  if (!newlyCreated)
  {
    if (Seek(0, SEEK_SET) != 0)
    {
      CLog::LogF(LOGERROR, "Can't move i/o pointer");
      Close();
      if (m_smbFile)
        m_lastSMBFileErr = ERROR_INVALID_DATA; // indicate internal errors
      return false;
    }
  }
  else
  { // newly created file
    /* set "hidden" attribute if filename starts with a period */
    size_t lastSlash = pathnameW.rfind(L'\\');
    if (lastSlash < pathnameW.length() - 1 // is two checks in one: lastSlash != std::string::npos && lastSlash + 1 < pathnameW.length()
        && pathnameW[lastSlash + 1] == L'.')
    {
      FILE_BASIC_INFO basicInfo;
      bool hiddenSet = false;
      if (GetFileInformationByHandleEx(m_hFile, FileBasicInfo, &basicInfo, sizeof(basicInfo)) != 0)
      {
        if ((basicInfo.FileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0)
          hiddenSet = true;
        else
        {
          basicInfo.FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
          hiddenSet = SetFileInformationByHandle(m_hFile, FileBasicInfo, &basicInfo, sizeof(basicInfo)) != 0;
        }
      }
      if (!hiddenSet)
        CLog::LogFW(LOGWARNING, L"Can't set hidden attribute for file \"%ls\"", pathnameW.c_str());
    }
  }

  m_allowWrite = true;

  return true;
}

void CWin32File::Close()
{
  if (m_hFile != INVALID_HANDLE_VALUE)
    CloseHandle(m_hFile);
  
  m_hFile = INVALID_HANDLE_VALUE;
  m_filePos = -1;
  m_allowWrite = false;
  m_lastSMBFileErr = ERROR_SUCCESS;
  m_filepathnameW.clear();
}

ssize_t CWin32File::Read(void* lpBuf, size_t uiBufSize)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  assert(lpBuf != NULL || uiBufSize == 0);
  if (lpBuf == NULL && uiBufSize != 0)
    return -1;

  if (uiBufSize == 0)
  { // allow "test" read with zero size
    XUTILS::auto_buffer dummyBuf(255);
    DWORD bytesRead = 0;
    if (!ReadFile(m_hFile, dummyBuf.get(), 0, &bytesRead, NULL))
      return -1;

    assert(bytesRead == 0);
    return 0;
  }

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  ssize_t read = 0;

  // if uiBufSize is larger than ReadFile() can read at one time (larger than DWORD_MAX)
  // repeat ReadFile until buffer is filled
  while (uiBufSize > 0)
  {
    DWORD lastRead = 0;
    if (!ReadFile(m_hFile, ((BYTE*)lpBuf) + read, (uiBufSize > DWORD_MAX) ? DWORD_MAX : (DWORD)uiBufSize, &lastRead, NULL))
    {
      m_filePos = -1;
      if (read > 0)
        return read; // return number of successfully read bytes
      else
        return -1;
    }
    read += lastRead;
    // if m_filePos is set - update it
    if (m_filePos >= 0)
      m_filePos += read;

    // If buffer can be filled with one ReadFile() call and ReadFile() return smaller
    // number of bytes than requested, just return number of read bytes (work as
    // transparent wrapper for ReadFile() ).
    // If ReadFile() can't read any more bytes than don't try to fill buffer.
    if (uiBufSize <= DWORD_MAX || lastRead == 0)
      return read;
    assert(lastRead <= uiBufSize);
    uiBufSize -= lastRead;
  }
  return read;
}

ssize_t CWin32File::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  assert(lpBuf != NULL || uiBufSize == 0);
  if (lpBuf == NULL && uiBufSize != 0)
    return -1;

  if (!m_allowWrite)
  {
    CLog::LogF(LOGERROR, "Attempt to write file opened for reading");
    return -1;
  }

  if (uiBufSize == 0)
  { // allow "test" write with zero size
    XUTILS::auto_buffer dummyBuf(255);
    dummyBuf.get()[0] = 0;
    DWORD bytesWritten = 0;
    if (!WriteFile(m_hFile, dummyBuf.get(), 0, &bytesWritten, NULL))
      return -1;

    assert(bytesWritten == 0);
    return 0;
  }

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  ssize_t written = 0;
  while (uiBufSize > 0)
  {
    DWORD lastWritten = 0;
    const DWORD toWrite = uiBufSize > DWORD_MAX ? DWORD_MAX : (DWORD)uiBufSize;
    if (!WriteFile(m_hFile, ((const BYTE*)lpBuf) + written, toWrite, &lastWritten, NULL))
    {
      m_filePos = -1;
      if (written > 0)
        return written; // return number of successfully written bytes
      else
        return -1;
    }
    written += lastWritten;
    uiBufSize -= lastWritten;
    // if m_filePos is set - update it
    if (m_filePos >= 0)
      m_filePos += lastWritten;

    if (lastWritten != toWrite)
      break;
  }
  return written;
}

int64_t CWin32File::Seek(int64_t iFilePosition, int iWhence /*= SEEK_SET*/)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  LARGE_INTEGER distance, newPos = {};
  distance.QuadPart = iFilePosition;
  DWORD moveMethod;
  if (iWhence == SEEK_SET)
    moveMethod = FILE_BEGIN;
  else if (iWhence == SEEK_CUR)
    moveMethod = FILE_CURRENT;
  else if (iWhence == SEEK_END)
    moveMethod = FILE_END;
  else
    return -1;

  if (!SetFilePointerEx(m_hFile, distance, &newPos, moveMethod))
    m_filePos = -1; // Seek failed, invalidate position
  else
    m_filePos = newPos.QuadPart;

  return m_filePos;
}

int CWin32File::Truncate(int64_t toSize)
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  int64_t prevPos = GetPosition();
  if (prevPos < 0)
    return -1;
  
  if (Seek(toSize) != toSize)
  {
    Seek(prevPos);
    return -1;
  }
  const int ret = (SetEndOfFile(m_hFile) != 0) ? 0 : -1;
  Seek(prevPos);

  return ret;
}

int64_t CWin32File::GetPosition()
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  if (m_filePos < 0)
  {
    LARGE_INTEGER zeroDist, curPos = {};
    zeroDist.QuadPart = 0;
    if (!SetFilePointerEx(m_hFile, zeroDist, &curPos, FILE_CURRENT))
      m_filePos = -1;
    else
      m_filePos = curPos.QuadPart;
  }

  return m_filePos;
}

int64_t CWin32File::GetLength()
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  LARGE_INTEGER fileSize = {};
  // always request current size from filesystem, as file can be changed externally
  if (!GetFileSizeEx(m_hFile, &fileSize))
    return -1;
  else
    return fileSize.QuadPart;
}

void CWin32File::Flush()
{
  if (m_hFile == INVALID_HANDLE_VALUE)
    return;

  if (!m_allowWrite)
  {
    CLog::LogF(LOGERROR, "Attempt to flush file opened for reading");
    return;
  }

  FlushFileBuffers(m_hFile);
}

bool CWin32File::Delete(const CURL& url)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  const bool result = (DeleteFileW(pathnameW.c_str()) != 0);
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return result;
}

bool CWin32File::Rename(const CURL& urlCurrentName, const CURL& urlNewName)
{
  assert((!m_smbFile && urlCurrentName.GetProtocol().empty()) || (m_smbFile && urlCurrentName.IsProtocol("smb"))); // function suitable only for local or SMB files
  assert((!m_smbFile && urlNewName.GetProtocol().empty()) || (m_smbFile && urlNewName.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  //! @todo check whether it's file or directory
  std::wstring curNameW(CWIN32Util::ConvertPathToWin32Form(urlCurrentName));
  if (curNameW.empty())
    return false;

  std::wstring newNameW(CWIN32Util::ConvertPathToWin32Form(urlNewName));
  if (newNameW.empty())
    return false;

  const bool result = (MoveFileExW(curNameW.c_str(), newNameW.c_str(), MOVEFILE_COPY_ALLOWED) != 0);
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return result;
}

bool CWin32File::SetHidden(const CURL& url, bool hidden)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  const DWORD attrs = GetFileAttributesW(pathnameW.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
    return false;
  
  // check whether attribute is already set/cleared
  if (((attrs & FILE_ATTRIBUTE_HIDDEN) != 0) == hidden)
    return true;

  bool result;
  if (hidden)
    result = (SetFileAttributesW(pathnameW.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN) != 0);
  else
    result = SetFileAttributesW(pathnameW.c_str(), attrs & ~FILE_ATTRIBUTE_HIDDEN) != 0;
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return result;
}

bool CWin32File::Exists(const CURL& url)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
    return false;

  const DWORD attrs = GetFileAttributesW(pathnameW.c_str());
  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

int CWin32File::Stat(const CURL& url, struct __stat64* statData)
{
  assert((!m_smbFile && url.GetProtocol().empty()) || (m_smbFile && url.IsProtocol("smb"))); // function suitable only for local or SMB files
  if (m_smbFile)
    m_lastSMBFileErr = ERROR_INVALID_DATA; // used to indicate internal errors, cleared by successful file operation

  if (!statData)
    return -1;

  std::wstring pathnameW(CWIN32Util::ConvertPathToWin32Form(url));
  if (pathnameW.empty())
  {
    errno = ENOENT;
    return -1;
  }

  if (pathnameW.length() <= 6) // 6 is length of "\\?\x:"
    return -1; // pathnameW is empty or points to device ("\\?\x:"), on win32 stat() for devices is not supported

  assert((pathnameW.compare(4, 4, L"UNC\\", 4) == 0 && m_smbFile) || !m_smbFile);

  // get maximum information about file from search function
  HANDLE hSearch;
  WIN32_FIND_DATAW findData;
  if (CSysInfo::IsWindowsVersionAtLeast(CSysInfo::WindowsVersionWin7))
    hSearch = FindFirstFileExW(pathnameW.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, 0);
  else
    hSearch = FindFirstFileExW(pathnameW.c_str(), FindExInfoStandard, &findData, FindExSearchNameMatch, NULL, 0);

  if (m_smbFile)
    m_lastSMBFileErr = GetLastError(); // set real error state

  if (hSearch == INVALID_HANDLE_VALUE)
    return -1;
  
  FindClose(hSearch);

  /* set st_gid */
  statData->st_gid = 0; // UNIX group ID is always zero on Win32

  /* set st_uid */
  statData->st_uid = 0; // UNIX user ID is always zero on Win32

  /* set st_ino */
  statData->st_ino = 0; // inode number is not implemented on Win32

  /* set st_size */
  statData->st_size = (__int64(findData.nFileSizeHigh) << 32) + __int64(findData.nFileSizeLow);

  /* set st_dev and st_rdev */
  if (!m_smbFile)
  {
    assert(pathnameW.compare(0, 4, L"\\\\?\\", 4) == 0);
    assert(pathnameW.length() >= 7); // '7' is the minimal length of "\\?\x:\"
    assert(pathnameW[5] == L':');
    const wchar_t driveLetter = pathnameW[4];
    assert((driveLetter >= L'A' && driveLetter <= L'Z') || (driveLetter >= L'a' && driveLetter <= L'z'));
    statData->st_dev = (driveLetter >= L'a') ? driveLetter - L'a' : driveLetter - L'A';
  }
  else
    statData->st_dev = 0;
  statData->st_rdev = statData->st_dev;

  const HANDLE hFile = CreateFileW(pathnameW.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  /* set st_nlink */
  statData->st_nlink = 1; // fallback value
  if (hFile != INVALID_HANDLE_VALUE)
  {
    _FILE_STANDARD_INFO stdInfo;
    if (GetFileInformationByHandleEx(hFile, FileStandardInfo, &stdInfo, sizeof(stdInfo)) != 0)
      statData->st_nlink = (stdInfo.NumberOfLinks > SHORT_MAX) ? SHORT_MAX : short(stdInfo.NumberOfLinks);
  }

  /* set st_mtime, st_atime, st_ctime */
  statData->st_mtime = 0;
  statData->st_atime = 0;
  statData->st_ctime = 0;
  if (hFile != INVALID_HANDLE_VALUE)
  {
    FILE_BASIC_INFO basicInfo;
    if (GetFileInformationByHandleEx(hFile, FileBasicInfo, &basicInfo, sizeof(basicInfo)) != 0)
    {
      statData->st_mtime = CWIN32Util::fileTimeToTimeT(basicInfo.LastWriteTime);
      statData->st_atime = CWIN32Util::fileTimeToTimeT(basicInfo.LastAccessTime);
      statData->st_ctime = CWIN32Util::fileTimeToTimeT(basicInfo.CreationTime);
    }
    CloseHandle(hFile);
  }
  if (statData->st_mtime == 0)
    statData->st_mtime = CWIN32Util::fileTimeToTimeT(findData.ftLastWriteTime);

  if (statData->st_atime == 0)
    statData->st_atime = CWIN32Util::fileTimeToTimeT(findData.ftLastAccessTime);
  if (statData->st_atime == 0)
    statData->st_atime = statData->st_mtime;

  if (statData->st_ctime == 0)
    statData->st_ctime = CWIN32Util::fileTimeToTimeT(findData.ftCreationTime);
  if (statData->st_ctime == 0)
    statData->st_ctime = statData->st_mtime;

  /* set st_mode */
  statData->st_mode = _S_IREAD; // Always assume Read permission for file owner
  if ((findData.dwFileAttributes & FILE_READ_ONLY) == 0)
    statData->st_mode |= _S_IWRITE; // Write possible
  if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    statData->st_mode |= _S_IFDIR | _S_IEXEC; // directory
  else
  {
    statData->st_mode |= _S_IFREG; // file
    // following large piece of code is disabled
    // as it intended only to set _S_IEXEC flag
    // which is not used by callers
#ifdef WIN32_USE_FILE_STAT_MAX_INFO
    // set _S_IEXEC if file has executable extension
    const size_t lastDot = pathnameW.rfind(L'.');
    if (lastDot != std::wstring::npos && pathnameW.rfind(L'\\') < lastDot)
    { // file has some extension
      const std::wstring fileExt(pathnameW, lastDot);
      XUTILS::auto_buffer buf(32767 * sizeof(wchar_t)); // maximum possible size
      const DWORD envRes = GetEnvironmentVariableW(L"PATHEXT", (wchar_t*)buf.get(), buf.size() / sizeof(wchar_t));
      std::vector<std::wstring> listExts;
      if (envRes == 0 || envRes > (buf.size() / sizeof(wchar_t)))
      {
        buf.clear();
        static const wchar_t* extArr[] = { L".exe", L".bat", L".cmd", L".com" };
        listExts.assign(extArr, extArr + (sizeof(extArr) / sizeof(extArr[0])));
      }
      else
      {
        std::wstring envPathextW((wchar_t*)buf.get(), envRes);
        buf.clear();
        size_t posExt = envPathextW.find_first_not_of(L';'); // skip ';' at the start
        while (posExt != std::wstring::npos)
        {
          const size_t posSemicol = envPathextW.find(L';', posExt);
          listExts.push_back(envPathextW.substr(posExt, posSemicol - posExt));
          posExt = envPathextW.find_first_not_of(L";", posSemicol);
        }
      }
      const wchar_t* const fileExtC = fileExt.c_str();
      for (std::vector<std::wstring>::const_iterator it = listExts.cbegin(); it != listExts.cend(); ++it)
      {
        if (_wcsicmp(fileExtC, it->c_str()) == 0)
        {
          statData->st_mode |= _S_IEXEC; // file can be executed
          break;
        }
      }
    }
#endif // WIN32_USE_FILE_STAT_MAX_INFO
  }
  // copy user RWX rights to group rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 3;
  // copy user RWX rights to other rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 6;

  return 0;
}

int CWin32File::Stat(struct __stat64* statData)
{
  if (!statData)
    return -1;

  if (m_hFile == INVALID_HANDLE_VALUE)
    return -1;

  /* set st_gid */
  statData->st_gid = 0; // UNIX group ID is always zero on Win32

  /* set st_uid */
  statData->st_uid = 0; // UNIX user ID is always zero on Win32

  /* set st_ino */
  statData->st_ino = 0; // inode number is not implemented on Win32

  /* set st_mtime, st_atime, st_ctime */
  statData->st_mtime = 0;
  statData->st_atime = 0;
  statData->st_ctime = 0;
  FILE_BASIC_INFO basicInfo;
  if (GetFileInformationByHandleEx(m_hFile, FileBasicInfo, &basicInfo, sizeof(basicInfo)) == 0)
    return -1; // can't get basic file information

  statData->st_mtime = CWIN32Util::fileTimeToTimeT(basicInfo.LastWriteTime);

  statData->st_atime = CWIN32Util::fileTimeToTimeT(basicInfo.LastAccessTime);
  if (statData->st_atime == 0)
    statData->st_atime = statData->st_mtime;

  statData->st_ctime = CWIN32Util::fileTimeToTimeT(basicInfo.CreationTime);
  if (statData->st_ctime == 0)
    statData->st_ctime = statData->st_mtime;

  /* set st_size and st_nlink */
  FILE_STANDARD_INFO stdInfo;
  if (GetFileInformationByHandleEx(m_hFile, FileStandardInfo, &stdInfo, sizeof(stdInfo)) != 0)
  {
    statData->st_size = __int64(stdInfo.EndOfFile.QuadPart);
    statData->st_nlink = (stdInfo.NumberOfLinks > SHORT_MAX) ? SHORT_MAX : short(stdInfo.NumberOfLinks);
  }
  else
  {
    statData->st_size = GetLength();
    if (statData->st_size < 0)
      return -1; // can't get file size
    statData->st_nlink = 1; // fallback value
  }

  /* set st_dev and st_rdev */
  if (!m_smbFile)
  {
    assert(m_filepathnameW.compare(0, 4, L"\\\\?\\", 4) == 0);
    assert(m_filepathnameW.length() >= 7); // '7' is the minimal length of "\\?\x:\"
    assert(m_filepathnameW[5] == L':');
    const wchar_t driveLetter = m_filepathnameW[4];
    assert((driveLetter >= L'A' && driveLetter <= L'Z') || (driveLetter >= L'a' && driveLetter <= L'z'));
    statData->st_dev = (driveLetter >= L'a') ? driveLetter - L'a' : driveLetter - L'A';
  }
  else
    statData->st_dev = 0;
  statData->st_rdev = statData->st_dev;
  
  /* set st_mode */
  statData->st_mode = _S_IREAD; // Always assume Read permission for file owner
  if ((basicInfo.FileAttributes & FILE_READ_ONLY) == 0)
    statData->st_mode |= _S_IWRITE; // Write possible
  if ((basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    statData->st_mode |= _S_IFDIR | _S_IEXEC; // directory
  else
  {
    statData->st_mode |= _S_IFREG; // file
    // following large piece of code is disabled
    // as it intended only to set _S_IEXEC flag
    // which is not used by callers
#ifdef WIN32_USE_FILE_STAT_MAX_INFO
    // set _S_IEXEC if file has executable extension
    std::wstring pathnameW;
    XUTILS::auto_buffer nameInfoBuf(sizeof(FILE_NAME_INFO) + sizeof(wchar_t) * MAX_PATH * 2);
    if (GetFileInformationByHandleEx(m_hFile, FileNameInfo, nameInfoBuf.get(), nameInfoBuf.size()) != 0)
    {
      // real path and name without drive
      const PFILE_NAME_INFO pNameInfo = PFILE_NAME_INFO(nameInfoBuf.get());
      pathnameW.assign(pNameInfo->FileName, pNameInfo->FileNameLength / sizeof(wchar_t));
      nameInfoBuf.clear();
    }
    else
      pathnameW = m_filepathnameW;

    const size_t lastDot = pathnameW.rfind(L'.');
    const size_t lastSlash = pathnameW.rfind(L'\\');
    if (lastDot != std::wstring::npos &&
        (lastSlash == std::wstring::npos || lastSlash<lastDot))
    { // file has some extension
      const std::wstring fileExt(pathnameW, lastDot);
      XUTILS::auto_buffer buf(32767 * sizeof(wchar_t)); // maximum possible size
      const DWORD envRes = GetEnvironmentVariableW(L"PATHEXT", (wchar_t*)buf.get(), buf.size() / sizeof(wchar_t));
      std::vector<std::wstring> listExts;
      if (envRes == 0 || envRes > (buf.size() / sizeof(wchar_t)))
      {
        buf.clear();
        static const wchar_t* extArr[] = { L".exe", L".bat", L".cmd", L".com" };
        listExts.assign(extArr, extArr + (sizeof(extArr) / sizeof(extArr[0])));
      }
      else
      {
        std::wstring envPathextW((wchar_t*)buf.get(), envRes);
        buf.clear();
        size_t posExt = envPathextW.find_first_not_of(L';'); // skip ';' at the start
        while (posExt != std::wstring::npos)
        {
          const size_t posSemicol = envPathextW.find(L';', posExt);
          listExts.push_back(envPathextW.substr(posExt, posSemicol - posExt));
          posExt = envPathextW.find_first_not_of(L";", posSemicol);
        }
      }
      const wchar_t* const fileExtC = fileExt.c_str();
      for (std::vector<std::wstring>::const_iterator it = listExts.cbegin(); it != listExts.cend(); ++it)
      {
        if (_wcsicmp(fileExtC, it->c_str()) == 0)
        {
          statData->st_mode |= _S_IEXEC; // file can be executed
          break;
        }
      }
    }
#endif // WIN32_USE_FILE_STAT_MAX_INFO
  }
  // copy user RWX rights to group rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 3;
  // copy user RWX rights to other rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 6;

  return 0;
}

#endif // TARGET_WINDOWS
