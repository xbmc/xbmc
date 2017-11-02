/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "WinLibraryFile.h"
#include "WinLibraryDirectory.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"

#include <string>
#include <robuffer.h>

using namespace XFILE;
using namespace KODI::PLATFORM::WINDOWS;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;
using namespace Windows::Storage::Search;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation::Collections;

byte* GetUnderlyingBuffer(IBuffer^ buf)
{
  Microsoft::WRL::ComPtr<IBufferByteAccess> bufferByteAccess;
  HRESULT hr = reinterpret_cast<IUnknown*>(buf)->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
  byte* raw_buffer;
  hr = bufferByteAccess->Buffer(&raw_buffer);
  return raw_buffer;
}

bool CWinLibraryFile::IsValid(const CURL & url)
{
  return CWinLibraryDirectory::IsValid(url)
    && !url.GetFileName().empty()
    && !URIUtils::HasSlashAtEnd(url.GetFileName(), false);
}

CWinLibraryFile::CWinLibraryFile()
  : m_allowWrite(false)
  , m_sFile(nullptr)
  , m_fileStream(nullptr)
{
}

CWinLibraryFile::~CWinLibraryFile(void)
{
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
    // see https://docs.microsoft.com/en-us/uwp/api/windows.storage.streams.irandomaccessstream
    // m_fileStream->Close(); // where it is? 
    m_fileStream = nullptr;
  }
  if (m_sFile)
    m_sFile = nullptr;
}

ssize_t CWinLibraryFile::Read(void * lpBuf, size_t uiBufSize)
{
  if (!m_fileStream)
    return -1;

  IBuffer^ buf = ref new Buffer(uiBufSize);
  Wait(m_fileStream->ReadAsync(buf, uiBufSize, InputStreamOptions::None));

  memcpy(lpBuf, GetUnderlyingBuffer(buf), buf->Length);

  return buf->Length;
}

ssize_t CWinLibraryFile::Write(const void * lpBuf, size_t uiBufSize)
{
  if (!m_fileStream || !m_allowWrite)
    return -1;

  IBuffer^ buf = ref new Buffer(uiBufSize);
  memcpy(GetUnderlyingBuffer(buf), lpBuf, uiBufSize);

  Wait(m_fileStream->WriteAsync(buf));

  return buf->Length;
}

int64_t CWinLibraryFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fileStream != nullptr)
  {
    int64_t pos = iFilePosition;
    if (iWhence == SEEK_CUR)
      pos += m_fileStream->Position;
    else if (iWhence == SEEK_END)
      pos += m_fileStream->Size;

    m_fileStream->Seek(pos);

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
  return m_fileStream != nullptr ? m_fileStream->Position : -1;
}

int64_t CWinLibraryFile::GetLength()
{
  if (m_fileStream != nullptr)
    return m_fileStream->Size;
  return 0;
}

void CWinLibraryFile::Flush()
{
}

bool CWinLibraryFile::Delete(const CURL & url)
{
  bool success = false;
  auto file = GetFile(url);
  if (file)
  {
    Wait(file->DeleteAsync());
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
      Wait(currFile->MoveAndReplaceAsync(destFile));
      return true;
    }

    // move
    CURL defFolder = CURL(urlNewName.GetWithoutFilename());
    StorageFolder^ destFolder = CWinLibraryDirectory::GetFolder(defFolder);
    if (destFolder)
    {
      Wait(currFile->MoveAsync(destFolder));
      return true;
    }
  }
  return false;
}

bool CWinLibraryFile::SetHidden(const CURL & url, bool hidden)
{
  return false;
}

bool CWinLibraryFile::Exists(const CURL& url)
{
  return GetFile(url) != nullptr;
}

int CWinLibraryFile::Stat(const CURL & url, struct __stat64 * statData)
{
  auto file = GetFile(url);
  // TODO get stats
  return 0;
}

int CWinLibraryFile::Stat(struct __stat64 * statData)
{
  return 0;
}

bool CWinLibraryFile::IsInAccessList(const CURL& url)
{
  // skip local folder and installation folder
  using KODI::PLATFORM::WINDOWS::FromW;

  auto localFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
  std::string path = FromW(localFolder->Path->Data(), localFolder->Path->Length());
  if (StringUtils::StartsWithNoCase(url.Get(), path))
    return false;

  auto appFolder = Windows::ApplicationModel::Package::Current->InstalledLocation;
  path = FromW(appFolder->Path->Data(), appFolder->Path->Length());
  if (StringUtils::StartsWithNoCase(url.Get(), path))
    return false;

  return IsInList(url, StorageApplicationPermissions::FutureAccessList)
      || IsInList(url, StorageApplicationPermissions::MostRecentlyUsedList);
}

bool CWinLibraryFile::OpenIntenal(const CURL &url, FileAccessMode mode)
{
  std::string filePath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
  std::wstring wpath = ToW(filePath);
  try
  {
    auto exitingFile = GetFile(url);
    if (exitingFile)
      m_sFile = exitingFile;
    else if (mode == FileAccessMode::ReadWrite)
    { 
      auto destFolder = CURL(url.GetWithoutFilename());
      auto folder = CWinLibraryDirectory::GetFolder(destFolder);
      if (folder)
      {
        std::wstring fileNameW = ToW(url.GetFileNameWithoutPath());
        Platform::String^ strRT = ref new Platform::String(fileNameW.c_str());
        auto newFile = Wait(folder->CreateFileAsync(strRT, CreationCollisionOption::ReplaceExisting));
        if (newFile)
          m_sFile = newFile;
      }
    }

    if (m_sFile)
      m_fileStream = Wait(m_sFile->OpenAsync(mode));
  }
  catch (Platform::Exception^ ex)
  {
    // TODO logging error
    return false;
  }

  return m_fileStream != nullptr;
}

StorageFile^ CWinLibraryFile::GetFile(const CURL & url)
{
  // check that url is library url
  if (CWinLibraryDirectory::IsValid(url))
  {
    StorageFolder^ rootFolder = CWinLibraryDirectory::GetRootFolder(url);

    std::string filePath = URIUtils::FixSlashesAndDups(url.GetFileName(), '\\');
    std::wstring wpath = ToW(filePath);

    try
    {
      Platform::String^ pFilePath = ref new Platform::String(wpath.c_str());
      auto item = Wait(rootFolder->TryGetItemAsync(pFilePath));
      return (item != nullptr && item->IsOfType(StorageItemTypes::File)) ? (StorageFile^)item : nullptr;
    }
    catch (Platform::Exception^ ex)
    {
      // TODO logging error
    }
  }
  else if (url.GetProtocol() == "file" || url.GetProtocol().empty())
  {
    // check that a file in feature access list or most rescently used list
    // search in FAL
    IStorageItemAccessList^ list = StorageApplicationPermissions::FutureAccessList;
    Platform::String^ token = GetTokenFromList(url, list);
    if (!token || token->IsEmpty())
    {
      // serach in MRU list
      IStorageItemAccessList^ list = StorageApplicationPermissions::MostRecentlyUsedList;
      token = GetTokenFromList(url, list);
    }
    if (token && !token->IsEmpty())
      return Wait(list->GetFileAsync(token));
  }

  return nullptr;
}

bool CWinLibraryFile::IsInList(const CURL& url, IStorageItemAccessList^ list)
{
  Platform::String^ token = GetTokenFromList(url, list);
  return token != nullptr && !token->IsEmpty();
}

Platform::String^ CWinLibraryFile::GetTokenFromList(const CURL& url, IStorageItemAccessList^ list)
{
  AccessListEntryView^ listview = list->Entries;
  if (listview->Size == 0)
    return nullptr;

  using KODI::PLATFORM::WINDOWS::ToW;
  std::string filePath = url.Get();
  std::wstring filePathW = ToW(filePath);
  Platform::String^ itemKey = ref new Platform::String(filePathW.c_str());

  for (int i = 0; i < listview->Size; i++)
  {
    auto listEntry = listview->GetAt(i);
    if (listEntry.Metadata->Equals(itemKey))
    {
      return listEntry.Token;
    }
  }

  return nullptr;
}
