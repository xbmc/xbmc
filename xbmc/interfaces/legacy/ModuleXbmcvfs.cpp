/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef LEGACY_MODULEXBMCVFS_H_INCLUDED
#define LEGACY_MODULEXBMCVFS_H_INCLUDED
#include "ModuleXbmcvfs.h"
#endif

#ifndef LEGACY_LANGUAGEHOOK_H_INCLUDED
#define LEGACY_LANGUAGEHOOK_H_INCLUDED
#include "LanguageHook.h"
#endif

#ifndef LEGACY_FILESYSTEM_FILE_H_INCLUDED
#define LEGACY_FILESYSTEM_FILE_H_INCLUDED
#include "filesystem/File.h"
#endif

#ifndef LEGACY_FILESYSTEM_DIRECTORY_H_INCLUDED
#define LEGACY_FILESYSTEM_DIRECTORY_H_INCLUDED
#include "filesystem/Directory.h"
#endif

#ifndef LEGACY_UTILS_FILEUTILS_H_INCLUDED
#define LEGACY_UTILS_FILEUTILS_H_INCLUDED
#include "utils/FileUtils.h"
#endif

#ifndef LEGACY_UTILS_URIUTILS_H_INCLUDED
#define LEGACY_UTILS_URIUTILS_H_INCLUDED
#include "utils/URIUtils.h"
#endif

#ifndef LEGACY_UTIL_H_INCLUDED
#define LEGACY_UTIL_H_INCLUDED
#include "Util.h"
#endif


namespace XBMCAddon
{

  namespace xbmcvfs
  {
    bool copy(const String& strSource, const String& strDestnation)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Cache(strSource, strDestnation);
    }

    // delete a file
    bool deleteFile(const String& strSource)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Delete(strSource);
    }

    // rename a file
    bool rename(const String& file, const String& newFile)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Rename(file,newFile);
    }  

    // check for a file or folder existance, mimics Pythons os.path.exists()
    bool exists(const String& path)
    {
      DelayedCallGuard dg;
      if (URIUtils::HasSlashAtEnd(path, true))
        return XFILE::CDirectory::Exists(path);
      return XFILE::CFile::Exists(path, false);
    }      

    // make a directory
    bool mkdir(const String& path)
    {
      DelayedCallGuard dg;
      return XFILE::CDirectory::Create(path);
    }      

    // make all directories along the path
    bool mkdirs(const String& path)
    {
      DelayedCallGuard dg;
      return CUtil::CreateDirectoryEx(path);
    }

    bool rmdir(const String& path, bool force)
    {
      DelayedCallGuard dg;
      return (force ? CFileUtils::DeleteItem(path,force) : XFILE::CDirectory::Remove(path));
    }      

    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path)
    {
      CFileItemList items;
      CStdString strSource;
      strSource = path;
      XFILE::CDirectory::GetDirectory(strSource, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);

      Tuple<std::vector<String>, std::vector<String> > ret;
      // initialize the Tuple to two values
      ret.second();

      for (int i=0; i < items.Size(); i++)
      {
        CStdString itemPath = items[i]->GetPath();
        
        if (URIUtils::HasSlashAtEnd(itemPath)) // folder
        {
          URIUtils::RemoveSlashAtEnd(itemPath);
          CStdString strFileName = URIUtils::GetFileName(itemPath);
          ret.first().push_back(strFileName);
        }
        else // file
        {
          CStdString strFileName = URIUtils::GetFileName(itemPath);
          ret.second().push_back(strFileName);
        }
      }

      return ret;
    }
  }
}
