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

#ifndef FILESYSTEM_APKDIRECTORY_H_INCLUDED
#define FILESYSTEM_APKDIRECTORY_H_INCLUDED
#include "APKDirectory.h"
#endif

#ifndef FILESYSTEM_APKFILE_H_INCLUDED
#define FILESYSTEM_APKFILE_H_INCLUDED
#include "APKFile.h"
#endif

#ifndef FILESYSTEM_FILEITEM_H_INCLUDED
#define FILESYSTEM_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef FILESYSTEM_UTILS_CHARSETCONVERTER_H_INCLUDED
#define FILESYSTEM_UTILS_CHARSETCONVERTER_H_INCLUDED
#include "utils/CharsetConverter.h"
#endif

#ifndef FILESYSTEM_UTILS_LOG_H_INCLUDED
#define FILESYSTEM_UTILS_LOG_H_INCLUDED
#include "utils/log.h"
#endif

#ifndef FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_STRINGUTILS_H_INCLUDED
#include "utils/StringUtils.h"
#endif

#ifndef FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#define FILESYSTEM_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif


#include <zip.h>

using namespace XFILE;

// Android apk directory i/o. Depends on libzip
// Basically the same format as zip.
// We might want to refactor CZipDirectory someday...
//////////////////////////////////////////////////////////////////////
bool CAPKDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  // uses a <fully qualified path>/filename.apk/...
  CURL url(strPath);

  CStdString path = url.GetFileName();
  CStdString host = url.GetHostName();
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

  CStdString test_name;
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

bool CAPKDirectory::ContainsFiles(const CStdString& strPath)
{
  // TODO: why might we need this ?
  return false;
}

DIR_CACHE_TYPE CAPKDirectory::GetCacheType(const CStdString& strPath) const
{
  return DIR_CACHE_ALWAYS;
}

bool CAPKDirectory::Exists(const char* strPath)
{
  // uses a <fully qualified path>/filename.apk/...
  CAPKFile apk;
  CURL url(strPath);
  return apk.Exists(url);
}
