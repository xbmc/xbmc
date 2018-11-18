/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UwpSMBFile.h"
#include "URL.h"
#include "UwpSMBDirectory.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win10/CustomBuffer.h"
#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <robuffer.h>
#include <string>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace XFILE;
using namespace KODI::PLATFORM::WINDOWS;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;

bool CUwpSMBFile::IsValid(const CURL& url)
{
  return !url.GetFileName().empty();
}

bool CUwpSMBFile::Open(const CURL& url)
{
  return OpenIntenal(url, FileAccessMode::Read);
}

bool CUwpSMBFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  return OpenIntenal(url, FileAccessMode::ReadWrite);
}

void CUwpSMBFile::Close()
{
  if (m_fileStream != nullptr)
  {
    // see https://docs.microsoft.com/en-us/uwp/api/windows.storage.streams.irandomaccessstream
    // m_fileStream->Close(); // where it is?
    m_fileStream = nullptr;
  }
  if (m_sFile)
    m_sFile = nullptr;
}

ssize_t CUwpSMBFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_fileStream)
    return -1;

  IBuffer buf = winrt::make<CustomBuffer>(lpBuf, static_cast<uint32_t>(uiBufSize));
  Wait(m_fileStream.ReadAsync(buf, buf.Capacity(), InputStreamOptions::None));

  return static_cast<ssize_t>(buf.Length());
}

ssize_t CUwpSMBFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_fileStream || !m_allowWrite)
    return -1;

  uint8_t* buff = (uint8_t*)lpBuf;
  auto winrt_buffer = CryptographicBuffer::CreateFromByteArray({buff, buff + uiBufSize});

  uint32_t result = Wait(m_fileStream.WriteAsync(winrt_buffer));
  return static_cast<ssize_t>(result);
}

int64_t CUwpSMBFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fileStream != nullptr)
  {
    int64_t pos = iFilePosition;
    if (iWhence == SEEK_CUR)
      pos += m_fileStream.Position();
    else if (iWhence == SEEK_END)
      pos += m_fileStream.Size();

    uint64_t seekTo;
    if (pos < 0)
      seekTo = 0;
    else if (static_cast<uint64_t>(pos) > m_fileStream.Size())
      seekTo = m_fileStream.Size();
    else
      seekTo = static_cast<uint64_t>(pos);

    m_fileStream.Seek(seekTo);
    return GetPosition();
  }
  return -1;
}

int CUwpSMBFile::Truncate(int64_t toSize)
{
  // not allowed
  return -1;
}

int64_t CUwpSMBFile::GetPosition()
{
  if (m_fileStream != nullptr)
    return static_cast<int64_t>(m_fileStream.Position());

  return -1;
}

int64_t CUwpSMBFile::GetLength()
{
  if (m_fileStream != nullptr)
    return m_fileStream.Size();

  return 0;
}

void CUwpSMBFile::Flush()
{
  if (m_fileStream != nullptr)
    m_fileStream.FlushAsync();
}

bool CUwpSMBFile::Delete(const CURL& url)
{
  bool success = false;
  auto file = GetFile(url);
  if (file)
  {
    try
    {
      Wait(file.DeleteAsync());
    }
    catch (const winrt::hresult_error&)
    {
      return false;
    }
    return true;
  }
  return false;
}

bool CUwpSMBFile::Rename(const CURL& urlCurrentName, const CURL& urlNewName)
{
  if (!IsValid(urlNewName))
    return false;

  auto currFile = GetFile(urlCurrentName);
  if (currFile)
  {
    auto destFile = GetFile(urlNewName);
    if (destFile)
    {
      // replace exiting
      try
      {
        Wait(currFile.MoveAndReplaceAsync(destFile));
      }
      catch (const winrt::hresult_error&)
      {
        return false;
      }
      return true;
    }

    // move
    CURL defFolder = CURL(urlNewName.GetWithoutFilename());
    StorageFolder destFolder = CUwpSMBDirectory::GetFolder(defFolder);
    if (destFolder != nullptr)
    {
      try
      {
        Wait(currFile.MoveAsync(destFolder));
      }
      catch (const winrt::hresult_error&)
      {
        return false;
      }
      return true;
    }
  }
  return false;
}

bool CUwpSMBFile::SetHidden(const CURL& url, bool hidden)
{
  return false;
}

bool CUwpSMBFile::Exists(const CURL& url)
{
  return GetFile(url) != nullptr;
}

int CUwpSMBFile::Stat(const CURL& url, struct __stat64* statData)
{
  if (URIUtils::HasSlashAtEnd(url.GetFileName(), false))
  {
    // stat for directory
    return CUwpSMBDirectory::StatDirectory(url, statData);
  }
  else
  {
    auto file = GetFile(url);
    return Stat(file, statData);
  }
}

int CUwpSMBFile::Stat(struct __stat64* statData)
{
  return Stat(m_sFile, statData);
}

bool CUwpSMBFile::OpenIntenal(const CURL& url, FileAccessMode mode)
{
  // cannot open directories
  if (URIUtils::HasSlashAtEnd(url.GetFileName(), false))
    return false;

  try
  {
    if (mode == FileAccessMode::Read)
    {
      m_sFile = GetFile(url);
    }
    else if (mode == FileAccessMode::ReadWrite)
    {
      auto destFolder = CURL(URIUtils::GetParentPath(url.Get()));
      auto folder = CUwpSMBDirectory::GetFolder(destFolder);
      if (folder)
      {
        std::wstring fileNameW = ToW(url.GetFileNameWithoutPath());
        m_sFile = Wait(folder.CreateFileAsync(fileNameW, CreationCollisionOption::ReplaceExisting));
        if (m_sFile)
          m_allowWrite = true;
      }
    }

    if (m_sFile)
      m_fileStream = Wait(m_sFile.OpenAsync(mode));
  }
  catch (const winrt::hresult_error& ex)
  {
    std::string error = FromW(ex.message().c_str());
    CLog::LogF(LOGERROR, "an exception occurs while openning a file '%s' (mode: %s) : %s",
               url.GetRedacted().c_str(), mode == FileAccessMode::Read ? "r" : "rw", error.c_str());
    return false;
  }

  return m_fileStream != nullptr;
}

StorageFile CUwpSMBFile::GetFile(const CURL& url)
{
  auto requestedPath = CWIN32Util::SmbToUnc(url.GetRedacted());
  try
  {
    StorageFile file =
        Wait(StorageFile::GetFileFromPathAsync(KODI::PLATFORM::WINDOWS::ToW(requestedPath)));
    if (file == nullptr)
    {
      return nullptr;
    }

    return file;
  }
  catch (const winrt::hresult_error& ex)
  {
    std::string error = FromW(ex.message().c_str());
    CLog::LogF(LOGERROR, "unable to get file '%s' with error %s - requestedPath(%s)",
               url.GetRedacted().c_str(), error.c_str(), requestedPath.c_str());
  }

  return nullptr;
}

int CUwpSMBFile::Stat(const StorageFile& file, struct __stat64* statData)
{
  if (!statData)
    return -1;

  if (file == nullptr)
    return -1;

  /* set st_gid */
  statData->st_gid = 0; // UNIX group ID is always zero on Win32
  /* set st_uid */
  statData->st_uid = 0; // UNIX user ID is always zero on Win32
  /* set st_ino */
  statData->st_ino = 0; // inode number is not implemented on Win32

  auto requestedProps = Wait(file.Properties().RetrievePropertiesAsync(
      {L"System.DateAccessed", L"System.DateCreated", L"System.DateModified", L"System.Size"}));

  auto dateAccessed = requestedProps.Lookup(L"System.DateAccessed").as<winrt::IPropertyValue>();
  if (dateAccessed)
  {
    statData->st_atime = winrt::clock::to_time_t(dateAccessed.GetDateTime());
  }
  auto dateCreated = requestedProps.Lookup(L"System.DateCreated").as<winrt::IPropertyValue>();
  if (dateCreated)
  {
    statData->st_ctime = winrt::clock::to_time_t(dateCreated.GetDateTime());
  }
  auto dateModified = requestedProps.Lookup(L"System.DateModified").as<winrt::IPropertyValue>();
  if (dateModified)
  {
    statData->st_mtime = winrt::clock::to_time_t(dateModified.GetDateTime());
  }
  auto fileSize = requestedProps.Lookup(L"System.Size").as<winrt::IPropertyValue>();
  if (fileSize)
  {
    /* set st_size */
    statData->st_size = fileSize.GetInt64();
  }

  statData->st_dev = 0;
  statData->st_rdev = statData->st_dev;
  /* set st_nlink */
  statData->st_nlink = 1;
  /* set st_mode */
  statData->st_mode = _S_IREAD; // only read permission for file from library
  // copy user RWX rights to group rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 3;
  // copy user RWX rights to other rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 6;

  return 0;
}
