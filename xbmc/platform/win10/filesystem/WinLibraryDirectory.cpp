/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinLibraryDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"

#include <string>

#include <winrt/Windows.Storage.FileProperties.h>

using namespace XFILE;
using namespace KODI::PLATFORM::WINDOWS;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::FileProperties;
using namespace winrt::Windows::Storage::Search;
using namespace winrt::Windows::Foundation::Collections;
namespace winrt
{
using namespace Windows::Foundation;
}

bool CWinLibraryDirectory::GetStoragePath(std::string library, std::string& path)
{
  CURL url;
  url.SetProtocol("win-lib");
  url.SetHostName(library);

  if (!IsValid(url))
    return false;

  path = url.Get();
  return true;
}

StorageFolder CWinLibraryDirectory::GetRootFolder(const CURL& url)
{
  if (!url.IsProtocol("win-lib"))
    return nullptr;

  std::string lib = url.GetHostName();
  try
  {
    if (lib == "music")
      return KnownFolders::MusicLibrary();
    if (lib == "video")
      return KnownFolders::VideosLibrary();
    if (lib == "pictures")
      return KnownFolders::PicturesLibrary();
    if (lib == "photos")
      return KnownFolders::CameraRoll();
    if (lib == "documents")
      return KnownFolders::DocumentsLibrary();
    if (lib == "removable")
      return KnownFolders::RemovableDevices();
  }
  catch (const winrt::hresult_error& ex)
  {
    std::string strError = KODI::PLATFORM::WINDOWS::FromW(ex.message().c_str());
    CLog::LogF(LOGERROR, "unexpected error occurs during WinRT API call: {}", strError);
  }

  return nullptr;
}

bool CWinLibraryDirectory::IsValid(const CURL& url)
{
  if (!url.IsProtocol("win-lib"))
    return false;

  std::string lib = url.GetHostName();

  if (lib == "music" || lib == "video" || lib == "pictures" || lib == "photos" ||
      lib == "documents" || lib == "removable")
    return true;
  else
    return false;
}

CWinLibraryDirectory::CWinLibraryDirectory() = default;
CWinLibraryDirectory::~CWinLibraryDirectory(void) = default;

bool CWinLibraryDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  items.Clear();

  auto folder = GetFolder(url);
  if (!folder)
    return false;

  // We accept win-lib://library/path[/]
  std::string path = url.Get();
  URIUtils::AddSlashAtEnd(path); //be sure the dir ends with a slash

  auto vectorView = Wait(folder.GetItemsAsync());
  for (unsigned i = 0; i < vectorView.Size(); i++)
  {
    IStorageItem item = vectorView.GetAt(i);
    std::string itemName = FromW(item.Name().c_str());

    CFileItemPtr pItem(new CFileItem(itemName));
    pItem->m_bIsFolder =
        (item.Attributes() & FileAttributes::Directory) == FileAttributes::Directory;
    IStorageItemProperties storageItemProperties = item.as<IStorageItemProperties>();
    if (item != nullptr)
    {
      pItem->m_strTitle = FromW(storageItemProperties.DisplayName().c_str());
    }

    if (pItem->m_bIsFolder)
      pItem->SetPath(path + itemName + "/");
    else
      pItem->SetPath(path + itemName);

    if (itemName.front() == '.')
      pItem->SetProperty("file:hidden", true);

    auto props = Wait(item.GetBasicPropertiesAsync());

    FILETIME fileTime1 = winrt::clock::to_FILETIME(props.DateModified());
    KODI::TIME::FileTime fileTime2;
    fileTime2.highDateTime = fileTime1.dwHighDateTime;
    fileTime2.lowDateTime = fileTime1.dwLowDateTime;
    pItem->m_dateTime = fileTime2;
    if (!pItem->m_bIsFolder)
      pItem->m_dwSize = static_cast<int64_t>(props.Size());

    items.Add(pItem);
  }

  return true;
}

bool CWinLibraryDirectory::Create(const CURL& url)
{
  auto folder = GetFolder(url);
  if (folder) // already exists
    return true;

  CURL parentUrl = CURL(URIUtils::GetParentPath(url.Get()));
  folder = GetFolder(parentUrl);
  if (!folder)
    return false;

  try
  {
    std::wstring wStrPath = ToW(url.GetFileNameWithoutPath());
    Wait(folder.CreateFolderAsync(wStrPath));
  }
  catch (const winrt::hresult_error&)
  {
    return false;
  }

  return true;
}

bool CWinLibraryDirectory::Exists(const CURL& url)
{
  return GetFolder(url) != nullptr;
}

bool CWinLibraryDirectory::Remove(const CURL& url)
{
  bool exists = false;
  auto folder = GetFolder(url);
  if (!folder)
    return false;
  try
  {
    Wait(folder.DeleteAsync(StorageDeleteOption::PermanentDelete));
    exists = true;
  }
  catch (const winrt::hresult_error& ex)
  {
    std::string error = FromW(ex.message().c_str());
    CLog::LogF(LOGERROR, __FUNCTION__, "unable remove folder '{}' with error", url.Get(), error);
    exists = false;
  }
  return exists;
}

StorageFolder CWinLibraryDirectory::GetFolder(const CURL& url)
{
  StorageFolder rootFolder = GetRootFolder(url);
  if (!rootFolder)
    return nullptr;

  // find inner folder
  std::string folderPath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
  if (!folderPath.empty())
  {
    try
    {
      std::wstring wStrPath = ToW(folderPath);

      auto item = Wait(rootFolder.TryGetItemAsync(wStrPath));
      if (item && item.IsOfType(StorageItemTypes::Folder))
        return item.as<StorageFolder>();

      return nullptr;
    }
    catch (const winrt::hresult_error& ex)
    {
      std::string error = FromW(ex.message().c_str());
      CLog::LogF(LOGERROR, "unable to get folder '{}' with error", url.GetRedacted(), error);
    }
    return nullptr;
  }

  return rootFolder;
}

int CWinLibraryDirectory::StatDirectory(const CURL& url, struct __stat64* statData)
{
  if (!statData)
    return -1;

  auto dir = GetFolder(url);
  if (dir == nullptr)
    return -1;

  /* set st_gid */
  statData->st_gid = 0; // UNIX group ID is always zero on Win32
  /* set st_uid */
  statData->st_uid = 0; // UNIX user ID is always zero on Win32
  /* set st_ino */
  statData->st_ino = 0; // inode number is not implemented on Win32

  statData->st_atime = 0;
  statData->st_ctime = 0;
  statData->st_mtime = 0;

  auto requestedProps = Wait(dir.Properties().RetrievePropertiesAsync(
      {L"System.DateAccessed", L"System.DateCreated", L"System.DateModified"}));

  if (requestedProps.HasKey(L"System.DateAccessed") && 
      requestedProps.Lookup(L"System.DateAccessed"))
  {
    auto dateAccessed = requestedProps.Lookup(L"System.DateAccessed").as<winrt::IPropertyValue>();
    if (dateAccessed)
    {
      statData->st_atime = winrt::clock::to_time_t(dateAccessed.GetDateTime());
    }
  }
  if (requestedProps.HasKey(L"System.DateCreated") && 
      requestedProps.Lookup(L"System.DateCreated"))
  {
    auto dateCreated = requestedProps.Lookup(L"System.DateCreated").as<winrt::IPropertyValue>();
    if (dateCreated)
    {
      statData->st_ctime = winrt::clock::to_time_t(dateCreated.GetDateTime());
    }
  }
  if (requestedProps.HasKey(L"System.DateModified") && 
      requestedProps.Lookup(L"System.DateModified"))
  {
    auto dateModified = requestedProps.Lookup(L"System.DateModified").as<winrt::IPropertyValue>();
    if (dateModified)
    {
      statData->st_mtime = winrt::clock::to_time_t(dateModified.GetDateTime());
    }
  }

  statData->st_dev = 0;
  statData->st_rdev = statData->st_dev;
  /* set st_nlink */
  statData->st_nlink = 0;
  /* set st_mode */
  statData->st_mode = _S_IREAD | _S_IFDIR | _S_IEXEC; // only read permission for directory from library
  // copy user RWX rights to group rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 3;
  // copy user RWX rights to other rights
  statData->st_mode |= (statData->st_mode & (_S_IREAD | _S_IWRITE | _S_IEXEC)) >> 6;

  return 0;
}
