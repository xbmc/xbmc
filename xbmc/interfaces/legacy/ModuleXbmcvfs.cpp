/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "ModuleXbmcvfs.h"
#include "LanguageHook.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "Util.h"

namespace XBMCAddon
{

  namespace xbmcvfs
  {
    bool copy(const String& strSource, const String& strDestination)
    {
      DelayedCallGuard dg;
      return XFILE::CFile::Copy(strSource, strDestination);
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

    // check for a file or folder existence, mimics Pythons os.path.exists()
    bool exists(const String& path)
    {
      DelayedCallGuard dg;
      if (URIUtils::HasSlashAtEnd(path, true))
        return XFILE::CDirectory::Exists(path, false);
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

      if (force)
        return CFileUtils::DeleteItem(path);
      else
        return XFILE::CDirectory::Remove(path);
    }      

    Tuple<std::vector<String>, std::vector<String> > listdir(const String& path)
    {
      DelayedCallGuard dg;
      CFileItemList items;
      std::string strSource;
      strSource = path;
      XFILE::CDirectory::GetDirectory(strSource, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);

      Tuple<std::vector<String>, std::vector<String> > ret;
      // initialize the Tuple to two values
      ret.second();

      for (int i=0; i < items.Size(); i++)
      {
        std::string itemPath = items[i]->GetPath();
        
        if (URIUtils::HasSlashAtEnd(itemPath)) // folder
        {
          URIUtils::RemoveSlashAtEnd(itemPath);
          std::string strFileName = URIUtils::GetFileName(itemPath);
          if (strFileName.empty())
          {
            CURL url(itemPath);
            strFileName = url.GetHostName();
          }
          ret.first().push_back(strFileName);
        }
        else // file
        {
          std::string strFileName = URIUtils::GetFileName(itemPath);
          ret.second().push_back(strFileName);
        }
      }

      return ret;
    }
  }
}
