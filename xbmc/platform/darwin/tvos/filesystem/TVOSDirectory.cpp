/*
 *      Copyright (C) 2018 Team MrMC
 *      https://github.com/MrMC
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
 *  along with MrMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "TVOSDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/URIUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include "platform/darwin/tvos/TVOSNSUserDefaults.h"
#include "platform/darwin/tvos/filesystem/TVOSFile.h"
#include "platform/darwin/tvos/filesystem/TVOSFileUtils.h"
#include "platform/posix/filesystem/PosixDirectory.h"


using namespace XFILE;

bool CTVOSDirectory::WantsDirectory(const CURL& url)
{
  auto rootpath = CSpecialProtocol::TranslatePath(url);
  auto found = rootpath.find(CTVOSFileUtils::GetUserHomeDirectory());
  if (found == std::string::npos)
    return false;

  return true;
}

bool CTVOSDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  // tvos keeps dirs and non-xml files in non-persistent Caches directory
  // xml files are vectored into NSUserDefaults which is persistent, so
  // we fetch dirs and non-xml files using CPosixDirectory, then fill
  // in any missing xml files from NSUserDefaults. The incoming url
  // will be a path, ending at some dir, so we only need to fetch any
  // xml files that might be at that dir level.
  bool rtn = CPosixDirectory::GetDirectory(url, items);
  if (!rtn)
    return false;

  // To see user home xml files in the file manager,
  // we have to populate a list on a directory request.
  auto rootpath = CSpecialProtocol::TranslatePath(url);
  // quick return check, we do not care if
  // not going to user home.
  auto found = rootpath.find(CTVOSFileUtils::GetUserHomeDirectory());
  if (found == std::string::npos)
    return rtn;

  // The directory request will point to the right path '.../home/userdata/..'
  // so we ask for files in ending directory, if we get xml file paths back
  // then we are re-vectoring them to persistent storage and need to
  // create CFileItems that will translate into CTVOSFile object later
  // when accessed.

  // GetDirectoryContents will return full paths
  std::vector<std::string> contents;
  CTVOSNSUserDefaults::GetDirectoryContents(rootpath, contents);
  for (const auto& path : contents)
  {
    CFileItemPtr pItem(new CFileItem(URIUtils::GetFileName(path)));
    // we only save files to persistent storage
    pItem->m_bIsFolder = false;
    // path must a full path, with no protocol
    // or they will not get intercepted in CFileFactory
    pItem->SetPath(path);
    if (!(m_flags & DIR_FLAG_NO_FILE_INFO))
    {
      struct __stat64 buffer;
      CTVOSFile tvOSFile;
      CURL url2(pItem->GetPath());
      if (tvOSFile.Stat(url2, &buffer) == 0)
      {
        // fake the datetime
        KODI::TIME::FileTime fileTime, localTime;
        KODI::TIME::TimeTToFileTime(buffer.st_mtime, &fileTime);
        KODI::TIME::FileTimeToLocalFileTime(&fileTime, &localTime);
        pItem->m_dateTime = localTime;
        // all this to get the file size
        pItem->m_dwSize = buffer.st_size;
      }
    }
    items.Add(pItem);
  }

  return rtn || !contents.empty();
}

bool CTVOSDirectory::Create(const CURL& url)
{
  return CPosixDirectory::Create(url);
}

bool CTVOSDirectory::Exists(const CURL& url)
{
  return CPosixDirectory::Exists(url);
}

bool CTVOSDirectory::Remove(const CURL& url)
{
  return CPosixDirectory::Remove(url);
}
