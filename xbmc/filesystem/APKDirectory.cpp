/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "APKDirectory.h"
#include "APKFile.h"
#include "FileItem.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <zip.h>

using namespace XFILE;

// Android apk directory i/o. Depends on libzip
// Basically the same format as zip.
// We might want to refactor CZipDirectory someday...
//////////////////////////////////////////////////////////////////////
bool CAPKDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  // uses a <fully qualified path>/filename.apk/...
  std::string path = url.GetFileName();
  std::string host = url.GetHostName();
  URIUtils::AddSlashAtEnd(path);

  int zip_flags = 0, zip_error = 0;
  struct zip *zip_archive;
  zip_archive = zip_open(host.c_str(), zip_flags, &zip_error);
  if (!zip_archive || zip_error)
  {
    CLog::Log(LOGERROR, "CAPKDirectory::GetDirectory: Unable to open archive : '%s'",
      host.c_str());
    return false;
  }

  std::string test_name;
  int numFiles = zip_get_num_files(zip_archive);
  for (int zip_index = 0; zip_index < numFiles; zip_index++)
  {
    test_name = zip_get_name(zip_archive, zip_index, zip_flags);

    // check for non matching path.
    if (!StringUtils::StartsWith(test_name, path))
      continue;

    // libzip does not index folders, only filenames. We search for a /,
    // add it if it's not in our list already, and hope that no one has
    // any "file/name.exe" files in a zip.

    size_t dir_marker = test_name.find('/', path.size() + 1);
    if (dir_marker != std::string::npos)
    {
      // return items relative to path
      test_name=test_name.substr(0, dir_marker);

      if (items.Contains(host + "/" + test_name))
        continue;
    }

    struct zip_stat sb;
    zip_stat_init(&sb);
    if (zip_stat_index(zip_archive, zip_index, zip_flags, &sb) != -1)
    {
      g_charsetConverter.unknownToUTF8(test_name);
      CFileItemPtr pItem(new CFileItem(test_name));      
      pItem->m_dwSize    = sb.size;
      pItem->m_dateTime  = sb.mtime;    
      pItem->m_bIsFolder = dir_marker > 0 ;
      pItem->SetPath(host + "/" + test_name);
      pItem->SetLabel(test_name.substr(path.size()));
      items.Add(pItem);      
    }
  }
  zip_close(zip_archive);

  return true;
}

bool CAPKDirectory::ContainsFiles(const CURL& url)
{
  //! @todo why might we need this ?
  return false;
}

DIR_CACHE_TYPE CAPKDirectory::GetCacheType(const CURL& url) const
{
  return DIR_CACHE_ALWAYS;
}

bool CAPKDirectory::Exists(const CURL& url)
{
  // uses a <fully qualified path>/filename.apk/...
  CAPKFile apk;
  return apk.Exists(url);
}
