/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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

#include "WinLibraryDirectory.h"
#include "FileItem.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include <string>

using namespace XFILE;
using namespace KODI::PLATFORM::WINDOWS;
using namespace Windows::Storage;
using namespace Windows::Storage::Search;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

bool CWinLibraryDirectory::GetStoragePath(std::string library, std::string & path)
{
  CURL url;
  url.SetProtocol("win-lib");
  url.SetHostName(library);

  if (!IsValid(url))
    return false;

  path = url.Get();
  return true;
}

StorageFolder^ CWinLibraryDirectory::GetRootFolder(const CURL& url)
{
  if (!url.IsProtocol("win-lib"))
    return nullptr;

  std::string lib = url.GetHostName();
  if (lib == "music")
    return KnownFolders::MusicLibrary;
  if (lib == "video")
    return KnownFolders::VideosLibrary;
  if (lib == "pictures")
    return KnownFolders::PicturesLibrary;
  if (lib == "photos")
    return KnownFolders::CameraRoll;
  if (lib == "documents")
    return KnownFolders::DocumentsLibrary;
  if (lib == "removable")
    return KnownFolders::RemovableDevices;

  return nullptr;
}

bool CWinLibraryDirectory::IsValid(const CURL & url)
{
  if (!url.IsProtocol("win-lib"))
    return false;

  std::string lib = url.GetHostName();

  if ( lib == "music"
    || lib == "video"
    || lib == "pictures"
    || lib == "photos"
    || lib == "documents"
    || lib == "removable")
    return true;
  else
    return false;
}

CWinLibraryDirectory::CWinLibraryDirectory() = default;
CWinLibraryDirectory::~CWinLibraryDirectory(void) = default;

bool CWinLibraryDirectory::GetDirectory(const CURL &url, CFileItemList &items)
{
  items.Clear();

  auto folder = GetFolder(url);
  if (!folder)
    return false;

  // We accept win-lib://library/path[/]
  std::string path = url.Get();
  URIUtils::AddSlashAtEnd(path); //be sure the dir ends with a slash

  auto vectorView = Wait(folder->GetItemsAsync());
  for (unsigned i = 0; i < vectorView->Size; i++)
  {
    IStorageItem^ item = vectorView->GetAt(i);
    std::string itemName = FromW(std::wstring(item->Name->Data()));

    CFileItemPtr pItem(new CFileItem(itemName));
    pItem->m_bIsFolder = (item->Attributes & FileAttributes::Directory) == FileAttributes::Directory;
    IStorageItemProperties^ storageItemProperties = dynamic_cast<IStorageItemProperties^>(item);
    if (item != nullptr)
    {
      pItem->m_strTitle = FromW(storageItemProperties->DisplayName->Data());
    }

    if (pItem->m_bIsFolder)
      pItem->SetPath(path + itemName + "/");
    else
      pItem->SetPath(path + itemName);

    if (itemName.front() == '.')
      pItem->SetProperty("file:hidden", true);

    auto props = Wait(item->GetBasicPropertiesAsync());

    ULARGE_INTEGER ularge = { 0 };
    if (props->DateModified.UniversalTime > 0)
      ularge.QuadPart = static_cast<uint64_t>(props->DateModified.UniversalTime);

    FILETIME localTime = 
    { 
      ularge.LowPart, 
      ularge.HighPart 
    };
    pItem->m_dateTime = localTime;
    if (!pItem->m_bIsFolder)
      pItem->m_dwSize = static_cast<int64_t>(props->Size);

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
    Wait(folder->CreateFolderAsync(ref new Platform::String(wStrPath.c_str())));
  }
  catch (Platform::Exception^ ex)
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
    Wait(folder->DeleteAsync(StorageDeleteOption::PermanentDelete));
    exists = true;
  }
  catch(Platform::Exception^ ex)
  {
    std::string error = FromW(std::wstring(ex->Message->Data()));
    CLog::LogF(LOGERROR, __FUNCTION__, "unable remove folder '%s' with error", url.Get(), error.c_str());
    exists = false;
  }
  return exists;
}

StorageFolder^ CWinLibraryDirectory::GetFolder(const CURL& url)
{
  StorageFolder^ rootFolder = GetRootFolder(url);
  if (!rootFolder)
    return nullptr;

  // find inner folder
  std::string folderPath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
  if (url.GetHostName() == "removable")
  {
    // here path has the form e\path where first segment is drive letter
    // we should make path form like regular e:\path
    auto index = folderPath.find('\\');
    if (index != std::string::npos)
    {
      folderPath = folderPath.insert(index, 1, ':');
    }
    // win-lib://removable/F -> folderPath contains only drive letter
    else if (index == std::string::npos && folderPath.length() == 1)
    {
      folderPath += ":\\";
    }
    else
    {
      return nullptr;
    }
  }

  if (!folderPath.empty())
  {
    try
    {
      std::wstring wStrPath = ToW(folderPath);
      Platform::String^ pPath = ref new Platform::String(wStrPath.c_str());
      return Wait(rootFolder->GetFolderAsync(pPath));
    }
    catch (Platform::Exception^ ex)
    {
      std::string error = FromW(std::wstring(ex->Message->Data()));
      CLog::LogF(LOGERROR, "unable to get folder '%s' with error", url.GetRedacted().c_str(), error.c_str());
    }
    return nullptr;
  }

  return rootFolder;
}
