/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinLibraryFile.h"

#include "URL.h"
#include "WinLibraryDirectory.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"

#include <string>

#include <robuffer.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.AccessCache.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Search.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace XFILE;
using namespace KODI::PLATFORM::WINDOWS;
namespace winrt
{
  using namespace Windows::Foundation;
}
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::AccessCache;
using namespace winrt::Windows::Storage::Search;
using namespace winrt::Windows::Storage::Streams;

struct __declspec(uuid("905a0fef-bc53-11df-8c49-001e4fc686da")) IBufferByteAccess : ::IUnknown
{
  virtual HRESULT __stdcall Buffer(void** value) = 0;
};

struct CustomBuffer : winrt::implements<CustomBuffer, IBuffer, IBufferByteAccess>
{
  void *m_address;
  uint32_t m_capacity;
  uint32_t m_length;

  CustomBuffer(void *address, uint32_t capacity) : m_address(address), m_capacity(capacity), m_length(0) { }
  uint32_t Capacity() const { return m_capacity; }
  uint32_t Length() const { return m_length; }
  void Length(uint32_t length) { m_length = length; }

  HRESULT __stdcall Buffer(void** value) final
  {
    *value = m_address;
    return S_OK;
  }
};

CWinLibraryFile::CWinLibraryFile() = default;
CWinLibraryFile::~CWinLibraryFile(void) = default;

bool CWinLibraryFile::IsValid(const CURL & url)
{
  return CWinLibraryDirectory::IsValid(url)
    && !url.GetFileName().empty();
}

bool CWinLibraryFile::Open(const CURL& url)
{
  return OpenIntenal(url, FileAccessMode::Read);
}

bool CWinLibraryFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  return OpenIntenal(url, FileAccessMode::ReadWrite);
}

void CWinLibraryFile::Close()
{
  if (m_fileStream != nullptr)
  {
    m_fileStream.Close();
    m_fileStream = nullptr;
  }
  if (m_sFile)
    m_sFile = nullptr;
}

ssize_t CWinLibraryFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_fileStream)
    return -1;

  IBuffer buf = winrt::make<CustomBuffer>(lpBuf, static_cast<uint32_t>(uiBufSize));
  try
  {
    Wait(m_fileStream.ReadAsync(buf, buf.Capacity(), InputStreamOptions::None));
    return static_cast<ssize_t>(buf.Length());
  }
  catch (const winrt::hresult_error& ex)
  {
    using KODI::PLATFORM::WINDOWS::FromW;
    CLog::LogF(LOGERROR, "unable to read file ({})", winrt::to_string(ex.message()));
    return -1;
  }
}

ssize_t CWinLibraryFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_fileStream || !m_allowWrite)
    return -1;

  uint8_t* buff = (uint8_t*)lpBuf;
  const auto winrt_buffer = CryptographicBuffer::CreateFromByteArray({ buff, buff + uiBufSize });

  try
  {
    const uint32_t result = Wait(m_fileStream.WriteAsync(winrt_buffer));
    return static_cast<ssize_t>(result);
  }
  catch (const winrt::hresult_error& ex)
  {
    using KODI::PLATFORM::WINDOWS::FromW;
    CLog::LogF(LOGERROR, "unable write to file ({})", winrt::to_string(ex.message()));
    return -1;
  }
}

int64_t CWinLibraryFile::Seek(int64_t iFilePosition, int iWhence)
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

int CWinLibraryFile::Truncate(int64_t toSize)
{
  // not allowed
  return -1;
}

int64_t CWinLibraryFile::GetPosition()
{
  if (m_fileStream != nullptr)
    return static_cast<int64_t>(m_fileStream.Position());

  return -1;
}

int64_t CWinLibraryFile::GetLength()
{
  if (m_fileStream != nullptr)
    return m_fileStream.Size();

  return 0;
}

void CWinLibraryFile::Flush()
{
  if (m_fileStream != nullptr)
    m_fileStream.FlushAsync();
}

bool CWinLibraryFile::Delete(const CURL & url)
{
  bool success = false;
  auto file = GetFile(url);
  if (file)
  {
    try
    {
      Wait(file.DeleteAsync());
    }
    catch(const winrt::hresult_error&)
    {
      return false;
    }
    return true;
  }
  return false;
}

bool CWinLibraryFile::Rename(const CURL & urlCurrentName, const CURL & urlNewName)
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
    StorageFolder destFolder = CWinLibraryDirectory::GetFolder(defFolder);
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

bool CWinLibraryFile::SetHidden(const CURL& url, bool hidden)
{
  return false;
}

bool CWinLibraryFile::Exists(const CURL& url)
{
  return GetFile(url) != nullptr;
}

int CWinLibraryFile::Stat(const CURL& url, struct __stat64* statData)
{
  if (URIUtils::HasSlashAtEnd(url.GetFileName(), false))
  {
    // stat for directory
    return CWinLibraryDirectory::StatDirectory(url, statData);
  }
  else
  {
    auto file = GetFile(url);
    return Stat(file, statData);
  }
}

int CWinLibraryFile::Stat(struct __stat64* statData)
{
  return Stat(m_sFile, statData);
}

bool CWinLibraryFile::IsInAccessList(const CURL& url)
{
  using KODI::PLATFORM::WINDOWS::FromW;
  static std::string localPath;
  static std::string packagePath;

  try
  {
    if (localPath.empty())
      localPath = FromW(ApplicationData::Current().LocalFolder().Path().c_str());

    if (packagePath.empty())
      packagePath = FromW(Package::Current().InstalledLocation().Path().c_str());

    // don't check files inside local folder and installation folder
    if ( StringUtils::StartsWithNoCase(url.Get(), localPath)
      || StringUtils::StartsWithNoCase(url.Get(), packagePath))
      return false;

    return IsInList(url, StorageApplicationPermissions::FutureAccessList())
        || IsInList(url, StorageApplicationPermissions::MostRecentlyUsedList());
  }
  catch (const winrt::hresult_error& ex)
  {
    std::string strError = FromW(ex.message().c_str());
    CLog::LogF(LOGERROR, "unexpected error occurs during WinRT API call: {}", strError);
  }
  return false;
}

bool CWinLibraryFile::OpenIntenal(const CURL &url, FileAccessMode mode)
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
      auto folder = CWinLibraryDirectory::GetFolder(destFolder);
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
    CLog::LogF(LOGERROR, "an exception occurs while opening a file '{}' (mode: {}) : {}",
               url.GetRedacted(), mode == FileAccessMode::Read ? "r" : "rw", error);
    return false;
  }

  return m_fileStream != nullptr;
}

StorageFile CWinLibraryFile::GetFile(const CURL& url)
{
  // check that url is library url
  if (CWinLibraryDirectory::IsValid(url))
  {
    StorageFolder rootFolder = CWinLibraryDirectory::GetRootFolder(url);
    if (rootFolder == nullptr)
      return nullptr;

    std::string filePath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
    try
    {
      std::wstring wpath = ToW(filePath);
      auto item = Wait(rootFolder.TryGetItemAsync(wpath));
      if (item && item.IsOfType(StorageItemTypes::File))
        return item.as<StorageFile>();

      return nullptr;
    }
    catch (const winrt::hresult_error& ex)
    {
      std::string error = FromW(ex.message().c_str());
      CLog::LogF(LOGERROR, "unable to get file '{}' with error {}", url.GetRedacted(), error);
    }
  }
  else if (url.IsProtocol("file") || url.GetProtocol().empty())
  {
    // check that a file in feature access list or most rescently used list
    // search in FAL
    IStorageItemAccessList list = StorageApplicationPermissions::FutureAccessList();
    winrt::hstring token = GetTokenFromList(url, list);
    if (token.empty())
    {
      // search in MRU list
      list = StorageApplicationPermissions::MostRecentlyUsedList();
      token = GetTokenFromList(url, list);
    }
    if (!token.empty())
      return Wait(list.GetFileAsync(token));
  }

  return nullptr;
}

bool CWinLibraryFile::IsInList(const CURL& url, const IStorageItemAccessList& list)
{
  auto token = GetTokenFromList(url, list);
  return !token.empty();
}

winrt::hstring CWinLibraryFile::GetTokenFromList(const CURL& url, const IStorageItemAccessList& list)
{
  AccessListEntryView listview = list.Entries();
  if (listview.Size() == 0)
    return winrt::hstring();

  using KODI::PLATFORM::WINDOWS::ToW;
  std::wstring filePathW = ToW(url.Get());

  for(auto&& listEntry : listview)
  {
    if (listEntry.Metadata == filePathW)
    {
      return listEntry.Token;
    }
  }

  return winrt::hstring();
}

int CWinLibraryFile::Stat(const StorageFile& file, struct __stat64* statData)
{
  if (!statData)
    return -1;

  if (file == nullptr)
    return -1;

  *statData = {};

  /* set st_gid */
  statData->st_gid = 0; // UNIX group ID is always zero on Win32
  /* set st_uid */
  statData->st_uid = 0; // UNIX user ID is always zero on Win32
  /* set st_ino */
  statData->st_ino = 0; // inode number is not implemented on Win32

  auto requestedProps = Wait(file.Properties().RetrievePropertiesAsync({
    L"System.DateAccessed",
    L"System.DateCreated",
    L"System.DateModified",
    L"System.Size"
  }));

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
