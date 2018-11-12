/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UwpSMBDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"
#include "platform/win32/WIN32Util.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
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

bool CUwpSMBDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  items.Clear();

  auto folder = GetFolder(url);
  if (!folder)
    return false;

  // We accept win-lib://library/path[/]
  std::string path = url.Get();
  URIUtils::AddSlashAtEnd(path); //be sure the dir ends with a slash

  auto vectorView = Wait(folder.GetItemsAsync());
  for (auto& item : vectorView)
  {
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

    pItem->m_dateTime = winrt::clock::to_FILETIME(props.DateModified());
    if (!pItem->m_bIsFolder)
      pItem->m_dwSize = static_cast<int64_t>(props.Size());

    items.Add(pItem);
  }

  return true;
}

bool CUwpSMBDirectory::Create(const CURL& url)
{
  auto folder = GetFolder(url);
  if (folder) // already exists
    return true;

  CURL parentUrl = CURL(URIUtils::GetParentPath(url.Get()));
  folder = GetFolder(parentUrl);
  if (!folder)
  {
    return false;
  }

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

bool CUwpSMBDirectory::Exists(const CURL& url)
{
  return GetFolder(url) != nullptr;
}

bool CUwpSMBDirectory::Remove(const CURL& url)
{
  auto folder = GetFolder(url);
  if (folder)
  {
    try
    {
      Wait(folder.DeleteAsync(StorageDeleteOption::PermanentDelete));
      return true;
    }
    catch (const winrt::hresult_error& ex)
    {
      std::string error = FromW(ex.message().c_str());
      CLog::LogF(LOGERROR, __FUNCTION__, "unable remove folder '%s' with error", url.Get(),
                 error.c_str());
    }
  }
  return false;
}

StorageFolder CUwpSMBDirectory::GetFolder(const CURL& url)
{
  StorageFolder folder{nullptr};

  std::string requestedPath = CWIN32Util::SmbToUnc(url.GetRedacted());
  try
  {
    folder = Wait(StorageFolder::GetFolderFromPathAsync(KODI::PLATFORM::WINDOWS::ToW(requestedPath)));
    if (folder == nullptr)
    {
      return nullptr;
    }
  }
  catch (const winrt::hresult_error& ex)
  {
    std::string error = FromW(ex.message().c_str());
    CLog::LogF(LOGERROR, "unable to get folder '%s' with error %s - requestedPath(%s)",
               url.GetRedacted().c_str(), error.c_str(), requestedPath.c_str());
  }

  return folder;
}

int CUwpSMBDirectory::StatDirectory(const CURL& url, struct __stat64* statData)
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
