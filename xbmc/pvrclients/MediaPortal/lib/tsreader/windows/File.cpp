/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "client.h" // For XBMC->Log
#include "../File.h"
#include "utils.h"

using namespace PLATFORM;
using namespace ADDON;
using namespace std;

CFile::CFile() :
  m_hFile(INVALID_HANDLE_VALUE),
  m_flags(0),
  m_bReadOnly(true)
{
}


CFile::~CFile()
{
  Close();
}


void CFile::Close()
{
  try
  {
    if (m_hFile != INVALID_HANDLE_VALUE)
    {
      ::CloseHandle(m_hFile);
      m_hFile = INVALID_HANDLE_VALUE; // Invalidate the file
    }
  }
  catch(...)
  {
    XBMC->Log(LOG_ERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return;
}


int64_t CFile::GetLength()
{
  // Do not get file size if static file or first time 
  if (m_bReadOnly || !m_fileSize)
  {
    LARGE_INTEGER li;
    if (::GetFileSizeEx(m_hFile, &li) == 0)
    {
      DWORD dwErr = GetLastError();
      XBMC->Log(LOG_ERROR, "%s GetFileSizeEx failed. Error %d.", __FUNCTION__, dwErr);
      return E_FAIL;
    }
    m_fileSize = li.QuadPart;
  }
  return m_fileSize;
}


bool CFile::Open(const CStdString& strFileName, unsigned int flags)
{
  m_flags = flags;

  CStdStringW strWFile = UTF8Util::ConvertUTF8ToUTF16(strFileName.c_str());

  // Do not try to open a tsbuffer file without SHARE_WRITE so skip this try if we have a buffer file
  if (strstr(strFileName.c_str(), ".ts.tsbuffer") == NULL) 
  {
    // Try to open the file
    m_hFile = ::CreateFileW(strWFile, // The filename
            (DWORD) GENERIC_READ,             // File access
            (DWORD) FILE_SHARE_READ,          // Share access
            NULL,                             // Security
            (DWORD) OPEN_EXISTING,            // Open flags
            (DWORD) 0,                        // More flags
            NULL);                            // Template

    m_bReadOnly = false;
    if (!IsInvalid())
      return true;
  }

  //Test incase file is being recorded to
  m_hFile = ::CreateFileW(strWFile, // The filename
            (DWORD) GENERIC_READ,             // File access
            (DWORD) (FILE_SHARE_READ |
            FILE_SHARE_WRITE),                // Share access
            NULL,                             // Security
            (DWORD) OPEN_EXISTING,            // Open flags
            (DWORD) FILE_ATTRIBUTE_NORMAL,    // More flags
            NULL);                            // Template

  if (IsInvalid())
  {
    DWORD dwErr = GetLastError();
    XBMC->Log(LOG_ERROR, "%s: error opening file '%s'. Error code %d", __FUNCTION__, strFileName.c_str(), dwErr);
    return false;
  }

  return true;
}


bool CFile::Exists(const CStdString& strFileName, bool bUseCache /* = true */)
{
  DWORD dwAttr = GetFileAttributes(strFileName.c_str());

  if (dwAttr == 0xffffffff)
  {
    DWORD dwError = GetLastError();
    if (dwError == ERROR_FILE_NOT_FOUND)
    {
      // file not found
      return false;
    }
    else if (dwError == ERROR_PATH_NOT_FOUND)
    {
      // path not found
      return false;
    }
    else if (dwError == ERROR_ACCESS_DENIED)
    {
      // file or directory exists, but access is denied
      return false;
    }
    else
    {
      // some other error has occured
      return false;
    }
  }

  return true;
}


unsigned long CFile::Read(void *lpBuf, int64_t uiBufSize)
{
  if (IsInvalid())
    return 0;

  // Read file data into buffer
  BOOL  ret;
  DWORD dwReadBytes;
    
  ret = ::ReadFile(m_hFile, lpBuf, (DWORD) uiBufSize, &dwReadBytes, NULL);

  if (!ret)
  {
    XBMC->Log(LOG_ERROR, "FileReader::Read() read failed - error = %d",  HRESULT_FROM_WIN32(GetLastError()));
    return 0;
  }

  return dwReadBytes;
}


int64_t CFile::Seek(int64_t iFilePosition, int iWhence)
{
  // FILE_BEGIN =   SEEK_SET: offset relative to the beginning of the file
  // FILE_CURRENT = SEEK_CUR: offset relative to the current position in the file
  // FILE_END =     SEEK_END: offset relative to the end of the file

  LARGE_INTEGER liNewPos;
  LARGE_INTEGER liSetPos;

  liSetPos.QuadPart = iFilePosition;

  if (!::SetFilePointerEx(m_hFile, liSetPos, &liNewPos, iWhence) )
  {
    XBMC->Log(LOG_ERROR, "%s: SetFilePointerEx failed with error code %i\n", __FUNCTION__, GetLastError());
    return -1;
  }

  return liNewPos.QuadPart;
}


int64_t CFile::GetPosition()
{
  LARGE_INTEGER li, seekoffset;
  seekoffset.QuadPart = 0;

  if (::SetFilePointerEx(m_hFile, seekoffset, &li, FILE_CURRENT) == 0)
  {
    // Error
    XBMC->Log(LOG_ERROR, "%s: ::SetFilePointerEx failed", __FUNCTION__);
    return -1;
  }

  return li.QuadPart;
}
